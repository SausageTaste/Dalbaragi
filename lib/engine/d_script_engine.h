#pragma once


struct lua_State;


namespace dal {

    class LuaState {

    private:
        lua_State* const m_lua;

    public:
        LuaState();

        ~LuaState();

        void exec(const char* const statements);

    };

}
