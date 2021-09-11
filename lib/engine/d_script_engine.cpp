#include "d_script_engine.h"

#include <string>
#include <vector>

#include <fmt/format.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "d_logger.h"
#include "d_scene.h"
#include "d_resource_man.h"
#include "d_timer.h"


// Dependencies
namespace {

    dal::Scene* g_scene = nullptr;
    dal::ResourceManager* g_res_man = nullptr;

    bool are_dependencies_ready() {
        return nullptr != g_scene && nullptr != g_res_man;
    }

}


// Lua state functions
namespace {

    class LuaFuncListBuilder {

    private:
        std::vector<luaL_Reg> m_list;

    public:
        LuaFuncListBuilder() {
            this->add_null_pair();
        }

        void add(const char* const name, lua_CFunction func) {
            this->m_list.back().name = name;
            this->m_list.back().func = func;
            this->add_null_pair();
        }

        void apply_to_global(lua_State* const L) const {
            lua_getglobal(L, "_G");
            luaL_setfuncs(L, this->data(), 0);
            lua_pop(L, 1);
        }

        const luaL_Reg* data() const {
            return this->m_list.data();
        }

        void reserve(const size_t s) {
            this->m_list.reserve(s + 1);
        }

    private:
        void add_null_pair() {
            this->m_list.emplace_back();
            this->m_list.back().func = nullptr;
            this->m_list.back().name = nullptr;
        }

    };


    // The target table must be on top of the stack
    void push_lua_constant(lua_State* const L, const char* const name, const lua_Number value) {
        lua_pushstring(L, name);
        lua_pushnumber(L, value);
        lua_settable(L, -3);
    }

    void set_lua_func_to_table(lua_State* L, const luaL_Reg* l, int nup) {
        for (; l->name; l++) {
            int i;
            lua_pushstring(L, l->name);
            for (i=0; i<nup; i++)  /* copy upvalues to the top */
            lua_pushvalue(L, -(nup+1));
            lua_pushcclosure(L, l->func, nup);
            lua_settable(L, -(nup+3));
        }
        lua_pop(L, nup);  /* remove upvalues */
    }

    template <typename T>
    auto& check_udata(lua_State* const L, const int index, const char* const type_name) {
        void* const ud = luaL_checkudata(L, index, type_name);
        return *static_cast<T*>(ud);
    }

    template <typename T>
    T* test_udata(lua_State* const L, const int index, const char* const type_name) {
        void* const ud = luaL_testudata(L, index, type_name);
        if (nullptr != ud)
            return static_cast<T*>(ud);
        else
            return nullptr;
    }

    void add_metatable_definition(lua_State* const L, const char* const name, const luaL_Reg* const functions) {
        luaL_newmetatable(L, name);
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2);  /* pushes the metatable */
        lua_settable(L, -3);  /* metatable.__index = metatable */
        set_lua_func_to_table(L, functions, 0);
    }

    template <typename T>
    auto& push_meta_object(lua_State* const L, const char* const type_name) {
        const auto ud = lua_newuserdata(L, sizeof(T));
        const auto ud_ptr = static_cast<T*>(ud);

        luaL_getmetatable(L, type_name);
        lua_setmetatable(L, -2);

        return *ud_ptr;
    }

}


// Lua lib: logger
namespace {

    namespace logger {

