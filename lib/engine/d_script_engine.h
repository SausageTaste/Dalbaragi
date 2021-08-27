#pragma once


struct lua_State;


namespace dal {

    class Scene;
    class ResourceManager;


    class LuaState {

    private:
        lua_State* const m_lua;

    public:
        LuaState();

        ~LuaState();

        void exec(const char* const statements);

        static void give_dependencies(
            dal::Scene& scene,
            dal::ResourceManager& res_man
        );

        static void clear_dependencies();

    };

}
