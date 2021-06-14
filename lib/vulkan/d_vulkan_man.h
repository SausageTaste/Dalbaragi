#pragma once

#include <vector>
#include <functional>

#include "d_renderer.h"
#include "d_filesystem.h"
#include "d_actor.h"
#include "d_task_thread.h"


namespace dal {

    class VulkanState : public IRenderer {

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
            dal::Filesystem& filesys,
            dal::TaskManager& task_man,
            const std::vector<const char*>& extensions,
            std::function<void*(void*)> surface_create_func
        );

        void destroy();

        bool is_ready() const;

        void update(const ICamera& camera);

        void wait_device_idle() const;

        void on_screen_resize(const unsigned width, const unsigned height);

    };

}