        // logger.log(int log_level, T a0, T a1, T a2...) -> None
        // Type of T must be one of built-in types or implement __tostring method
        int log(lua_State* const L) {
            static_assert(5 == static_cast<int>(dal::LogLevel::fatal));

            const auto log_level_int = static_cast<int>(luaL_checknumber(L, 1));
            if (log_level_int < static_cast<int>(dal::LogLevel::verbose) || log_level_int > static_cast<int>(dal::LogLevel::fatal))
                return luaL_error(L, fmt::format("Invalid log level value: {}", log_level_int).c_str());
            const auto log_level = static_cast<dal::LogLevel>(log_level_int);

            const auto nargs = lua_gettop(L);

            std::string buffer;
            for (int i = 2; i <= nargs; ++i) {
                if (lua_isnil(L, i))
                    break;

                const auto trial_1 = lua_tostring(L, i);
                if (nullptr != trial_1) {
                    buffer += trial_1;
                    continue;
                }

                const auto trial_2 = luaL_tolstring(L, i, nullptr);
                if (nullptr != trial_2) {
                    buffer += trial_2;
                    continue;
                }
            }

            if (!buffer.empty()) {
                dalLog(log_level, buffer.c_str());
            }

            return 0;
        }

    }


    // name of this function is not flexible
    int luaopen_logger(lua_State* const L) {
        LuaFuncListBuilder func_list;
        func_list.add("log", ::logger::log);

        luaL_newlib(L, func_list.data());

        push_lua_constant(L, "VERBOSE", static_cast<int>(dal::LogLevel::verbose));
        push_lua_constant(L, "INFO", static_cast<int>(dal::LogLevel::info));
        push_lua_constant(L, "DEBUG", static_cast<int>(dal::LogLevel::debug));
        push_lua_constant(L, "WARNING", static_cast<int>(dal::LogLevel::warning));
        push_lua_constant(L, "ERROR", static_cast<int>(dal::LogLevel::error));
        push_lua_constant(L, "FATAL", static_cast<int>(dal::LogLevel::fatal));

        return 1;
    }

}


// Lua lib: scene
namespace {

    const char* const DAL_VEC3 = "dalbaragi.Vec3";
    const char* const DAL_VEC3_VIEW = "dalbaragi.Vec3View";
    const char* const DAL_DLIGHT = "dalbaragi.DLight";
    const char* const DAL_SLIGHT = "dalbaragi.SLight";
    const char* const DAL_PLIGHT = "dalbaragi.PLight";
    const char* const DAL_TRANSFORM_VIEW = "dalbaragi.TransformView";
    const char* const DAL_ACTOR_STATIC = "dalbaragi.ActorStatic";
    const char* const DAL_ACTOR_SKINNED = "dalbaragi.ActorSkinned";


    glm::vec3& check_vec3(lua_State* const L, const int index = 1) {
        const auto p_vec3view = ::test_udata<glm::vec3*>(L, index, ::DAL_VEC3_VIEW);
        if (nullptr != p_vec3view)
            return **p_vec3view;

        return ::check_udata<glm::vec3>(L, index, ::DAL_VEC3);
    }

    dal::DLight& check_dlight(lua_State* const L, const int index = 1) {
        return *::check_udata<dal::DLight*>(L, index, ::DAL_DLIGHT);
    }

    dal::SLight& check_slight(lua_State* const L, const int index = 1) {
        return *::check_udata<dal::SLight*>(L, index, ::DAL_SLIGHT);
    }

    dal::PLight& check_plight(lua_State* const L, const int index = 1) {
        return *::check_udata<dal::PLight*>(L, index, ::DAL_PLIGHT);
    }

    dal::Transform& check_transform_view(lua_State* const L, const int index = 1) {
        return *::check_udata<dal::Transform*>(L, index, ::DAL_TRANSFORM_VIEW);
    }

    entt::entity check_actor_static(lua_State* const L, const int index = 1) {
        return ::check_udata<entt::entity>(L, index, ::DAL_ACTOR_STATIC);
    }

    entt::entity check_actor_skinned(lua_State* const L, const int index = 1) {
        return ::check_udata<entt::entity>(L, index, ::DAL_ACTOR_SKINNED);
    }


    namespace scene {

