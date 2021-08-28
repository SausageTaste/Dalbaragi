#include "filesys_android.h"

#include <fstream>
#include <filesystem>

#include <d_konsts.h>


namespace fs = std::filesystem;


namespace {

    class FileReadOnly_STL : public dal::FileReadOnly {

    private:
        std::ifstream m_file;
        size_t m_size = 0;

    public:
        bool open(const fs::path& path) {
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


    class FileWriteOnly_STL : public dal::IFileWriteOnly {

    private:
        std::ofstream m_file;

    public:
        bool open(const fs::path& path) {
            this->close();

            this->m_file.open(path, std::ios::ate | std::ios::binary);
            return this->is_ready();
        }

        void close() override {
            this->m_file.close();
        }

        bool is_ready() override {
            return this->m_file.is_open();
        }

        bool write(const void* const data, const size_t data_size) override {
            this->m_file.write(reinterpret_cast<const char*>(data), data_size);
            return true;
        }

    };

}


namespace {

    class AssetFile {

    private:
        AAsset* m_asset = nullptr;
        size_t m_file_size = 0;

    public:
        AssetFile() = default;

        AssetFile(const char* const path, AAssetManager* const asset_mgr) noexcept {
            this->open(path, asset_mgr);
        }

        ~AssetFile() {
            this->close();
        }

        bool open(const char* const path, AAssetManager* const asset_mgr) noexcept {
            this->close();

            this->m_asset = AAssetManager_open(static_cast<AAssetManager*>(asset_mgr), path, AASSET_MODE_UNKNOWN);
            if (!this->is_ready())
                return false;

            this->m_file_size = static_cast<size_t>(AAsset_getLength64(this->m_asset));
            return true;
        }

        void close() {
            if (nullptr != this->m_asset)
                AAsset_close(this->m_asset);

            this->m_asset = nullptr;
            this->m_file_size = 0;
        }

        [[nodiscard]]
        bool is_ready() const noexcept {
            return nullptr != this->m_asset;
        }

        [[nodiscard]]
        size_t tell() const {
            const auto cur_pos = AAsset_getRemainingLength(this->m_asset);
            return this->m_file_size - static_cast<size_t>(cur_pos);
        }

        [[nodiscard]]
        size_t size() const {
            return this->m_file_size;
        }

        bool read(void* const dst, const size_t dst_size) {
            // Android asset manager implicitly read beyond file range WTF!!!
            const auto remaining = this->m_file_size - this->tell();
            const auto size_to_read = dst_size < remaining ? dst_size : remaining;
            if (size_to_read <= 0)
                return false;

            const auto read_bytes = AAsset_read(this->m_asset, dst, size_to_read);
            return read_bytes > 0;
        }

    };


    class FileReadOnly_AndroidAsset : public dal::FileReadOnly {

    private:
        AAssetManager* m_ptr_asset_manager = nullptr;
        ::AssetFile m_file;

    public:
        explicit
        FileReadOnly_AndroidAsset(AAssetManager* const ptr_asset_manager)
            : m_ptr_asset_manager(ptr_asset_manager)
        {

        }

        ~FileReadOnly_AndroidAsset() override {
            this->m_file.close();
            this->m_ptr_asset_manager = nullptr;
        }

        bool open(const char* const path) {
            return this->m_file.open(path, this->m_ptr_asset_manager);
        }

        void close() override {
            this->m_file.close();
        }

        bool read(void* const dst, const size_t dst_size) override {
            return this->m_file.read(dst, dst_size);
        }

        size_t size() override {
            return this->m_file.size();
        }

        bool is_ready() override {
            return this->m_file.is_ready();
        }

        size_t tell() {
            return this->m_file.tell();
        }

    };


    class AssetFileIterator {

    private:
        class Iterator {

        private:
            AAssetDir* m_dir = nullptr;
            const char* m_filename = nullptr;

        public:
            Iterator() = default;

            Iterator(const Iterator&) = delete;
            Iterator& operator=(const Iterator&) = delete;

        public:
            explicit
            Iterator(AAssetDir* const dir)
                : m_dir(dir)
                , m_filename(AAssetDir_getNextFileName(this->m_dir))
            {

            }

            ~Iterator() {
                if (nullptr != this->m_dir) {
                    AAssetDir_close(this->m_dir);
                    this->m_dir = nullptr;
                }
            }

