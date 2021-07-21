#include "d_filesystem.h"

#include <array>
#include <codecvt>
#include <fstream>

#include <fmt/format.h>

#include "d_konsts.h"

#if defined(DAL_OS_WINDOWS)
    #include <filesystem>
    #include <Shlobj.h>

    #define DAL_STD_FILESYSTEM

#elif defined(DAL_OS_LINUX)
    #include <filesystem>
    #include <unistd.h>

    #define DAL_STD_FILESYSTEM

#elif defined(DAL_OS_ANDROID)
    #include <android/asset_manager.h>
    #include <android/asset_manager_jni.h>

#endif


namespace fs = std::filesystem;


// Constants
namespace {

    const char* FOLDER_NAME_ASSET = "asset";

    const char* SPECIAL_NAMESPACE_ASSET = "_asset";
    const char* SPECIAL_NAMESPACE_LOG = "_log";

}


// Common functions
namespace {

    std::optional<std::u16string> utf8_to_utf16(const std::string& src) {
        if (src.empty())
            return std::u16string{};
        if (src.length() > static_cast<size_t>((std::numeric_limits<int>::max)()))  // windows.h defines min, max macro
            return std::nullopt;

        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        return converter.from_bytes(src);
    }

    std::optional<std::string> utf16_to_utf8(const std::u16string& src) {
        static_assert(1 == sizeof(std::string::value_type));

        if (src.empty())
            return std::string{};
        if (src.length() > static_cast<size_t>((std::numeric_limits<int>::max)()))  // windows.h defines min, max macro
            return std::nullopt;

        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        return converter.to_bytes(src);
    }


    bool is_char_separator(const char c) {
        return '\\' == c || '/' == c;
    }

    bool is_char_wildcard(const char c) {
        if ('?' == c)
            return true;
        else if ('*' == c)
            return true;
        else
            return false;
    }

    bool is_str_wildcard(const char* const str) {
        if ('\0' != str[1])
            return false;
        else
            return ::is_char_wildcard(str[0]);
    }

    bool is_valid_folder_name(const std::string& name) {
        const std::array<char, 11> INVALID_CHAR{
            ':',
            '/',
            '<',
            '>',
            '"',
            '/',
            '\\',
            '|',
            '?',
            '*',
            '\0',
        };

        for (const auto c : INVALID_CHAR) {
            if (std::string::npos != name.find(c)) {
                return false;
            }
        }

        if (name.empty())
            return false;

        return true;
    }

    void split_path(const char* const path, std::vector<std::string>& output) {
        output.clear();
        output.emplace_back();

        for (auto ptr = path; *ptr != '\0'; ++ptr) {
            const auto c = *ptr;

            if (::is_char_separator(c)) {
                output.emplace_back();
            }
            else {
                output.back().push_back(c);
            }
        }
    }

    std::vector<std::string> split_path(const char* const path) {
        std::vector<std::string> output;
        split_path(path, output);
        return output;
    }

    template <typename _Iterator>
    std::string join_path(const _Iterator dir_list_begin, const _Iterator dir_list_end, const char separator) {
        std::string output;

        for (auto ptr = dir_list_begin; ptr != dir_list_end; ++ptr) {
            if (ptr->empty())
                continue;

            output += *ptr;
            output += separator;
        }

        if (!output.empty())
            output.pop_back();

        return output;
    }

    std::string join_path(const std::initializer_list<std::string>& dir_list, const char seperator) {
        return ::join_path(dir_list.begin(), dir_list.end(), seperator);
    }

    std::vector<std::string> remove_duplicate_question_marks(const std::vector<std::string>& list) {
        std::vector<std::string> output;
        output.reserve(list.size());
        output.push_back(list.front());

        for (size_t i = 1; i < list.size(); ++i) {
            if (list[i] == "?" && output.back() == "?") {
                continue;
            }
            else if (list[i] == "*" && output.back() == "?") {
                continue;
            }
            else if (list[i] == "?" && output.back() == "*") {
                output.back() = "?";
            }
            else {
                output.push_back(list[i]);
            }
        }

        return output;
    }


    class FileReadOnly_Null : public dal::filesystem::FileReadOnly {