        // scene.create_actor_static(string name, string resource_path) -> ActorStatic
        int create_actor_static(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            const auto name = luaL_checkstring(L, -2);
            const auto res_path = luaL_checkstring(L, -1);

            const auto entity = g_scene->m_registry.create();
            auto& cpnt_actor = g_scene->m_registry.emplace<dal::cpnt::ActorStatic>(entity);
            cpnt_actor.m_model = g_res_man->request_model(res_path);
            cpnt_actor.m_actor = g_res_man->request_actor();

            //--------------------------------------------------------------------------------------------

            auto& obj = ::push_meta_object<entt::entity>(L, ::DAL_ACTOR_STATIC);
            obj = entity;

            return 1;
        }

        // scene.create_actor_static(string name, string resource_path) -> ActorSkinned
        int create_actor_skinned(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            const auto name = luaL_checkstring(L, -2);
            const auto res_path = luaL_checkstring(L, -1);

            const auto entity = g_scene->m_registry.create();
            auto& cpnt_actor = g_scene->m_registry.emplace<dal::cpnt::ActorAnimated>(entity);
            cpnt_actor.m_model = g_res_man->request_model_skinned(res_path);
            cpnt_actor.m_actor = g_res_man->request_actor_skinned();

            //--------------------------------------------------------------------------------------------

            auto& obj = ::push_meta_object<entt::entity>(L, ::DAL_ACTOR_SKINNED);
            obj = entity;

            return 1;
        }

        // scene.get_sun_light_handle() -> DLight
        int get_sun_light_handle(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            auto& obj = ::push_meta_object<dal::DLight*>(L, ::DAL_DLIGHT);
            obj = &g_scene->m_sun_light;

            return 1;
        }

        // scene.get_moon_light_handle() -> DLight
        int get_moon_light_handle(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            auto& obj = ::push_meta_object<dal::DLight*>(L, ::DAL_DLIGHT);
            obj = &g_scene->m_moon_light;

            return 1;
        }

        // scene.get_slight_count() -> int
        int get_slight_count(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            lua_pushnumber(L, g_scene->m_slights.size());
            return 1;
        }

        // scene.get_slight_at() -> SLight
        int get_slight_at(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            const auto index = static_cast<size_t>(luaL_checknumber(L, 1));
            const auto slight_count = g_scene->m_slights.size();
            if (index >= slight_count)
                return luaL_error(L, fmt::format("Spot light index out of range: input={}, size={}", index, slight_count).c_str());

            auto& obj = ::push_meta_object<dal::SLight*>(L, ::DAL_SLIGHT);
            obj = &g_scene->m_slights[index];

            return 1;
        }

        // scene.create_slight() -> SLight
        int create_slight(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            auto& obj = ::push_meta_object<dal::SLight*>(L, ::DAL_SLIGHT);
            obj = &g_scene->m_slights.emplace_back();

            return 1;
        }

        // scene.get_plight_count() -> int
        int get_plight_count(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            lua_pushnumber(L, g_scene->m_plights.size());
            return 1;
        }

        // scene.get_plight_at() -> PLight
        int get_plight_at(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            const auto index = luaL_checknumber(L, 1);
            const auto plight_count = g_scene->m_plights.size();
            if (index >= plight_count)
                return luaL_error(L, fmt::format("Point light index out of range: input={}, size={}", index, plight_count).c_str());

            auto& obj = ::push_meta_object<dal::PLight*>(L, ::DAL_PLIGHT);
            obj = &g_scene->m_plights[index];

            return 1;
        }

        // scene.create_plight() -> PLight
        int create_plight(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            auto& obj = ::push_meta_object<dal::PLight*>(L, ::DAL_PLIGHT);
            obj = &g_scene->m_plights.emplace_back();

            return 1;
        }

        int get_ambient_light(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            auto& obj = ::push_meta_object<glm::vec3*>(L, ::DAL_VEC3_VIEW);
            obj = &g_scene->m_ambient_light;

            return 1;
        }

        int Vec3(lua_State* const L) {
            auto& obj = ::push_meta_object<glm::vec3>(L, ::DAL_VEC3);

            obj.x = static_cast<float>(lua_tonumberx(L, 1, nullptr));
            obj.y = static_cast<float>(lua_tonumberx(L, 2, nullptr));
            obj.z = static_cast<float>(lua_tonumberx(L, 3, nullptr));

            return 1;
        }

    }


