#include <Ignis/Vulkan/Descriptor.hpp>

namespace Ignis::Vulkan {
    namespace DescriptorLayout {
        void Builder::clear() {
            m_Flags = {};
            m_Bindings.clear();
            m_BindingFlags.clear();
        }

        Builder &Builder::setFlags(const vk::DescriptorSetLayoutCreateFlags flags) {
            m_Flags = flags;
            return *this;
        }

        Builder &Builder::addBinding(
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

        Builder &Builder::addBinding(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const vk::DescriptorType         type,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, 1, type, stages);
        }

        Builder &Builder::addBinding(
            const uint32_t             binding,
            const uint32_t             count,
            const vk::DescriptorType   type,
            const vk::ShaderStageFlags stages) {
            return addBinding({}, binding, count, type, stages);
        }

        Builder &Builder::addBinding(
            const uint32_t             binding,
            const vk::DescriptorType   type,
            const vk::ShaderStageFlags stages) {
            return addBinding({}, binding, 1, type, stages);
        }

        Builder &Builder::addSampler(
            const vk::DescriptorBindingFlags flags,

            const uint32_t             binding,
            const uint32_t             count,
            const vk::ShaderStageFlags stages) {
            return addBinding(flags, binding, count, vk::DescriptorType::eSampler, stages);
        }

        Builder &Builder::addCombinedImageSampler(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const uint32_t                   count,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, count, vk::DescriptorType::eCombinedImageSampler, stages);
        }

        Builder &Builder::addSampledImage(
            const vk::DescriptorBindingFlags flags,

            const uint32_t             binding,
            const uint32_t             count,
            const vk::ShaderStageFlags stages) {
            return addBinding(flags, binding, count, vk::DescriptorType::eSampledImage, stages);
        }

