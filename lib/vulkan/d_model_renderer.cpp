#include "d_model_renderer.h"

#include <fmt/format.h>

#include "d_logger.h"
#include "d_timer.h"
#include "d_model_parser.h"


namespace {

    class Task_LoadModel : public dal::ITask {

    public:
        dal::Filesystem& m_filesys;
        dal::ResPath m_respath;

        std::optional<dal::ModelStatic> out_model_data;

    public:
        Task_LoadModel(const dal::ResPath& respath, dal::Filesystem& filesys)
            : m_filesys(filesys)
            , m_respath(respath)
        {

        }

        void run() override {
            dal::Timer timer;

            auto file = this->m_filesys.open(this->m_respath);
            const auto model_content = file->read_stl<std::vector<uint8_t>>();
            dalInfo(fmt::format("Model res loaded ({}): {}", timer.check_get_elapsed(), this->m_respath.make_str()).c_str());

            this->out_model_data = dal::parse_model_dmd(model_content->data(), model_content->size());
            dalInfo(fmt::format("Model res parsed ({}): {}", timer.check_get_elapsed(), this->m_respath.make_str()).c_str());
        }

    };

}


// RenderUnit
namespace dal {

    void RenderUnit::destroy(const VkDevice logi_device) {
        this->m_material.m_ubuf.destroy(logi_device);
        this->m_vert_buffer.destroy(logi_device);
    }

    bool RenderUnit::prepare(
        DescPool& desc_pool,
        const VkSampler sampler,
        const VkDescriptorSetLayout layout_per_material,
        const VkDevice logi_device
    ) {
        if (this->is_ready())
            return true;
        if (!this->m_material.m_albedo_map->is_ready())
            return false;

        this->m_material.m_descset = desc_pool.allocate(layout_per_material, logi_device);

        auto albedo_map = reinterpret_cast<TextureUnit*>(this->m_material.m_albedo_map.get());

        this->m_material.m_descset.record_material(
            this->m_material.m_ubuf,
            albedo_map->view().get(),
            sampler,
            logi_device
        );

        return true;
    }

    bool RenderUnit::is_ready() const {
        if (!this->m_material.m_descset.is_ready())
            return false;

        return true;
    }

}


// ModelRenderer
namespace dal {

    void ModelRenderer::init(
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_ubuf_per_actor.init(phys_device, logi_device);
    }

    void ModelRenderer::upload_meshes(
        const dal::ModelStatic& model_data,
        dal::CommandPool& cmd_pool,
        TextureManager& tex_man,
        const char* const fallback_file_namespace,
        const VkDescriptorSetLayout layout_per_actor,
        const VkDescriptorSetLayout layout_per_material,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_desc_pool.init(
            1 * model_data.m_units.size() + 5,
            1 * model_data.m_units.size() + 5,
            5,
            1 * model_data.m_units.size() + 5,
            logi_device
        );

        this->m_desc_per_actor = this->m_desc_pool.allocate(layout_per_actor, logi_device);
        this->m_desc_per_actor.record_per_actor(this->m_ubuf_per_actor, logi_device);

        for (auto& unit_data : model_data.m_units) {
            auto& unit = this->m_units.emplace_back();

            unit.m_weight_center = unit_data.m_weight_center;

            unit.m_vert_buffer.init(
                unit_data.m_vertices,
                unit_data.m_indices,
                cmd_pool,
                graphics_queue,
                phys_device,
                logi_device
            );

            unit.m_material.m_alpha_blend = unit_data.m_material.m_alpha_blending;
            unit.m_material.m_data.m_roughness = unit_data.m_material.m_roughness;
            unit.m_material.m_data.m_metallic = unit_data.m_material.m_metallic;
            unit.m_material.m_ubuf.init(phys_device, logi_device);
            unit.m_material.m_ubuf.copy_to_buffer(unit.m_material.m_data, logi_device);

            const auto albedo_map_path = fmt::format("{}/?/{}", fallback_file_namespace, unit_data.m_material.m_albedo_map);
            unit.m_material.m_albedo_map = tex_man.request_asset_tex(albedo_map_path);
        }
    }

    void ModelRenderer::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_units)
            x.destroy(logi_device);

        this->m_units.clear();
        this->m_desc_pool.destroy(logi_device);
        this->m_ubuf_per_actor.destroy(logi_device);
    }

    bool ModelRenderer::fetch_one_resource(const VkDescriptorSetLayout layout_per_material, const VkSampler sampler, const VkDevice logi_device) {
        for (auto& unit : this->m_units) {
            if (unit.is_ready())
                continue;
            if (unit.prepare(this->m_desc_pool, sampler, layout_per_material, logi_device))
                return true;
        }

        return false;
    }

    bool ModelRenderer::is_ready() const {
        if (this->m_units.empty())
            return false;

        for (auto& unit : this->m_units) {
            if (!unit.is_ready())
                return false;
        }

        return true;
    }

}


// ModelManager
namespace dal {

    void ModelManager::destroy(const VkDevice logi_device) {
        this->m_sent_task.clear();
        this->m_cmd_pool.destroy(logi_device);

        for (auto& x : this->m_models) {
            x.second.destroy(logi_device);
        }
        this->m_models.clear();
    }

    void ModelManager::update() {
        for (auto& [res_id, model] : this->m_models) {
            const auto did_fetch = model.fetch_one_resource(
                this->m_desc_layout_man->layout_per_material(),
                this->m_tex_man->sampler_tex().get(),
                this->m_logi_device
            );

            if (did_fetch)
                return;
        }
    }

    void ModelManager::notify_task_done(std::unique_ptr<ITask> task) {
        const auto iter = this->m_sent_task.find(task.get());
        if (this->m_sent_task.end() != iter) {
            auto task_load = reinterpret_cast<::Task_LoadModel*>(task.get());

            iter->second->upload_meshes(
                task_load->out_model_data.value(),
                this->m_cmd_pool,
                *this->m_tex_man,
                task_load->m_respath.dir_list().front().c_str(),
                this->m_desc_layout_man->layout_per_actor(),
                this->m_desc_layout_man->layout_per_material(),
                this->m_graphics_queue,
                this->m_phys_device,
                this->m_logi_device
            );

            this->m_sent_task.erase(iter);
        }
    }

    ModelRenderer& ModelManager::request_model(const dal::ResPath& respath) {
        const auto resolved_path = this->m_filesys->resolve_respath(respath);
        const auto respath_str = resolved_path->make_str();

        const auto iter = this->m_models.find(respath_str);
        if (this->m_models.end() == iter) {
            this->m_models[respath_str] = ModelRenderer{};
            auto& model = this->m_models[respath_str];

            model.init(
                this->m_phys_device,
                this->m_logi_device
            );

            std::unique_ptr<dal::ITask> task{ new ::Task_LoadModel(respath, *this->m_filesys) };
            this->m_sent_task[task.get()] = &model;
            this->m_task_man->order_task(std::move(task), this);

            return model;
        }
        else {
            return iter->second;
        }
    }

}