            const char* operator*() const noexcept {
                return this->m_filename;
            }

            void operator++() {
                this->m_filename = AAssetDir_getNextFileName(this->m_dir);
            }

            bool operator!=(const Iterator& other) const noexcept {
                return nullptr != this->m_filename;
            }

        };

    private:
        AAssetManager* m_asset_mgr = nullptr;
        std::string m_path;

    public:
        AssetFileIterator() = delete;

        AssetFileIterator(AAssetManager* const asset_mgr, const std::string& path)
            : m_asset_mgr(asset_mgr)
            , m_path(path)
        {
            if (!this->m_path.empty() && this->m_path.back() == '/') {
                this->m_path.pop_back();
            }
        }

        [[nodiscard]]
        Iterator begin() const {
            return Iterator{ AAssetManager_openDir(this->m_asset_mgr, this->m_path.c_str()) };
        }

        [[nodiscard]]
        static Iterator end() {
            return Iterator{};
        }

    };


    class {

    private:
        class AssetFolderDef {

        public:
            struct PathNamePair {
                std::string m_path, m_name;
            };

        public:
            std::string m_name;
            std::vector<AssetFolderDef> m_sub_fol;

        public:
            const AssetFolderDef* get_sub_folder(const char* const folder_name) const {
                for (const auto& x : this->m_sub_fol) {
                    if (x.m_name == folder_name)
                        return &x;
                }

                return nullptr;
            }

            bool has_sub_folder(const char* const folder_name) const {
                return this->get_sub_folder(folder_name) != nullptr;
            }

            size_t make_fol_list(std::vector<std::string>& output) const {
                output.clear();

                for (const auto& x : this->m_sub_fol) {
                    output.push_back(x.m_name);
                }

                return output.size();
            }

            void put_all_sub_folders(std::vector<PathNamePair>& output, const char* const prefix) const {
                const std::string this_path = dal::join_path({prefix, this->m_name}, '/');
                output.push_back(PathNamePair{this_path, this->m_name});

                for (const auto& x : this->m_sub_fol) {
                    x.put_all_sub_folders(output, this_path.c_str());
                }
            }

            void put_all_sub_folders_files(std::vector<PathNamePair>& output, const char* const prefix, AAssetManager* const asset_mgr) const {
                const std::string this_path = dal::join_path({prefix, this->m_name}, '/');
                output.push_back(PathNamePair{this_path, this->m_name});

                for (const auto file_name : ::AssetFileIterator(asset_mgr, this_path)) {
                    output.push_back(PathNamePair{
                        dal::join_path({ this_path, file_name }, '/'),
                        file_name
                    });
                }

                for (const auto& x : this->m_sub_fol) {
                    x.put_all_sub_folders_files(output, this_path.c_str(), asset_mgr);
                }
            }

        };

        const AssetFolderDef ASSET_DIRECTORY {"", {
            {"glsl", {}},
            {"image", {
                {"honoka", {}},
                {"sponza", {}},
                {"\xED\x85\x8D\xEC\x8A\xA4\xEC\xB2\x98", {}},
            }},
            {"model", {}},
            {"script", {}},
            {"spv", {}},
        }};

    public:
        const AssetFolderDef* get_folder_info(const char* const path) const {
            if (0 == std::strlen(path)) {
                return &this->ASSET_DIRECTORY;
            }

            const auto path_split = dal::split_path(path);
            const AssetFolderDef* cur_dir = &this->ASSET_DIRECTORY;

            for (const auto& x : path_split) {
                auto next_dir = cur_dir->get_sub_folder(x.c_str());
                if (nullptr == next_dir) {
                    return nullptr;
                }
                else {
                    cur_dir = next_dir;
                }
            }

            return cur_dir;
        }

        bool is_folder(const char* const path) const {
            return this->get_folder_info(path) != nullptr;
        }

        // Output strings are only folder names
        size_t list_folder(const char* const path, std::vector<std::string>& output) const {
            auto next_dir = this->ASSET_DIRECTORY.get_sub_folder(path);
            if (nullptr != next_dir)
                return next_dir->make_fol_list(output);
            else
                return 0;
        }

        std::vector<std::string> list_folder(const char* const path) const {
            std::vector<std::string> output;
            this->list_folder(path, output);
            return output;
        }

