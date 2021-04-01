#pragma once

#include <vector>
#include <cstdint>
#include <optional>


namespace dal {

    enum class ImageFormat {
        r8g8b8a8_srgb,
    };

    class ImageData {

    private:
        std::vector<uint8_t> m_data;
        uint32_t m_width, m_height, m_channels;
        ImageFormat m_format;

    public:
        ImageData(
            const void* const data,
            const uint32_t width,
            const uint32_t height,
            const uint32_t channels,
            const ImageFormat format
        );

        void set(
            const void* const data,
            const uint32_t width,
            const uint32_t height,
            const uint32_t channels,
            const ImageFormat format
        );

        auto width() const {
            return this->m_width;
        }

        auto height() const {
            return this->m_height;
        }

        auto channels() const {
            return this->m_channels;
        }

        auto format() const {
            return this->m_format;
        }

        auto data_size() const {
            return this->m_data.size();
        }

        auto data() const {
            return this->m_data.data();
        }

    };


    std::optional<ImageData> parse_image_stb(const uint8_t* const buf, const size_t buf_size);

}
