#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>


namespace dal {

    bool is_char_separator(const char c);

    bool is_char_wildcard(const char c);

    bool is_str_wildcard(const char* const str);

    bool is_valid_folder_name(const std::string& name);

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

    std::string join_path(const std::initializer_list<std::string>& dir_list, const char seperator);

    void split_path(const char* const path, std::vector<std::string>& output);

    std::vector<std::string> split_path(const char* const path);

    std::vector<std::string> remove_duplicate_question_marks(const std::vector<std::string>& list);


    class ResPath {

    private:
        static constexpr char SEPARATOR = '/';

    private:
        std::vector<std::string> m_dir_list;
        std::string m_src_str;

    public:
        ResPath() = default;

        ResPath(const char* const path);

        ResPath(const std::string& path)
            : ResPath(path.c_str())
        {

        }

        std::string make_str() const;

        bool is_valid() const;

        auto& dir_list() const {
            return this->m_dir_list;
        }

    };


    class FileReadOnly {

    public:
        virtual ~FileReadOnly() = default;

        virtual void close() = 0;

        virtual bool is_ready() = 0;

        virtual size_t size() = 0;

        virtual bool read(void* const dst, const size_t dst_size) = 0;

        template <typename T>
        bool read_stl(T& output) {
            if (!this->is_ready())
                return false;

            output.resize(this->size());
            return this->read(output.data(), output.size());
        }

        template <typename T>
        std::optional<T> read_stl() {
            T output{};

            if (!this->is_ready())
                return std::nullopt;

            output.resize(this->size());
            return this->read_stl(output) ? std::optional<T>{output} : std::nullopt;
        }

    };


    std::unique_ptr<FileReadOnly> make_file_read_only_null();


    class IFileManager {

    public:
        virtual bool is_file(const ResPath& path) = 0;

        virtual bool is_folder(const ResPath& path) = 0;

        virtual size_t list_files(const ResPath& path, std::vector<std::string>& output) = 0;

        virtual size_t list_folders(const ResPath& path, std::vector<std::string>& output) = 0;

        virtual std::optional<ResPath> resolve(const ResPath& path) = 0;

        virtual std::unique_ptr<FileReadOnly> open(const ResPath& path) = 0;

    };


    class IAssetManager : public IFileManager {

    };


    class IUserDataManager : public IFileManager {

    };


    class Filesystem {

    private:
        std::unique_ptr<IAssetManager> m_asset_mgr;
        std::unique_ptr<IUserDataManager> m_userdata_mgr;

    public:
        void init(
            std::unique_ptr<IAssetManager>&& asset_mgr,
            std::unique_ptr<IUserDataManager>&& userdata_mgr
        ) {
            this->m_asset_mgr = std::move(asset_mgr);
            this->m_userdata_mgr = std::move(userdata_mgr);
        }

        std::optional<ResPath> resolve(const ResPath& path);

        std::unique_ptr<FileReadOnly> open(const ResPath& path);

    };

}
