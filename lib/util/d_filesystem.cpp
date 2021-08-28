#include "d_filesystem.h"

#include <array>

#include <fmt/format.h>

#include "d_konsts.h"
#include "d_logger.h"


namespace {

    namespace fs = std::filesystem;


    class FileReadOnly_Null : public dal::FileReadOnly {

    public:
        void close() override {}

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


    class FileWriteOnly_Null : public dal::IFileWriteOnly {

    public:
        void close() override {}

        bool is_ready() override {
            return false;
        }

        bool write(const void* const data, const size_t data_size) override {
            return false;
        }

    };


    enum class FileType {
        asset,
        userdata,
        internal,
        unknown
    };

    FileType dispatch_file_manager(const dal::ResPath& path) {
        if (!path.is_valid())
            return ::FileType::unknown;

        if (dal::SPECIAL_NAMESPACE_ASSET == path.dir_list().front())
            return ::FileType::asset;
        else if (dal::SPECIAL_NAMESPACE_INTERNAL == path.dir_list().front())
            return ::FileType::internal;
        else
            return ::FileType::userdata;
    }

    std::optional<std::filesystem::path> resolve_question_path(const std::filesystem::path& domain_dir, const std::string& entry_to_find) {
        if (!fs::is_directory(domain_dir))
            return std::nullopt;

        for (auto& e : fs::recursive_directory_iterator(domain_dir)) {
            if (e.path().filename().u8string() == entry_to_find) {
                return e.path();
            }
        }

        return std::nullopt;
    }

    std::optional<std::filesystem::path> resolve_asterisk_path(const std::filesystem::path& domain_dir, const std::string& entry_to_find) {
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

    size_t calc_path_length(const fs::path& path) {
        size_t output = 0;

        for (auto& x : path)
            ++output;

        return output;
    }

}



// Library functions
namespace dal {

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
            return dal::is_char_wildcard(str[0]);
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

    std::string join_path(const std::initializer_list<std::string>& dir_list, const char seperator) {
        return dal::join_path(dir_list.begin(), dir_list.end(), seperator);
    }

    void split_path(const char* const path, std::vector<std::string>& output) {
        output.clear();
        output.emplace_back();

        for (auto ptr = path; *ptr != '\0'; ++ptr) {
            const auto c = *ptr;

            if (dal::is_char_separator(c)) {
                output.emplace_back();
            }
            else {
                output.back().push_back(c);
            }
        }
    }

