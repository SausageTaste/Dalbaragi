#include "d_model_renderer.h"

#include <fmt/format.h>

#include "d_model_parser.h"


namespace {

    class Task_LoadModel : public dal::ITask {

    public:
        dal::Filesystem& m_filesys;
        dal::ResPath m_respath;

        std::optional<dal::ModelStatic> out_model_data;

    public:
        Task_LoadModel(const dal::ResPath& respath, dal::Filesystem filesys)
            : m_filesys(filesys)
            , m_respath(respath)
        {

        }

        void run() override {
            auto file = this->m_filesys.open(this->m_respath);
            const auto model_content = file->read_stl<std::vector<uint8_t>>();
            this->out_model_data = dal::parse_model_dmd(model_content->data(), model_content->size());
        }

    };

}


// ModelRenderer
namespace dal {

    void ModelRenderer::init(
        const dal::ModelStatic& model_data,
        dal::CommandPool& cmd_pool,
        TextureManager& tex_man,
        const char* const fallback_file_namespace,
        const VkDescriptorSetLayout layout_per_material,
        const VkDescriptorSetLayout layout_per_actor,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_desc_pool.init(
            1 * model_data.m_units.size() + 5,
            1 * model_data.m_units.size() + 5,
            5,
            1 * model_data.m_units.size() + 5,
            logi_device
        );

        this->m_ubuf_per_actor.init(phys_device, logi_device);

        this->m_desc_per_actor = this->m_desc_pool.allocate(layout_per_actor, logi_device);
        this->m_desc_per_actor.record_per_actor(this->m_ubuf_per_actor, logi_device);

        for (auto& unit_data : model_data.m_units) {
            auto& unit = this->m_units.emplace_back();

            unit.m_vert_buffer.init(
                unit_data.m_vertices,
                unit_data.m_indices,
                cmd_pool,
                graphics_queue,
                phys_device,
                logi_device
            );

            unit.m_ubuf.init(phys_device, logi_device);
            dal::U_PerMaterial ubuf_data;
            ubuf_data.m_roughness = unit_data.m_material.m_roughness;
            ubuf_data.m_metallic = unit_data.m_material.m_metallic;
            unit.m_ubuf.copy_to_buffer(ubuf_data, logi_device);

            const auto albedo_map_path = fmt::format("{}/?/{}", fallback_file_namespace, unit_data.m_material.m_albedo_map);

            unit.m_desc_set = this->m_desc_pool.allocate(layout_per_material, logi_device);
            unit.m_desc_set.record_material(
                unit.m_ubuf,
                tex_man.request_asset_tex(albedo_map_path).m_view.get(),
                tex_man.sampler_tex().get(),
                logi_device
            );
        }
    }

    void ModelRenderer::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_units) {
            x.m_vert_buffer.destroy(logi_device);
            x.m_ubuf.destroy(logi_device);
        }
        this->m_units.clear();
        this->m_desc_pool.destroy(logi_device);
    }

    bool ModelRenderer::is_ready() const {
        return !this->m_units.empty();
    }

}


// ModelManager
namespace dal {

    void ModelManager::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_models) {
            x.second.destroy(logi_device);
        }
        this->m_models.clear();
    }

    void ModelManager::notify_task_done(std::unique_ptr<ITask> task) {
        const auto iter = this->m_sent_task.find(task.get());
        if (this->m_sent_task.end() != iter) {
            auto task_load = reinterpret_cast<::Task_LoadModel*>(task.get());

            iter->second->init(
                task_load->out_model_data.value(),
                this->m_cmd_man->pool_single_time(),
                *this->m_tex_man,
                task_load->m_respath.dir_list().front().c_str(),
                this->m_desc_layout_man->layout_per_material(),
                this->m_desc_layout_man->layout_per_actor(),
                this->m_graphics_queue,
                this->m_phys_device,
                this->m_logi_device
            );
        }
    }

    ModelRenderer& ModelManager::request_model(const dal::ResPath& respath) {
        const auto resolved_path = this->m_filesys->resolve_respath(respath);
        const auto respath_str = resolved_path->make_str();

        const auto iter = this->m_models.find(respath_str);
        if (this->m_models.end() == iter) {
            this->m_models[respath_str] = ModelRenderer{};
            auto& model = this->m_models[respath_str];

            std::unique_ptr<dal::ITask> task{ new ::Task_LoadModel(respath, *this->m_filesys) };
            this->m_sent_task[task.get()] = &model;

/*
            this->m_task_man->order_task(std::move(task), this);
/*/
            reinterpret_cast<::Task_LoadModel*>(task.get())->run();
            this->notify_task_done(std::move(task));
//*/

            return model;
        }
        else {
            return iter->second;
        }
    }

}
