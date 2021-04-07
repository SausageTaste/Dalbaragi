#pragma once

#include <vector>
#include <functional>

#include "d_filesystem.h"
#include "d_actor.h"


namespace dal {

    class VulkanState {

    private:
        class Pimpl;
        Pimpl* m_pimpl = nullptr;

    public:
        VulkanState() = default;

        ~VulkanState();

        void init(
            const char* const window_title,
            const unsigned init_width,
            const unsigned init_height,
            dal::filesystem::AssetManager& asset_mgr,
            const std::vector<const char*>& extensions,
            std::function<void*(void*)> surface_create_func
        );

        void destroy();

        void update(const EulerCamera& camera);

        void wait_device_idle() const;

        void on_screen_resize(const unsigned width, const unsigned height);

    };

}
