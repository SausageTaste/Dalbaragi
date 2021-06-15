#include "d_uniform.h"

#include <array>

#include "d_logger.h"
#include "d_static_list.h"


namespace {

    class DescLayoutBuilder {

    private:
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;

    public:
        uint32_t size() const {
            return this->m_bindings.size();
        }

        auto data() const {
            return this->m_bindings.data();
        }

        VkDescriptorSetLayoutCreateInfo make_create_info() const {
            VkDescriptorSetLayoutCreateInfo layout_info{};
            layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_info.pBindings = this->data();
            layout_info.bindingCount = this->size();

            return layout_info;
        }

        void add_ubuf(const VkShaderStageFlags stage_flags, const uint32_t desc_count = 1) {
            this->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage_flags, desc_count);
        }

        void add_attach(const VkShaderStageFlags stage_flags, const uint32_t desc_count = 1) {
            this->add(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, stage_flags, desc_count);
        }

        void add_combined_img_sampler(const VkShaderStageFlags stage_flags, const uint32_t desc_count = 1) {
            this->add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage_flags, desc_count);
        }

    private:
        void add(const VkDescriptorType desc_type, const VkShaderStageFlags stage_flags, const uint32_t desc_count) {
            const auto index = this->m_bindings.size();
            auto& a = this->m_bindings.emplace_back();

            a.binding = index;
            a.descriptorType = desc_type;
            a.stageFlags = stage_flags;
            a.descriptorCount = desc_count;
            a.pImmutableSamplers = nullptr;
        }

    };


    VkDescriptorSetLayout create_layout_final(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        bindings.add_combined_img_sampler(VK_SHADER_STAGE_FRAGMENT_BIT);
        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT);

        //----------------------------------------------------------------------------------

        const auto layout_info = bindings.make_create_info();

        VkDescriptorSetLayout output = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &layout_info, nullptr, &output))
            dalAbort("failed to create descriptor set layout!");

        return output;
    }

    VkDescriptorSetLayout create_layout_per_frame(const VkDevice logiDevice) {
        ::DescLayoutBuilder bindings;

        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT);

        //----------------------------------------------------------------------------------

        const auto layout_info = bindings.make_create_info();

        VkDescriptorSetLayout output = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logiDevice, &layout_info, nullptr, &output))
            dalAbort("failed to create descriptor set layout!");

        return output;
    }

    VkDescriptorSetLayout create_layout_per_material(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);
        bindings.add_combined_img_sampler(VK_SHADER_STAGE_FRAGMENT_BIT);

        //----------------------------------------------------------------------------------

        const auto layout_info = bindings.make_create_info();

        VkDescriptorSetLayout output = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &layout_info, nullptr, &output))
            dalAbort("failed to create descriptor set layout!");

        return output;
    }

    VkDescriptorSetLayout create_layout_per_actor(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT);

        //----------------------------------------------------------------------------------

        const auto layout_info = bindings.make_create_info();

        VkDescriptorSetLayout output = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &layout_info, nullptr, &output))
            dalAbort("failed to create descriptor set layout!");

        return output;
    }

    VkDescriptorSetLayout create_layout_per_world(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);  // U_GlobalLight
        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);  // U_PerFrame_Alpha

        //----------------------------------------------------------------------------------

        const auto layout_info = bindings.make_create_info();

        VkDescriptorSetLayout result = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &layout_info, nullptr, &result)) {
            dalAbort("failed to create descriptor set layout!");
        }

        return result;
    }

    VkDescriptorSetLayout create_layout_animation(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT);  // U_AnimTransform

        //----------------------------------------------------------------------------------

        const auto layout_info = bindings.make_create_info();

        VkDescriptorSetLayout result = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &layout_info, nullptr, &result))
            dalAbort("failed to create descriptor set layout!");

        return result;
    }

    VkDescriptorSetLayout create_layout_composition(const VkDevice logiDevice) {
        ::DescLayoutBuilder bindings{};

        bindings.add_attach(VK_SHADER_STAGE_FRAGMENT_BIT);
        bindings.add_attach(VK_SHADER_STAGE_FRAGMENT_BIT);
        bindings.add_attach(VK_SHADER_STAGE_FRAGMENT_BIT);
        bindings.add_attach(VK_SHADER_STAGE_FRAGMENT_BIT);

        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);  // Ubuf U_GlobalLight
        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);  // Ubuf U_PerFrame_Composition

        //----------------------------------------------------------------------------------

        const auto layout_info = bindings.make_create_info();

        VkDescriptorSetLayout result = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logiDevice, &layout_info, nullptr, &result))
            dalAbort("failed to create descriptor set layout!");

        return result;
    }

}


