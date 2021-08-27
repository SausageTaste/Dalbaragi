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

}


// Lua lib: console
namespace {

    int console_log(lua_State* const L) {
        std::string buffer;

        const auto nargs = lua_gettop(L);
        for ( int i = 1; i <= nargs; ++i ) {
            if ( lua_isnil(L, i) ) {
                break;
            }

            auto arg = lua_tostring(L, i);
            if ( nullptr == arg ) {
                continue;
            }

            buffer.append(arg);
            buffer += ' ';
        }

        if (!buffer.empty()) {
            dalInfo(buffer.c_str());
        }

        return 0;
    }

    //name of this function is not flexible
    int luaopen_console(lua_State* L) {
        LuaFuncListBuilder func_list;
        func_list.add("log", ::console_log);

        luaL_newlib(L, func_list.data());
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
            dalError(fmt::format("[LUA] {}", lua_tostring(this->m_lua, -1)).c_str());
            lua_pop(this->m_lua, 1);
        }
    }

}
