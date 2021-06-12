#include "d_uniform.h"

#include <array>

#include "d_logger.h"


namespace {

    VkDescriptorSetLayout create_layout_final(const VkDevice logi_device) {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

        bindings.at(0).binding = 0;
        bindings.at(0).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings.at(0).descriptorCount = 1;
        bindings.at(0).stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.at(0).pImmutableSamplers = nullptr;

        bindings.at(1).binding = 1;
        bindings.at(1).descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings.at(1).descriptorCount = 1;
        bindings.at(1).stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings.at(1).pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pBindings = bindings.data();
        layout_info.bindingCount = bindings.size();

        VkDescriptorSetLayout output = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &layout_info, nullptr, &output)) {
            dalAbort("failed to create descriptor set layout!");
        }

        return output;
    }

    VkDescriptorSetLayout create_layout_per_frame(const VkDevice logiDevice) {
        std::array<VkDescriptorSetLayoutBinding, 1> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pBindings = bindings.data();
        layout_info.bindingCount = bindings.size();

        VkDescriptorSetLayout output = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logiDevice, &layout_info, nullptr, &output)) {
            dalAbort("failed to create descriptor set layout!");
        }

        return output;
    }

    VkDescriptorSetLayout create_layout_per_material(const VkDevice logi_device) {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        bindings.at(1).binding = 1;
        bindings.at(1).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings.at(1).descriptorCount = 1;
        bindings.at(1).stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.at(1).pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pBindings = bindings.data();
        layout_info.bindingCount = bindings.size();

        VkDescriptorSetLayout output = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &layout_info, nullptr, &output)) {
            dalAbort("failed to create descriptor set layout!");
        }

        return output;
    }

    VkDescriptorSetLayout create_layout_per_actor(const VkDevice logi_device) {
        std::array<VkDescriptorSetLayoutBinding, 1> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pBindings = bindings.data();
        layout_info.bindingCount = bindings.size();

        VkDescriptorSetLayout output = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &layout_info, nullptr, &output)) {
            dalAbort("failed to create descriptor set layout!");
        }

        return output;
    }

    VkDescriptorSetLayout create_layout_per_world(const VkDevice logi_device) {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        // U_GlobalLight
        bindings.emplace_back();
        bindings.back().binding = bindings.size() - 1;
        bindings.back().descriptorCount = 1;
        bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // U_PerFrame_Alpha
        bindings.emplace_back();
        bindings.back().binding = bindings.size() - 1;
        bindings.back().descriptorCount = 1;
        bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        //----------------------------------------------------------------------------------

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = bindings.size();
        layout_info.pBindings = bindings.data();

        VkDescriptorSetLayout result = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &layout_info, nullptr, &result)) {
            dalAbort("failed to create descriptor set layout!");
        }

        return result;
    }

    VkDescriptorSetLayout create_layout_composition(const VkDevice logiDevice) {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        bindings.emplace_back();
        bindings.back().binding = bindings.size() - 1;
        bindings.back().descriptorCount = 1;
        bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.emplace_back();
        bindings.back().binding = bindings.size() - 1;
        bindings.back().descriptorCount = 1;
        bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.emplace_back();
        bindings.back().binding = bindings.size() - 1;
        bindings.back().descriptorCount = 1;
        bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.emplace_back();
        bindings.back().binding = bindings.size() - 1;
        bindings.back().descriptorCount = 1;
        bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Ubuf U_GlobalLight
        bindings.emplace_back();
        bindings.back().binding = bindings.size() - 1;
        bindings.back().descriptorCount = 1;
        bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Ubuf U_PerFrame_Composition
        bindings.emplace_back();
        bindings.back().binding = bindings.size() - 1;
        bindings.back().descriptorCount = 1;
        bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout result = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logiDevice, &layoutInfo, nullptr, &result)) {
            dalAbort("failed to create descriptor set layout!");
        }

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

        if (VK_NULL_HANDLE != this->m_layout_composition) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_composition, nullptr);
            this->m_layout_composition = VK_NULL_HANDLE;
        }
    }

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
        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = color_view;
        image_info.sampler = sampler;

        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = ubuf_per_frame.buffer();
        buffer_info.offset = 0;
        buffer_info.range = ubuf_per_frame.data_size();

        //--------------------------------------------------------------------

        std::array<VkWriteDescriptorSet, 2> desc_writes{};

        desc_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[0].dstSet = this->m_handle;
        desc_writes[0].dstBinding = 0;
        desc_writes[0].dstArrayElement = 0;
        desc_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc_writes[0].descriptorCount = 1;
        desc_writes[0].pImageInfo = &image_info;

        desc_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[1].dstSet = this->m_handle;
        desc_writes[1].dstBinding = 1;
        desc_writes[1].dstArrayElement = 0;
        desc_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc_writes[1].descriptorCount = 1;
        desc_writes[1].pBufferInfo = &buffer_info;

        //--------------------------------------------------------------------

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_per_frame(
        const UniformBuffer<U_PerFrame>& ubuf_per_frame,
        const VkDevice logi_device
    ) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = ubuf_per_frame.buffer();
        buffer_info.offset = 0;
        buffer_info.range = ubuf_per_frame.data_size();

        //--------------------------------------------------------------------

        std::array<VkWriteDescriptorSet, 1> desc_writes{};

        desc_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[0].dstSet = this->m_handle;
        desc_writes[0].dstBinding = 0;  // specified in shader code
        desc_writes[0].dstArrayElement = 0;
        desc_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc_writes[0].descriptorCount = 1;
        desc_writes[0].pBufferInfo = &buffer_info;
        desc_writes[0].pImageInfo = nullptr;
        desc_writes[0].pTexelBufferView = nullptr;

        //--------------------------------------------------------------------

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_material(
        const UniformBuffer<U_PerMaterial>& ubuf_per_material,
        const VkImageView texture_view,
        const VkSampler sampler,
        const VkDevice logi_device
    ) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = ubuf_per_material.buffer();
        buffer_info.offset = 0;
        buffer_info.range = ubuf_per_material.data_size();

        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = texture_view;
        image_info.sampler = sampler;

        //--------------------------------------------------------------------

        std::array<VkWriteDescriptorSet, 2> desc_writes{};

        desc_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[0].dstSet = this->m_handle;
        desc_writes[0].dstBinding = 0;  // specified in shader code
        desc_writes[0].dstArrayElement = 0;
        desc_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc_writes[0].descriptorCount = 1;
        desc_writes[0].pBufferInfo = &buffer_info;
        desc_writes[0].pImageInfo = nullptr;
        desc_writes[0].pTexelBufferView = nullptr;

        desc_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[1].dstSet = this->m_handle;
        desc_writes[1].dstBinding = 1;
        desc_writes[1].dstArrayElement = 0;
        desc_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc_writes[1].descriptorCount = 1;
        desc_writes[1].pImageInfo = &image_info;

        //--------------------------------------------------------------------

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_per_actor(
        const UniformBuffer<U_PerActor>& ubuf_per_actor,
        const VkDevice logi_device
    ) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = ubuf_per_actor.buffer();
        buffer_info.offset = 0;
        buffer_info.range = ubuf_per_actor.data_size();

        //--------------------------------------------------------------------

        std::array<VkWriteDescriptorSet, 1> desc_writes{};

        desc_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[0].dstSet = this->m_handle;
        desc_writes[0].dstBinding = 0;  // specified in shader code
        desc_writes[0].dstArrayElement = 0;
        desc_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc_writes[0].descriptorCount = 1;
        desc_writes[0].pBufferInfo = &buffer_info;
        desc_writes[0].pImageInfo = nullptr;
        desc_writes[0].pTexelBufferView = nullptr;

        //--------------------------------------------------------------------

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_per_world(
        const UniformBuffer<U_GlobalLight>& ubuf_global_light,
        const UniformBuffer<U_PerFrame_Alpha>& ubuf_per_frame_alpha,
        const VkDevice logi_device
    ) {
        VkDescriptorBufferInfo ubuf_info_global_light{};
        ubuf_info_global_light.buffer = ubuf_global_light.buffer();
        ubuf_info_global_light.offset = 0;
        ubuf_info_global_light.range = ubuf_global_light.data_size();

        VkDescriptorBufferInfo ubuf_info_per_frame_alpha{};
        ubuf_info_per_frame_alpha.buffer = ubuf_per_frame_alpha.buffer();
        ubuf_info_per_frame_alpha.range = ubuf_per_frame_alpha.data_size();
        ubuf_info_per_frame_alpha.offset = 0;

        //--------------------------------------------------------------------

        std::vector<VkWriteDescriptorSet> desc_writes{};

        // U_GlobalLight
        {
            auto& x = desc_writes.emplace_back();
            x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            x.dstSet = this->m_handle;
            x.dstBinding = desc_writes.size() - 1;
            x.dstArrayElement = 0;
            x.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            x.descriptorCount = 1;
            x.pBufferInfo = &ubuf_info_global_light;
            x.pImageInfo = nullptr;
            x.pTexelBufferView = nullptr;

            dalAssert(0 == x.dstBinding);
        }

        // U_PerFrame_Alpha
        {
            auto& x = desc_writes.emplace_back();
            x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            x.dstSet = this->m_handle;
            x.dstBinding = desc_writes.size() - 1;
            x.dstArrayElement = 0;
            x.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            x.descriptorCount = 1;
            x.pBufferInfo = &ubuf_info_per_frame_alpha;
            x.pImageInfo = nullptr;
            x.pTexelBufferView = nullptr;

            dalAssert(1 == x.dstBinding);
        }

        //--------------------------------------------------------------------

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_composition(
        const std::vector<VkImageView>& attachment_views,
        const UniformBuffer<U_GlobalLight>& ubuf_global_light,
        const UniformBuffer<U_PerFrame_Composition>& ubuf_per_frame,
        const VkDevice logi_device
    ) {
        std::vector<VkDescriptorImageInfo> attachments_info(attachment_views.size());
        for (size_t i = 0; i < attachment_views.size(); ++i) {
            auto& x = attachments_info.at(i);
            x.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            x.imageView = attachment_views.at(i);
            x.sampler = VK_NULL_HANDLE;
        }

        VkDescriptorBufferInfo ubuf_info_global_light{};
        ubuf_info_global_light.buffer = ubuf_global_light.buffer();
        ubuf_info_global_light.offset = 0;
        ubuf_info_global_light.range = ubuf_global_light.data_size();

        VkDescriptorBufferInfo ubuf_info_per_frame{};
        ubuf_info_per_frame.buffer = ubuf_per_frame.buffer();
        ubuf_info_per_frame.offset = 0;
        ubuf_info_per_frame.range = ubuf_per_frame.data_size();

        //--------------------------------------------------------------------

        std::vector<VkWriteDescriptorSet> desc_writes{};

        for (size_t i = 0; i < attachments_info.size(); ++i) {
            auto& x = desc_writes.emplace_back();

            x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            x.dstSet = this->m_handle;
            x.dstBinding = desc_writes.size() - 1;
            x.dstArrayElement = 0;
            x.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            x.descriptorCount = 1;
            x.pBufferInfo = nullptr;
            x.pImageInfo = &attachments_info[i];
            x.pTexelBufferView = nullptr;
        }

        {
            auto& x = desc_writes.emplace_back();
            x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            x.dstSet = this->m_handle;
            x.dstBinding = desc_writes.size() - 1;
            x.dstArrayElement = 0;
            x.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            x.descriptorCount = 1;
            x.pBufferInfo = &ubuf_info_global_light;
            x.pImageInfo = nullptr;
            x.pTexelBufferView = nullptr;
        }

        {
            auto& x = desc_writes.emplace_back();
            x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            x.dstSet = this->m_handle;
            x.dstBinding = desc_writes.size() - 1;
            x.dstArrayElement = 0;
            x.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            x.descriptorCount = 1;
            x.pBufferInfo = &ubuf_info_per_frame;
            x.pImageInfo = nullptr;
            x.pTexelBufferView = nullptr;
        }

        //--------------------------------------------------------------------

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