    public:
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


// STD Filesystem functions
namespace {
#ifdef DAL_STD_FILESYSTEM
namespace stdfs {

}
#endif
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


    std::optional<std::filesystem::path> find_asset_dir() {
        std::filesystem::path cur_dir = ".";

        for (int i = 0; i < 16; ++i) {
            for (const auto& entry : std::filesystem::directory_iterator(cur_dir)) {
                if (entry.path().filename() == FOLDER_NAME_ASSET) {
                    return cur_dir / FOLDER_NAME_ASSET;
                }
            }

            cur_dir /= "..";
        }

        return std::nullopt;
    }

    std::optional<fs::path> find_userdata_dir() {

#if defined(DAL_OS_WINDOWS)
        WCHAR path[MAX_PATH];
        const auto result = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
        if (S_OK != result)
            return std::nullopt;

        auto output_path = fs::path{ path } / dal::APP_NAME;
#else
        char username_buf[128];
        getlogin_r(username_buf, 128);

        const fs::path output_path = fmt::format("/home/{}/Documents/{}", username_buf, dal::APP_NAME);
#endif
        if (!fs::is_directory(output_path))
            fs::create_directory(output_path);

        return output_path;
    }

    void listfile_asset(const char* const path, std::vector<std::string>& result) {
        const auto asset_dir = ::desktop::find_asset_dir();
        if (asset_dir.has_value()) {
            for (const auto& x : std::filesystem::directory_iterator(*asset_dir / path)) {
                result.push_back(x.path().u8string());
            }
        }
    }

}
#endif
}


// Android functions
namespace {
#ifdef DAL_OS_ANDROID
namespace android {

    class AssetFile {

    private:
        AAsset* m_asset = nullptr;
        size_t m_file_size = 0;

    public:
        AssetFile() = default;

        AssetFile(const char* const path, void* const asset_mgr) noexcept {
            this->open(path, asset_mgr);
        }

        ~AssetFile() {
            this->close();
        }

        bool open(const char* const path, void* const asset_mgr) noexcept {
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

        AssetFileIterator(void* const asset_mgr, const std::string& path)
            : m_asset_mgr(reinterpret_cast<AAssetManager*>(asset_mgr))
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
        Iterator end() const {
            return Iterator{};
        }

    };


    bool is_file_asset(const char* const path, void* asset_mgr) {
        ::android::AssetFile file{ path, asset_mgr };
        return file.is_ready();
    }


    class FileReadOnly_AndroidAsset : public dal::filesystem::FileReadOnly {

    private:
        AAssetManager* m_ptr_asset_manager = nullptr;
        ::android::AssetFile m_file;

    public:
        explicit
        FileReadOnly_AndroidAsset(void* const ptr_asset_manager)
            : m_ptr_asset_manager(reinterpret_cast<AAssetManager*>(ptr_asset_manager))
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

}
#endif
}


// ResPath
namespace dal {

    ResPath::ResPath(const char* const path)
        : m_src_str(path)
    {
        ::split_path(path, this->m_dir_list);
        this->m_dir_list = ::remove_duplicate_question_marks(this->m_dir_list);
    }

    std::string ResPath::make_str() const {
        return ::join_path(this->dir_list().begin(), this->dir_list().end(), this->SEPARATOR);
    }

    bool ResPath::is_valid() const {
        for (const auto& x : this->m_dir_list) {
            if (!::is_valid_folder_name(x) && !::is_str_wildcard(x.c_str()))
                return false;
        }

        return !::is_str_wildcard(this->m_dir_list.back().c_str());
    }

}


// Resolve resource path
namespace {

#if defined(DAL_OS_WINDOWS) || defined(DAL_OS_LINUX)

    std::optional<fs::path> resolve_question_path(const fs::path& domain_dir, const std::string& entry_to_find) {
        if (!std::filesystem::is_directory(domain_dir))
            return std::nullopt;

        for (auto& e : fs::recursive_directory_iterator(domain_dir)) {
            if (e.path().filename().u8string() == entry_to_find) {
                return e.path();
            }
        }

        return std::nullopt;
    }