    namespace scene::vec3 {

        int to_string(lua_State* const L) {
            const auto& v = ::check_vec3(L);

            //--------------------------------------------------------------------------------------------

            lua_pushstring(L, fmt::format("Vec3{{ {}, {}, {} }}", v.x, v.y, v.z).c_str());
            return 1;
        }

        // Vec3.get_xyz(Vec3 self) -> <float, float, float>
        int get_xyz(lua_State* const L) {
            const auto& v = ::check_vec3(L);

            lua_pushnumber(L, v.x);
            lua_pushnumber(L, v.y);
            lua_pushnumber(L, v.z);
            return 3;
        }

        // Vec3.set_x(Vec3 self, float value) -> None
        int set_x(lua_State* const L) {
            auto& v = ::check_vec3(L);
            const auto value = luaL_checknumber(L, 2);

            v.x = value;

            return 0;
        }

        // Vec3.set_y(Vec3 self, float value) -> None
        int set_y(lua_State* const L) {
            auto& v = ::check_vec3(L);
            const auto value = luaL_checknumber(L, 2);

            v.y = value;

            return 0;
        }

        // Vec3.set_z(Vec3 self, float value) -> None
        int set_z(lua_State* const L) {
            auto& v = ::check_vec3(L);
            const auto value = luaL_checknumber(L, 2);

            v.z = value;

            return 0;
        }

        // Vec3.set_xyz(Vec3 self, float x, float y, float z) -> None
        int set_xyz(lua_State* const L) {
            auto& v = ::check_vec3(L);
            const auto x = luaL_checknumber(L, 2);
            const auto y = luaL_checknumber(L, 3);
            const auto z = luaL_checknumber(L, 4);

            v.x = x;
            v.y = y;
            v.z = z;

            return 0;
        }

        int unary_minus(lua_State* const L) {
            const auto& a = ::check_vec3(L);
            auto& output = ::push_meta_object<glm::vec3>(L, ::DAL_VEC3);

            output = -a;
            return 1;
        }

        int add(lua_State* const L) {
            const auto& a = ::check_vec3(L);
            const auto& b = ::check_vec3(L, 2);
            auto& output = ::push_meta_object<glm::vec3>(L, ::DAL_VEC3);

            output = a + b;
            return 1;
        }

        int sub(lua_State* const L) {
            const auto& a = ::check_vec3(L);
            const auto& b = ::check_vec3(L, 2);
            auto& output = ::push_meta_object<glm::vec3>(L, ::DAL_VEC3);

            output = a - b;
            return 1;
        }

        int mul(lua_State* const L) {
            const auto& a = ::check_vec3(L);
            auto& output = ::push_meta_object<glm::vec3>(L, ::DAL_VEC3);

            if (lua_isnumber(L, 2)) {
                const auto b = lua_tonumberx(L, 2, nullptr);
                output = a * static_cast<float>(b);
            }
            else {
                const auto& b = ::check_vec3(L, 2);
                output = a * b;
            }

            return 1;
        }

        int div(lua_State* const L) {
            const auto& a = ::check_vec3(L);
            auto& output = ::push_meta_object<glm::vec3>(L, ::DAL_VEC3);

            if (lua_isnumber(L, 2)) {
                const auto b = lua_tonumberx(L, 2, nullptr);
                output = a / static_cast<float>(b);
            }
            else {
                const auto& b = ::check_vec3(L, 2);
                output = a / b;
            }

            return 1;
        }

        int dot(lua_State* const L) {
            const auto& a = check_vec3(L, 1);
            const auto& b = check_vec3(L, 2);

            lua_pushnumber(L, glm::dot(a, b));

            return 1;
        }

