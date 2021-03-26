#include "d_filesystem.h"

#include <fstream>

#include "d_logger.h"

#if defined(DAL_OS_WINDOWS) || defined(DAL_OS_LINUX)
    #include <filesystem>

#elif defined(DAL_OS_ANDROID)
    #include <android/asset_manager.h>
    #include <android/asset_manager_jni.h>

#endif


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
#if defined(DAL_OS_WINDOWS) || defined(DAL_OS_LINUX)
namespace desktop {

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
        static const auto path = ::desktop::find_asset_dir();
        return path;
    }

    void listfile(const char* const path, std::vector<std::string>& result) {
        for (const auto & entry : std::filesystem::directory_iterator(::desktop::get_asset_dir() / path)) {
            result.push_back(entry.path().filename().string());
        }
    }

}
#endif
}


// Android functions
namespace {
#ifdef DAL_OS_ANDROID
namespace android {

    size_t listfile_asset(std::string path, std::vector<std::string>& dirs, void* asset_mgr) {
        dirs.clear();
        if ( !path.empty() && path.back() == '/' ) {
            path.pop_back();
        }

        AAssetDir* assetDir = AAssetManager_openDir(reinterpret_cast<AAssetManager*>(asset_mgr), path.c_str());
        while ( true ) {
            auto filename = AAssetDir_getNextFileName(assetDir);
            if ( filename == nullptr ) {
                break;
            }
            dirs.emplace_back(filename);
        }
        AAssetDir_close(assetDir);

        return dirs.size();
    }


    class FileReadOnly_AndroidAsset : public dal::filesystem::FileReadOnly {

    private:
        AAssetManager* m_ptr_asset_manager = nullptr;
        AAsset* m_asset = nullptr;
        size_t m_fileSize = 0;

    public:
        FileReadOnly_AndroidAsset(void* const ptr_asset_manager)
            : m_ptr_asset_manager(reinterpret_cast<AAssetManager*>(ptr_asset_manager))
        {

        }

        ~FileReadOnly_AndroidAsset() override {
            this->close();
        }

        bool open(const char* const path) override {
            using namespace std::literals;

            this->close();

            this->m_asset = AAssetManager_open(this->m_ptr_asset_manager, path, AASSET_MODE_UNKNOWN);
            if ( nullptr == this->m_asset ) {
                return false;
            }

            this->m_fileSize = static_cast<size_t>(AAsset_getLength64(this->m_asset));
            if ( this->m_fileSize <= 0 ) {
                dalWarn(("File contents' length is 0 for: "s + path).c_str());
            }

            return true;
        }

        void close() override {
            if (this->is_ready())
                AAsset_close(this->m_asset);

            this->m_asset = nullptr;
            this->m_fileSize = 0;
        }

        bool read(void* const dst, const size_t dst_size) override {
            // Android asset manager implicitly read beyond file range WTF!!!
            const auto remaining = this->m_fileSize - this->tell();
            auto sizeToRead = dst_size < remaining ? dst_size : remaining;
            if (sizeToRead <= 0)
                return false;

            const auto readBytes = AAsset_read(this->m_asset, dst, sizeToRead);
            if ( readBytes < 0 ) {
                dalError("Failed to read asset.");
                return false;
            }
            else if ( 0 == readBytes ) {
                dalError("Tried to read after end of asset.");
                return false;
            }
            else {
                assert(readBytes == sizeToRead);
                return true;
            }
        }

        size_t size() override {
            return this->m_fileSize;
        }

        bool is_ready() override {
            return nullptr != this->m_asset;
        }

        size_t tell(void) {
            const auto curPos = AAsset_getRemainingLength(this->m_asset);
            return this->m_fileSize - static_cast<size_t>(curPos);
        }

    };

}
#endif
}


namespace dal::filesystem {

    std::vector<std::string> AssetManager::listfile(const char* const path) {
        std::vector<std::string> result;

#if defined(DAL_OS_WINDOWS) || defined(DAL_OS_LINUX)
        ::desktop::listfile(path, result);

#elif defined(DAL_OS_ANDROID)
        dalAssert(nullptr != this->m_ptr_asset_manager);
        ::android::listfile_asset(path, result, this->m_ptr_asset_manager);

#endif

        return result;
    }

    std::unique_ptr<FileReadOnly> AssetManager::open(const char* const path) {

#if defined(DAL_OS_WINDOWS) || defined(DAL_OS_LINUX)
        const auto file_path = ::desktop::get_asset_dir() / path;
        std::unique_ptr<dal::filesystem::FileReadOnly> file{ new ::desktop::FileReadOnly_STL };
        file->open(file_path.string().c_str());

#elif defined(DAL_OS_ANDROID)
        std::unique_ptr<FileReadOnly> file{ new ::android::FileReadOnly_AndroidAsset(this->m_ptr_asset_manager) };
        file->open(path);

#endif

        if ( !file->is_ready() ) {
            return std::unique_ptr<FileReadOnly>{ new FileReadOnly_Null };
        }
        else {
            return file;
        }
    }

}
