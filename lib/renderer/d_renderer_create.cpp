#include "d_renderer_create.h"

#include "d_vulkan_man.h"


namespace {

    class NullRenderer : public dal::IRenderer {

    private:
        dal::FrameInFlightIndex m_no_one_cares_about_this;

    public:
        void update(const dal::ICamera& camera, dal::RenderList& render_list) override {}

        const dal::FrameInFlightIndex& in_flight_index() const override {
            return this->m_no_one_cares_about_this;
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
        dal::ITextureManager& texture_man,
        const std::vector<const char*>& extensions,
        surface_create_func_t surface_create_func
    ) {
        return std::unique_ptr<IRenderer>(new dal::VulkanState(
            window_title,
            init_width,
            init_height,
            filesys,
            task_man,
            texture_man,
            extensions,
            surface_create_func
        ));
    }

}