        int cross(lua_State* const L) {
            const auto& a = check_vec3(L, 1);
            const auto& b = check_vec3(L, 2);
            auto& output = ::push_meta_object<glm::vec3>(L, ::DAL_VEC3);

            output = glm::cross(a, b);

            return 1;
        }

        int normalize(lua_State* const L) {
            const auto& a = check_vec3(L, 1);
            auto& output = ::push_meta_object<glm::vec3>(L, ::DAL_VEC3);

            output = glm::normalize(a);

            return 1;
        }

    }


    namespace scene::dlight {

        int to_string(lua_State* const L) {
            const auto& light = ::check_dlight(L);
            const auto v = light.to_light_direc();

            //--------------------------------------------------------------------------------------------

            lua_pushstring(L, fmt::format("DLight{{ to light=({}, {}, {}) }}", v.x, v.y, v.z).c_str());
            return 1;
        }

        // DLight.set_direction_to_light(DLight self, float x, float y, float z) -> None
        int set_direction_to_light(lua_State* const L) {
            auto& light = ::check_dlight(L);
            const auto x = luaL_checknumber(L, 2);
            const auto y = luaL_checknumber(L, 3);
            const auto z = luaL_checknumber(L, 4);

            light.set_direc_to_light(x, y, z);

            return 0;
        }

        // DLight.get_color(DLight self) -> Vec3
        int get_color(lua_State* const L) {
            auto& light = ::check_dlight(L);

            auto& obj = ::push_meta_object<glm::vec3*>(L, ::DAL_VEC3_VIEW);
            obj = &light.m_color;

            return 1;
        }

    }


    namespace scene::slight {

        int to_string(lua_State* const L) {
            const auto& light = ::check_slight(L);
            const auto v = light.to_light_direc();

            //--------------------------------------------------------------------------------------------

            lua_pushstring(L, fmt::format("SLight{{ to light=({}, {}, {}) }}", v.x, v.y, v.z).c_str());
            return 1;
        }

        // SLight.set_direction_to_light(SLight self, float x, float y, float z) -> None
        int set_direction_to_light(lua_State* const L) {
            auto& light = ::check_slight(L);
            const auto x = luaL_checknumber(L, 2);
            const auto y = luaL_checknumber(L, 3);
            const auto z = luaL_checknumber(L, 4);

            light.set_direc_to_light(x, y, z);

            return 0;
        }

        // SLight.get_pos(SLight self) -> Vec3
        int get_pos(lua_State* const L) {
            auto& light = ::check_slight(L);

            auto& obj = ::push_meta_object<glm::vec3*>(L, ::DAL_VEC3_VIEW);
            obj = &light.m_pos;

            return 1;
        }

        // SLight.get_color(SLight self) -> Vec3
        int get_color(lua_State* const L) {
            auto& light = ::check_slight(L);

            auto& obj = ::push_meta_object<glm::vec3*>(L, ::DAL_VEC3_VIEW);
            obj = &light.m_color;

            return 1;
        }

        // SLight.set_fade_start_degree(SLight self, float value) -> None
        int set_fade_start_degree(lua_State* const L) {
            auto& light = ::check_slight(L);
            const auto value = luaL_checknumber(L, 2);

            light.set_fade_start_degree(value);

            return 0;
        }

        // SLight.set_fade_end_degree(SLight self, float value) -> None
        int set_fade_end_degree(lua_State* const L) {
            auto& light = ::check_slight(L);
            const auto value = luaL_checknumber(L, 2);

            light.set_fade_end_degree(value);

            return 0;
        }

    }


    namespace scene::plight {

        int to_string(lua_State* const L) {
            const auto& light = ::check_plight(L);
            const auto v = light.m_pos;

            //--------------------------------------------------------------------------------------------

            lua_pushstring(L, fmt::format("SLight{{ pos=({}, {}, {}) }}", v.x, v.y, v.z).c_str());
            return 1;
        }

