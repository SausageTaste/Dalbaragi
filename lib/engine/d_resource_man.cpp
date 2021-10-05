#include "d_resource_man.h"

#include <fmt/format.h>

#include <daltools/model_parser.h>
#include <daltools/modifier.h>

#include "d_timer.h"
#include "d_logger.h"
#include "d_image_parser.h"


namespace {

    const char* const MISSING_TEX_PATH = "_asset/image/missing_tex.png";
    const char* const MISSING_MODEL_PATH = "_asset/model/missing_model.dmd";

    const dal::crypto::PublicKeySignature::PublicKey DAL_KEY_PUBLIC_ASSET{"99b50a1d0c13a3b1ab0442e33d3107f99d9f61567121666144903e4cef061b2b"};


    template <typename _VertType>
    glm::vec3 calc_weight_center(const std::vector<_VertType>& vertices) {
        double x_sum = 0.0;
        double y_sum = 0.0;
        double z_sum = 0.0;

        for (const auto& v : vertices) {
            x_sum += v.m_pos.x;
            y_sum += v.m_pos.y;
            z_sum += v.m_pos.z;
        }

        const double vert_count = static_cast<double>(vertices.size());

        return glm::vec3{
            x_sum / vert_count,
            y_sum / vert_count,
            z_sum / vert_count,
        };
    }

    void copy_material(dal::Material& dst, const dal::parser::Material& src) {
        dst.m_roughness = src.m_roughness;
        dst.m_metallic = src.m_metallic;
        dst.m_albedo_map = src.m_albedo_map;
        dst.m_alpha_blending = src.alpha_blend;
    }


    class Task_LoadImage : public dal::IPriorityTask {

    public:
        dal::Filesystem& m_filesys;
        dal::ResPath m_respath;

        std::vector<uint8_t> m_file_data;
        std::optional<dal::ImageData> out_image;

    private:
        int m_stage = 0;

    public:
        Task_LoadImage(const dal::ResPath& respath, dal::Filesystem& filesys)
            : dal::IPriorityTask(dal::PriorityClass::can_be_delayed)
            , m_filesys(filesys)
            , m_respath(respath)
            , out_image(std::nullopt)
        {

        }

        bool work() override {
            switch (this->m_stage) {
                case 0:
                    return this->stage_0();
                case 1:
                    return this->stage_1();
                default:
                    return true;
            }
        }

    private:
        bool stage_0() {
            auto file = this->m_filesys.open(this->m_respath);
            if (!file->is_ready())
                return true;

            if (!file->read_stl<std::vector<uint8_t>>(this->m_file_data))
                return true;

            this->m_stage = 1;
            return false;
        }

        bool stage_1() {
            this->out_image = dal::parse_image_stb(this->m_file_data.data(), this->m_file_data.size());

            this->m_stage = 2;
            return true;
        }

    };


    class Task_LoadModel : public dal::IPriorityTask {

    public:
        dal::crypto::PublicKeySignature& m_sign_mgr;
        dal::Filesystem& m_filesys;
        dal::ResPath m_respath;

        dal::parser::Model m_parsed_model;
        std::optional<dal::ModelStatic> out_model;

    private:
        int m_stage = 0;

    public:
        Task_LoadModel(const dal::ResPath& respath, dal::Filesystem& filesys, dal::crypto::PublicKeySignature& sign_mgr)
            : dal::IPriorityTask(dal::PriorityClass::can_be_delayed)
            , m_sign_mgr(sign_mgr)
            , m_filesys(filesys)
            , m_respath(respath)
        {

        }

        bool work() override {
            switch (this->m_stage) {
                case 0:
                    return this->stage_0();
                case 1:
                    return this->stage_1();
                case 2:
                    return this->stage_2();
                case 3:
                    return this->stage_3();
                case 4:
                    return this->stage_4();
                default:
                    return true;
            }
        }

