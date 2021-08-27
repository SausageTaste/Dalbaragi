#include "d_filesystem_std.h"

#include <codecvt>
#include <fstream>
#include <filesystem>

#include "d_defines.h"

#if defined(DAL_OS_WINDOWS)
    #include <Shlobj.h>

#elif defined(DAL_OS_LINUX)
    #include <unistd.h>

#endif

#include "d_konsts.h"


namespace fs = std::filesystem;


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


    std::optional<fs::path> find_asset_dir() {
        std::filesystem::path cur_dir = ".";

        for (int i = 0; i < 16; ++i) {
            for (const auto& entry : std::filesystem::directory_iterator(cur_dir)) {
                if (entry.path().filename() == dal::FOLDER_NAME_ASSET) {
                    return cur_dir / dal::FOLDER_NAME_ASSET;
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

    std::optional<fs::path> convert_asset_respath(const dal::ResPath& path) {
        const auto asset_dir = ::find_asset_dir();
        if (!asset_dir.has_value())
            return std::nullopt;

        const auto asset_path = ::join_path(&path.dir_list().front() + 1, &path.dir_list().back() + 1, '/');

#if defined(DAL_OS_WINDOWS)
        const auto file_path = *asset_dir / ::utf8_to_utf16(asset_path).value();
#elif defined(DAL_OS_LINUX)
        const auto file_path = *asset_dir / asset_path;
#endif

        return file_path;
    }

    std::optional<fs::path> convert_userdata_respath(const dal::ResPath& path) {
        const auto domain_dir = ::find_userdata_dir();
        if (!domain_dir.has_value())
            return std::nullopt;

        const auto target_path = ::join_path(&path.dir_list().front(), &path.dir_list().back() + 1, '/');

#if defined(DAL_OS_WINDOWS)
        return *domain_dir / ::utf8_to_utf16(target_path).value();
#elif defined(DAL_OS_LINUX)
        return *domain_dir / target_path;
#endif

    }


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

}


// Resolve functions
namespace {

    std::optional<fs::path> resolve_question_path(const fs::path& domain_dir, const std::string& entry_to_find) {
        if (!fs::is_directory(domain_dir))
            return std::nullopt;

        for (auto& e : fs::recursive_directory_iterator(domain_dir)) {
            if (e.path().filename().u8string() == entry_to_find) {
                return e.path();
            }
        }

        return std::nullopt;
    }

    std::optional<fs::path> resolve_asterisk_path(const fs::path& domain_dir, const std::string& entry_to_find) {
        if (!fs::is_directory(domain_dir))
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

        if (!fs::is_regular_file(cur_path))
            return std::nullopt;

        return cur_path.generic_u8string().substr(start_dir.generic_u8string().size() + 1);
    }

    std::optional<dal::ResPath> resolve_asset_path(const dal::ResPath& respath) {
        if (respath.dir_list().front() != dal::SPECIAL_NAMESPACE_ASSET)
            return std::nullopt;

        const auto start_dir = ::find_asset_dir();
        if (!start_dir.has_value())
            return std::nullopt;

        const auto result = ::resolve_path(respath, *start_dir, 1);
        if (!result.has_value())
            return std::nullopt;

        const auto res_path_str = std::string{} + dal::SPECIAL_NAMESPACE_ASSET + '/' + result.value();
        return dal::ResPath{ res_path_str };
    }

    std::optional<dal::ResPath> resolve_userdata_path(const dal::ResPath& respath) {
        const auto start_dir = ::find_userdata_dir();
        if (!start_dir.has_value())
            return std::nullopt;

        const auto result = ::resolve_path(respath, *start_dir, 0);
        if (!result.has_value())
            return std::nullopt;

        return dal::ResPath{ result.value() };
    }

}


// AssetManagerSTD
namespace dal {

    bool AssetManagerSTD::is_file(const ResPath& path) {
        const auto normal_path = ::convert_asset_respath(path);
        if (!normal_path.has_value())
            return 0;

        return fs::is_regular_file(*normal_path);
    }

    bool AssetManagerSTD::is_folder(const ResPath& path) {
        const auto normal_path = ::convert_asset_respath(path);
        if (!normal_path.has_value())
            return 0;

        return fs::is_directory(*normal_path);
    }

    size_t AssetManagerSTD::list_files(const ResPath& path, std::vector<std::string>& output) {
        output.clear();

        const auto normal_path = ::convert_asset_respath(path);
        if (!normal_path.has_value())
            return 0;

        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_regular_file(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    size_t AssetManagerSTD::list_folders(const ResPath& path, std::vector<std::string>& output) {
        output.clear();

        const auto normal_path = ::convert_asset_respath(path);
        if (!normal_path.has_value())
            return 0;

        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_directory(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    std::optional<ResPath> AssetManagerSTD::resolve(const ResPath& path) {
        return ::resolve_asset_path(path);
    }

    std::unique_ptr<FileReadOnly> AssetManagerSTD::open(const ResPath& path) {
        if (path.dir_list().front() != dal::SPECIAL_NAMESPACE_ASSET)
            return make_file_read_only_null();
        if (path.dir_list().size() < 2)
            return make_file_read_only_null();

        const auto file_path = ::convert_asset_respath(path);
        if (!file_path.has_value())
            return make_file_read_only_null();

        auto file = std::make_unique<::FileReadOnly_STL>();
        file->open(*file_path);

        if (!file->is_ready())
            return make_file_read_only_null();
        else
            return file;
    }

}


// UserDataManagerSTD
namespace dal {

    bool UserDataManagerSTD::is_file(const dal::ResPath& path) {
        const auto normal_path = ::convert_userdata_respath(path);
        if (!normal_path.has_value())
            return 0;

        return fs::is_regular_file(*normal_path);
    }

    bool UserDataManagerSTD::is_folder(const dal::ResPath& path) {
        const auto normal_path = ::convert_userdata_respath(path);
        if (!normal_path.has_value())
            return 0;

        return fs::is_directory(*normal_path);
    }

    size_t UserDataManagerSTD::list_files(const dal::ResPath& path, std::vector<std::string>& output) {
        output.clear();

        const auto normal_path = ::convert_userdata_respath(path);
        if (!normal_path.has_value())
            return 0;

        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_regular_file(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    size_t UserDataManagerSTD::list_folders(const dal::ResPath& path, std::vector<std::string>& output) {
        output.clear();

        const auto normal_path = ::convert_userdata_respath(path);
        if (!normal_path.has_value())
            return 0;

        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_directory(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    std::optional<ResPath> UserDataManagerSTD::resolve(const ResPath& path) {
        return ::resolve_userdata_path(path);
    }

    std::unique_ptr<FileReadOnly> UserDataManagerSTD::open(const dal::ResPath& path) {
        const auto file_path = ::convert_userdata_respath(path);
        if (!file_path.has_value())
            return make_file_read_only_null();

        auto file = std::make_unique<::FileReadOnly_STL>();
        file->open(*file_path);

        if (!file->is_ready())
            return make_file_read_only_null();
        else
            return file;
    }

}
