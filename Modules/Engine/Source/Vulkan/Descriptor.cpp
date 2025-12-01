#include <Ignis/Vulkan.hpp>

namespace Ignis {
    void Vulkan::DescriptorSetLayoutBuilder::clear() {
        m_Flags = {};
        m_Bindings.clear();
        m_BindingFlags.clear();
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::setFlags(const vk::DescriptorSetLayoutCreateFlags flags) {
        m_Flags = flags;
        return *this;
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addBinding(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const uint32_t                   count,
        const vk::DescriptorType         type,
        const vk::ShaderStageFlags       stages) {
        vk::DescriptorSetLayoutBinding layout_binding{};
        layout_binding
            .setBinding(binding)
            .setDescriptorCount(count)
            .setDescriptorType(type)
            .setStageFlags(stages);
        m_Bindings.push_back(layout_binding);
        m_BindingFlags.push_back(flags);
        return *this;
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addBinding(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const vk::DescriptorType         type,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, 1, type, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addBinding(
        const uint32_t             binding,
        const uint32_t             count,
        const vk::DescriptorType   type,
        const vk::ShaderStageFlags stages) {
        return addBinding({}, binding, count, type, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addBinding(
        const uint32_t             binding,
        const vk::DescriptorType   type,
        const vk::ShaderStageFlags stages) {
        return addBinding({}, binding, 1, type, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addSampler(
        const vk::DescriptorBindingFlags flags,

        const uint32_t             binding,
        const uint32_t             count,
        const vk::ShaderStageFlags stages) {
        return addBinding(flags, binding, count, vk::DescriptorType::eSampler, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addCombinedImageSampler(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const uint32_t                   count,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, count, vk::DescriptorType::eCombinedImageSampler, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addSampledImage(
        const vk::DescriptorBindingFlags flags,

        const uint32_t             binding,
        const uint32_t             count,
        const vk::ShaderStageFlags stages) {
        return addBinding(flags, binding, count, vk::DescriptorType::eSampledImage, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addStorageImage(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const uint32_t                   count,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, count, vk::DescriptorType::eStorageImage, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addUniformBuffer(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const uint32_t                   count,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, count, vk::DescriptorType::eUniformBuffer, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addStorageBuffer(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const uint32_t                   count,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, count, vk::DescriptorType::eStorageBuffer, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addSampler(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, 1, vk::DescriptorType::eSampler, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addCombinedImageSampler(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, 1, vk::DescriptorType::eCombinedImageSampler, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addSampledImage(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, 1, vk::DescriptorType::eSampledImage, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addStorageImage(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, 1, vk::DescriptorType::eStorageImage, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addUniformBuffer(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, 1, vk::DescriptorType::eUniformBuffer, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addStorageBuffer(
        const vk::DescriptorBindingFlags flags,
        const uint32_t                   binding,
        const vk::ShaderStageFlags       stages) {
        return addBinding(flags, binding, 1, vk::DescriptorType::eStorageBuffer, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addSampler(
        const uint32_t             binding,
        const uint32_t             count,
        const vk::ShaderStageFlags stages) {
        return addSampler({}, binding, count, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addCombinedImageSampler(
        const uint32_t             binding,
        const uint32_t             count,
        const vk::ShaderStageFlags stages) {
        return addCombinedImageSampler({}, binding, count, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addSampledImage(
        const uint32_t             binding,
        const uint32_t             count,
        const vk::ShaderStageFlags stages) {
        return addSampledImage({}, binding, count, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addStorageImage(
        const uint32_t             binding,
        const uint32_t             count,
        const vk::ShaderStageFlags stages) {
        return addStorageImage({}, binding, count, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addUniformBuffer(
        const uint32_t             binding,
        const uint32_t             count,
        const vk::ShaderStageFlags stages) {
        return addUniformBuffer({}, binding, count, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addStorageBuffer(
        const uint32_t             binding,
        const uint32_t             count,
        const vk::ShaderStageFlags stages) {
        return addStorageBuffer({}, binding, count, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addSampler(
        const uint32_t             binding,
        const vk::ShaderStageFlags stages) {
        return addSampler({}, binding, 1, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addCombinedImageSampler(
        const uint32_t             binding,
        const vk::ShaderStageFlags stages) {
        return addCombinedImageSampler({}, binding, 1, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addSampledImage(
        const uint32_t             binding,
        const vk::ShaderStageFlags stages) {
        return addSampledImage({}, binding, 1, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addStorageImage(
        const uint32_t             binding,
        const vk::ShaderStageFlags stages) {
        return addStorageImage({}, binding, 1, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addUniformBuffer(
        const uint32_t             binding,
        const vk::ShaderStageFlags stages) {
        return addUniformBuffer({}, binding, 1, stages);
    }

    Vulkan::DescriptorSetLayoutBuilder &Vulkan::DescriptorSetLayoutBuilder::addStorageBuffer(
        const uint32_t             binding,
        const vk::ShaderStageFlags stages) {
        return addStorageBuffer({}, binding, 1, stages);
    }

    vk::DescriptorSetLayout Vulkan::DescriptorSetLayoutBuilder::build() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info{};
        binding_flags_create_info
            .setBindingFlags(m_BindingFlags);
        vk::DescriptorSetLayoutCreateInfo create_info{};
        create_info
            .setPNext(&binding_flags_create_info)
            .setFlags(m_Flags)
            .setBindings(m_Bindings);
        auto [result, layout] = s_pInstance->m_Device.createDescriptorSetLayout(create_info);
        DIGNIS_VK_CHECK(result);
        return layout;
    }

    void Vulkan::DescriptorSetWriter::clear() {
        m_ImageWrites.clear();
        m_ImageInfos.clear();
        m_BufferWrites.clear();
        m_BufferInfos.clear();
    }

    void Vulkan::DescriptorSetWriter::update(const vk::DescriptorSet set) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        std::vector<vk::WriteDescriptorSet> descriptor_writes{};
        descriptor_writes.reserve(m_ImageInfos.size() + m_BufferInfos.size());

        for (const auto &[binding,
                          index,
                          info_index,
                          type] :
             m_ImageWrites) {
            vk::WriteDescriptorSet descriptor_write{};
            descriptor_write
                .setDstSet(set)
                .setDstBinding(binding)
                .setDstArrayElement(index)
                .setDescriptorType(type)
                .setDescriptorCount(1)
                .setImageInfo(m_ImageInfos[info_index]);
            descriptor_writes.push_back(descriptor_write);
        }

        for (const auto &[binding,
                          index,
                          info_index,
                          type] :
             m_BufferWrites) {
            vk::WriteDescriptorSet descriptor_write{};
            descriptor_write
                .setDstSet(set)
                .setDstBinding(binding)
                .setDstArrayElement(index)
                .setDescriptorType(type)
                .setDescriptorCount(1)
                .setBufferInfo(m_BufferInfos[info_index]);
            descriptor_writes.push_back(descriptor_write);
        }

        s_pInstance->m_Device.updateDescriptorSets(descriptor_writes, {});
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeSampler(
        const uint32_t    binding,
        const uint32_t    index,
        const vk::Sampler sampler) {
        Write write{};
        write.Binding   = binding;
        write.Index     = index;
        write.Type      = vk::DescriptorType::eSampler;
        write.InfoIndex = m_ImageInfos.size();
        m_ImageWrites.push_back(write);

        vk::DescriptorImageInfo image_info{};
        image_info.setSampler(sampler);
        m_ImageInfos.push_back(image_info);

        return *this;
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeCombinedImageSampler(
        const uint32_t        binding,
        const uint32_t        index,
        const vk::ImageView   view,
        const vk::ImageLayout layout,
        const vk::Sampler     sampler) {
        Write write{};
        write.Binding   = binding;
        write.Index     = index;
        write.Type      = vk::DescriptorType::eCombinedImageSampler;
        write.InfoIndex = m_ImageInfos.size();
        m_ImageWrites.push_back(write);

        vk::DescriptorImageInfo image_info{};
        image_info
            .setImageView(view)
            .setImageLayout(layout)
            .setSampler(sampler);
        m_ImageInfos.push_back(image_info);

        return *this;
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeSampledImage(
        const uint32_t        binding,
        const uint32_t        index,
        const vk::ImageView   view,
        const vk::ImageLayout layout) {
        Write write{};
        write.Binding   = binding;
        write.Index     = index;
        write.Type      = vk::DescriptorType::eSampledImage;
        write.InfoIndex = m_ImageInfos.size();
        m_ImageWrites.push_back(write);

        vk::DescriptorImageInfo image_info{};
        image_info
            .setImageView(view)
            .setImageLayout(layout);
        m_ImageInfos.push_back(image_info);

        return *this;
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeStorageImage(
        const uint32_t        binding,
        const uint32_t        index,
        const vk::ImageView   view,
        const vk::ImageLayout layout) {
        Write write{};
        write.Binding   = binding;
        write.Index     = index;
        write.Type      = vk::DescriptorType::eStorageImage;
        write.InfoIndex = m_ImageInfos.size();
        m_ImageWrites.push_back(write);

        vk::DescriptorImageInfo image_info{};
        image_info
            .setImageView(view)
            .setImageLayout(layout);
        m_ImageInfos.push_back(image_info);

        return *this;
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeUniformBuffer(
        const uint32_t   binding,
        const uint32_t   index,
        const vk::Buffer buffer,
        const uint64_t   offset,
        const uint64_t   size) {
        Write write{};
        write.Binding   = binding;
        write.Index     = index;
        write.Type      = vk::DescriptorType::eUniformBuffer;
        write.InfoIndex = m_BufferInfos.size();
        m_BufferWrites.push_back(write);

        vk::DescriptorBufferInfo buffer_info{};
        buffer_info
            .setBuffer(buffer)
            .setOffset(offset)
            .setRange(size);
        m_BufferInfos.push_back(buffer_info);

        return *this;
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeStorageBuffer(
        const uint32_t   binding,
        const uint32_t   index,
        const vk::Buffer buffer,
        const uint64_t   offset,
        const uint64_t   size) {
        Write write{};
        write.Binding   = binding;
        write.Index     = index;
        write.Type      = vk::DescriptorType::eStorageBuffer;
        write.InfoIndex = m_BufferInfos.size();
        m_BufferWrites.push_back(write);

        vk::DescriptorBufferInfo buffer_info{};
        buffer_info
            .setBuffer(buffer)
            .setOffset(offset)
            .setRange(size);
        m_BufferInfos.push_back(buffer_info);

        return *this;
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeSampler(
        const uint32_t    binding,
        const vk::Sampler sampler) {
        return writeSampler(binding, 0, sampler);
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeCombinedImageSampler(
        const uint32_t        binding,
        const vk::ImageView   view,
        const vk::ImageLayout layout,
        const vk::Sampler     sampler) {
        return writeCombinedImageSampler(binding, 0, view, layout, sampler);
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeSampledImage(
        const uint32_t        binding,
        const vk::ImageView   view,
        const vk::ImageLayout layout) {
        return writeSampledImage(binding, 0, view, layout);
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeStorageImage(
        const uint32_t        binding,
        const vk::ImageView   view,
        const vk::ImageLayout layout) {
        return writeStorageImage(binding, 0, view, layout);
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeUniformBuffer(
        const uint32_t   binding,
        const vk::Buffer buffer,
        const uint64_t   offset,
        const uint64_t   size) {
        return writeUniformBuffer(binding, 0, buffer, offset, size);
    }

    Vulkan::DescriptorSetWriter &Vulkan::DescriptorSetWriter::writeStorageBuffer(
        const uint32_t   binding,
        const vk::Buffer buffer,
        const uint64_t   offset,
        const uint64_t   size) {
        return writeStorageBuffer(binding, 0, buffer, offset, size);
    }

    void Vulkan::DestroyDescriptorPool(const vk::DescriptorPool descriptor_pool) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        s_pInstance->m_Device.destroyDescriptorPool(descriptor_pool);
    }

    void Vulkan::DestroyDescriptorSetLayout(const vk::DescriptorSetLayout layout) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        s_pInstance->m_Device.destroyDescriptorSetLayout(layout);
    }

    vk::DescriptorPool Vulkan::CreateDescriptorPool(
        const vk::DescriptorPoolCreateFlags           flags,
        const uint32_t                                max_sets,
        const vk::ArrayProxy<vk::DescriptorPoolSize> &pool_sizes) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::DescriptorPoolCreateInfo create_info{};
        create_info
            .setFlags(flags)
            .setMaxSets(max_sets)
            .setPoolSizes(pool_sizes);
        auto [result, pool] = s_pInstance->m_Device.createDescriptorPool(create_info);
        DIGNIS_VK_CHECK(result);
        return pool;
    }

    std::vector<vk::DescriptorSet> Vulkan::AllocateDescriptorSets(
        const vk::ArrayProxy<uint32_t>                &runtime_sizes,
        const vk::ArrayProxy<vk::DescriptorSetLayout> &layouts,
        const vk::DescriptorPool                       pool) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        DIGNIS_ASSERT(
            runtime_sizes.size() == layouts.size(),
            "Ignis::Vulkan::AllocateDescriptorSets: number of runtime_sizes differ from number of layouts: {} runtime sizes, {} layouts.",
            runtime_sizes.size(), layouts.size());
        const vk::DescriptorSetVariableDescriptorCountAllocateInfo runtime_size_allocate_info{runtime_sizes};

        vk::DescriptorSetAllocateInfo allocate_info{};
        allocate_info
            .setPNext(&runtime_size_allocate_info)
            .setDescriptorSetCount(static_cast<uint32_t>(runtime_sizes.size()))
            .setSetLayouts(layouts)
            .setDescriptorPool(pool);
        auto [result, sets] = s_pInstance->m_Device.allocateDescriptorSets(allocate_info);
        DIGNIS_VK_CHECK(result);
        DIGNIS_ASSERT(
            layouts.size() == sets.size(),
            "Ignis::Vulkan::AllocateDescriptorSets: requested number of descriptor sets differ from number of returned descriptor sets: {} requested, {} returned.",
            layouts.size(), sets.size());
        return sets;
    }

    vk::DescriptorSet Vulkan::AllocateDescriptorSet(
        const uint32_t                runtime_size,
        const vk::DescriptorSetLayout layout,
        const vk::DescriptorPool      pool) {
        const auto sets = AllocateDescriptorSets(runtime_size, layout, pool);
        return sets[0];
    }

    std::vector<vk::DescriptorSet> Vulkan::AllocateDescriptorSets(
        const vk::ArrayProxy<vk::DescriptorSetLayout> &layouts,
        const vk::DescriptorPool                       pool) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::DescriptorSetAllocateInfo allocate_info{};
        allocate_info
            .setDescriptorSetCount(static_cast<uint32_t>(layouts.size()))
            .setSetLayouts(layouts)
            .setDescriptorPool(pool);
        auto [result, sets] = s_pInstance->m_Device.allocateDescriptorSets(allocate_info);
        DIGNIS_VK_CHECK(result);
        DIGNIS_ASSERT(
            layouts.size() == sets.size(),
            "Ignis::Vulkan::AllocateDescriptorSets: requested number of descriptor sets differ from number of returned descriptor sets: {} requested, {} returned.",
            layouts.size(), sets.size());
        return sets;
    }

    vk::DescriptorSet Vulkan::AllocateDescriptorSet(
        const vk::DescriptorSetLayout layout,
        const vk::DescriptorPool      pool) {
        const auto sets = AllocateDescriptorSets(layout, pool);
        return sets[0];
    }
}  // namespace Ignis