// DescSetLayoutManager
namespace dal {

    void DescSetLayoutManager::init(const VkDevice logiDevice) {
        this->destroy(logiDevice);

        this->m_layout_final = ::create_layout_final(logiDevice);

        this->m_layout_per_frame = ::create_layout_per_frame(logiDevice);
        this->m_layout_per_material = ::create_layout_per_material(logiDevice);
        this->m_layout_per_actor = ::create_layout_per_actor(logiDevice);
        this->m_layout_per_world = ::create_layout_per_world(logiDevice);

        this->m_layout_animation = ::create_layout_animation(logiDevice);

        this->m_layout_composition = ::create_layout_composition(logiDevice);
    }

    void DescSetLayoutManager::destroy(const VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->m_layout_final) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_final, nullptr);
            this->m_layout_final = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_layout_per_frame) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_per_frame, nullptr);
            this->m_layout_per_frame = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_layout_per_material) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_per_material, nullptr);
            this->m_layout_per_material = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_layout_per_actor) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_per_actor, nullptr);
            this->m_layout_per_actor = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_layout_per_world) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_per_world, nullptr);
            this->m_layout_per_world = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_layout_animation) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_animation, nullptr);
            this->m_layout_animation = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_layout_composition) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_composition, nullptr);
            this->m_layout_composition = VK_NULL_HANDLE;
        }
    }

}


namespace {

    class WriteDescBuilder {

    private:
        VkDescriptorSet m_desc_set = VK_NULL_HANDLE;
        std::vector<VkWriteDescriptorSet> m_desc_writes;

        dal::StaticVector<VkDescriptorBufferInfo, 16> m_buffer_info;
        dal::StaticVector<VkDescriptorImageInfo, 16> m_image_info;

    public:
        WriteDescBuilder(const VkDescriptorSet desc_set)
            : m_desc_set(desc_set)
        {

        }

        uint32_t size() const {
            return this->m_desc_writes.size();
        }

        const VkWriteDescriptorSet* data() const {
            return this->m_desc_writes.data();
        }

        template <typename T>
        size_t add_buffer(const dal::UniformBuffer<T>& ubuf) {
            auto& info = this->m_buffer_info.emplace_back();
            info.buffer = ubuf.buffer();
            info.range = ubuf.data_size();
            info.offset = 0;

            const auto index = this->m_desc_writes.size();

            auto& write = this->m_desc_writes.emplace_back();
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = this->m_desc_set;
            write.dstBinding = index;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &info;
            write.pImageInfo = nullptr;
            write.pTexelBufferView = nullptr;

            return index;
        }

        size_t add_img_sampler(const VkImageView img_view, const VkSampler sampler) {
            auto& info = this->m_image_info.emplace_back();
            info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info.imageView = img_view;
            info.sampler = sampler;

            const auto index = this->m_desc_writes.size();

            auto& write = this->m_desc_writes.emplace_back();
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = this->m_desc_set;
            write.dstBinding = index;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pBufferInfo = nullptr;
            write.pImageInfo = &info;
            write.pTexelBufferView = nullptr;

            return index;
        }

