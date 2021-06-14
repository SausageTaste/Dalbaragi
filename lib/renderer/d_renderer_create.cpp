#include "d_renderer_create.h"


namespace {

    class NullRenderer : public dal::IRenderer {

    public:
        void update(const dal::ICamera& camera) override {

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

}
