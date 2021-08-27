#include "d_filesystem2.h"

#include "d_konsts.h"


namespace {

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


namespace dal {

    std::unique_ptr<FileReadOnly> make_file_read_only_null() {
        return std::unique_ptr<dal::FileReadOnly>{ new ::FileReadOnly_Null };
    }

}


// Filesystem2
namespace dal {

    std::optional<ResPath> Filesystem2::resolve_respath(const dal::ResPath& path) {
        return std::nullopt;
    }

    std::unique_ptr<FileReadOnly> Filesystem2::open(const ResPath& path) {
        if (!path.is_valid())
            return make_file_read_only_null();

        if (path.dir_list().front() == dal::SPECIAL_NAMESPACE_ASSET)
            return this->m_asset_mgr->open(path);
        else
            return this->m_userdata_mgr->open(path);

        return make_file_read_only_null();
    }

}