    private:
        bool stage_0() {
            auto file = this->m_filesys.open(this->m_respath);
            if (!file->is_ready()) {
                out_model = std::nullopt;
                return true;
            }

            const auto model_content = file->read_stl<std::vector<uint8_t>>();
            if (!model_content.has_value()) {
                out_model = std::nullopt;
                return true;
            }

            const auto unzipped = dal::parser::unzip_dmd(model_content->data(), model_content->size());
            if (!unzipped) {
                out_model = std::nullopt;
                return true;
            }

            const auto parse_result = dal::parser::parse_dmd(this->m_parsed_model, unzipped->data(), unzipped->size());
            if (dal::parser::ModelParseResult::success != parse_result) {
                out_model = std::nullopt;
                return true;
            }

            if ("_asset" == this->m_respath.dir_list().front()) {
                const dal::crypto::PublicKeySignature::Signature signature{ this->m_parsed_model.m_signature_hex };
                const auto result = this->m_sign_mgr.verify(unzipped->data(), unzipped->size() - this->m_parsed_model.m_signature_hex.size() - 1, ::DAL_KEY_PUBLIC_ASSET, signature);

                if (!result) {
                    out_model = std::nullopt;
                    return true;
                }
            }

            this->out_model = dal::ModelStatic{};

            this->m_stage = 1;
            return false;
        }

        bool stage_1() {
            static_assert(sizeof(dal::parser::Vertex) == sizeof(dal::VertexStatic));
            static_assert(offsetof(dal::parser::Vertex, m_position) == offsetof(dal::VertexStatic, m_pos));
            static_assert(offsetof(dal::parser::Vertex, m_normal) == offsetof(dal::VertexStatic, m_normal));
            static_assert(offsetof(dal::parser::Vertex, m_uv_coords) == offsetof(dal::VertexStatic, m_uv_coord));

            for (const auto& src_unit : this->m_parsed_model.m_units_indexed) {
                auto& dst_unit = this->out_model->m_units.emplace_back();

                dst_unit.m_vertices.resize(src_unit.m_mesh.m_vertices.size());
                memcpy(dst_unit.m_vertices.data(), src_unit.m_mesh.m_vertices.data(), dst_unit.m_vertices.size() * sizeof(dal::VertexStatic));

                dst_unit.m_indices.assign(src_unit.m_mesh.m_indices.begin(), src_unit.m_mesh.m_indices.end());
                ::copy_material(dst_unit.m_material, src_unit.m_material);
                dst_unit.m_weight_center = ::calc_weight_center(dst_unit.m_vertices);
            }

            this->m_stage = 2;
            return false;
        }

        bool stage_2() {
            for (const auto& src_unit : this->m_parsed_model.m_units_indexed_joint) {
                auto& dst_unit = this->out_model->m_units.emplace_back();

                dst_unit.m_vertices.resize(src_unit.m_mesh.m_vertices.size());
                for (size_t i = 0; i < dst_unit.m_vertices.size(); ++i) {
                    auto& out_vert = dst_unit.m_vertices[i];
                    auto& in_vert = src_unit.m_mesh.m_vertices[i];

                    out_vert.m_pos = in_vert.m_position;
                    out_vert.m_normal = in_vert.m_normal;
                    out_vert.m_uv_coord = in_vert.m_uv_coords;
                }

                dst_unit.m_indices.assign(src_unit.m_mesh.m_indices.begin(), src_unit.m_mesh.m_indices.end());
                ::copy_material(dst_unit.m_material, src_unit.m_material);
                dst_unit.m_weight_center = ::calc_weight_center(dst_unit.m_vertices);
            }

            this->m_stage = 3;
            return false;
        }

        bool stage_3() {
            if (!this->m_parsed_model.m_units_straight.empty())
                dalWarn("Not supported vertex data: straight");

            this->m_stage = 4;
            return false;
        }

        bool stage_4() {
            if (!this->m_parsed_model.m_units_straight_joint.empty())
                dalWarn("Not supported vertex data: straight joint");

            this->m_stage = 5;
            return true;
        }

    };


    class Task_LoadModelSkinned : public dal::IPriorityTask {

    public:
        dal::crypto::PublicKeySignature& m_sign_mgr;
        dal::Filesystem& m_filesys;
        dal::ResPath m_respath;

        dal::parser::Model m_parsed_model;
        std::optional<dal::ModelSkinned> out_model;

    private:
        int m_stage = 0;

    public:
        Task_LoadModelSkinned(const dal::ResPath& respath, dal::Filesystem& filesys, dal::crypto::PublicKeySignature& sign_mgr)
            : dal::IPriorityTask(dal::PriorityClass::can_be_delayed)
            , m_sign_mgr(sign_mgr)
            , m_filesys(filesys)
            , m_respath(respath)
        {
            this->set_priority_class(dal::PriorityClass::can_be_delayed);
        }

