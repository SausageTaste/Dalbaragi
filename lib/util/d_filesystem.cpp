#include "d_filesystem.h"

#include <array>

#include "d_konsts.h"


namespace {

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


namespace dal {

    std::unique_ptr<FileReadOnly> make_file_read_only_null() {
        return std::unique_ptr<dal::FileReadOnly>{ new ::FileReadOnly_Null };
    }

}


// Filesystem
namespace dal {

    std::optional<ResPath> Filesystem::resolve(const dal::ResPath& path) {
        if (!path.is_valid())
            return std::nullopt;

        if (path.dir_list().front() == dal::SPECIAL_NAMESPACE_ASSET) {
            return this->m_asset_mgr->resolve(path);
        }
        else if (path.dir_list().front() == "?") {
            auto result_asset = this->m_asset_mgr->resolve(path);
            if (result_asset.has_value())
                return result_asset;

            auto result_userdata = this->m_userdata_mgr->resolve(path);
            if (result_userdata.has_value())
                return result_userdata;
        }
        else {
            return this->m_userdata_mgr->resolve(path);
        }

        return std::nullopt;
    }

    std::unique_ptr<FileReadOnly> Filesystem::open(const ResPath& path) {
        if (!path.is_valid())
            return make_file_read_only_null();

        if (path.dir_list().front() == dal::SPECIAL_NAMESPACE_ASSET)
            return this->m_asset_mgr->open(path);
        else
            return this->m_userdata_mgr->open(path);

        return make_file_read_only_null();
    }

}