        size_t add_input_attachment(const VkImageView img_view) {
            auto& info = this->m_image_info.emplace_back();
            info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info.imageView = img_view;
            info.sampler = VK_NULL_HANDLE;

            const auto index = this->m_desc_writes.size();

            auto& write = this->m_desc_writes.emplace_back();
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = this->m_desc_set;
            write.dstBinding = index;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            write.descriptorCount = 1;
            write.pBufferInfo = nullptr;
            write.pImageInfo = &info;
            write.pTexelBufferView = nullptr;

            return index;
        }

    };

}


// DescSet
namespace dal {

    bool DescSet::is_ready() const {
        return VK_NULL_HANDLE != this->m_handle;
    }

    void DescSet::record_final(
        const VkImageView color_view,
        const VkSampler sampler,
        const UniformBuffer<U_PerFrame_InFinal>& ubuf_per_frame,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_img_sampler(color_view, sampler);
        desc_writes.add_buffer(ubuf_per_frame);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_per_frame(
        const UniformBuffer<U_PerFrame>& ubuf_per_frame,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_buffer(ubuf_per_frame);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_material(
        const UniformBuffer<U_PerMaterial>& ubuf_per_material,
        const VkImageView texture_view,
        const VkSampler sampler,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_buffer(ubuf_per_material);
        desc_writes.add_img_sampler(texture_view, sampler);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_per_actor(
        const UniformBuffer<U_PerActor>& ubuf_per_actor,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_buffer(ubuf_per_actor);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_per_world(
        const UniformBuffer<U_GlobalLight>& ubuf_global_light,
        const UniformBuffer<U_PerFrame_Alpha>& ubuf_per_frame_alpha,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_buffer(ubuf_global_light);
        desc_writes.add_buffer(ubuf_per_frame_alpha);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_animation(
        const UniformBuffer<U_AnimTransform>& ubuf_animation,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_buffer(ubuf_animation);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_composition(
        const std::vector<VkImageView>& attachment_views,
        const UniformBuffer<U_GlobalLight>& ubuf_global_light,
        const UniformBuffer<U_PerFrame_Composition>& ubuf_per_frame,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        for (size_t i = 0; i < attachment_views.size(); ++i)
            desc_writes.add_input_attachment(attachment_views.at(i));

        const auto global_light_index = desc_writes.add_buffer(ubuf_global_light);
        dalAssert(4 == global_light_index);

        desc_writes.add_buffer(ubuf_per_frame);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

}


// DescPool
namespace dal {

    void DescPool::init(
        const uint32_t uniform_buf_count,
        const uint32_t image_sampler_count,
        const uint32_t input_attachment_count,
        const uint32_t desc_set_count,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = uniform_buf_count;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = image_sampler_count;
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        poolSizes[2].descriptorCount = input_attachment_count;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = desc_set_count;

        if (VK_SUCCESS != vkCreateDescriptorPool(logi_device, &poolInfo, nullptr, &this->m_handle)) {
            dalAbort("failed to create descriptor pool!");
        }
    }

    void DescPool::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_handle) {
            vkDestroyDescriptorPool(logi_device, this->m_handle, nullptr);
            this->m_handle = VK_NULL_HANDLE;
        }
    }

    void DescPool::reset(const VkDevice logi_device) {
        if (VK_SUCCESS != vkResetDescriptorPool(logi_device, this->get(), 0)) {
            dalAbort("failed to reset descriptor pool");
        }
    }

    DescSet DescPool::allocate(const VkDescriptorSetLayout layout, const VkDevice logi_device) {
        return this->allocate(1, layout, logi_device).at(0);
    }

    std::vector<DescSet> DescPool::allocate(const uint32_t count, const VkDescriptorSetLayout layout, const VkDevice logi_device) {
        std::vector<DescSet> result;

        const std::vector<VkDescriptorSetLayout> layouts(count, layout);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = this->get();
        alloc_info.descriptorSetCount = count;
        alloc_info.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> desc_sets(count);
        if (VK_SUCCESS != vkAllocateDescriptorSets(logi_device, &alloc_info, desc_sets.data())) {
            dalAbort("failed to allocate descriptor sets!");
        }

        result.reserve(count);
        for (const auto x : desc_sets) {
            result.emplace_back().set(x);
        }

        return result;
    }

}


// DescriptorManager
namespace dal {

    void DescriptorManager::init(const uint32_t swapchain_count, const VkDevice logi_device) {
        this->destroy(logi_device);

        constexpr uint32_t POOL_SIZE_MULTIPLIER = 50;

        this->m_pool_simple.init(
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            logi_device
        );

        this->m_pool_composition.init(
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            logi_device
        );

        this->m_pool_final.resize(swapchain_count);
        for (auto& pool : this->m_pool_final) {
            pool.init(
                swapchain_count * POOL_SIZE_MULTIPLIER,
                swapchain_count * POOL_SIZE_MULTIPLIER,
                swapchain_count * POOL_SIZE_MULTIPLIER,
                swapchain_count * POOL_SIZE_MULTIPLIER,
                logi_device
            );
        }

        this->m_descset_final.resize(swapchain_count);
    }

    void DescriptorManager::destroy(VkDevice logiDevice) {
        this->m_pool_simple.destroy(logiDevice);
        this->m_pool_composition.destroy(logiDevice);

        for (auto& pool : this->m_pool_final) {
            pool.destroy(logiDevice);
        }
        this->m_pool_final.clear();

        this->m_descset_per_frame.clear();
        this->m_descset_composition.clear();
    }

    void DescriptorManager::init_desc_sets_per_frame(
        const dal::UniformBufferArray<U_PerFrame>& ubufs_simple,
        const uint32_t swapchain_count,
        const VkDescriptorSetLayout desc_layout_simple,
        const VkDevice logi_device
    ) {
        this->m_descset_per_frame = this->m_pool_simple.allocate(swapchain_count, desc_layout_simple, logi_device);

        for (size_t i = 0; i < this->m_descset_per_frame.size(); ++i) {
            auto& desc_set = this->m_descset_per_frame.at(i);
            desc_set.record_per_frame(ubufs_simple.at(i), logi_device);
        }
    }

    void DescriptorManager::init_desc_sets_per_world(
        const UniformBufferArray<U_GlobalLight>& ubufs_global_light,
        const UniformBufferArray<U_PerFrame_Alpha>& ubufs_per_frame_alpha,
        const uint32_t swapchain_count,
        const VkDescriptorSetLayout desc_layout_world,
        const VkDevice logi_device
    ) {
        this->m_descset_per_world = this->m_pool_simple.allocate(swapchain_count, desc_layout_world, logi_device);

        for (size_t i = 0; i < this->m_descset_per_world.size(); ++i) {
            auto& desc_set = this->m_descset_per_world.at(i);
            desc_set.record_per_world(ubufs_global_light.at(i), ubufs_per_frame_alpha.at(i), logi_device);
        }
    }

    void DescriptorManager::init_desc_sets_final(
        const uint32_t index,
        const UniformBuffer<U_PerFrame_InFinal>& ubuf_per_frame,
        const VkImageView color_view,
        const VkSampler sampler,
        const VkDescriptorSetLayout desc_layout_final,
        const VkDevice logi_device
    ) {
        this->m_pool_final.at(index).reset(logi_device);
        this->m_descset_final.at(index) = this->m_pool_final.at(index).allocate(desc_layout_final, logi_device);
        this->m_descset_final.at(index).record_final(color_view, sampler, ubuf_per_frame, logi_device);
    }

    void DescriptorManager::add_desc_set_composition(
        const std::vector<VkImageView>& attachment_views,
        const UniformBuffer<U_GlobalLight>& ubuf_global_light,
        const UniformBuffer<U_PerFrame_Composition>& ubuf_per_frame,
        const VkDescriptorSetLayout desc_layout_composition,
        const VkDevice logi_device
    ) {
        auto& new_desc = this->m_descset_composition.emplace_back();
        new_desc = this->m_pool_composition.allocate(desc_layout_composition, logi_device);
        new_desc.record_composition(attachment_views, ubuf_global_light, ubuf_per_frame, logi_device);
    }

}
