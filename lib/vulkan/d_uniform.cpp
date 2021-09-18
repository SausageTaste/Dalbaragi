#include "d_uniform.h"

#include <array>

#include "d_logger.h"
#include "d_static_list.h"


namespace {

    constexpr int MAX_PUSH_CONST_SIZE = 128;
    constexpr int MAX_UBUF_SIZE = 16 * 1024;  // 16 KB

    static_assert(MAX_PUSH_CONST_SIZE >= sizeof(dal::U_PC_Shadow));
    //static_assert(MAX_PUSH_CONST_SIZE >= sizeof(dal::U_PC_OnMirror));

    static_assert(MAX_UBUF_SIZE >= sizeof(dal::U_Shader_Final));
    static_assert(MAX_UBUF_SIZE >= sizeof(dal::U_CameraTransform));
    static_assert(MAX_UBUF_SIZE >= sizeof(dal::U_PerMaterial));
    static_assert(MAX_UBUF_SIZE >= sizeof(dal::U_PerActor));
    static_assert(MAX_UBUF_SIZE >= sizeof(dal::U_AnimTransform));
    static_assert(MAX_UBUF_SIZE >= sizeof(dal::U_GlobalLight));

}


namespace {

    class DescLayoutBuilder {

    private:
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;

    public:
        uint32_t size() const noexcept {
            return this->m_bindings.size();
        }

        auto data() const noexcept {
            return this->m_bindings.data();
        }

        VkDescriptorSetLayoutCreateInfo make_create_info() const noexcept {
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

}


// Descriptor set layouts
namespace dal {

    void IDescSetLayout::destroy(const VkDevice logi_device) noexcept {
        if (VK_NULL_HANDLE != this->m_layout) {
            vkDestroyDescriptorSetLayout(logi_device, this->m_layout, nullptr);
            this->m_layout = VK_NULL_HANDLE;
        }
    }

    void IDescSetLayout::build(const VkDescriptorSetLayoutCreateInfo& create_info, const VkDevice logi_device) noexcept {
        this->destroy(logi_device);

        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logi_device, &create_info, nullptr, &this->m_layout))
            dalAbort("failed to create descriptor set layout!");
    }


    void DescLayout_Final::init(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        // rendered color result
        bindings.add_combined_img_sampler(VK_SHADER_STAGE_FRAGMENT_BIT);
        // U_Shader_Final
        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT);

        this->build(bindings.make_create_info(), logi_device);
    }

    void DescLayout_PerGlobal::init(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        // U_CameraTransform
        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        // U_GlobalLight
        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);

        this->build(bindings.make_create_info(), logi_device);
    }

    void DescLayout_PerMaterial::init(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        // U_PerMaterial
        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);
        // albedo map
        bindings.add_combined_img_sampler(VK_SHADER_STAGE_FRAGMENT_BIT);

        this->build(bindings.make_create_info(), logi_device);
    }

    void DescLayout_PerActor::init(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        // U_PerActor
        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT);

        this->build(bindings.make_create_info(), logi_device);
    }

    void DescLayout_ActorAnimated::init(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        // U_PerActor
        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT);
        // U_AnimTransform
        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT);

        this->build(bindings.make_create_info(), logi_device);
    }

    void DescLayout_Composition::init(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings{};

        bindings.add_attach(VK_SHADER_STAGE_FRAGMENT_BIT);
        bindings.add_attach(VK_SHADER_STAGE_FRAGMENT_BIT);
        bindings.add_attach(VK_SHADER_STAGE_FRAGMENT_BIT);
        bindings.add_attach(VK_SHADER_STAGE_FRAGMENT_BIT);

        // Ubuf U_GlobalLight
        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);
        // Ubuf U_CameraTransform
        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);
        // directional light shadow maps
        bindings.add_combined_img_sampler(VK_SHADER_STAGE_FRAGMENT_BIT, dal::MAX_DLIGHT_COUNT);
        // spot light shadow maps
        bindings.add_combined_img_sampler(VK_SHADER_STAGE_FRAGMENT_BIT, dal::MAX_SLIGHT_COUNT);

        this->build(bindings.make_create_info(), logi_device);
    }

    void DescLayout_Alpha::init(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        // U_CameraTransform
        bindings.add_ubuf(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        // U_GlobalLight
        bindings.add_ubuf(VK_SHADER_STAGE_FRAGMENT_BIT);
        // directional light shadow maps
        bindings.add_combined_img_sampler(VK_SHADER_STAGE_FRAGMENT_BIT, dal::MAX_DLIGHT_COUNT);
        // spot light shadow maps
        bindings.add_combined_img_sampler(VK_SHADER_STAGE_FRAGMENT_BIT, dal::MAX_SLIGHT_COUNT);

        this->build(bindings.make_create_info(), logi_device);
    }

    void DescLayout_Mirror::init(const VkDevice logi_device) {
        ::DescLayoutBuilder bindings;

        // reflection map
        bindings.add_combined_img_sampler(VK_SHADER_STAGE_FRAGMENT_BIT);

        this->build(bindings.make_create_info(), logi_device);
    }

}