        bool work() override {
            switch (this->m_stage) {
                case 0:
                    return this->stage_0();
                case 1:
                    return this->stage_1();
                case 2:
                    return this->stage_2();
                case 3:
                    return this->stage_3();
                case 4:
                    return this->stage_4();
                case 5:
                    return this->stage_5();
                default:
                    return true;
            }
        }

    private:
        bool stage_0() {
            auto file = this->m_filesys.open(this->m_respath);
            if (!file->is_ready()) {
                out_model = std::nullopt;
                return true;
            }

            const auto model_content = file->read_stl<std::vector<uint8_t>>();
            if (!model_content.has_value()) {
                out_model = std::nullopt;
                return true;
            }

            const auto unzipped = dal::parser::unzip_dmd(model_content->data(), model_content->size());
            if (!unzipped) {
                out_model = std::nullopt;
                return true;
            }

            const auto parse_result = dal::parser::parse_dmd(this->m_parsed_model, unzipped->data(), unzipped->size());
            if (dal::parser::ModelParseResult::success != parse_result) {
                out_model = std::nullopt;
                return true;
            }

            if ("_asset" == this->m_respath.dir_list().front()) {
                const dal::crypto::PublicKeySignature::Signature signature{ this->m_parsed_model.m_signature_hex };
                const auto result = this->m_sign_mgr.verify(unzipped->data(), unzipped->size() - this->m_parsed_model.m_signature_hex.size() - 1, ::DAL_KEY_PUBLIC_ASSET, signature);

                if (!result) {
                    out_model = std::nullopt;
                    return true;
                }
            }

            this->out_model = dal::ModelSkinned{};

            this->m_stage = 1;
            return false;
        }

        bool stage_1() {
            for (const auto& src_unit : this->m_parsed_model.m_units_indexed) {
                auto& dst_unit = this->out_model->m_units.emplace_back();

                dst_unit.m_vertices.resize(src_unit.m_mesh.m_vertices.size());
                for (size_t i = 0; i < dst_unit.m_vertices.size(); ++i) {
                    auto& dst_vert = dst_unit.m_vertices[i];
                    auto& src_vert = src_unit.m_mesh.m_vertices[i];

                    dst_vert.m_joint_ids     = glm::ivec4{-1, -1, -1, -1};
                    dst_vert.m_joint_weights = glm::vec4{0, 0, 0, 0};
                    dst_vert.m_pos           = src_vert.m_position;
                    dst_vert.m_normal        = src_vert.m_normal;
                    dst_vert.m_uv_coord      = src_vert.m_uv_coords;
                }

                dst_unit.m_indices.assign(src_unit.m_mesh.m_indices.begin(), src_unit.m_mesh.m_indices.end());
                ::copy_material(dst_unit.m_material, src_unit.m_material);
                dst_unit.m_weight_center = ::calc_weight_center(dst_unit.m_vertices);
            }

            this->m_stage = 2;
            return false;
        }

        bool stage_2() {
            for (const auto& src_unit : this->m_parsed_model.m_units_indexed_joint) {
                auto& dst_unit = this->out_model->m_units.emplace_back();

                dst_unit.m_vertices.resize(src_unit.m_mesh.m_vertices.size());
                for (size_t i = 0; i < dst_unit.m_vertices.size(); ++i) {
                    auto& dst_vert = dst_unit.m_vertices[i];
                    auto& src_vert = src_unit.m_mesh.m_vertices[i];

                    dst_vert.m_joint_ids     = src_vert.m_joint_indices;
                    dst_vert.m_joint_weights = src_vert.m_joint_weights;
                    dst_vert.m_pos           = src_vert.m_position;
                    dst_vert.m_normal        = src_vert.m_normal;
                    dst_vert.m_uv_coord      = src_vert.m_uv_coords;
                }

                dst_unit.m_indices.assign(src_unit.m_mesh.m_indices.begin(), src_unit.m_mesh.m_indices.end());
                ::copy_material(dst_unit.m_material, src_unit.m_material);
                dst_unit.m_weight_center = ::calc_weight_center(dst_unit.m_vertices);
            }

            this->m_stage = 3;
            return false;
        }

