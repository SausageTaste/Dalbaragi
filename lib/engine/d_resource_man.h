#pragma once

#include <unordered_map>

#include "d_renderer.h"
#include "d_filesystem.h"
#include "d_task_thread.h"


namespace dal {

    class TextureBuilder : public ITaskListener {

    private:
        std::unordered_map<std::string, HTexture> m_waiting_file;

        IRenderer* m_renderer = nullptr;

    public:
        void update();

        void set_renderer(IRenderer& renderer);

        void invalidate_renderer();

        void notify_task_done(std::unique_ptr<ITask> task) override;

        void start(const ResPath& respath, HTexture h_texture, Filesystem& filesys, TaskManager& task_man);

    };


    class ModelBuilder : public ITaskListener {

    private:
        std::unordered_map<std::string, HRenModel> m_waiting_file;
        std::vector<HRenModel> m_waiting_prepare;

        IRenderer* m_renderer = nullptr;

    public:
        void update();

        void set_renderer(IRenderer& renderer);

        void invalidate_renderer();

        void notify_task_done(std::unique_ptr<ITask> task) override;

        void start(const ResPath& respath, HRenModel h_model, Filesystem& filesys, TaskManager& task_man);

    };


    class ResourceManager : public ITextureManager {

    private:
        std::unordered_map<std::string, HTexture> m_textures;
        std::unordered_map<std::string, HRenModel> m_models;
        std::unordered_map<std::string, HRenModelSkinned> m_skinned_models;
        std::vector<HActor> m_actors;
        HTexture m_missing_tex;

        TextureBuilder m_tex_builder;
        ModelBuilder m_model_builder;

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

        void update();

        void set_renderer(IRenderer& renderer);

        void invalidate_renderer();

        HTexture request_texture(const ResPath& respath) override;

        HRenModel request_model(const ResPath& respath);

        HRenModelSkinned request_model_skinned(const ResPath& respath);

        HActor request_actor();

    };

}
