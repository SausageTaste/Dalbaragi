#pragma once

#include "d_defines.h"

#if defined(DAL_OS_WINDOWS) || defined(DAL_OS_LINUX)

#include "d_filesystem.h"


namespace dal {

    class AssetManagerSTD : public IAssetManager {

    public:
        bool is_file(const dal::ResPath& path) override;

        bool is_folder(const dal::ResPath& path) override;

        size_t list_files(const dal::ResPath& path, std::vector<std::string>& output) override;

        size_t list_folders(const dal::ResPath& path, std::vector<std::string>& output) override;

        std::optional<ResPath> resolve(const ResPath& path) override;

        std::unique_ptr<FileReadOnly> open(const dal::ResPath& path) override;

    };


    class UserDataManagerSTD : public IUserDataManager {

    public:
        bool is_file(const dal::ResPath& path) override;

        bool is_folder(const dal::ResPath& path) override;

        size_t list_files(const dal::ResPath& path, std::vector<std::string>& output) override;

        size_t list_folders(const dal::ResPath& path, std::vector<std::string>& output) override;

        std::optional<ResPath> resolve(const ResPath& path) override;

        std::unique_ptr<FileReadOnly> open(const dal::ResPath& path) override;

    };


    class InternalManagerSTD : public IInternalManager {

    public:
        bool is_file(const dal::ResPath& path) override;

        bool is_folder(const dal::ResPath& path) override;

        size_t list_files(const dal::ResPath& path, std::vector<std::string>& output) override;

        size_t list_folders(const dal::ResPath& path, std::vector<std::string>& output) override;

        std::optional<ResPath> resolve(const ResPath& path) override;

        std::unique_ptr<FileReadOnly> open_read(const ResPath& path) override;

        std::unique_ptr<IFileWriteOnly> open_write(const ResPath& path) override;

    };

}

#endif