        std::vector<AssetFolderDef::PathNamePair> walk_all_folders(const char* const path) const {
            std::vector<AssetFolderDef::PathNamePair> output;
            const auto dir_info = this->get_folder_info(path);
            if (nullptr == dir_info)
                return output;

            dir_info->put_all_sub_folders(output, path);
            return output;
        }

        std::vector<AssetFolderDef::PathNamePair> walk_all_folders_files(const char* const path, AAssetManager* const asset_mgr) const {
            std::vector<AssetFolderDef::PathNamePair> output;
            const auto dir_info = this->get_folder_info(path);
            if (nullptr == dir_info)
                return output;

            dir_info->put_all_sub_folders_files(output, path, asset_mgr);
            return output;
        }

    } g_asset_folders;


    auto make_asset_path(const dal::ResPath& path) {
        return dal::join_path(&path.dir_list().front() + 1, &path.dir_list().back() + 1, '/');
    }

    std::optional<std::string> convert_to_internal_path(const dal::ResPath& path, const std::string& domain_folder) {
        if (dal::SPECIAL_NAMESPACE_INTERNAL != path.dir_list().front())
            return std::nullopt;

        const auto target_path = dal::join_path(&path.dir_list().front() + 1, &path.dir_list().back() + 1, '/');
        return domain_folder + '/' + target_path;
    }

    bool is_file_asset(const char* const path, AAssetManager* const asset_mgr) {
        ::AssetFile file{ path, asset_mgr };
        return file.is_ready();
    }

}


// Resolve functions
namespace {

    std::optional<std::string> resolve_question_asset_path(
        const std::string& domain_dir,
        const std::string& entry_to_find,
        AAssetManager* const asset_mgr
    ) {
        if (!g_asset_folders.is_folder(domain_dir.c_str()))
            return std::nullopt;

        for (const auto& e : g_asset_folders.walk_all_folders_files(domain_dir.c_str(), asset_mgr)) {
            if (e.m_name == entry_to_find) {
                return e.m_path;
            }
        }

        return std::nullopt;
    }

    std::optional<std::string> resolve_asterisk_asset_path(const std::string& domain_dir, const std::string& entry_to_find) {
        if (!g_asset_folders.is_folder(domain_dir.c_str()))
            return std::nullopt;

        for (auto& folder0 : g_asset_folders.list_folder(domain_dir.c_str())) {
            const auto entry0 = dal::join_path({ domain_dir, folder0 }, '/');
            for (auto& folder1 : g_asset_folders.list_folder(entry0.c_str())) {
                if (folder1 == entry_to_find) {
                    return dal::join_path({ entry0, folder1 }, '/');
                }
            }
        }

        return std::nullopt;
    }

    std::optional<dal::ResPath> resolve_asset_path(const dal::ResPath& respath, AAssetManager* const asset_mgr) {
        if (respath.dir_list().front() != dal::SPECIAL_NAMESPACE_ASSET)
            return std::nullopt;

        std::string cur_path;

        for (size_t i = 1; i < respath.dir_list().size(); ++i) {
            const auto dir_element = respath.dir_list().at(i);

            if (dir_element == "?") {
                const auto resolve_result = ::resolve_question_asset_path(cur_path, respath.dir_list().at(i + 1), asset_mgr);
                if (!resolve_result.has_value()) {
                    return std::nullopt;
                }
                else {
                    cur_path = resolve_result.value();
                    ++i;
                }
            }
            else if (dir_element == "*") {
                const auto resolve_result = ::resolve_asterisk_asset_path(cur_path, respath.dir_list().at(i + 1));
                if (!resolve_result.has_value()) {
                    return std::nullopt;
                }
                else {
                    cur_path = resolve_result.value();
                    ++i;
                }
            }
            else {
                cur_path = dal::join_path({ cur_path, dir_element }, '/');
            }
        }

        if (!::is_file_asset(cur_path.c_str(), asset_mgr))
            return std::nullopt;

        return dal::ResPath{ dal::join_path({dal::SPECIAL_NAMESPACE_ASSET, cur_path}, '/') };
    }

    std::optional<dal::ResPath> resolve_userdata_path(const dal::ResPath& respath) {
        return std::nullopt;
    }

}


// AssetManagerAndroid
namespace dal {

