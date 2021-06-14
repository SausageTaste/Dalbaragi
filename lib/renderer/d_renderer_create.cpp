#include "d_renderer_create.h"

#include "d_vulkan_man.h"


namespace {

    class NullRenderer : public dal::IRenderer {

    public:
        void update(const dal::ICamera& camera, const dal::RenderList& render_list) override {

        }

        void wait_idle() override {

        }

        void on_screen_resize(const unsigned width, const unsigned height) override {

        }

    };

}


namespace dal {

    std::unique_ptr<IRenderer> create_renderer_null() {
        return std::make_unique<NullRenderer>();
    }

    std::unique_ptr<IRenderer> create_renderer_vulkan(
        const char* const window_title,
        const unsigned init_width,
        const unsigned init_height,
        dal::Filesystem& filesys,
        dal::TaskManager& task_man,
        const std::vector<const char*>& extensions,
        std::function<void*(void*)> surface_create_func
    ) {
        return std::unique_ptr<IRenderer>(new dal::VulkanState(
            window_title,
            init_width,
            init_height,
            filesys,
            task_man,
            extensions,
            surface_create_func
        ));
    }

}
