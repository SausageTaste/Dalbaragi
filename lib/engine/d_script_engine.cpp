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
    auto check_udata(lua_State* const L, const int index, const char* const type_name) {
        void* const ud = luaL_checkudata(L, index, type_name);
        return *static_cast<T*>(ud);
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
    const char* const DAL_DLIGHT = "dalbaragi.DLight";
    const char* const DAL_SLIGHT = "dalbaragi.SLight";
    const char* const DAL_TRANSFORM_VIEW = "dalbaragi.TransformView";
    const char* const DAL_ACTOR_STATIC = "dalbaragi.ActorStatic";
    const char* const DAL_ACTOR_SKINNED = "dalbaragi.ActorSkinned";


    glm::vec3& check_vec3(lua_State* const L, const int index = 1) {
        return *::check_udata<glm::vec3*>(L, index, ::DAL_VEC3);
    }

    dal::DLight& check_dlight(lua_State* const L, const int index = 1) {
        return *::check_udata<dal::DLight*>(L, index, ::DAL_DLIGHT);
    }

    dal::SLight& check_slight(lua_State* const L, const int index = 1) {
        return *::check_udata<dal::SLight*>(L, index, ::DAL_SLIGHT);
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

            const auto ud = lua_newuserdata(L, sizeof(entt::entity));
            const auto ud_ptr = static_cast<entt::entity*>(ud);
            *ud_ptr = entity;

            luaL_getmetatable(L, ::DAL_ACTOR_STATIC);
            lua_setmetatable(L, -2);

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

            const auto ud = lua_newuserdata(L, sizeof(entt::entity));
            const auto ud_ptr = static_cast<entt::entity*>(ud);
            *ud_ptr = entity;

            luaL_getmetatable(L, ::DAL_ACTOR_SKINNED);
            lua_setmetatable(L, -2);

            return 1;
        }

        // scene.get_dlight_handle() -> DLight
        int get_dlight_handle(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            const auto ud = lua_newuserdata(L, sizeof(dal::DLight*));
            const auto ud_ptr = static_cast<dal::DLight**>(ud);
            *ud_ptr = &g_scene->m_dlight;

            luaL_getmetatable(L, ::DAL_DLIGHT);
            lua_setmetatable(L, -2);

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

            const auto index = luaL_checknumber(L, 1);
            const auto slight_count = g_scene->m_slights.size();
            if (index >= slight_count)
                return luaL_error(L, fmt::format("Spot light index out of range: input={}, size={}", index, slight_count).c_str());

            const auto ud = lua_newuserdata(L, sizeof(dal::SLight*));
            const auto ud_ptr = static_cast<dal::SLight**>(ud);
            *ud_ptr = &g_scene->m_slights.at(index);

            luaL_getmetatable(L, ::DAL_SLIGHT);
            lua_setmetatable(L, -2);

            return 1;
        }

        // scene.create_slight() -> SLight
        int create_slight(lua_State* const L) {
            dalAssert(::are_dependencies_ready());

            const auto ud = lua_newuserdata(L, sizeof(dal::SLight*));
            const auto ud_ptr = static_cast<dal::SLight**>(ud);
            *ud_ptr = &g_scene->m_slights.emplace_back();

            luaL_getmetatable(L, ::DAL_SLIGHT);
            lua_setmetatable(L, -2);

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

            const auto ud = lua_newuserdata(L, sizeof(glm::vec3*));
            const auto ud_ptr = static_cast<glm::vec3**>(ud);
            *ud_ptr = &light.m_color;

            luaL_getmetatable(L, ::DAL_VEC3);
            lua_setmetatable(L, -2);

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

            const auto ud = lua_newuserdata(L, sizeof(glm::vec3*));
            const auto ud_ptr = static_cast<glm::vec3**>(ud);
            *ud_ptr = &light.m_pos;

            luaL_getmetatable(L, ::DAL_VEC3);
            lua_setmetatable(L, -2);

            return 1;
        }

        // SLight.get_color(SLight self) -> Vec3
        int get_color(lua_State* const L) {
            auto& light = ::check_slight(L);

            const auto ud = lua_newuserdata(L, sizeof(glm::vec3*));
            const auto ud_ptr = static_cast<glm::vec3**>(ud);
            *ud_ptr = &light.m_color;

            luaL_getmetatable(L, ::DAL_VEC3);
            lua_setmetatable(L, -2);

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

            const auto ud = lua_newuserdata(L, sizeof(glm::vec3*));
            const auto ud_ptr = static_cast<glm::vec3**>(ud);
            *ud_ptr = &t.m_pos;

            luaL_getmetatable(L, ::DAL_VEC3);
            lua_setmetatable(L, -2);

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
        int get_transform_view(lua_State* const L) {
            const auto entity = ::check_actor_static(L);

            const auto actor = g_scene->m_registry.try_get<dal::cpnt::ActorStatic>(entity);
            if (nullptr == actor)
                return luaL_error(L, "Invalid entity for a static actor");

            //--------------------------------------------------------------------------------------------

            const auto ud = lua_newuserdata(L, sizeof(dal::Transform*));
            const auto ptr = static_cast<dal::Transform**>(ud);
            *ptr = &actor->m_actor->m_transform;

            luaL_getmetatable(L, ::DAL_TRANSFORM_VIEW);
            lua_setmetatable(L, -2);

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
        int get_transform_view(lua_State* const L) {
            const auto entity = ::check_actor_skinned(L);

            const auto actor_animated = g_scene->m_registry.try_get<dal::cpnt::ActorAnimated>(entity);
            if (nullptr == actor_animated)
                return luaL_error(L, "Invalid entity for a skinned actor");

            //--------------------------------------------------------------------------------------------

            const auto ud = lua_newuserdata(L, sizeof(dal::Transform*));
            const auto ptr = static_cast<dal::Transform**>(ud);
            *ptr = &actor_animated->m_actor->m_transform;

            luaL_getmetatable(L, ::DAL_TRANSFORM_VIEW);
            lua_setmetatable(L, -2);

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

            luaL_newmetatable(L, ::DAL_VEC3);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_settable(L, -3);
            set_lua_func_to_table(L, methods.data(), 0);
        }

        // DLight
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", scene::dlight::to_string);
            methods.add("set_direction_to_light", scene::dlight::set_direction_to_light);
            methods.add("get_color", scene::dlight::get_color);

            luaL_newmetatable(L, ::DAL_DLIGHT);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_settable(L, -3);
            set_lua_func_to_table(L, methods.data(), 0);
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

            luaL_newmetatable(L, ::DAL_SLIGHT);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_settable(L, -3);
            set_lua_func_to_table(L, methods.data(), 0);
        }

        // TransformView
        {
            LuaFuncListBuilder transform_viwe_methods;
            transform_viwe_methods.add("__tostring", ::scene::transform_view::to_string);
            transform_viwe_methods.add("get_pos", ::scene::transform_view::get_pos);
            transform_viwe_methods.add("rotate_degree", ::scene::transform_view::rotate_degree);
            transform_viwe_methods.add("get_scale", ::scene::transform_view::get_scale);
            transform_viwe_methods.add("set_scale", ::scene::transform_view::set_scale);

            luaL_newmetatable(L, ::DAL_TRANSFORM_VIEW);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);  /* pushes the metatable */
            lua_settable(L, -3);  /* metatable.__index = metatable */
            set_lua_func_to_table(L, transform_viwe_methods.data(), 0);
        }

        // ActorStatic
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", ::scene::actor_static::to_string);
            methods.add("get_transform_view", ::scene::actor_static::get_transform_view);
            methods.add("notify_transform_change", ::scene::actor_static::notify_transform_change);

            luaL_newmetatable(L, ::DAL_ACTOR_STATIC);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_settable(L, -3);
            set_lua_func_to_table(L, methods.data(), 0);
        }

        // ActorSkinned
        {
            LuaFuncListBuilder methods;
            methods.add("__tostring", ::scene::actor_skinned::to_string);
            methods.add("get_transform_view", ::scene::actor_skinned::get_transform_view);
            methods.add("notify_transform_change", ::scene::actor_skinned::notify_transform_change);
            methods.add("set_anim_index", ::scene::actor_skinned::set_anim_index);

            luaL_newmetatable(L, ::DAL_ACTOR_SKINNED);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_settable(L, -3);
            set_lua_func_to_table(L, methods.data(), 0);
        }

        {
            LuaFuncListBuilder func_list;
            func_list.add("create_actor_static", ::scene::create_actor_static);
            func_list.add("create_actor_skinned", ::scene::create_actor_skinned);
            func_list.add("get_dlight_handle", scene::get_dlight_handle);
            func_list.add("get_slight_count", scene::get_slight_count);
            func_list.add("get_slight_at", scene::get_slight_at);
            func_list.add("create_slight", scene::create_slight);
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