    std::vector<std::string> split_path(const char* const path) {
        std::vector<std::string> output;
        dal::split_path(path, output);
        return output;
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

    std::unique_ptr<FileReadOnly> make_file_read_only_null() {
        return std::unique_ptr<dal::FileReadOnly>{ new ::FileReadOnly_Null };
    }

    std::unique_ptr<IFileWriteOnly> make_file_write_only_null() {
        return std::make_unique<FileWriteOnly_Null>();
    }

    std::optional<std::string> resolve_path(const dal::ResPath& respath, const std::filesystem::path& start_dir, const size_t start_index) {
        auto cur_path = start_dir;

        for (size_t i = start_index; i < respath.dir_list().size(); ++i) {
            const auto dir_element = respath.dir_list().at(i);

            if (dir_element == "?") {
                const auto resolve_result = resolve_question_path(cur_path, respath.dir_list().at(i + 1));
                if (!resolve_result.has_value()) {
                    return std::nullopt;
                }
                else {
                    cur_path = resolve_result.value();
                    ++i;
                }
            }
            else if (dir_element == "*") {
                const auto resolve_result = resolve_asterisk_path(cur_path, respath.dir_list().at(i + 1));
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

    void create_folders_of_path(const std::filesystem::path& path, const size_t exclude_last_n) {
        const auto path_length = ::calc_path_length(path);
        fs::path cur_path;

        size_t index = 0;
        for (const auto& x : path) {
            if ((path_length - (++index)) < exclude_last_n)
                break;

            cur_path /= x;

            if (!fs::is_directory(cur_path))
                fs::create_directory(cur_path);
        }
    }

}


// ResPath
namespace dal {

    ResPath::ResPath(const char* const path)
        : m_src_str(path)
    {
        dal::split_path(path, this->m_dir_list);
        this->m_dir_list = dal::remove_duplicate_question_marks(this->m_dir_list);
    }

    std::string ResPath::make_str() const {
        return dal::join_path(this->dir_list().begin(), this->dir_list().end(), this->SEPARATOR);
    }

    bool ResPath::is_valid() const {
        for (const auto& x : this->m_dir_list) {
            if (!dal::is_valid_folder_name(x) && !dal::is_str_wildcard(x.c_str()))
                return false;
        }

        return !dal::is_str_wildcard(this->m_dir_list.back().c_str());
    }

}


// FileReadOnly_STL
namespace dal {

    bool FileReadOnly_STL::open(const std::filesystem::path& path) {
        this->close();

        this->m_file.open(path, std::ios::ate | std::ios::binary);
        if (!this->m_file)
            return false;

        this->m_size = this->m_file.tellg();
        return this->m_file.is_open();
    }

    void FileReadOnly_STL::close() {
        this->m_file.close();
        this->m_size = 0;
    }

    bool FileReadOnly_STL::is_ready() {
        return this->m_file.is_open();
    }

    size_t FileReadOnly_STL::size() {
        return this->m_size;
    }

    bool FileReadOnly_STL::read(void* const dst, const size_t dst_size) {
        this->m_file.seekg(0);
        this->m_file.read(reinterpret_cast<char*>(dst), dst_size);
        return true;
    }

}


// FileWriteOnly_STL
namespace dal {

    bool FileWriteOnly_STL::open(const std::filesystem::path& path) {
        this->close();

        this->m_file.open(path, std::ios::ate | std::ios::binary);
        return this->is_ready();
    }

    void FileWriteOnly_STL::close() {
        this->m_file.close();
    }

    bool FileWriteOnly_STL::is_ready() {
        return this->m_file.is_open();
    }

    bool FileWriteOnly_STL::write(const void* const data, const size_t data_size) {
        this->m_file.write(reinterpret_cast<const char*>(data), data_size);
        return true;
    }

}


// Filesystem
namespace dal {

    bool Filesystem::is_file(const ResPath& path) {
        switch (::dispatch_file_manager(path)) {
            case ::FileType::asset:
                return this->m_asset_mgr->is_file(path);
            case ::FileType::internal:
                return this->m_internal_mgr->is_file(path);
            case ::FileType::userdata:
                return this->m_userdata_mgr->is_file(path);
            default:
                return 0;
        }
    }

    bool Filesystem::is_folder(const ResPath& path) {
        switch (::dispatch_file_manager(path)) {
            case ::FileType::asset:
                return this->m_asset_mgr->is_folder(path);
            case ::FileType::internal:
                return this->m_internal_mgr->is_folder(path);
            case ::FileType::userdata:
                return this->m_userdata_mgr->is_folder(path);
            default:
                return 0;
        }
    }

    size_t Filesystem::list_files(const ResPath& path, std::vector<std::string>& output) {
        switch (::dispatch_file_manager(path)) {
            case ::FileType::asset:
                return this->m_asset_mgr->list_files(path, output);
            case ::FileType::internal:
                return this->m_internal_mgr->list_files(path, output);
            case ::FileType::userdata:
                return this->m_userdata_mgr->list_files(path, output);
            default:
                return 0;
        }
    }

    size_t Filesystem::list_folders(const ResPath& path, std::vector<std::string>& output) {
        switch (::dispatch_file_manager(path)) {
            case ::FileType::asset:
                return this->m_asset_mgr->list_folders(path, output);
            case ::FileType::internal:
                return this->m_internal_mgr->list_folders(path, output);
            case ::FileType::userdata:
                return this->m_userdata_mgr->list_folders(path, output);
            default:
                return 0;
        }
    }

    std::optional<ResPath> Filesystem::resolve(const dal::ResPath& path) {
        if (!path.is_valid())
            return std::nullopt;

        if (path.dir_list().front() == dal::SPECIAL_NAMESPACE_ASSET) {
            return this->m_asset_mgr->resolve(path);
        }
        else if (path.dir_list().front() == dal::SPECIAL_NAMESPACE_INTERNAL) {
            return this->m_internal_mgr->resolve(path);
        }
        else if (path.dir_list().front() == "?") {
            auto result_asset = this->m_asset_mgr->resolve(path);
            if (result_asset.has_value())
                return result_asset;

            auto result_userdata = this->m_userdata_mgr->resolve(path);
            if (result_userdata.has_value())
                return result_userdata;

            auto result_internal = this->m_internal_mgr->resolve(path);
            if (result_internal.has_value())
                return result_internal;
        }
        else {
            return this->m_userdata_mgr->resolve(path);
        }

        return std::nullopt;
    }

    std::unique_ptr<FileReadOnly> Filesystem::open(const ResPath& path) {
        switch (::dispatch_file_manager(path)) {
            case ::FileType::asset:
                return this->m_asset_mgr->open(path);
            case ::FileType::internal:
                return this->m_internal_mgr->open_read(path);
            case ::FileType::userdata:
                return this->m_userdata_mgr->open(path);
            default:
                return make_file_read_only_null();
        }
    }

    std::unique_ptr<IFileWriteOnly> Filesystem::open_write(const ResPath& path) {
        if (!path.is_valid())
            return make_file_write_only_null();

        if (dal::SPECIAL_NAMESPACE_INTERNAL == path.dir_list().front())
            return this->m_internal_mgr->open_write(path);

        return make_file_write_only_null();
    }

}
