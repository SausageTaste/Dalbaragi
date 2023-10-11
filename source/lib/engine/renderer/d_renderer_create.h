#pragma once

#include <memory>
#include <functional>

#include "dal/util/filesystem.h"
#include "dal/util/task_thread.h"
#include "d_renderer.h"


namespace dal {

    std::unique_ptr<IRenderer> create_renderer_null();

    std::unique_ptr<IRenderer> create_renderer_vulkan(
        const char* const window_title,
        const unsigned init_width,
        const unsigned init_height,
        const dal::RendererConfig& config,
        dal::Filesystem& filesys,
        dal::TaskManager& task_man,
        dal::ITextureManager& texture_man,
        const std::vector<const char*>& extensions,
        surface_create_func_t surface_create_func
    );

}
