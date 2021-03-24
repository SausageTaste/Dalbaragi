#include "d_filesystem.h"

#include <filesystem>
#include <fstream>

#include "d_logger.h"


// Constants
namespace {

    const char* FOLDER_NAME_ASSET = "asset";

}


// Common functions
namespace {

    class FileReadOnly_Null : public dal::filesystem::FileReadOnly {

    public:
        bool open(const char* const path) override {
            return false;
        }

        void close() override {

        }

        bool is_ready() override {
            return false;
        }

        size_t size() override {
            return 0;
        }

        bool read(void* const dst, const size_t dst_size) override {
            return false;
        }

    };

}


// Desktop functions (Windows and Linux)
namespace {

    class FileReadOnly_STL : public dal::filesystem::FileReadOnly {

    private:
        std::ifstream m_file;
        size_t m_size = 0;

    public:
        bool open(const char* const path) override {
            this->close();

            this->m_file.open(path, std::ios::ate | std::ios::binary);
            if (!this->m_file)
                return false;

            this->m_size = this->m_file.tellg();
            return this->m_file.is_open();
        }

        void close() override {
            this->m_file.close();
            this->m_size = 0;
        }

        bool is_ready() override {
            return this->m_file.is_open();
        }

        size_t size() override {
            return this->m_size;
        }

        bool read(void* const dst, const size_t dst_size) override {
            this->m_file.seekg(0);
            this->m_file.read(reinterpret_cast<char*>(dst), dst_size);
            return true;
        }

    };


    std::filesystem::path find_asset_dir() {
        std::filesystem::path cur_dir = ".";

        for (int i = 0; i < 16; ++i) {
            for (const auto& entry : std::filesystem::directory_iterator(cur_dir)) {
                if (entry.path().filename() == FOLDER_NAME_ASSET) {
                    return cur_dir / FOLDER_NAME_ASSET;
                }
            }

            cur_dir /= "..";
        }

        dalAbort("Failed to find resource asset");
    }

    std::filesystem::path get_asset_dir() {
        static const auto path = ::find_asset_dir();
        return path;
    }

}


namespace dal::filesystem::asset {

    std::vector<std::string> listfile(const char* const path) {
        std::vector<std::string> result;

        for (const auto & entry : std::filesystem::directory_iterator(::get_asset_dir() / path)) {
            result.push_back(entry.path().filename().string());
        }

        return result;
    }

    std::unique_ptr<FileReadOnly> open(const char* const path) {
        const auto file_path = ::get_asset_dir() / path;
        std::unique_ptr<FileReadOnly> file{ new FileReadOnly_STL };

        if ( !file->open(file_path.string().c_str()) ) {
            return std::unique_ptr<FileReadOnly>{ new FileReadOnly_Null };
        }
        else {
            return file;
        }
    }

}
