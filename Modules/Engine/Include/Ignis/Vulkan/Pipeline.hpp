#pragma once

#include <Ignis/Vulkan/Types.hpp>

namespace Ignis::Vulkan {
    namespace PipelineLayout {
        vk::PipelineLayout Create(
            const vk::ArrayProxy<vk::PushConstantRange>   &push_constants,
            const vk::ArrayProxy<vk::DescriptorSetLayout> &set_layouts,
            vk::Device                                     device);
        vk::PipelineLayout Create(
            const vk::ArrayProxy<vk::DescriptorSetLayout> &set_layouts,
            vk::Device                                     device);
    }  // namespace PipelineLayout

    namespace ComputePipeline {
        vk::Pipeline Create(
            vk::PipelineCreateFlags flags,
            std::string_view        entry,
            vk::ShaderModule        shader_module,
            vk::PipelineLayout      layout,
            vk::Device              device);
        vk::Pipeline Create(
            std::string_view   entry,
            vk::ShaderModule   shader_module,
            vk::PipelineLayout layout,
            vk::Device         device);
    }  // namespace ComputePipeline
}  // namespace Ignis::Vulkan