        bool stage_3() {
            for (auto& src_anim : this->m_parsed_model.m_animations) {
                if (src_anim.m_joints.size() > dal::MAX_JOINT_COUNT) {
                    dalWarn(fmt::format("Joint count {} is bigger than limit {}", src_anim.m_joints.size(), dal::MAX_JOINT_COUNT).c_str());
                }

                auto& dst_anim = this->out_model->m_animations.emplace_back(src_anim.m_name, src_anim.m_ticks_par_sec, src_anim.m_duration_tick);
                for (auto& out_joint : src_anim.m_joints) {
                    dst_anim.new_joint().m_data = out_joint;
                }
            }

            this->m_stage = 4;
            return false;
        }

        bool stage_4() {
            for (auto& src_joint : this->m_parsed_model.m_skeleton.m_joints) {
                const auto jid = this->out_model->m_skeleton.get_or_make_index_of(src_joint.m_name);
                auto& dst_joint = this->out_model->m_skeleton.at(jid);
                dst_joint.set(src_joint);
            }

            this->m_stage = 5;
            return false;
        }

        bool stage_5() {
            if (this->out_model->m_skeleton.size() > 0) {
                // Character lies on ground without this line.
                this->out_model->m_skeleton.at(0).set_parent_mat(this->out_model->m_skeleton.at(0).offset());

                for ( int i = 1; i < this->out_model->m_skeleton.size(); ++i ) {
                    auto& thisInfo = this->out_model->m_skeleton.at(i);
                    const auto& parentInfo = this->out_model->m_skeleton.at(thisInfo.parent_index());
                    thisInfo.set_parent_mat(parentInfo);
                }
            }

            if (!this->m_parsed_model.m_units_straight.empty())
                dalWarn("Not supported vertex data: straight");
            if (!this->m_parsed_model.m_units_straight_joint.empty())
                dalWarn("Not supported vertex data: straight joint");

            this->m_stage = 6;
            return true;
        }

    };


    class Task_SlowTest : public dal::IPriorityTask {

    private:
        size_t m_counter = 0;
        size_t m_upper_bound = 3;

    public:
        Task_SlowTest()
            : dal::IPriorityTask(dal::PriorityClass::least_wanted)
        {

        }

        bool work() override {
            dalInfo(fmt::format("{}", this->m_counter).c_str());
            dal::sleep_for(1);
            return ++this->m_counter >= this->m_upper_bound;
        }

    };

}


// TextureBuilder
namespace dal {

    void TextureBuilder::update() {

    }

    void TextureBuilder::set_renderer(IRenderer& renderer) {
        this->m_renderer = &renderer;
    }

    void TextureBuilder::invalidate_renderer() {
        this->m_waiting_file.clear();

        this->m_renderer = nullptr;
    }

    void TextureBuilder::notify_task_done(HTask& task) {
        auto& task_result = *reinterpret_cast<Task_LoadImage*>(task.get());
        const auto found = this->m_waiting_file.find(task_result.m_respath.make_str());

        if (this->m_waiting_file.end() != found) {
            this->m_renderer->init(
                *found->second.get(),
                task_result.out_image.value()
            );

            this->m_waiting_file.erase(found);
        }
    }

    void TextureBuilder::start(const ResPath& respath, HTexture h_texture, Filesystem& filesys, TaskManager& task_man) {
        dalAssert(nullptr != this->m_renderer);

        auto task = std::make_shared<::Task_LoadImage>(respath, filesys);
        auto [iter, success] = this->m_waiting_file.emplace(respath.make_str(), h_texture);
        //task_man.order_task(std::make_shared<::Task_SlowTest>(), nullptr);
        task_man.order_task(task, this);
    }

}


// ModelBuilder
namespace dal {

    void ModelBuilder::update() {
        for (auto iter = this->m_waiting_prepare.begin(); iter < this->m_waiting_prepare.end();) {
            auto& model = **iter;

            if (model.is_ready()) {
                iter = this->m_waiting_prepare.erase(iter);
            }
            else {
                if (this->m_renderer->prepare(model))
                    return;

                ++iter;
            }
        }
    }

    void ModelBuilder::set_renderer(IRenderer& renderer) {
        this->m_renderer = &renderer;
    }

    void ModelBuilder::invalidate_renderer() {
        this->m_waiting_file.clear();
        this->m_waiting_prepare.clear();

        this->m_renderer = nullptr;
    }