    std::optional<fs::path> resolve_asterisk_path(const fs::path& domain_dir, const std::string& entry_to_find) {
        if (!std::filesystem::is_directory(domain_dir))
            return std::nullopt;

        for (auto& e0 : fs::directory_iterator(domain_dir)) {
            for (auto& e1 : fs::directory_iterator(e0.path())) {
                if (e1.path().filename().u8string() == entry_to_find) {
                    return e1.path();
                }
            }
        }

        return std::nullopt;
    }

    std::optional<std::string> resolve_path(const dal::ResPath& respath, const fs::path& start_dir, const size_t start_index) {
        auto cur_path = start_dir;

        for (size_t i = start_index; i < respath.dir_list().size(); ++i) {
            const auto dir_element = respath.dir_list().at(i);

            if (dir_element == "?") {
                const auto resolve_result = ::resolve_question_path(cur_path, respath.dir_list().at(i + 1));
                if (!resolve_result.has_value()) {
                    return std::nullopt;
                }
                else {
                    cur_path = resolve_result.value();
                    ++i;
                }
            }
            else if (dir_element == "*") {
                const auto resolve_result = ::resolve_asterisk_path(cur_path, respath.dir_list().at(i + 1));
                if (!resolve_result.has_value()) {
                    return std::nullopt;
                }
                else {
                    cur_path = resolve_result.value();
                    ++i;
                }
            }
            else {
                cur_path = cur_path / dir_element;
            }
        }

        if (!std::filesystem::is_regular_file(cur_path))
            return std::nullopt;

        return cur_path.generic_u8string().substr(start_dir.generic_u8string().size() + 1);
    }

    std::optional<dal::ResPath> resolve_asset_path(const dal::ResPath& respath) {
        if (respath.dir_list().front() != ::SPECIAL_NAMESPACE_ASSET)
            return std::nullopt;

        const auto start_dir = ::desktop::find_asset_dir();
        if (!start_dir.has_value())
            return std::nullopt;

        const auto result = ::resolve_path(respath, *start_dir, 1);
        if (!result.has_value())
            return std::nullopt;

        const auto res_path_str = std::string{} + ::SPECIAL_NAMESPACE_ASSET + '/' + result.value();
        return dal::ResPath{ res_path_str };
    }

    std::optional<dal::ResPath> resolve_userdata_path(const dal::ResPath& respath) {
        const auto start_dir = ::desktop::find_userdata_dir();
        if (!start_dir.has_value())
            return std::nullopt;

        const auto result = ::resolve_path(respath, *start_dir, 0);
        if (!result.has_value())
            return std::nullopt;

        return dal::ResPath{ result.value() };
    }

#elif defined(DAL_OS_ANDROID)

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

            [[nodiscard]]
            std::vector<std::string> make_fol_list() const {
                std::vector<std::string> output;
                output.reserve(this->m_sub_fol.size());

                for (const auto& x : this->m_sub_fol) {
                    output.push_back(x.m_name);
                }

                return output;
            }

            void put_all_sub_folders(std::vector<PathNamePair>& output, const char* const prefix) const {
                const std::string this_path = ::join_path({prefix, this->m_name}, '/');
                output.push_back(PathNamePair{this_path, this->m_name});

                for (const auto& x : this->m_sub_fol) {
                    x.put_all_sub_folders(output, this_path.c_str());
                }
            }

