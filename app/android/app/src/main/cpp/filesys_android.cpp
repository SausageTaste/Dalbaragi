#include "filesys_android.h"


namespace dal {

    AssetManagerAndroid::AssetManagerAndroid(AAssetManager* const asset_mgr_ptr)
        : m_asset_mgr_ptr(asset_mgr_ptr)
    {

    }

    bool AssetManagerAndroid::is_file(const dal::ResPath& path) {
        return false;
    }

    bool AssetManagerAndroid::is_folder(const dal::ResPath& path) {
        return false;
    }

    size_t AssetManagerAndroid::list_files(const dal::ResPath& path, std::vector<std::string>& output) {
        return 0;
    }

    size_t AssetManagerAndroid::list_folders(const dal::ResPath& path, std::vector<std::string>& output) {
        return 0;
    }

    std::optional<ResPath> AssetManagerAndroid::resolve(const ResPath& path) {
        return std::nullopt;
    }

    std::unique_ptr<FileReadOnly> AssetManagerAndroid::open(const dal::ResPath& path) {
        return make_file_read_only_null();
    }

}


namespace dal {

    bool UserDataManagerAndroid::is_file(const dal::ResPath& path) {
        return false;
    }

    bool UserDataManagerAndroid::is_folder(const dal::ResPath& path) {
        return false;
    }

    size_t UserDataManagerAndroid::list_files(const dal::ResPath& path, std::vector<std::string>& output) {
        return 0;
    }

    size_t UserDataManagerAndroid::list_folders(const dal::ResPath& path, std::vector<std::string>& output) {
        return 0;
    }

    std::optional<ResPath> UserDataManagerAndroid::resolve(const ResPath& path) {
        return std::nullopt;
    }

    std::unique_ptr<FileReadOnly> UserDataManagerAndroid::open(const dal::ResPath& path) {
        return make_file_read_only_null();
    }

}