        // PLight.get_pos(PLight self) -> Vec3
        int get_pos(lua_State* const L) {
            auto& light = ::check_plight(L);

            auto& obj = ::push_meta_object<glm::vec3*>(L, ::DAL_VEC3_VIEW);
            obj = &light.m_pos;

            return 1;
        }

        // PLight.get_color(PLight self) -> Vec3
        int get_color(lua_State* const L) {
            auto& light = ::check_plight(L);

            auto& obj = ::push_meta_object<glm::vec3*>(L, ::DAL_VEC3_VIEW);
            obj = &light.m_color;

            return 1;
        }

        // PLight.set_max_distance(SLight self, float value) -> None
        int set_max_distance(lua_State* const L) {
            auto& light = ::check_plight(L);
            const auto value = luaL_checknumber(L, 2);

            light.m_max_dist = value;

            return 0;
        }

    }


    namespace scene::transform_view {

        int to_string(lua_State* const L) {
            const auto& t = ::check_transform_view(L);

            //--------------------------------------------------------------------------------------------

            lua_pushstring(L, fmt::format(
                "TransformView{{ pos=({}, {}, {}), quat=({}, {}, {}, {}), scale={} }}",
                t.m_pos.x, t.m_pos.y, t.m_pos.z,
                t.m_quat.w, t.m_quat.x, t.m_quat.y, t.m_quat.z,
                t.m_scale
            ).c_str());
            return 1;
        }

        // TransformView.get_pos(TransformView self) -> Vec3
        int get_pos(lua_State* const L) {
            auto& t = ::check_transform_view(L);

            auto& obj = ::push_meta_object<glm::vec3*>(L, ::DAL_VEC3_VIEW);
            obj = &t.m_pos;

            return 1;
        }

        // TransformView.rotate_degree(TransformView self, float degree, float axis_x, float axis_y, float axis_z) -> None
        int rotate_degree(lua_State* const L) {
            auto& t = ::check_transform_view(L);
            const auto degree = luaL_checknumber(L, 2);
            const auto x_axis = luaL_checknumber(L, 3);
            const auto y_axis = luaL_checknumber(L, 4);
            const auto z_axis = luaL_checknumber(L, 5);

            t.rotate(glm::radians<float>(degree), glm::vec3{ x_axis, y_axis, z_axis });

            return 0;
        }

        // TransformView.get_scale(TransformView self) -> float
        int get_scale(lua_State* const L) {
            auto& t = ::check_transform_view(L);

            lua_pushnumber(L, t.m_scale);
            return 1;
        }

        // TransformView.set_scale(TransformView self, float value) -> None
        int set_scale(lua_State* const L) {
            auto& t = ::check_transform_view(L);
            const auto value = luaL_checknumber(L, 2);

            t.m_scale = value;

            return 0;
        }

    }


    namespace scene::actor_static {

        int to_string(lua_State* const L) {
            const auto entity = ::check_actor_static(L);

            //--------------------------------------------------------------------------------------------

            lua_pushstring(L, fmt::format("ActorStatic{{ {} }}", entity).c_str());
            return 1;
        }

        // ActorStatic.get_transform_view(ActorStatic self) -> TransformView
        int get_transform(lua_State* const L) {
            const auto entity = ::check_actor_static(L);

            const auto actor = g_scene->m_registry.try_get<dal::cpnt::ActorStatic>(entity);
            if (nullptr == actor)
                return luaL_error(L, "Invalid entity for a static actor");

            //--------------------------------------------------------------------------------------------

            auto& obj = ::push_meta_object<dal::Transform*>(L, ::DAL_TRANSFORM_VIEW);
            obj = &actor->m_actor->m_transform;

            return 1;
        }

        // ActorStatic.notify_transform_change(ActorStatic self) -> None
        int notify_transform_change(lua_State* const L) {
            const auto entity = ::check_actor_static(L);

            const auto actor = g_scene->m_registry.try_get<dal::cpnt::ActorStatic>(entity);
            if (nullptr == actor)
                return luaL_error(L, "Invalid entity for a skinned actor");

            actor->m_actor->notify_transform_change();

            return 0;
        }

    }


