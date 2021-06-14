#include "d_resource_man.h"

#include <fmt/format.h>

#include "d_timer.h"
#include "d_logger.h"
#include "d_image_parser.h"


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

}


namespace dal {

    void ResourceManager::set_renderer(IRenderer& renderer) {
        this->m_renderer = &renderer;
    }

    void ResourceManager::invalidate_renderer() {
        this->m_renderer = nullptr;
    }

    HTexture ResourceManager::request_texture(const ResPath respath) {
        const auto resolved_respath = this->m_filesys.resolve_respath(respath);
        if (!resolved_respath.has_value())
            dalAbort(fmt::format("Failed to find texture file: {}", respath.make_str()).c_str());
        const auto path_str = resolved_respath->make_str();

        const auto result = this->m_textures.find(path_str);
        if (this->m_textures.end() != result) {
            return result->second;
        }
        else {
            auto [iter, result] = this->m_textures.emplace(path_str, this->m_renderer->create_texture());
            dalAssert(result);

            auto task = std::make_unique<::Task_LoadImage>(resolved_respath.value(), this->m_filesys);
            task->run();

            this->m_renderer->init_texture(*iter->second.get(), task->out_image_data.value());
            return iter->second;
        }
    }

}
