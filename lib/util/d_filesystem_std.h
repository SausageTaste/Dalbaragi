#pragma once

#include "d_filesystem2.h"


namespace dal {

    class AssetManagerSTD : public IAssetManager {

    public:
        bool is_file(const dal::ResPath& path) override;

        bool is_folder(const dal::ResPath& path) override;

        size_t list_files(const dal::ResPath& path, std::vector<std::string>& output) override;

        size_t list_folders(const dal::ResPath& path, std::vector<std::string>& output) override;

        std::unique_ptr<FileReadOnly> open(const dal::ResPath& path) override;

    };

}