    void ModelBuilder::notify_task_done(HTask& task) {
        auto& task_result = *reinterpret_cast<Task_LoadModel*>(task.get());
        const auto found = this->m_waiting_file.find(task_result.m_respath.make_str());

        if (this->m_waiting_file.end() != found) {
            if (task_result.out_model.has_value()) {
                this->m_renderer->init(
                    *found->second.get(),
                    task_result.out_model.value(),
                    task_result.m_respath.dir_list().front().c_str()
                );

                this->m_waiting_prepare.push_back(found->second);
            }
            else {
                const auto msg = fmt::format("Failed to load model: {}", task_result.m_respath.make_str());
                dalError(msg.c_str());
            }

            this->m_waiting_file.erase(found);
        }
    }

    void ModelBuilder::start(
        const ResPath& respath,
        HRenModel h_model,
        Filesystem& filesys,
        TaskManager& task_man,
        crypto::PublicKeySignature& sign_mgr
    ) {
        dalAssert(nullptr != this->m_renderer);

        auto task = std::make_unique<::Task_LoadModel>(respath, filesys, sign_mgr);
        auto [iter, success] = this->m_waiting_file.emplace(respath.make_str(), h_model);
        //task_man.order_task(std::make_shared<::Task_SlowTest>(), nullptr);
        task_man.order_task(std::move(task), this);
    }

}


// ModelSkinnedBuilder
namespace dal {

    void ModelSkinnedBuilder::update() {
        for (auto iter = this->m_waiting_prepare.begin(); iter < this->m_waiting_prepare.end();) {
            auto& model = **iter;

            if (model.is_ready()) {
                iter = this->m_waiting_prepare.erase(iter);
            }
            else {
                if (this->m_renderer->prepare(model))
                    return;

                ++iter;
            }
        }
    }

    void ModelSkinnedBuilder::set_renderer(IRenderer& renderer) {
        this->m_renderer = &renderer;
    }

    void ModelSkinnedBuilder::invalidate_renderer() {
        this->m_waiting_file.clear();
        this->m_waiting_prepare.clear();

        this->m_renderer = nullptr;
    }

    void ModelSkinnedBuilder::notify_task_done(HTask& task) {
        auto& task_result = *reinterpret_cast<Task_LoadModelSkinned*>(task.get());
        const auto found = this->m_waiting_file.find(task_result.m_respath.make_str());

        if (this->m_waiting_file.end() != found) {
            if (task_result.out_model.has_value()) {
                this->m_renderer->init(
                    *found->second.get(),
                    task_result.out_model.value(),
                    task_result.m_respath.dir_list().front().c_str()
                );

                for (auto& anim : found->second->animations()) {
                    anim = anim.make_compatible_with(found->second->skeleton());
                }

                this->m_waiting_prepare.push_back(found->second);
            }
            else {
                const auto msg = fmt::format("Failed to load model: {}", task_result.m_respath.make_str());
                dalError(msg.c_str());
            }

            this->m_waiting_file.erase(found);
        }
    }

    void ModelSkinnedBuilder::start(
        const ResPath& respath,
        HRenModelSkinned h_model,
        Filesystem& filesys,
        TaskManager& task_man,
        crypto::PublicKeySignature& sign_mgr
    ) {
        dalAssert(nullptr != this->m_renderer);

        auto task = std::make_unique<::Task_LoadModelSkinned>(respath, filesys, sign_mgr);
        auto [iter, success] = this->m_waiting_file.emplace(respath.make_str(), h_model);
        //task_man.order_task(std::make_shared<::Task_SlowTest>(), nullptr);
        task_man.order_task(std::move(task), this);
    }

}


// ResourceManager
namespace dal {

    void ResourceManager::update() {
        this->m_tex_builder.update();
        this->m_model_builder.update();
        this->m_model_skinned_builder.update();
    }

    void ResourceManager::set_renderer(IRenderer& renderer) {
        this->m_renderer = &renderer;

        this->m_tex_builder.set_renderer(renderer);
        this->m_model_builder.set_renderer(renderer);
        this->m_model_skinned_builder.set_renderer(renderer);

        for (auto& [respath, texture] : this->m_textures) {
            this->m_tex_builder.start(respath, texture, this->m_filesys, this->m_task_man);
        }

        for (auto& [respath, model] : this->m_models) {
            this->m_model_builder.start(respath, model, this->m_filesys, this->m_task_man, this->m_sign_mgr);
        }

        for (auto& [respath, model] : this->m_skinned_models) {
            this->m_model_skinned_builder.start(respath, model, this->m_filesys, this->m_task_man, this->m_sign_mgr);
        }

        for (auto& actor : this->m_actors) {
            this->m_renderer->init(*actor.get());
        }

        for (auto& actor : this->m_skinned_actors)
            this->m_renderer->init(*actor.get());

        this->m_missing_tex = this->request_texture(::MISSING_TEX_PATH);
        this->m_missing_model = this->request_model(::MISSING_MODEL_PATH);
        this->m_missing_model_skinned = this->request_model_skinned(::MISSING_MODEL_PATH);
    }