    namespace scene::actor_skinned {

        int to_string(lua_State* const L) {
            const auto entity = ::check_actor_skinned(L);

            //--------------------------------------------------------------------------------------------

            lua_pushstring(L, fmt::format("ActorSkinned{{ {} }}", entity).c_str());
            return 1;
        }

        // ActorSkinned.get_transform_view(ActorSkinned self) -> TransformView
        int get_transform(lua_State* const L) {
            const auto entity = ::check_actor_skinned(L);

            const auto actor_animated = g_scene->m_registry.try_get<dal::cpnt::ActorAnimated>(entity);
            if (nullptr == actor_animated)
                return luaL_error(L, "Invalid entity for a skinned actor");

            //--------------------------------------------------------------------------------------------

            auto& obj = ::push_meta_object<dal::Transform*>(L, ::DAL_TRANSFORM_VIEW);
            obj = &actor_animated->m_actor->m_transform;

            return 1;
        }

        // ActorSkinned.notify_transform_change(ActorSkinned self) -> None
        int notify_transform_change(lua_State* const L) {
            const auto entity = ::check_actor_skinned(L);

            const auto actor_animated = g_scene->m_registry.try_get<dal::cpnt::ActorAnimated>(entity);
            if (nullptr == actor_animated)
                return luaL_error(L, "Invalid entity for a skinned actor");

            actor_animated->m_actor->notify_transform_change();

            return 0;
        }

        // ActorSkinned.set_anim_index(ActorSkinned self, int index) -> None
        int set_anim_index(lua_State* const L) {
            const auto entity = ::check_actor_skinned(L);
            const auto value = luaL_checknumber(L, 2);

            const auto actor = g_scene->m_registry.try_get<dal::cpnt::ActorAnimated>(entity);
            if (nullptr == actor)
                return luaL_error(L, "Invalid entity for a skinned actor");

            actor->m_actor->m_anim_state.set_anim_index(value);

            return 0;
        }

    }


    int luaopen_scene(lua_State* const L) {
        // Vec3
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", scene::vec3::to_string);
            methods.add("get_xyz", scene::vec3::get_xyz);
            methods.add("set_x", scene::vec3::set_x);
            methods.add("set_y", scene::vec3::set_y);
            methods.add("set_z", scene::vec3::set_z);
            methods.add("set_xyz", scene::vec3::set_xyz);
            methods.add("__unm", scene::vec3::unary_minus);
            methods.add("__add", scene::vec3::add);
            methods.add("__sub", scene::vec3::sub);
            methods.add("__mul", scene::vec3::mul);
            methods.add("__div", scene::vec3::div);
            methods.add("dot", scene::vec3::dot);
            methods.add("cross", scene::vec3::cross);
            methods.add("normalize", scene::vec3::normalize);

            add_metatable_definition(L, ::DAL_VEC3, methods.data());
            add_metatable_definition(L, ::DAL_VEC3_VIEW, methods.data());
        }

        // DLight
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", scene::dlight::to_string);
            methods.add("set_direction_to_light", scene::dlight::set_direction_to_light);
            methods.add("get_color", scene::dlight::get_color);

