#pragma once

#include <dal/util/defines.h>
#include <dal/util/filesystem.h>

#include <android_native_app_glue.h>


namespace dal {

    class AssetManagerAndroid : public IAssetManager {

    private:
        AAssetManager* const m_asset_mgr_ptr;

    public:
        AssetManagerAndroid(AAssetManager* asset_mgr_ptr);

        bool is_file(const dal::ResPath& path) override;

        bool is_folder(const dal::ResPath& path) override;

        size_t list_files(const dal::ResPath& path, std::vector<std::string>& output) override;

        size_t list_folders(const dal::ResPath& path, std::vector<std::string>& output) override;

        std::optional<ResPath> resolve(const ResPath& path) override;

        std::unique_ptr<FileReadOnly> open(const dal::ResPath& path) override;

    };


    class UserDataManagerAndroid : public IUserDataManager {

    public:
        bool is_file(const dal::ResPath& path) override;

        bool is_folder(const dal::ResPath& path) override;

        size_t list_files(const dal::ResPath& path, std::vector<std::string>& output) override;

        size_t list_folders(const dal::ResPath& path, std::vector<std::string>& output) override;

        std::optional<ResPath> resolve(const ResPath& path) override;

        std::unique_ptr<FileReadOnly> open(const dal::ResPath& path) override;

    };


    class InternalManagerAndroid : public IInternalManager {

    private:
        std::string m_domain_dir;

    public:
        InternalManagerAndroid(const char* const domain_dir);

        bool is_file(const dal::ResPath& path) override;

        bool is_folder(const dal::ResPath& path) override;

        size_t list_files(const dal::ResPath& path, std::vector<std::string>& output) override;

        size_t list_folders(const dal::ResPath& path, std::vector<std::string>& output) override;

        std::optional<ResPath> resolve(const ResPath& path) override;

        std::unique_ptr<FileReadOnly> open_read(const ResPath& path) override;

        std::unique_ptr<IFileWriteOnly> open_write(const ResPath& path) override;

    };

}
