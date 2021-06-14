#pragma once

#include <unordered_map>

#include "d_renderer.h"
#include "d_filesystem.h"
#include "d_task_thread.h"


namespace dal {

    class ResourceManager : public ITextureManager {

    private:
        std::unordered_map<std::string, HTexture> m_textures;

        TaskManager& m_task_man;
        Filesystem& m_filesys;
        IRenderer* m_renderer;

    public:
        ResourceManager(TaskManager& task_man, Filesystem& filesys)
            : m_task_man(task_man)
            , m_filesys(filesys)
            , m_renderer(nullptr)
        {

        }

        void set_renderer(IRenderer& renderer);

        void invalidate_renderer();

        HTexture request_texture(const ResPath& respath) override;

    };

}
