#include "d_resource_man.h"

#include <fmt/format.h>

#include "d_timer.h"
#include "d_logger.h"
#include "d_image_parser.h"
#include "d_model_parser.h"


namespace {

    constexpr char* const MISSING_TEX_PATH = "_asset/image/missing_tex.png";


    class Task_LoadImage : public dal::ITask {

    public:
        dal::Filesystem& m_filesys;
        dal::ResPath m_respath;

        std::optional<dal::ImageData> out_image_data;

    public:
        Task_LoadImage(const dal::ResPath& respath, dal::Filesystem& filesys)
            : m_filesys(filesys)
            , m_respath(respath)
            , out_image_data(std::nullopt)
        {

        }

        void run() override {
            dal::Timer timer;

            auto file = this->m_filesys.open(this->m_respath);
            dalAssert(file->is_ready());
            const auto file_data = file->read_stl<std::vector<uint8_t>>();
            dalAssert(file_data.has_value());
            this->out_image_data = dal::parse_image_stb(file_data->data(), file_data->size());
            dalInfo(fmt::format("Image res loaded ({}): {}", timer.check_get_elapsed(), this->m_respath.make_str()).c_str());
        }

    };


    class Task_LoadModel : public dal::ITask {

    public:
        dal::Filesystem& m_filesys;
        dal::ResPath m_respath;

        std::optional<dal::ModelStatic> out_model_data;

    public:
        Task_LoadModel(const dal::ResPath& respath, dal::Filesystem& filesys)
            : m_filesys(filesys)
            , m_respath(respath)
        {

        }

        void run() override {
            dal::Timer timer;

            auto file = this->m_filesys.open(this->m_respath);
            const auto model_content = file->read_stl<std::vector<uint8_t>>();
            dalInfo(fmt::format("Model res loaded ({}): {}", timer.check_get_elapsed(), this->m_respath.make_str()).c_str());

            this->out_model_data = dal::parse_model_dmd(model_content->data(), model_content->size());
            dalInfo(fmt::format("Model res parsed ({}): {}", timer.check_get_elapsed(), this->m_respath.make_str()).c_str());
        }

    };

}


// TextureBuilder
namespace dal {

    void TextureBuilder::update() {

    }

    void TextureBuilder::set_renderer(IRenderer& renderer) {
        this->m_renderer = &renderer;
    }

    void TextureBuilder::invalidate_renderer() {
        this->m_waiting_file.clear();

        this->m_renderer = nullptr;
    }

    void TextureBuilder::notify_task_done(std::unique_ptr<ITask> task) {
        auto& task_result = *reinterpret_cast<Task_LoadImage*>(task.get());
        const auto found = this->m_waiting_file.find(task_result.m_respath.make_str());

        if (this->m_waiting_file.end() != found) {
            this->m_renderer->init(
                *found->second.get(),
                task_result.out_image_data.value()
            );

            this->m_waiting_file.erase(found);
        }
    }

    void TextureBuilder::start(const ResPath& respath, HTexture h_texture, Filesystem& filesys, TaskManager& task_man) {
        dalAssert(nullptr != this->m_renderer);

        auto task = std::make_unique<::Task_LoadImage>(respath, filesys);
        task_man.order_task(std::move(task), this);
        auto [iter, success] = this->m_waiting_file.emplace(respath.make_str(), h_texture);
    }

}


// ModelBuilder
namespace dal {

    void ModelBuilder::update() {
        for (auto iter = this->m_waiting_prepare.begin(); iter < this->m_waiting_prepare.end();) {
            auto& model = **iter;

            if (model.is_ready()) {
                iter = this->m_waiting_prepare.erase(iter);
            }
            else {
                if (this->m_renderer->prepare(model))
                    return;

                ++iter;
            }
        }
    }

    void ModelBuilder::set_renderer(IRenderer& renderer) {
        this->m_renderer = &renderer;
    }

    void ModelBuilder::invalidate_renderer() {
        this->m_waiting_file.clear();
        this->m_waiting_prepare.clear();

        this->m_renderer = nullptr;
    }

    void ModelBuilder::notify_task_done(std::unique_ptr<ITask> task) {
        auto& task_result = *reinterpret_cast<Task_LoadModel*>(task.get());
        const auto found = this->m_waiting_file.find(task_result.m_respath.make_str());

        if (this->m_waiting_file.end() != found) {
            this->m_renderer->init(
                *found->second.get(),
                task_result.out_model_data.value(),
                task_result.m_respath.dir_list().front().c_str()
            );

            this->m_waiting_prepare.push_back(found->second);
            this->m_waiting_file.erase(found);
        }
    }

    void ModelBuilder::start(const ResPath& respath, HRenModel h_model, Filesystem& filesys, TaskManager& task_man) {
        dalAssert(nullptr != this->m_renderer);

        auto task = std::make_unique<::Task_LoadModel>(respath, filesys);
        task_man.order_task(std::move(task), this);
        auto [iter, success] = this->m_waiting_file.emplace(respath.make_str(), h_model);
    }

}


// ResourceManager
namespace dal {

    void ResourceManager::update() {
        this->m_tex_builder.update();
        this->m_model_builder.update();
    }

    void ResourceManager::set_renderer(IRenderer& renderer) {
        this->m_renderer = &renderer;

        this->m_tex_builder.set_renderer(renderer);
        this->m_model_builder.set_renderer(renderer);

        for (auto& x : this->m_textures) {

        }

        this->m_missing_tex = this->request_texture(::MISSING_TEX_PATH);
    }

    void ResourceManager::invalidate_renderer() {
        this->m_missing_tex.reset();

        this->m_model_builder.invalidate_renderer();
        this->m_tex_builder.invalidate_renderer();

        for (auto& x : this->m_textures)
            x.second->destroy();

        for (auto& x : this->m_models)
            x.second->destroy();

        for (auto& x : this->m_actors)
            x->destroy();

        this->m_renderer = nullptr;
    }

    HTexture ResourceManager::request_texture(const ResPath& respath) {
        const auto resolved_respath = this->m_filesys.resolve_respath(respath);
        if (!resolved_respath.has_value()) {
            dalError(fmt::format("Failed to find texture file: {}", respath.make_str()).c_str());
            dalAssert(!!this->m_missing_tex);
            return this->m_missing_tex;
        }

        const auto path_str = resolved_respath->make_str();
        const auto result = this->m_textures.find(path_str);

        if (this->m_textures.end() != result) {
            return result->second;
        }
        else {
            auto [iter, result] = this->m_textures.emplace(path_str, this->m_renderer->create_texture());
            dalAssert(result);
            this->m_tex_builder.start(*resolved_respath, iter->second, this->m_filesys, this->m_task_man);

            return iter->second;
        }
    }

    HRenModel ResourceManager::request_model(const ResPath& respath) {
        const auto resolved_respath = this->m_filesys.resolve_respath(respath);
        if (!resolved_respath.has_value())
            dalAbort(fmt::format("Failed to find model file: {}", respath.make_str()).c_str());

        const auto path_str = resolved_respath->make_str();
        const auto result = this->m_models.find(path_str);

        if (this->m_models.end() != result) {
            return result->second;
        }
        else {
            auto [iter, result] = this->m_models.emplace(path_str, this->m_renderer->create_model());
            dalAssert(result);
            this->m_model_builder.start(*resolved_respath, iter->second, this->m_filesys, this->m_task_man);

            return iter->second;
        }
    }

    HActor ResourceManager::request_actor() {
        this->m_actors.push_back(this->m_renderer->create_actor());
        this->m_renderer->init(*this->m_actors.back().get());
        return this->m_actors.back();
    }

}
