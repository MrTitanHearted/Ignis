#pragma once

#include <Ignis/Engine/VulkanLayer/Types.hpp>

namespace Ignis::Vulkan {
    namespace DescriptorLayout {
        class Builder {
           public:
            Builder()  = default;
            ~Builder() = default;

            void clear();

            Builder &setFlags(vk::DescriptorSetLayoutCreateFlags flags);

            Builder &addBinding(
                vk::DescriptorBindingFlags flags,

                uint32_t             binding,
                uint32_t             count,
                vk::DescriptorType   type,
                vk::ShaderStageFlags stages);
            Builder &addBinding(
                vk::DescriptorBindingFlags flags,

                uint32_t             binding,
                vk::DescriptorType   type,
                vk::ShaderStageFlags stages);
            Builder &addBinding(
                uint32_t             binding,
                uint32_t             count,
                vk::DescriptorType   type,
                vk::ShaderStageFlags stages);
            Builder &addBinding(
                uint32_t             binding,
                vk::DescriptorType   type,
                vk::ShaderStageFlags stages);

            Builder &addSampler(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            Builder &addCombinedImageSampler(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            Builder &addSampledImage(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            Builder &addStorageImage(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            Builder &addUniformBuffer(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            Builder &addStorageBuffer(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);

            Builder &addSampler(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            Builder &addCombinedImageSampler(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            Builder &addSampledImage(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            Builder &addStorageImage(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            Builder &addUniformBuffer(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            Builder &addStorageBuffer(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);

            Builder &addSampler(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            Builder &addCombinedImageSampler(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            Builder &addSampledImage(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            Builder &addStorageImage(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            Builder &addUniformBuffer(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            Builder &addStorageBuffer(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);

            Builder &addSampler(uint32_t binding, vk::ShaderStageFlags stages);
            Builder &addCombinedImageSampler(uint32_t binding, vk::ShaderStageFlags stages);
            Builder &addSampledImage(uint32_t binding, vk::ShaderStageFlags stages);
            Builder &addStorageImage(uint32_t binding, vk::ShaderStageFlags stages);
            Builder &addUniformBuffer(uint32_t binding, vk::ShaderStageFlags stages);
            Builder &addStorageBuffer(uint32_t binding, vk::ShaderStageFlags stages);

            vk::DescriptorSetLayout build(vk::Device device);

           private:
            vk::DescriptorSetLayoutCreateFlags m_Flags;

            std::vector<vk::DescriptorSetLayoutBinding> m_Bindings;
            std::vector<vk::DescriptorBindingFlags>     m_BindingFlags;
        };
    }  // namespace DescriptorLayout

    namespace DescriptorPool {
        vk::DescriptorPool Create(
            vk::DescriptorPoolCreateFlags                 flags,
            uint32_t                                      max_sets,
            const vk::ArrayProxy<vk::DescriptorPoolSize> &pool_sizes,
            vk::Device                                    device);
        vk::DescriptorPool CreateFreeDescriptorSets(
            uint32_t                                      max_sets,
            const vk::ArrayProxy<vk::DescriptorPoolSize> &pool_sizes,
            vk::Device                                    device);
        vk::DescriptorPool Create(
            uint32_t                                      max_sets,
            const vk::ArrayProxy<vk::DescriptorPoolSize> &pool_sizes,
            vk::Device                                    device);
    }  // namespace DescriptorPool

    namespace DescriptorSet {
        std::vector<vk::DescriptorSet> Allocate(
            const vk::ArrayProxy<uint32_t>                &runtime_sizes,
            const vk::ArrayProxy<vk::DescriptorSetLayout> &layouts,
            vk::DescriptorPool                             pool,
            vk::Device                                     device);
        vk::DescriptorSet Allocate(
            uint32_t                runtime_size,
            vk::DescriptorSetLayout layout,
            vk::DescriptorPool      pool,
            vk::Device              device);

        std::vector<vk::DescriptorSet> Allocate(
            const vk::ArrayProxy<vk::DescriptorSetLayout> &layouts,
            vk::DescriptorPool                             pool,
            vk::Device                                     device);
        vk::DescriptorSet Allocate(
            vk::DescriptorSetLayout layout,
            vk::DescriptorPool      pool,
            vk::Device              device);

        class Writer {
           public:
            Writer()  = default;
            ~Writer() = default;

            void clear();

            void update(vk::DescriptorSet set, vk::Device device);

            Writer &writeSampler(
                uint32_t    binding,
                uint32_t    index,
                vk::Sampler sampler);
            Writer &writeCombinedImageSampler(
                uint32_t        binding,
                uint32_t        index,
                vk::ImageView   view,
                vk::ImageLayout layout,
                vk::Sampler     sampler);
            Writer &writeSampledImage(
                uint32_t        binding,
                uint32_t        index,
                vk::ImageView   view,
                vk::ImageLayout layout);
            Writer &writeStorageImage(
                uint32_t        binding,
                uint32_t        index,
                vk::ImageView   view,
                vk::ImageLayout layout);
            Writer &writeUniformBuffer(
                uint32_t   binding,
                uint32_t   index,
                vk::Buffer buffer,
                uint64_t   offset,
                uint64_t   size);
            Writer &writeStorageBuffer(
                uint32_t   binding,
                uint32_t   index,
                vk::Buffer buffer,
                uint64_t   offset,
                uint64_t   size);

            Writer &writeSampler(
                uint32_t    binding,
                vk::Sampler sampler);
            Writer &writeCombinedImageSampler(
                uint32_t        binding,
                vk::ImageView   view,
                vk::ImageLayout layout,
                vk::Sampler     sampler);
            Writer &writeSampledImage(
                uint32_t        binding,
                vk::ImageView   view,
                vk::ImageLayout layout);
            Writer &writeStorageImage(
                uint32_t        binding,
                vk::ImageView   view,
                vk::ImageLayout layout);
            Writer &writeUniformBuffer(
                uint32_t   binding,
                vk::Buffer buffer,
                uint64_t   offset,
                uint64_t   size);
            Writer &writeStorageBuffer(
                uint32_t   binding,
                vk::Buffer buffer,
                uint64_t   offset,
                uint64_t   size);

           private:
            struct Write {
                uint32_t Binding;
                uint32_t Index;
                uint32_t InfoIndex;

                vk::DescriptorType Type;
            };

           private:
            std::vector<vk::DescriptorImageInfo>  m_ImageInfos;
            std::vector<vk::DescriptorBufferInfo> m_BufferInfos;

            std::vector<Write> m_ImageWrites;
            std::vector<Write> m_BufferWrites;
        };
    }  // namespace DescriptorSet
}  // namespace Ignis::Vulkan