// DescSetLayoutManager
namespace dal {

    void DescSetLayoutManager::init(const VkDevice logiDevice) {
        this->destroy(logiDevice);

        this->m_layout_final.init(logiDevice);

        this->m_layout_per_global.init(logiDevice);
        this->m_layout_per_material.init(logiDevice);
        this->m_layout_per_actor.init(logiDevice);
        this->m_layout_actor_animated.init(logiDevice);

        this->m_layout_composition.init(logiDevice);
        this->m_layout_alpha.init(logiDevice);
        this->m_layout_mirror.init(logiDevice);
    }

    void DescSetLayoutManager::destroy(const VkDevice logiDevice) {
        this->m_layout_final.destroy(logiDevice);

        this->m_layout_per_global.destroy(logiDevice);
        this->m_layout_per_material.destroy(logiDevice);
        this->m_layout_per_actor.destroy(logiDevice);
        this->m_layout_actor_animated.destroy(logiDevice);

        this->m_layout_composition.destroy(logiDevice);
        this->m_layout_alpha.destroy(logiDevice);
        this->m_layout_mirror.destroy(logiDevice);
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

        template <typename _ImgViewIterator>
        size_t add_img_samplers(const _ImgViewIterator begin, const _ImgViewIterator end, const VkSampler sampler) {
            const auto first_info_index = this->m_image_info.m_size;

            for (auto iter = begin; iter != end; ++iter) {
                auto& info = this->m_image_info.emplace_back();
                info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                info.imageView = *iter;
                info.sampler = sampler;
            }

            const auto info_count = this->m_image_info.m_size - first_info_index;
            const auto index = this->m_desc_writes.size();

            auto& write = this->m_desc_writes.emplace_back();
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = this->m_desc_set;
            write.dstBinding = index;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = info_count;
            write.pBufferInfo = nullptr;
            write.pImageInfo = &this->m_image_info.m_data[first_info_index];
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

    DescSet::DescSet(DescSet&& other) {
        std::swap(this->m_handle, other.m_handle);
        std::swap(this->m_layout, other.m_layout);
    }

    DescSet& DescSet::operator=(DescSet&& other) {
        std::swap(this->m_handle, other.m_handle);
        std::swap(this->m_layout, other.m_layout);

        return *this;
    }

    bool DescSet::is_ready() const {
        return VK_NULL_HANDLE != this->m_handle;
    }

    void DescSet::record_final(
        const VkImageView color_view,
        const SamplerTexture& sampler,
        const UniformBuffer<U_Shader_Final>& ubuf_per_frame,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_img_sampler(color_view, sampler.get());
        desc_writes.add_buffer(ubuf_per_frame);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_per_global(
        const UniformBuffer<U_CameraTransform>& ubuf_per_frame,
        const UniformBuffer<U_GlobalLight>& ubuf_global_light,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_buffer(ubuf_per_frame);
        desc_writes.add_buffer(ubuf_global_light);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_material(
        const UniformBuffer<U_PerMaterial>& ubuf_per_material,
        const VkImageView texture_view,
        const SamplerTexture& sampler,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_buffer(ubuf_per_material);
        desc_writes.add_img_sampler(texture_view, sampler.get());

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

    void DescSet::record_actor_animated(
        const UniformBuffer<U_PerActor>& ubuf_per_actor,
        const UniformBuffer<U_AnimTransform>& ubuf_animation,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_buffer(ubuf_per_actor);
        desc_writes.add_buffer(ubuf_animation);

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_composition(
        const std::vector<VkImageView>& attachment_views,
        const UniformBuffer<U_GlobalLight>& ubuf_global_light,
        const UniformBuffer<U_CameraTransform>& ubuf_per_frame,
        const std::array<VkImageView, dal::MAX_DLIGHT_COUNT>& dlight_shadow_maps,
        const std::array<VkImageView, dal::MAX_SLIGHT_COUNT>& slight_shadow_maps,
        const SamplerDepth& sampler,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        for (size_t i = 0; i < attachment_views.size(); ++i)
            desc_writes.add_input_attachment(attachment_views.at(i));

        const auto global_light_index = desc_writes.add_buffer(ubuf_global_light);
        dalAssert(4 == global_light_index);

        desc_writes.add_buffer(ubuf_per_frame);
        desc_writes.add_img_samplers(dlight_shadow_maps.begin(), dlight_shadow_maps.end(), sampler.get());
        desc_writes.add_img_samplers(slight_shadow_maps.begin(), slight_shadow_maps.end(), sampler.get());

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_alpha(
        const UniformBuffer<U_CameraTransform>& ubuf_per_frame,
        const UniformBuffer<U_GlobalLight>& ubuf_global_light,
        const std::array<VkImageView, dal::MAX_DLIGHT_COUNT>& dlight_shadow_maps,
        const std::array<VkImageView, dal::MAX_SLIGHT_COUNT>& slight_shadow_maps,
        const SamplerDepth& sampler,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_buffer(ubuf_per_frame);
        desc_writes.add_buffer(ubuf_global_light);
        desc_writes.add_img_samplers(dlight_shadow_maps.begin(), dlight_shadow_maps.end(), sampler.get());
        desc_writes.add_img_samplers(slight_shadow_maps.begin(), slight_shadow_maps.end(), sampler.get());

        vkUpdateDescriptorSets(logi_device, desc_writes.size(), desc_writes.data(), 0, nullptr);
    }

    void DescSet::record_mirror(
        const VkImageView texture_view,
        const SamplerTexture& sampler,
        const VkDevice logi_device
    ) {
        ::WriteDescBuilder desc_writes{ this->m_handle };

        desc_writes.add_img_sampler(texture_view, sampler.get());

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

    DescSet DescPool::allocate(const dal::IDescSetLayout& layout, const VkDevice logi_device) {
        return std::move(this->allocate(1, layout, logi_device).at(0));
    }

    std::vector<DescSet> DescPool::allocate(const uint32_t count, const dal::IDescSetLayout& layout, const VkDevice logi_device) {
        std::vector<DescSet> result;

        const std::vector<VkDescriptorSetLayout> layouts(count, layout.get());

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
        for (auto& x : desc_sets) {
            result.emplace_back().set(std::move(x), layout.get());
        }

        return result;
    }

}


// DescAllocator
namespace dal {

    void DescAllocator::init(
        const uint32_t uniform_buf_count,
        const uint32_t image_sampler_count,
        const uint32_t input_attachment_count,
        const uint32_t desc_set_count,
        const VkDevice logi_device
    ) {
        this->m_pool.init(
            uniform_buf_count,
            image_sampler_count,
            input_attachment_count,
            desc_set_count,
            logi_device
        );
    }

    void DescAllocator::destroy(const VkDevice logi_device) {
        this->m_waiting_queue.clear();
        this->m_pool.destroy(logi_device);
    }

    void DescAllocator::reset(const VkDevice logi_device) {
        this->m_pool.reset(logi_device);
    }

    DescSet DescAllocator::allocate(const dal::IDescSetLayout& layout, const VkDevice logi_device) {
        ++this->m_allocated_outside;
        auto& queue = this->get_queue(layout.get());

        if (queue.empty()) {
            return this->m_pool.allocate(layout, logi_device);
        }
        else {
            auto output = std::move(queue.front());
            queue.pop_front();
            return output;
        }
    }

    std::vector<DescSet> DescAllocator::allocate(const uint32_t count, const dal::IDescSetLayout& layout, const VkDevice logi_device) {
        std::vector<DescSet> output;

        for (uint32_t i = 0; i < count; ++i)
            output.push_back(this->allocate(layout, logi_device));

        return output;
    }

    void DescAllocator::free(DescSet&& desc_set) {
        --this->m_allocated_outside;
        this->get_queue(desc_set.layout()).push_back(std::move(desc_set));
    }

    // Private

    std::deque<DescSet>& DescAllocator::get_queue(const VkDescriptorSetLayout layout) {
        auto iter = this->m_waiting_queue.find(layout);
        if (this->m_waiting_queue.end() != iter) {
            return iter->second;
        }
        else {
            return this->m_waiting_queue.emplace(layout, std::deque<DescSet>{}).first->second;
        }
    }

}


// DescriptorManager
namespace dal {

    void DescriptorManager::init(const uint32_t swapchain_count, const VkDevice logi_device) {
        this->destroy(logi_device);

        constexpr uint32_t POOL_SIZE_MULTIPLIER = 50;

        this->m_pool.init(
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            logi_device
        );
    }

    void DescriptorManager::destroy(VkDevice logiDevice) {
        this->m_pool.destroy(logiDevice);
        this->m_descset_per_global.clear();
        this->m_descset_final.clear();
        this->m_descset_composition.clear();
        this->m_descset_alpha.clear();
    }

    DescSet& DescriptorManager::add_descset_per_global(const dal::DescLayout_PerGlobal& desc_layout_per_global, const VkDevice logi_device) {
        auto& new_desc = this->m_descset_per_global.emplace_back();
        new_desc = this->m_pool.allocate(desc_layout_per_global, logi_device);
        return new_desc;
    }

    DescSet& DescriptorManager::add_descset_final(const dal::DescLayout_Final& desc_layout_final, const VkDevice logi_device) {
        auto& new_desc = this->m_descset_final.emplace_back();
        new_desc = this->m_pool.allocate(desc_layout_final, logi_device);
        return new_desc;
    }

    DescSet& DescriptorManager::add_descset_composition(const dal::DescLayout_Composition& desc_layout_composition, const VkDevice logi_device) {
        auto& new_desc = this->m_descset_composition.emplace_back();
        new_desc = this->m_pool.allocate(desc_layout_composition, logi_device);
        return new_desc;
    }

    DescSet& DescriptorManager::add_descset_alpha(const dal::DescLayout_Alpha& desc_layout_alpha, const VkDevice logi_device) {
        auto& new_desc = this->m_descset_alpha.emplace_back();
        new_desc = this->m_pool.allocate(desc_layout_alpha, logi_device);
        return new_desc;
    }

}
