#include "d_filesystem_std.h"

#if defined(DAL_OS_WINDOWS) || defined(DAL_OS_LINUX)

#include <codecvt>

#if defined(DAL_OS_WINDOWS)
    #include <Shlobj.h>

#elif defined(DAL_OS_LINUX)
    #include <unistd.h>

#endif

#include <fmt/format.h>

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

    std::optional<fs::path> find_document_dir() {

#if defined(DAL_OS_WINDOWS)
        WCHAR path[MAX_PATH];
        const auto result = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
        if (S_OK != result)
            return std::nullopt;

        return fs::path{ path };

#elif defined(DAL_OS_LINUX)
        char username_buf[128];
        getlogin_r(username_buf, 128);

        return fs::path{ fmt::format("/home/{}/Documents", username_buf) };

#endif

    }

    std::optional<fs::path> find_folder_in_document(const char* const folder_name) {
        const auto document_path = ::find_document_dir();
        if (!document_path.has_value())
            return std::nullopt;

        const auto app_area_path = *document_path / dal::APP_NAME;
        const auto output_path = app_area_path / folder_name;

        if (!fs::is_directory(app_area_path))
            fs::create_directory(app_area_path);

        if (!fs::is_directory(output_path))
            fs::create_directory(output_path);

        return output_path;
    }

    std::optional<fs::path> find_userdata_dir() {
        return ::find_folder_in_document(dal::FOLDER_NAME_USERDATA);
    }

    std::optional<fs::path> find_internal_dir() {
        return ::find_folder_in_document(dal::FOLDER_NAME_INTERNAL);
    }

    std::optional<fs::path> convert_asset_respath(const dal::ResPath& path) {
        const auto asset_dir = ::find_asset_dir();
        if (!asset_dir.has_value())
            return std::nullopt;

        const auto asset_path = dal::join_path(&path.dir_list().front() + 1, &path.dir_list().back() + 1, '/');

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

        const auto target_path = dal::join_path(&path.dir_list().front(), &path.dir_list().back() + 1, '/');

#if defined(DAL_OS_WINDOWS)
        return *domain_dir / ::utf8_to_utf16(target_path).value();
#elif defined(DAL_OS_LINUX)
        return *domain_dir / target_path;
#endif

    }

    std::optional<fs::path> convert_internal_respath(const dal::ResPath& path) {
        if (dal::SPECIAL_NAMESPACE_INTERNAL != path.dir_list().front())
            return std::nullopt;

        const auto domain_dir = ::find_internal_dir();
        if (!domain_dir.has_value())
            return std::nullopt;

        const auto target_path = dal::join_path(&path.dir_list().front() + 1, &path.dir_list().back() + 1, '/');

#if defined(DAL_OS_WINDOWS)
        return *domain_dir / ::utf8_to_utf16(target_path).value();
#elif defined(DAL_OS_LINUX)
        return *domain_dir / target_path;
#endif

    }

}


// Resolve functions
namespace {

    std::optional<dal::ResPath> resolve_asset_path(const dal::ResPath& respath) {
        if (respath.dir_list().front() != dal::SPECIAL_NAMESPACE_ASSET)
            return std::nullopt;

        const auto start_dir = ::find_asset_dir();
        if (!start_dir.has_value())
            return std::nullopt;

        const auto result = dal::resolve_path(respath, *start_dir, 1);
        if (!result.has_value())
            return std::nullopt;

        const auto res_path_str = std::string{} + dal::SPECIAL_NAMESPACE_ASSET + '/' + result.value();
        return dal::ResPath{ res_path_str };
    }

    std::optional<dal::ResPath> resolve_userdata_path(const dal::ResPath& respath) {
        const auto start_dir = ::find_userdata_dir();
        if (!start_dir.has_value())
            return std::nullopt;

        const auto result = dal::resolve_path(respath, *start_dir, 0);
        if (!result.has_value())
            return std::nullopt;

        return dal::ResPath{ result.value() };
    }

