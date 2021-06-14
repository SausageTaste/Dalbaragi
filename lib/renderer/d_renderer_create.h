#pragma once

#include <memory>

#include "d_renderer.h"

namespace dal {

    std::unique_ptr<IRenderer> create_renderer_null();

}