    AssetManagerAndroid::AssetManagerAndroid(AAssetManager* const asset_mgr_ptr)
        : m_asset_mgr_ptr(asset_mgr_ptr)
    {

    }

    bool AssetManagerAndroid::is_file(const dal::ResPath& path) {
        const auto asset_path = ::make_asset_path(path);
        return ::is_file_asset(asset_path.c_str(), this->m_asset_mgr_ptr);
    }

    bool AssetManagerAndroid::is_folder(const dal::ResPath& path) {
        const auto asset_path = ::make_asset_path(path);
        return g_asset_folders.is_folder(asset_path.c_str());
    }

    size_t AssetManagerAndroid::list_files(const dal::ResPath& path, std::vector<std::string>& output) {
        const auto asset_path = ::make_asset_path(path);
        ::AssetFileIterator iter{ this->m_asset_mgr_ptr, asset_path };

        for (auto x : iter) {
            output.push_back(x);
        }

        return output.size();
    }

    size_t AssetManagerAndroid::list_folders(const dal::ResPath& path, std::vector<std::string>& output) {
        const auto asset_path = ::make_asset_path(path);
        g_asset_folders.list_folder(asset_path.c_str(), output);
        return output.size();
    }

    std::optional<ResPath> AssetManagerAndroid::resolve(const ResPath& path) {
        return ::resolve_asset_path(path, this->m_asset_mgr_ptr);
    }

    std::unique_ptr<FileReadOnly> AssetManagerAndroid::open(const dal::ResPath& path) {
        if (path.dir_list().front() != dal::SPECIAL_NAMESPACE_ASSET)
            return make_file_read_only_null();
        if (path.dir_list().size() < 2)
            return make_file_read_only_null();

        const auto asset_path = ::make_asset_path(path);
        auto file = std::make_unique<::FileReadOnly_AndroidAsset>(this->m_asset_mgr_ptr);
        file->open(asset_path.c_str());

        if (!file->is_ready())
            return dal::make_file_read_only_null();
        else
            return file;
    }

}


// UserDataManagerAndroid
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


// InternalManagerAndroid
namespace dal {

    InternalManagerAndroid::InternalManagerAndroid(const char* const domain_dir)
        : m_domain_dir(domain_dir)
    {

    }

    bool InternalManagerAndroid::is_file(const dal::ResPath& path) {
        const auto normal_path = ::convert_to_internal_path(path, this->m_domain_dir);
        if (!normal_path.has_value())
            return false;

        return fs::is_regular_file(*normal_path);
    }

    bool InternalManagerAndroid::is_folder(const dal::ResPath& path) {
        const auto normal_path = ::convert_to_internal_path(path, this->m_domain_dir);
        if (!normal_path.has_value())
            return false;

        return fs::is_directory(*normal_path);
    }

    size_t InternalManagerAndroid::list_files(const dal::ResPath& path, std::vector<std::string>& output) {
        const auto normal_path = ::convert_to_internal_path(path, this->m_domain_dir);
        if (!normal_path.has_value())
            return 0;

        output.clear();
        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_regular_file(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    size_t InternalManagerAndroid::list_folders(const dal::ResPath& path, std::vector<std::string>& output) {
        const auto normal_path = ::convert_to_internal_path(path, this->m_domain_dir);
        if (!normal_path.has_value())
            return 0;

        output.clear();
        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_directory(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    std::optional<ResPath> InternalManagerAndroid::resolve(const ResPath& path) {
        return std::nullopt;
    }

    std::unique_ptr<FileReadOnly> InternalManagerAndroid::open_read(const ResPath& path) {
        const auto path_converted = ::convert_to_internal_path(path, this->m_domain_dir);
        if (!path_converted.has_value())
            return make_file_read_only_null();

        auto file = std::make_unique<::FileReadOnly_STL>();
        file->open(*path_converted);

        if (!file->is_ready())
            return make_file_read_only_null();
        else
            return file;
    }

    std::unique_ptr<IFileWriteOnly> InternalManagerAndroid::open_write(const ResPath& path) {
        const auto path_converted = ::convert_to_internal_path(path, this->m_domain_dir);
        if (!path_converted.has_value())
            return make_file_write_only_null();

        auto file = std::make_unique<::FileWriteOnly_STL>();
        file->open(*path_converted);

        if (!file->is_ready())
            return make_file_write_only_null();
        else
            return file;
    }

}