    std::optional<dal::ResPath> resolve_internal_path(const dal::ResPath& respath) {
        if (respath.dir_list().front() != dal::SPECIAL_NAMESPACE_INTERNAL)
            return std::nullopt;

        const auto start_dir = ::find_internal_dir();
        if (!start_dir.has_value())
            return std::nullopt;

        const auto result = dal::resolve_path(respath, *start_dir, 1);
        if (!result.has_value())
            return std::nullopt;

        return dal::ResPath{ fmt::format("{}/{}", dal::SPECIAL_NAMESPACE_INTERNAL, result.value()) };
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
        const auto normal_path = ::convert_asset_respath(path);
        if (!normal_path.has_value())
            return 0;

        output.clear();
        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_regular_file(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    size_t AssetManagerSTD::list_folders(const ResPath& path, std::vector<std::string>& output) {
        const auto normal_path = ::convert_asset_respath(path);
        if (!normal_path.has_value())
            return 0;

        output.clear();
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

        auto file = std::make_unique<dal::FileReadOnly_STL>();
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
        const auto normal_path = ::convert_userdata_respath(path);
        if (!normal_path.has_value())
            return 0;

        output.clear();
        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_regular_file(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    size_t UserDataManagerSTD::list_folders(const dal::ResPath& path, std::vector<std::string>& output) {
        const auto normal_path = ::convert_userdata_respath(path);
        if (!normal_path.has_value())
            return 0;

        output.clear();
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

        auto file = std::make_unique<dal::FileReadOnly_STL>();
        file->open(*file_path);

        if (!file->is_ready())
            return make_file_read_only_null();
        else
            return file;
    }

}


// InternalManagerSTD
namespace dal {

    bool InternalManagerSTD::is_file(const dal::ResPath& path) {
        const auto normal_path = ::convert_internal_respath(path);
        if (!normal_path.has_value())
            return false;

        return fs::is_regular_file(*normal_path);
    }

    bool InternalManagerSTD::is_folder(const dal::ResPath& path) {
        const auto normal_path = ::convert_internal_respath(path);
        if (!normal_path.has_value())
            return false;

        return fs::is_directory(*normal_path);
    }

    size_t InternalManagerSTD::list_files(const dal::ResPath& path, std::vector<std::string>& output) {
        const auto normal_path = ::convert_internal_respath(path);
        if (!normal_path.has_value())
            return 0;

        output.clear();
        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_regular_file(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    size_t InternalManagerSTD::list_folders(const dal::ResPath& path, std::vector<std::string>& output) {
        const auto normal_path = ::convert_internal_respath(path);
        if (!normal_path.has_value())
            return 0;

        output.clear();
        for (const auto& x : fs::directory_iterator(*normal_path)) {
            if (fs::is_directory(x))
                output.push_back(x.path().filename().u8string());
        }

        return output.size();
    }

    std::optional<ResPath> InternalManagerSTD::resolve(const ResPath& path) {
        return ::resolve_internal_path(path);
    }

    std::unique_ptr<FileReadOnly> InternalManagerSTD::open_read(const ResPath& path) {
        const auto file_path = ::convert_internal_respath(path);
        if (!file_path.has_value())
            return make_file_read_only_null();

        auto file = std::make_unique<dal::FileReadOnly_STL>();
        file->open(*file_path);

        if (!file->is_ready())
            return make_file_read_only_null();
        else
            return file;
    }

    std::unique_ptr<IFileWriteOnly> InternalManagerSTD::open_write(const ResPath& path) {
        const auto file_path = ::convert_internal_respath(path);
        if (!file_path.has_value())
            return make_file_write_only_null();

        dal::create_folders_of_path(*file_path, 1);

        auto file = std::make_unique<dal::FileWriteOnly_STL>();
        file->open(*file_path);

        if (!file->is_ready())
            return make_file_write_only_null();
        else
            return file;
    }

}

#endif