            void put_all_sub_folders_files(std::vector<PathNamePair>& output, const char* const prefix, void* const asset_mgr) const {
                const std::string this_path = ::join_path({prefix, this->m_name}, '/');
                output.push_back(PathNamePair{this_path, this->m_name});

                for (const auto file_name : ::android::AssetFileIterator(asset_mgr, this_path)) {
                    output.push_back(PathNamePair{
                        ::join_path({ this_path, file_name }, '/'),
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
            {"spv", {}},
        }};

    public:
        const AssetFolderDef* get_folder_info(const char* const path) const {
            if (0 == std::strlen(path)) {
                return &this->ASSET_DIRECTORY;
            }

            const auto path_split = ::split_path(path);
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
        std::vector<std::string> list_folder(const char* const path) const {
            auto next_dir = this->ASSET_DIRECTORY.get_sub_folder(path);
            if (nullptr != next_dir) {
                return next_dir->make_fol_list();
            }
            else {
                return {};
            }
        }

        std::vector<AssetFolderDef::PathNamePair> walk_all_folders(const char* const path) const {
            std::vector<AssetFolderDef::PathNamePair> output;
            const auto dir_info = this->get_folder_info(path);
            if (nullptr == dir_info)
                return output;

            dir_info->put_all_sub_folders(output, path);
            return output;
        }

        std::vector<AssetFolderDef::PathNamePair> walk_all_folders_files(const char* const path, void* const asset_mgr) const {
            std::vector<AssetFolderDef::PathNamePair> output;
            const auto dir_info = this->get_folder_info(path);
            if (nullptr == dir_info)
                return output;

            dir_info->put_all_sub_folders_files(output, path, asset_mgr);
            return output;
        }

    } g_asset_folders;


    std::optional<std::string> resolve_question_asset_path(
        const std::string& domain_dir,
        const std::string& entry_to_find,
        void* asset_mgr
    ) {
        if ( !g_asset_folders.is_folder(domain_dir.c_str()) )
            return std::nullopt;

        for (const auto& e : g_asset_folders.walk_all_folders_files(domain_dir.c_str(), asset_mgr)) {
            if (e.m_name == entry_to_find) {
                return e.m_path;
            }
        }

        return std::nullopt;
    }

    std::optional<std::string> resolve_asterisk_asset_path(const std::string& domain_dir, const std::string& entry_to_find) {
        if ( !g_asset_folders.is_folder(domain_dir.c_str()) )
            return std::nullopt;

        for (auto& folder0 : g_asset_folders.list_folder(domain_dir.c_str())) {
            const auto entry0 = ::join_path({ domain_dir, folder0 }, '/');
            for (auto& folder1 : g_asset_folders.list_folder(entry0.c_str())) {
                if (folder1 == entry_to_find) {
                    return ::join_path({ entry0, folder1 }, '/');
                }
            }
        }

        return std::nullopt;
    }


    std::optional<dal::ResPath> resolve_asset_path(const dal::ResPath& respath, void* const asset_mgr) {
        if (respath.dir_list().front() != ::SPECIAL_NAMESPACE_ASSET)
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
                cur_path = ::join_path({ cur_path, dir_element }, '/');
            }
        }

        if (!android::is_file_asset(cur_path.c_str(), asset_mgr))
            return std::nullopt;

        return dal::ResPath{ ::join_path({::SPECIAL_NAMESPACE_ASSET, cur_path}, '/') };
    }

    std::optional<dal::ResPath> resolve_userdata_path(const dal::ResPath& respath) {
        return std::nullopt;
    }

#endif

}


// AssetManager
namespace dal::filesystem {

    std::vector<std::string> AssetManager::listfile(const ResPath& respath) {
        std::vector<std::string> result;

        if (respath.dir_list().front() != ::SPECIAL_NAMESPACE_ASSET)
            return result;
        if (respath.dir_list().size() < 2)
            return result;

        const auto asset_path = ::join_path(&respath.dir_list().front() + 1, &respath.dir_list().back() + 1, '/');

#if defined(DAL_OS_WINDOWS) || defined(DAL_OS_LINUX)
        const auto asset_dir = ::desktop::find_asset_dir();
        if (!asset_dir.has_value())
            return result;

        ::desktop::listfile_asset((*asset_dir / asset_path).string().c_str(), result);

#elif defined(DAL_OS_ANDROID)
        for (const auto x : ::android::AssetFileIterator{ this->get_android_asset_manager(), asset_path }) {
            result.push_back(std::string{ x });
        }

#endif

        return result;
    }