            add_metatable_definition(L, ::DAL_DLIGHT, methods.data());
        }

        // SLight
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", scene::slight::to_string);
            methods.add("set_direction_to_light", scene::slight::set_direction_to_light);
            methods.add("get_pos", scene::slight::get_pos);
            methods.add("get_color", scene::slight::get_color);
            methods.add("set_fade_start_degree", scene::slight::set_fade_start_degree);
            methods.add("set_fade_end_degree", scene::slight::set_fade_end_degree);

            add_metatable_definition(L, ::DAL_SLIGHT, methods.data());
        }

        // PLight
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", scene::plight::to_string);
            methods.add("get_pos", scene::plight::get_pos);
            methods.add("get_color", scene::plight::get_color);
            methods.add("set_max_distance", scene::plight::set_max_distance);

            add_metatable_definition(L, ::DAL_PLIGHT, methods.data());
        }

        // TransformView
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", ::scene::transform_view::to_string);
            methods.add("get_pos", ::scene::transform_view::get_pos);
            methods.add("rotate_degree", ::scene::transform_view::rotate_degree);
            methods.add("get_scale", ::scene::transform_view::get_scale);
            methods.add("set_scale", ::scene::transform_view::set_scale);

            add_metatable_definition(L, ::DAL_TRANSFORM_VIEW, methods.data());
        }

        // ActorStatic
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", ::scene::actor_static::to_string);
            methods.add("get_transform", ::scene::actor_static::get_transform);
            methods.add("notify_transform_change", ::scene::actor_static::notify_transform_change);

            add_metatable_definition(L, ::DAL_ACTOR_STATIC, methods.data());
        }

        // ActorSkinned
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", ::scene::actor_skinned::to_string);
            methods.add("get_transform", ::scene::actor_skinned::get_transform);
            methods.add("notify_transform_change", ::scene::actor_skinned::notify_transform_change);
            methods.add("set_anim_index", ::scene::actor_skinned::set_anim_index);

            add_metatable_definition(L, ::DAL_ACTOR_SKINNED, methods.data());
        }

        {
            LuaFuncListBuilder func_list;
            func_list.add("create_actor_static", ::scene::create_actor_static);
            func_list.add("create_actor_skinned", ::scene::create_actor_skinned);
            func_list.add("get_sun_light_handle", ::scene::get_sun_light_handle);
            func_list.add("get_moon_light_handle", ::scene::get_moon_light_handle);
            func_list.add("get_slight_count", ::scene::get_slight_count);
            func_list.add("get_slight_at", ::scene::get_slight_at);
            func_list.add("create_slight", ::scene::create_slight);
            func_list.add("get_plight_count", ::scene::get_plight_count);
            func_list.add("get_plight_at", ::scene::get_plight_at);
            func_list.add("create_plight", ::scene::create_plight);
            func_list.add("get_ambient_light", ::scene::get_ambient_light);
            func_list.add("Vec3", ::scene::Vec3);

            luaL_newlib(L, func_list.data());
        }

        return 1;
    }

}


// Lua lib: sysinfo
namespace {

    namespace sysinfo {

        // sysinfo.get_cur_sec() -> double
        int time(lua_State* const L) {
            lua_pushnumber(L, dal::get_cur_sec());
            return 1;
        }

    }


    int luaopen_sysinfo(lua_State* const L) {
        {
            LuaFuncListBuilder func_list;
            func_list.add("time", ::sysinfo::time);
            luaL_newlib(L, func_list.data());
        }

        return 1;
    }

}


// LuaState
namespace dal {

    LuaState::LuaState()
        : m_lua(luaL_newstate())
    {
        luaL_openlibs(this->m_lua);

        luaL_requiref(this->m_lua, "logger", ::luaopen_logger, 0);
        luaL_requiref(this->m_lua, "scene", ::luaopen_scene, 0);
        luaL_requiref(this->m_lua, "sysinfo", ::luaopen_sysinfo, 0);
    }

    LuaState::~LuaState() {
        lua_close(this->m_lua);
    }

    void LuaState::exec(const char* const statements) {
        if (luaL_dostring(this->m_lua, statements)) {
            dalError(lua_tostring(this->m_lua, -1));
            lua_pop(this->m_lua, 1);
        }
    }

    // Static

    void LuaState::give_dependencies(
        dal::Scene& scene,
        dal::ResourceManager& res_man
    ) {
        g_scene = &scene;
        g_res_man = &res_man;
    }

    void LuaState::clear_dependencies() {
        g_scene = nullptr;
        g_res_man = nullptr;
    }

}
