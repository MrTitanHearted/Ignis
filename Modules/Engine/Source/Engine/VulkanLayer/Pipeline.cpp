#include <Ignis/Engine/VulkanLayer/Pipeline.hpp>

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

    namespace GraphicsPipeline {
        Builder::Builder() {
            clear();
        }

        void Builder::clear() {
            m_InputAssembly =
                vk::PipelineInputAssemblyStateCreateInfo{}
                    .setTopology(vk::PrimitiveTopology::eTriangleList)
                    .setPrimitiveRestartEnable(vk::False);
            m_Rasterization =
                vk::PipelineRasterizationStateCreateInfo{}
                    .setDepthClampEnable(vk::False)
                    .setRasterizerDiscardEnable(vk::False)
                    .setPolygonMode(vk::PolygonMode::eFill)
                    .setCullMode(vk::CullModeFlagBits::eNone)
                    .setFrontFace(vk::FrontFace::eCounterClockwise)
                    .setDepthBiasEnable(vk::False)
                    .setDepthBiasConstantFactor(0.0f)
                    .setDepthBiasClamp(0.0f)
                    .setDepthBiasSlopeFactor(0.0f)
                    .setLineWidth(1.0f);

            m_Multisample  = vk::PipelineMultisampleStateCreateInfo{};
            m_DepthStencil = vk::PipelineDepthStencilStateCreateInfo{};
            m_Rendering    = vk::PipelineRenderingCreateInfo{};

            m_ColorBlendAttachment = vk::PipelineColorBlendAttachmentState{};

            m_ShaderStages.clear();
            m_VertexInputBindingDescriptions.clear();
            m_VertexInputAttributeDescriptions.clear();

            setNoDepthTest();
            setNoBlending();
            setNoMultisampling();
        }

        Builder &Builder::setVertexShader(
            const std::string_view entry_point,
            const vk::ShaderModule module) {
            ShaderStageInfo shader_stage{};
            shader_stage.EntryPoint = entry_point;
            shader_stage.Module     = module;
            shader_stage.Stage      = vk::ShaderStageFlagBits::eVertex;
            m_ShaderStages.push_back(shader_stage);
            return *this;
        }

        Builder &Builder::setFragmentShader(
            const std::string_view entry_point,
            const vk::ShaderModule module) {
            ShaderStageInfo shader_stage{};
            shader_stage.EntryPoint = entry_point;
            shader_stage.Module     = module;
            shader_stage.Stage      = vk::ShaderStageFlagBits::eFragment;
            m_ShaderStages.push_back(shader_stage);
            return *this;
        }

        Builder &Builder::setVertexLayouts(const vk::ArrayProxy<VertexLayout> &vertex_layouts) {
            m_VertexInputBindingDescriptions.clear();
            m_VertexInputAttributeDescriptions.clear();

            m_VertexInputBindingDescriptions.reserve(vertex_layouts.size());

            for (const auto &[Binding, Attributes] : vertex_layouts) {
                m_VertexInputBindingDescriptions.push_back(Binding);

                m_VertexInputAttributeDescriptions.insert(
                    std::end(m_VertexInputAttributeDescriptions),
                    std::begin(Attributes),
                    std::end(Attributes));
            }
            return *this;
        }

        Builder &Builder::setInputTopology(const vk::PrimitiveTopology topology) {
            m_InputAssembly.setTopology(topology);
            return *this;
        }

        Builder &Builder::setPolygonMode(const vk::PolygonMode polygon_mode) {
            m_Rasterization.setPolygonMode(polygon_mode);
            return *this;
        }

        Builder &Builder::setCullMode(
            const vk::CullModeFlags cull_mode,
            const vk::FrontFace     front_face) {
            m_Rasterization
                .setCullMode(cull_mode)
                .setFrontFace(front_face);
            return *this;
        }

        Builder &Builder::setColorAttachmentFormats(const vk::ArrayProxy<vk::Format> &formats) {
            m_ColorAttachmentFormats.clear();
            m_ColorAttachmentFormats.reserve(formats.size());
            m_ColorAttachmentFormats.insert(
                std::begin(m_ColorAttachmentFormats),
                std::begin(formats),
                std::end(formats));
            return *this;
        }

        Builder &Builder::setDepthAttachmentFormat(const vk::Format format) {
            m_Rendering.setDepthAttachmentFormat(format);
            return *this;
        }

        Builder &Builder::setDepthTest(
            const bool          depth_write,
            const vk::CompareOp op) {
            m_DepthStencil
                .setDepthTestEnable(vk::True)
                .setDepthWriteEnable(depth_write ? vk::True : vk::False)
                .setDepthCompareOp(op)
                .setMinDepthBounds(0.0f)
                .setMaxDepthBounds(1.0f)
                .setDepthBoundsTestEnable(vk::False)
                .setStencilTestEnable(vk::False)
                .setFront(vk::StencilOpState{})
                .setBack(vk::StencilOpState{});
            return *this;
        }

        Builder &Builder::setBlendingAdditive() {
            m_ColorBlendAttachment
                .setColorWriteMask(vk::ColorComponentFlagBits::eR |
                                   vk::ColorComponentFlagBits::eG |
                                   vk::ColorComponentFlagBits::eB |
                                   vk::ColorComponentFlagBits::eA)
                .setBlendEnable(vk::True)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOne)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setAlphaBlendOp(vk::BlendOp::eAdd);
            return *this;
        }

        Builder &Builder::setBlendingAlphaBlended() {
            m_ColorBlendAttachment
                .setColorWriteMask(vk::ColorComponentFlagBits::eR |
                                   vk::ColorComponentFlagBits::eG |
                                   vk::ColorComponentFlagBits::eB |
                                   vk::ColorComponentFlagBits::eA)
                .setBlendEnable(vk::True)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setAlphaBlendOp(vk::BlendOp::eAdd);
            return *this;
        }

        Builder &Builder::setNoMultisampling() {
            m_Multisample
                .setSampleShadingEnable(vk::False)
                .setMinSampleShading(1.0f)
                .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                .setAlphaToCoverageEnable(vk::False)
                .setAlphaToOneEnable(vk::False);
            return *this;
        }

        Builder &Builder::setNoBlending() {
            m_ColorBlendAttachment
                .setColorWriteMask(vk::ColorComponentFlagBits::eR |
                                   vk::ColorComponentFlagBits::eG |
                                   vk::ColorComponentFlagBits::eB |
                                   vk::ColorComponentFlagBits::eA)
                .setBlendEnable(vk::False);
            return *this;
        }

        Builder &Builder::setNoDepthTest() {
            m_DepthStencil
                .setDepthTestEnable(vk::False)
                .setDepthWriteEnable(vk::False)
                .setDepthCompareOp(vk::CompareOp::eNever)
                .setMinDepthBounds(0.0f)
                .setMaxDepthBounds(1.0f)
                .setDepthBoundsTestEnable(vk::False)
                .setStencilTestEnable(vk::False)
                .setFront(vk::StencilOpState{})
                .setBack(vk::StencilOpState{});
            return *this;
        }

        vk::Pipeline Builder::build(
            const vk::PipelineLayout layout,
            const vk::Device         device) {
            m_Rendering.setColorAttachmentFormats(m_ColorAttachmentFormats);

            vk::Viewport viewport{};
            vk::Rect2D   scissor{};

            vk::PipelineViewportStateCreateInfo viewport_state_create_info{};
            viewport_state_create_info
                .setViewports(viewport)
                .setScissors(scissor);

            vk::DynamicState dynamic_states[] = {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor,
            };

            vk::PipelineDynamicStateCreateInfo dynamic_state_create_info{};
            dynamic_state_create_info
                .setDynamicStates(dynamic_states);

            vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{};
            color_blend_state_create_info
                .setLogicOpEnable(vk::False)
                .setLogicOp(vk::LogicOp::eNoOp)
                .setAttachments(m_ColorBlendAttachment);

            vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
            vertex_input_state_create_info
                .setVertexBindingDescriptions(m_VertexInputBindingDescriptions)
                .setVertexAttributeDescriptions(m_VertexInputAttributeDescriptions);

            std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{};
            shader_stages.reserve(m_ShaderStages.size());
            for (const auto &[entry_point, module, stage] : m_ShaderStages) {
                vk::PipelineShaderStageCreateInfo shader_stage{};
                shader_stage
                    .setFlags({})
                    .setPName(entry_point.c_str())
                    .setModule(module)
                    .setStage(stage);
                shader_stages.push_back(shader_stage);
            }

            vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info{};
            graphics_pipeline_create_info
                .setPNext(&m_Rendering)
                .setFlags({})
                .setStages(shader_stages)
                .setPVertexInputState(&vertex_input_state_create_info)
                .setPInputAssemblyState(&m_InputAssembly)
                .setPViewportState(&viewport_state_create_info)
                .setPRasterizationState(&m_Rasterization)
                .setPMultisampleState(&m_Multisample)
                .setPDepthStencilState(&m_DepthStencil)
                .setPColorBlendState(&color_blend_state_create_info)
                .setPDynamicState(&dynamic_state_create_info)
                .setLayout(layout);

            auto [result, pipeline] = device.createGraphicsPipeline(nullptr, graphics_pipeline_create_info);
            DIGNIS_VK_CHECK(result);
            return pipeline;
        }
    }  // namespace GraphicsPipeline
}  // namespace Ignis::Vulkan