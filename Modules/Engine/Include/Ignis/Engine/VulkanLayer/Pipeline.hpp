#pragma once

#include <Ignis/Engine/VulkanLayer/Types.hpp>

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

    namespace GraphicsPipeline {
        class Builder {
           public:
            Builder();
            ~Builder() = default;

            void clear();

            Builder &setVertexShader(std::string_view entry_point, vk::ShaderModule module);
            Builder &setFragmentShader(std::string_view entry_point, vk::ShaderModule module);
            Builder &setVertexLayouts(const vk::ArrayProxy<VertexLayout> &vertex_layouts);
            Builder &setInputTopology(vk::PrimitiveTopology topology);
            Builder &setPolygonMode(vk::PolygonMode polygon_mode);
            Builder &setCullMode(vk::CullModeFlags cull_mode, vk::FrontFace front_face);
            Builder &setColorAttachmentFormats(const vk::ArrayProxy<vk::Format> &formats);
            Builder &setDepthAttachmentFormat(vk::Format format);
            Builder &setDepthTest(bool depth_write, vk::CompareOp op);
            Builder &setBlendingAdditive();
            Builder &setBlendingAlphaBlended();
            Builder &setNoMultisampling();
            Builder &setNoBlending();
            Builder &setNoDepthTest();

            vk::Pipeline build(vk::PipelineLayout layout, vk::Device device);

           private:
            struct ShaderStageInfo {
                std::string             EntryPoint;
                vk::ShaderModule        Module;
                vk::ShaderStageFlagBits Stage;
            };

           private:
            std::vector<ShaderStageInfo> m_ShaderStages;

            std::vector<vk::VertexInputBindingDescription>   m_VertexInputBindingDescriptions;
            std::vector<vk::VertexInputAttributeDescription> m_VertexInputAttributeDescriptions;

            vk::PipelineInputAssemblyStateCreateInfo m_InputAssembly;
            vk::PipelineRasterizationStateCreateInfo m_Rasterization;
            vk::PipelineMultisampleStateCreateInfo   m_Multisample;
            vk::PipelineDepthStencilStateCreateInfo  m_DepthStencil;
            vk::PipelineRenderingCreateInfo          m_Rendering;

            vk::PipelineColorBlendAttachmentState m_ColorBlendAttachment;

            std::vector<vk::Format> m_ColorAttachmentFormats;
        };
    }  // namespace GraphicsPipeline
}  // namespace Ignis::Vulkan