    void ResourceManager::invalidate_renderer() {
        if (nullptr != this->m_renderer)
            this->m_renderer->wait_idle();

        this->m_missing_tex.reset();
        this->m_missing_model.reset();
        this->m_missing_model_skinned.reset();

        this->m_model_skinned_builder.invalidate_renderer();
        this->m_model_builder.invalidate_renderer();
        this->m_tex_builder.invalidate_renderer();

        for (auto& x : this->m_textures)
            x.second->destroy();

        for (auto& x : this->m_models)
            x.second->destroy();

        for (auto& x : this->m_skinned_models)
            x.second->destroy();

        for (auto& x : this->m_actors)
            x->destroy();

        for (auto& x : this->m_skinned_actors)
            x->destroy();

        this->m_renderer = nullptr;
    }

    HTexture ResourceManager::request_texture(const ResPath& respath) {
        const auto resolved_respath = this->m_filesys.resolve(respath);
        if (!resolved_respath.has_value()) {
            dalError(fmt::format("Failed to find texture file: {}", respath.make_str()).c_str());
            dalAssert(!!this->m_missing_tex);
            return this->m_missing_tex;
        }

        const auto path_str = resolved_respath->make_str();
        const auto result = this->m_textures.find(path_str);

        if (this->m_textures.end() != result) {
            return result->second;
        }
        else {
            auto [iter, result] = this->m_textures.emplace(path_str, this->m_renderer->create_texture());
            dalAssert(result);
            this->m_tex_builder.start(*resolved_respath, iter->second, this->m_filesys, this->m_task_man);

            return iter->second;
        }
    }

    HRenModel ResourceManager::request_model(const ResPath& respath) {
        const auto resolved_respath = this->m_filesys.resolve(respath);
        if (!resolved_respath.has_value()) {
            dalError(fmt::format("Failed to find model file: {}", respath.make_str()).c_str());
            dalAssert(!!this->m_missing_model);
            return this->m_missing_model;
        }

        const auto path_str = resolved_respath->make_str();
        const auto result = this->m_models.find(path_str);

        if (this->m_models.end() != result) {
            return result->second;
        }
        else {
            auto [iter, result] = this->m_models.emplace(path_str, this->m_renderer->create_model());
            dalAssert(result);
            this->m_model_builder.start(*resolved_respath, iter->second, this->m_filesys, this->m_task_man, this->m_sign_mgr);

            return iter->second;
        }
    }

    HRenModelSkinned ResourceManager::request_model_skinned(const ResPath& respath) {
        const auto resolved_respath = this->m_filesys.resolve(respath);
        if (!resolved_respath.has_value()) {
            dalError(fmt::format("Failed to find skinned model file: {}", respath.make_str()).c_str());
            dalAssert(!!this->m_missing_model_skinned);
            return this->m_missing_model_skinned;
        }

        const auto path_str = resolved_respath->make_str();
        const auto result = this->m_skinned_models.find(path_str);

        if (this->m_skinned_models.end() != result) {
            return result->second;
        }
        else {
            auto [iter, result] = this->m_skinned_models.emplace(path_str, this->m_renderer->create_model_skinned());
            auto task = std::make_unique<::Task_LoadModelSkinned>(respath, this->m_filesys, this->m_sign_mgr);
            this->m_model_skinned_builder.start(*resolved_respath, iter->second, this->m_filesys, this->m_task_man, this->m_sign_mgr);

            return iter->second;
        }
    }

    HActor ResourceManager::request_actor() {
        this->m_actors.push_back(this->m_renderer->create_actor());
        this->m_renderer->init(*this->m_actors.back().get());
        return this->m_actors.back();
    }

    HActorSkinned ResourceManager::request_actor_skinned() {
        this->m_skinned_actors.push_back(this->m_renderer->create_actor_skinned());
        this->m_renderer->init(*this->m_skinned_actors.back());
        return this->m_skinned_actors.back();
    }

}
