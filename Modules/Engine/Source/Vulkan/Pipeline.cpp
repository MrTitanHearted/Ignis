#include <Ignis/Vulkan/Pipeline.hpp>

namespace Ignis::Vulkan {
    namespace PipelineLayout {
        vk::PipelineLayout Create(
            const vk::ArrayProxy<vk::PushConstantRange>   &push_constants,
            const vk::ArrayProxy<vk::DescriptorSetLayout> &set_layouts,
            const vk::Device                               device) {
            vk::PipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
            vk_pipeline_layout_create_info
                .setPushConstantRanges(push_constants)
                .setSetLayouts(set_layouts);

            auto [result, pipeline_layout] = device.createPipelineLayout(vk_pipeline_layout_create_info);
            DIGNIS_VK_CHECK(result);
            return pipeline_layout;
        }

        vk::PipelineLayout Create(
            const vk::ArrayProxy<vk::DescriptorSetLayout> &set_layouts,
            const vk::Device                               device) {
            return Create({}, set_layouts, device);
        }
    }  // namespace PipelineLayout

    namespace ComputePipeline {

        vk::Pipeline Create(
            const vk::PipelineCreateFlags flags,
            const std::string_view        entry,
            const vk::ShaderModule        shader_module,
            const vk::PipelineLayout      layout,
            const vk::Device              device) {
            vk::PipelineShaderStageCreateInfo compute_stage{};
            compute_stage
                .setPName(entry.data())
                .setModule(shader_module)
                .setStage(vk::ShaderStageFlagBits::eCompute);

            vk::ComputePipelineCreateInfo vk_compute_pipeline_create_info{};
            vk_compute_pipeline_create_info
                .setFlags(flags)
                .setLayout(layout)
                .setStage(compute_stage);

            auto [result, pipeline] = device.createComputePipeline(nullptr, vk_compute_pipeline_create_info);
            DIGNIS_VK_CHECK(result);
            return pipeline;
        }

        vk::Pipeline Create(
            const std::string_view   entry,
            const vk::ShaderModule   shader_module,
            const vk::PipelineLayout layout,
            const vk::Device         device) {
            return Create({}, entry, shader_module, layout, device);
        }

    }  // namespace ComputePipeline
}  // namespace Ignis::Vulkan