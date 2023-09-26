#include "dal/util/image_parser.h"

#include <type_traits>

#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "dal/util/logger.h"


// ImageFormat
namespace dal {

    ImageData::ImageData(
        const void* const data,
        const uint32_t width,
        const uint32_t height,
        const uint32_t channels,
        const ImageFormat format
    ) {
        this->set(data, width, height, channels, format);
    }

    void ImageData::set(
        const void* const data,
        const uint32_t width,
        const uint32_t height,
        const uint32_t channels,
        const ImageFormat format
    ) {
        this->m_width = width;
        this->m_height = height;
        this->m_channels = 4;
        this->m_format = ImageFormat::r8g8b8a8_srgb;

        const auto image_data_size = static_cast<size_t>(width * height * 4);
        this->m_data.resize(image_data_size);
        memcpy(this->m_data.data(), data, image_data_size);
    }

}


namespace dal {

    std::optional<ImageData> parse_image_stb(const uint8_t* const buf, const size_t buf_size) {
        int width, height, channels;
        const auto pixels = stbi_load_from_memory(buf, buf_size, &width, &height, &channels, STBI_rgb_alpha);

        if (nullptr == pixels) return std::nullopt;
        dalAssert(nullptr != pixels);
        static_assert(std::is_same<uint8_t, stbi_uc>::value);

        ImageData output(pixels, width, height, 4, ImageFormat::r8g8b8a8_srgb);
        stbi_image_free(pixels);

        return output;
    }

}