        Builder &Builder::addStorageImage(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const uint32_t                   count,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, count, vk::DescriptorType::eStorageImage, stages);
        }

        Builder &Builder::addUniformBuffer(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const uint32_t                   count,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, count, vk::DescriptorType::eUniformBuffer, stages);
        }

        Builder &Builder::addStorageBuffer(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const uint32_t                   count,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, count, vk::DescriptorType::eStorageBuffer, stages);
        }

        Builder &Builder::addSampler(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, 1, vk::DescriptorType::eSampler, stages);
        }

        Builder &Builder::addCombinedImageSampler(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, 1, vk::DescriptorType::eCombinedImageSampler, stages);
        }

        Builder &Builder::addSampledImage(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, 1, vk::DescriptorType::eSampledImage, stages);
        }

        Builder &Builder::addStorageImage(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, 1, vk::DescriptorType::eStorageImage, stages);
        }

        Builder &Builder::addUniformBuffer(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, 1, vk::DescriptorType::eUniformBuffer, stages);
        }

        Builder &Builder::addStorageBuffer(
            const vk::DescriptorBindingFlags flags,
            const uint32_t                   binding,
            const vk::ShaderStageFlags       stages) {
            return addBinding(flags, binding, 1, vk::DescriptorType::eStorageBuffer, stages);
        }

        Builder &Builder::addSampler(
            const uint32_t             binding,
            const uint32_t             count,
            const vk::ShaderStageFlags stages) {
            return addSampler({}, binding, count, stages);
        }

        Builder &Builder::addCombinedImageSampler(
            const uint32_t             binding,
            const uint32_t             count,
            const vk::ShaderStageFlags stages) {
            return addCombinedImageSampler({}, binding, count, stages);
        }

        Builder &Builder::addSampledImage(
            const uint32_t             binding,
            const uint32_t             count,
            const vk::ShaderStageFlags stages) {
            return addSampledImage({}, binding, count, stages);
        }

        Builder &Builder::addStorageImage(
            const uint32_t             binding,
            const uint32_t             count,
            const vk::ShaderStageFlags stages) {
            return addStorageImage({}, binding, count, stages);
        }

        Builder &Builder::addUniformBuffer(
            const uint32_t             binding,
            const uint32_t             count,
            const vk::ShaderStageFlags stages) {
            return addUniformBuffer({}, binding, count, stages);
        }

        Builder &Builder::addStorageBuffer(
            const uint32_t             binding,
            const uint32_t             count,
            const vk::ShaderStageFlags stages) {
            return addStorageBuffer({}, binding, count, stages);
        }

        Builder &Builder::addSampler(
            const uint32_t             binding,
            const vk::ShaderStageFlags stages) {
            return addSampler({}, binding, 1, stages);
        }

        Builder &Builder::addCombinedImageSampler(
            const uint32_t             binding,
            const vk::ShaderStageFlags stages) {
            return addCombinedImageSampler({}, binding, 1, stages);
        }

        Builder &Builder::addSampledImage(
            const uint32_t             binding,
            const vk::ShaderStageFlags stages) {
            return addSampledImage({}, binding, 1, stages);
        }

        Builder &Builder::addStorageImage(
            const uint32_t             binding,
            const vk::ShaderStageFlags stages) {
            return addStorageImage({}, binding, 1, stages);
        }

        Builder &Builder::addUniformBuffer(
            const uint32_t             binding,
            const vk::ShaderStageFlags stages) {
            return addUniformBuffer({}, binding, 1, stages);
        }

        Builder &Builder::addStorageBuffer(
            const uint32_t             binding,
            const vk::ShaderStageFlags stages) {
            return addStorageBuffer({}, binding, 1, stages);
        }

        vk::DescriptorSetLayout Builder::build(const vk::Device device) {
            vk::DescriptorSetLayoutBindingFlagsCreateInfo vk_descriptor_set_layout_binding_flags_create_info{};
            vk_descriptor_set_layout_binding_flags_create_info
                .setBindingFlags(m_BindingFlags);
            vk::DescriptorSetLayoutCreateInfo vk_descriptor_set_layout_create_info{};
            vk_descriptor_set_layout_create_info
                .setPNext(&vk_descriptor_set_layout_binding_flags_create_info)
                .setFlags(m_Flags)
                .setBindings(m_Bindings);
            auto [result, layout] = device.createDescriptorSetLayout(vk_descriptor_set_layout_create_info);
            DIGNIS_VK_CHECK(result);
            return layout;
        }
    }  // namespace DescriptorLayout

    namespace DescriptorPool {
        vk::DescriptorPool Create(
            const vk::DescriptorPoolCreateFlags           flags,
            const uint32_t                                max_sets,
            const vk::ArrayProxy<vk::DescriptorPoolSize> &pool_sizes,
            const vk::Device                              device) {
            vk::DescriptorPoolCreateInfo vk_descriptor_pool_create_info{};
            vk_descriptor_pool_create_info
                .setFlags(flags)
                .setMaxSets(max_sets)
                .setPoolSizes(pool_sizes);
            auto [result, descriptor_pool] = device.createDescriptorPool(vk_descriptor_pool_create_info);
            DIGNIS_VK_CHECK(result);
            return descriptor_pool;
        }

        vk::DescriptorPool CreateFreeDescriptorSets(
            const uint32_t                                max_sets,
            const vk::ArrayProxy<vk::DescriptorPoolSize> &pool_sizes,
            const vk::Device                              device) {
            return Create(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, max_sets, pool_sizes, device);
        }

        vk::DescriptorPool Create(
            const uint32_t                                max_sets,
            const vk::ArrayProxy<vk::DescriptorPoolSize> &pool_sizes,
            const vk::Device                              device) {
            return Create(static_cast<vk::DescriptorPoolCreateFlags>(0), max_sets, pool_sizes, device);
        }
    }  // namespace DescriptorPool

    namespace DescriptorSet {
        std::vector<vk::DescriptorSet> Allocate(
            const vk::ArrayProxy<uint32_t>                &runtime_sizes,
            const vk::ArrayProxy<vk::DescriptorSetLayout> &layouts,
            const vk::DescriptorPool                       pool,
            const vk::Device                               device) {
            DIGNIS_ASSERT(layouts.size() == runtime_sizes.size());
            vk::DescriptorSetVariableDescriptorCountAllocateInfoEXT runtime_size_allocate_info{};
            runtime_size_allocate_info.setDescriptorCounts(runtime_sizes);
            vk::DescriptorSetAllocateInfo vk_descriptor_set_allocate_info{};
            vk_descriptor_set_allocate_info
                .setPNext(&runtime_size_allocate_info)
                .setDescriptorSetCount(static_cast<uint32_t>(layouts.size()))
                .setSetLayouts(layouts)
                .setDescriptorPool(pool);
            auto [result, sets] = device.allocateDescriptorSets(
                vk_descriptor_set_allocate_info);
            DIGNIS_VK_CHECK(result);
            DIGNIS_ASSERT(layouts.size() == sets.size());
            return sets;
        }

        vk::DescriptorSet Allocate(
            const uint32_t                runtime_size,
            const vk::DescriptorSetLayout layout,
            const vk::DescriptorPool      pool,
            const vk::Device              device) {
            const std::vector<vk::DescriptorSet> sets =
                Allocate(vk::ArrayProxy{runtime_size}, vk::ArrayProxy{layout}, pool, device);
            return sets[0];
        }

        std::vector<vk::DescriptorSet> Allocate(
            const vk::ArrayProxy<vk::DescriptorSetLayout> &layouts,
            const vk::DescriptorPool                       pool,
            const vk::Device                               device) {
            vk::DescriptorSetAllocateInfo vk_descriptor_set_allocate_info{};
            vk_descriptor_set_allocate_info
                .setDescriptorSetCount(static_cast<uint32_t>(layouts.size()))
                .setSetLayouts(layouts)
                .setDescriptorPool(pool);
            auto [result, sets] = device.allocateDescriptorSets(
                vk_descriptor_set_allocate_info);
            DIGNIS_VK_CHECK(result);
            DIGNIS_ASSERT(layouts.size() == sets.size());
            return sets;
        }

        vk::DescriptorSet Allocate(
            const vk::DescriptorSetLayout layout,
            const vk::DescriptorPool      pool,
            const vk::Device              device) {
            const std::vector<vk::DescriptorSet> sets = Allocate(vk::ArrayProxy{layout}, pool, device);
            return sets[0];
        }

        void Writer::clear() {
            m_ImageWrites.clear();
            m_BufferWrites.clear();
            m_ImageInfos.clear();
            m_BufferInfos.clear();
        }

        void Writer::update(
            const vk::DescriptorSet set,
            const vk::Device        device) {
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

            device.updateDescriptorSets(descriptor_writes, {});
        }

        Writer &Writer::writeSampler(
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

        Writer &Writer::writeCombinedImageSampler(
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

        Writer &Writer::writeSampledImage(
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

        Writer &Writer::writeStorageImage(
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

        Writer &Writer::writeUniformBuffer(
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

        Writer &Writer::writeStorageBuffer(
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

        Writer &Writer::writeSampler(
            const uint32_t    binding,
            const vk::Sampler sampler) {
            return writeSampler(binding, 0, sampler);
        }

        Writer &Writer::writeCombinedImageSampler(
            const uint32_t        binding,
            const vk::ImageView   view,
            const vk::ImageLayout layout,
            const vk::Sampler     sampler) {
            return writeCombinedImageSampler(binding, 0, view, layout, sampler);
        }

        Writer &Writer::writeSampledImage(
            const uint32_t        binding,
            const vk::ImageView   view,
            const vk::ImageLayout layout) {
            return writeSampledImage(binding, 0, view, layout);
        }

        Writer &Writer::writeStorageImage(
            const uint32_t        binding,
            const vk::ImageView   view,
            const vk::ImageLayout layout) {
            return writeStorageImage(binding, 0, view, layout);
        }

        Writer &Writer::writeUniformBuffer(
            const uint32_t   binding,
            const vk::Buffer buffer,
            const uint64_t   offset,
            const uint64_t   size) {
            return writeUniformBuffer(binding, 0, buffer, offset, size);
        }

        Writer &Writer::writeStorageBuffer(
            const uint32_t   binding,
            const vk::Buffer buffer,
            const uint64_t   offset,
            const uint64_t   size) {
            return writeStorageBuffer(binding, 0, buffer, offset, size);
        }
    }  // namespace DescriptorSet
}  // namespace Ignis::Vulkan