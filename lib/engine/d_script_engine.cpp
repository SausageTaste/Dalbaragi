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

}


// Lua lib: console
namespace {

    int console_log(lua_State* const L) {
        static_assert(4 == static_cast<int>(dal::LogLevel::error));

        std::string buffer;

        const auto nargs = lua_gettop(L);
        if (2 > nargs)
            return luaL_error(L, "Needs 2 arguments");

        if (!lua_isnumber(L, 1))
            return luaL_error(L, "First augument must be one of log levels {console.VERBOSE, console.INFO, ... console.ERROR}");

        const auto log_level_int = static_cast<int>(lua_tonumber(L, 1));
        if (log_level_int < static_cast<int>(dal::LogLevel::verbose) || log_level_int > static_cast<int>(dal::LogLevel::error))
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

        return 1;
    }

}


namespace dal {

    LuaState::LuaState()
        : m_lua(luaL_newstate())
    {
        luaL_openlibs(this->m_lua);

        luaL_requiref(this->m_lua, "console", ::luaopen_console, 0);
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

}
