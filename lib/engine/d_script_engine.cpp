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

}


// Lua lib: console
namespace {

    int console_log(lua_State* const L) {
        static_assert(5 == static_cast<int>(dal::LogLevel::fatal));

        std::string buffer;

        const auto nargs = lua_gettop(L);
        if (2 > nargs)
            return luaL_error(L, "Needs 2 arguments");

        if (!lua_isnumber(L, 1))
            return luaL_error(L, "First augument must be one of log levels {console.VERBOSE, console.INFO, ... console.FATAL}");

        const auto log_level_int = static_cast<int>(lua_tonumber(L, 1));
        if (log_level_int < static_cast<int>(dal::LogLevel::verbose) || log_level_int > static_cast<int>(dal::LogLevel::fatal))
            return luaL_error(L, fmt::format("Invalid log level value: {}", log_level_int).c_str());

        const auto log_level = static_cast<dal::LogLevel>(log_level_int);

        for (int i = 2; i <= nargs; ++i) {
            if (lua_isnil(L, i))
                break;

            const auto arg = lua_tostring(L, i);
            if (nullptr == arg)
                continue;

            buffer.append(arg);
            buffer += ' ';
        }

        if (!buffer.empty()) {
            dalLog(log_level, buffer.c_str());
        }

        return 0;
    }

    //name of this function is not flexible
    int luaopen_console(lua_State* const L) {
        LuaFuncListBuilder func_list;
        func_list.add("log", ::console_log);

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

    const char* const DAL_TRANSFORM_VIEW = "dalbaragi.TransformView";
    const char* const DAL_ACTOR_SKINNED = "dalbaragi.ActorSkinned";

    dal::Transform& check_transform_view(lua_State* const L) {
        void* const ud = luaL_checkudata(L, 1, ::DAL_TRANSFORM_VIEW);
        return **static_cast<dal::Transform**>(ud);
    }

    entt::entity check_actor_skinned(lua_State* const L) {
        void* const ud = luaL_checkudata(L, 1, ::DAL_ACTOR_SKINNED);
        return *static_cast<entt::entity*>(ud);
    }


    int scene_create_actor_skinned(lua_State* const L) {
        dalAssert(::are_dependencies_ready());

        const auto nargs = lua_gettop(L);
        if (2 != nargs)
            return luaL_error(L, "Needs 2 arguments");

        const auto name = lua_tostring(L, -2);
        const auto res_path = lua_tostring(L, -1);

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


    int scene_actor_skinned_get_transform_view(lua_State* const L) {
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

    int scene_actor_skinned_notify_transform_change(lua_State* const L) {
        const auto entity = ::check_actor_skinned(L);

        const auto actor_animated = g_scene->m_registry.try_get<dal::cpnt::ActorAnimated>(entity);
        if (nullptr == actor_animated)
            return luaL_error(L, "Invalid entity for a skinned actor");

        actor_animated->m_actor->notify_transform_change();

        return 0;
    }


    int scene_transform_view_get_pos(lua_State* const L) {
        auto& t = ::check_transform_view(L);

        lua_pushnumber(L, t.m_pos.x);
        lua_pushnumber(L, t.m_pos.y);
        lua_pushnumber(L, t.m_pos.z);
        return 3;
    }

    int scene_transform_view_set_pos_x(lua_State* const L) {
        auto& t = ::check_transform_view(L);
        const auto value = luaL_checknumber(L, 2);

        t.m_pos.x = value;

        return 0;
    }

    int scene_transform_view_set_pos_y(lua_State* const L) {
        auto& t = ::check_transform_view(L);
        const auto value = luaL_checknumber(L, 2);

        t.m_pos.y = value;

        return 0;
    }

    int scene_transform_view_set_pos_z(lua_State* const L) {
        auto& t = ::check_transform_view(L);
        const auto value = luaL_checknumber(L, 2);

        t.m_pos.z = value;

        return 0;
    }

    int scene_transform_view_rotate_degree(lua_State* const L) {
        auto& t = ::check_transform_view(L);
        const auto degree = luaL_checknumber(L, 2);
        const auto x_axis = luaL_checknumber(L, 3);
        const auto y_axis = luaL_checknumber(L, 4);
        const auto z_axis = luaL_checknumber(L, 5);

        t.rotate(glm::radians<float>(degree), glm::vec3{ x_axis, y_axis, z_axis });

        return 0;
    }

    int scene_transform_view_get_scale(lua_State* const L) {
        auto& t = ::check_transform_view(L);

        lua_pushnumber(L, t.m_scale);
        return 1;
    }

    int scene_transform_view_set_scale(lua_State* const L) {
        auto& t = ::check_transform_view(L);
        const auto value = luaL_checknumber(L, 2);

        t.m_scale = value;

        return 0;
    }


    //name of this function is not flexible
    int luaopen_scene(lua_State* const L) {
        {
            LuaFuncListBuilder transform_viwe_methods;
            transform_viwe_methods.add("get_pos", ::scene_transform_view_get_pos);
            transform_viwe_methods.add("set_pos_x", ::scene_transform_view_set_pos_x);
            transform_viwe_methods.add("set_pos_y", ::scene_transform_view_set_pos_y);
            transform_viwe_methods.add("set_pos_z", ::scene_transform_view_set_pos_z);
            transform_viwe_methods.add("rotate_degree", ::scene_transform_view_rotate_degree);
            transform_viwe_methods.add("get_scale", ::scene_transform_view_get_scale);
            transform_viwe_methods.add("set_scale", ::scene_transform_view_set_scale);

            luaL_newmetatable(L, ::DAL_TRANSFORM_VIEW);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);  /* pushes the metatable */
            lua_settable(L, -3);  /* metatable.__index = metatable */
            set_lua_func_to_table(L, transform_viwe_methods.data(), 0);
        }

        {
            LuaFuncListBuilder methods;
            methods.add("get_transform_view", ::scene_actor_skinned_get_transform_view);
            methods.add("notify_transform_change", ::scene_actor_skinned_notify_transform_change);

            luaL_newmetatable(L, ::DAL_ACTOR_SKINNED);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_settable(L, -3);
            set_lua_func_to_table(L, methods.data(), 0);
        }

        {
            LuaFuncListBuilder func_list;
            func_list.add("create_actor_skinned", ::scene_create_actor_skinned);
            func_list.add("transform_view_get_pos", ::scene_transform_view_get_pos);
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

        luaL_requiref(this->m_lua, "console", ::luaopen_console, 0);
        luaL_requiref(this->m_lua, "scene", ::luaopen_scene, 0);
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
