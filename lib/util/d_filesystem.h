#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>


namespace dal::filesystem {

    class FileReadOnly {

    public:
        virtual ~FileReadOnly() = default;

        virtual bool open(const char* const path) = 0;

        virtual void close() = 0;

        virtual size_t size() = 0;

        virtual bool read(void* const dst, const size_t dst_size) = 0;

        template <typename T>
        bool read_stl(T& output) {
            output.resize(this->size());
            return this->read(output.data(), output.size());
        }

        template <typename T>
        std::optional<T> read_stl() {
            T output{};
            output.resize(this->size());
            return this->read_stl(output) ? std::optional<T>{output} : std::nullopt;
        }

    };

}


namespace dal::filesystem::asset {

    std::vector<std::string> listfile(const char* const path);

    std::unique_ptr<FileReadOnly> open(const char* const path);

}