    std::unique_ptr<FileReadOnly> AssetManager::open(const ResPath& respath) {
        if (respath.dir_list().front() != ::SPECIAL_NAMESPACE_ASSET)
            return std::make_unique<::FileReadOnly_Null>();
        if (respath.dir_list().size() < 2)
            return std::make_unique<::FileReadOnly_Null>();

        const auto asset_path = ::join_path(&respath.dir_list().front() + 1, &respath.dir_list().back() + 1, '/');

#if defined(DAL_OS_WINDOWS)
        const auto asset_dir = ::desktop::find_asset_dir();
        if (!asset_dir.has_value())
            return std::make_unique<::FileReadOnly_Null>();

        const auto file_path = *asset_dir / ::utf8_to_utf16(asset_path).value();
        auto file = std::make_unique<::desktop::FileReadOnly_STL>();
        file->open(file_path);

#elif defined(DAL_OS_LINUX)
        const auto asset_dir = ::desktop::find_asset_dir();
        if (!asset_dir.has_value())
            return std::make_unique<::FileReadOnly_Null>();

        const auto file_path = *asset_dir / asset_path;
        auto file = std::make_unique<::desktop::FileReadOnly_STL>();
        file->open(file_path);

#elif defined(DAL_OS_ANDROID)
        auto file = std::make_unique<::android::FileReadOnly_AndroidAsset>(this->m_ptr_asset_manager);
        file->open(asset_path.c_str());

#endif

        if (!file->is_ready())
            return std::unique_ptr<FileReadOnly>{ new FileReadOnly_Null };
        else
            return file;
    }

}


// UserDataManager
namespace dal::filesystem {

    std::unique_ptr<FileReadOnly> UserDataManager::open(const dal::ResPath& respath) {
        if (respath.dir_list().size() < 2)
            return std::make_unique<::FileReadOnly_Null>();

        const auto userdata_path = ::join_path(&respath.dir_list().front(), &respath.dir_list().back() + 1, '/');

#if defined(DAL_OS_WINDOWS)
        const auto userdata_dir = ::desktop::find_userdata_dir();
        if (!userdata_dir.has_value())
            return std::make_unique<::FileReadOnly_Null>();

        const auto file_path = userdata_dir.value() / ::utf8_to_utf16(userdata_path).value();
        auto file = std::make_unique<::desktop::FileReadOnly_STL>();
        file->open(file_path);

#elif defined(DAL_OS_LINUX)
        const auto userdata_dir = ::desktop::find_userdata_dir();
        if (!userdata_dir.has_value())
            return std::make_unique<::FileReadOnly_Null>();

        const auto file_path = userdata_dir.value() / userdata_path;
        auto file = std::make_unique<::desktop::FileReadOnly_STL>();
        file->open(file_path);

#else
        auto file = std::make_unique<FileReadOnly_Null>();

#endif

        if (!file->is_ready())
            return std::make_unique<FileReadOnly_Null>();
        else
            return file;
    }

}


namespace dal {

    std::optional<dal::ResPath> Filesystem::resolve_respath(const dal::ResPath& respath) {
        if (!respath.is_valid())
            return std::nullopt;

        if (respath.dir_list().front() == ::SPECIAL_NAMESPACE_ASSET) {
#ifdef DAL_OS_ANDROID
            return ::resolve_asset_path(respath, this->asset_mgr().get_android_asset_manager());
#else
            return ::resolve_asset_path(respath);
#endif
        }
        else if (respath.dir_list().front() == "?") {
#ifdef DAL_OS_ANDROID
            auto result_asset = ::resolve_asset_path(respath, this->asset_mgr().get_android_asset_manager());
#else
            auto result_asset = ::resolve_asset_path(respath);
#endif
            if (result_asset.has_value())
                return result_asset;

            auto result_userdata = ::resolve_userdata_path(respath);
            if (result_userdata.has_value())
                return result_userdata;
        }
        else {
            return ::resolve_userdata_path(respath);
        }

        return std::nullopt;
    }

    std::unique_ptr<dal::filesystem::FileReadOnly> Filesystem::open(const dal::ResPath& respath) {
        if (!respath.is_valid())
            return std::unique_ptr<dal::filesystem::FileReadOnly>{ new FileReadOnly_Null };

        if (respath.dir_list().front() == ::SPECIAL_NAMESPACE_ASSET) {
            return this->asset_mgr().open(respath);
        }
        else {
            return this->m_ud_man.open(respath);
        }

        return std::unique_ptr<dal::filesystem::FileReadOnly>{ new FileReadOnly_Null };
    }

}
