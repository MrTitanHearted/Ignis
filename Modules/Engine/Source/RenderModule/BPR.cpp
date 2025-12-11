#include <Ignis/RenderModule/BPR.hpp>

namespace Ignis {
    BPR::BPR(const Settings &settings)
        : m_FrameGraph{Engine::GetRef().getFrameGraph()},
          m_ColorAttachmentFormat{settings.ColorAttachmentFormat},
          m_DepthAttachmentFormat{settings.DepthAttachmentFormat},
          m_MaxBindingCount{settings.MaxBindingCount},
          m_NextTextureID{0u},
          m_NextMaterialID{0u},
          m_NextPointLightID{0},
          m_NextSpotLightID{0},
          m_NextStaticModelID{0u},
          m_NextStaticInstanceID{0u},
          m_MaterialStagingBuffer{},
          m_MaterialBuffer{},
          m_CameraBuffer{},
          m_PointLightStagingBuffer{},
          m_SpotLightStagingBuffer{},
          m_DirectionalLightBuffer{},
          m_PointLightBuffer{},
          m_SpotLightBuffer{},
          m_LightDataBuffer{},
          m_FrameGraphMaterialBuffer{},
          m_FrameGraphCameraBuffer{},
          m_FrameGraphDirectionalLightBuffer{},
          m_FrameGraphPointLightBuffer{},
          m_FrameGraphSpotLightBuffer{},
          m_FrameGraphLightDataBuffer{} {
        const FileAsset static_shader_file = FileAsset::LoadBinaryFromPath("Assets/Shaders/Ignis/BPR/Static.spv").value();

        std::vector<uint32_t> static_shader_code{};
        static_shader_code.resize(static_shader_file.getSize() * sizeof(char) / sizeof(uint32_t));

        std::memcpy(static_shader_code.data(), static_shader_file.getContent().data(), static_shader_file.getSize());

        m_DescriptorPool = Vulkan::CreateDescriptorPool(
            {},
            10,
            {
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, m_MaxBindingCount},
            });

        m_MaterialDescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addCombinedImageSampler(0, m_MaxBindingCount, vk::ShaderStageFlagBits::eFragment)
                .addStorageBuffer(1, m_MaxBindingCount, vk::ShaderStageFlagBits::eFragment)
                .addUniformBuffer(2, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .build();

        m_MaterialDescriptor = Vulkan::AllocateDescriptorSet(m_MaterialDescriptorLayout, m_DescriptorPool);

        m_LightDescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addUniformBuffer(0, vk::ShaderStageFlagBits::eFragment)
                .addStorageBuffer(1, vk::ShaderStageFlagBits::eFragment)
                .addStorageBuffer(2, vk::ShaderStageFlagBits::eFragment)
                .addUniformBuffer(3, vk::ShaderStageFlagBits::eFragment)
                .build();

        m_LightDescriptor = Vulkan::AllocateDescriptorSet(m_LightDescriptorLayout, m_DescriptorPool);

        m_StaticDescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addStorageBuffer(0, m_MaxBindingCount, vk::ShaderStageFlagBits::eVertex)
                .build();

        m_StaticPipelineLayout = Vulkan::CreatePipelineLayout(
            vk::PushConstantRange{
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0, sizeof(StaticDrawData)},
            {m_MaterialDescriptorLayout, m_LightDescriptorLayout, m_StaticDescriptorLayout});

        m_StaticDescriptor = Vulkan::AllocateDescriptorSet(m_StaticDescriptorLayout, m_DescriptorPool);

        const vk::ShaderModule static_shader_module = Vulkan::CreateShaderModuleFromSPV(static_shader_code);

        m_StaticPipeline =
            Vulkan::GraphicsPipelineBuilder()
                .setVertexShader("vs_main", static_shader_module)
                .setFragmentShader("fs_main", static_shader_module)
                .setVertexLayouts({Vulkan::VertexLayout{
                    vk::VertexInputBindingDescription{0, sizeof(StaticVertex), vk::VertexInputRate::eVertex},
                    {
                        vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticVertex, Position)},
                        vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticVertex, Normal)},
                        vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32Sfloat, offsetof(StaticVertex, UV)},
                    },
                }})
                .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setCullMode(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
                .setColorAttachmentFormats({m_ColorAttachmentFormat})
                .setDepthAttachmentFormat(m_DepthAttachmentFormat)
                .setDepthTest(vk::True, vk::CompareOp::eLess)
                .setNoStencilTest()
                .setNoMultisampling()
                .setNoBlending()
                .build(m_StaticPipelineLayout);

        Vulkan::DestroyShaderModule(static_shader_module);

        m_Sampler = Vulkan::CreateSampler(vk::SamplerCreateInfo{});

        m_MaterialStagingBuffer =
            Vulkan::AllocateBuffer(
                vma::AllocationCreateFlagBits::eMapped,
                vma::MemoryUsage::eCpuOnly, {},
                sizeof(Material),
                vk::BufferUsageFlagBits::eTransferSrc |
                    vk::BufferUsageFlagBits::eTransferDst);

        m_MaterialBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            2 * sizeof(Material),
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_CameraBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(Camera),
            vk::BufferUsageFlagBits::eUniformBuffer);

        m_PointLightStagingBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(PointLight),
            vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_SpotLightStagingBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(SpotLight),
            vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_DirectionalLightBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(DirectionalLight),
            vk::BufferUsageFlagBits::eUniformBuffer);

        m_PointLightBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(PointLight) * 2,
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_SpotLightBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(SpotLight) * 2,
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_LightDataBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(LightData),
            vk::BufferUsageFlagBits::eUniformBuffer);

        m_FrameGraphMaterialBuffer = FrameGraph::BufferInfo{
            m_FrameGraph.importBuffer(m_MaterialBuffer.Handle, m_MaterialBuffer.Usage, 0, m_MaterialBuffer.Size),
            0u,
            m_MaterialBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphCameraBuffer = FrameGraph::BufferInfo{
            m_FrameGraph.importBuffer(m_CameraBuffer.Handle, m_CameraBuffer.Usage, 0, m_CameraBuffer.Size),
            0u,
            m_CameraBuffer.Size,
            vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphDirectionalLightBuffer = FrameGraph::BufferInfo{
            m_FrameGraph.importBuffer(
                m_DirectionalLightBuffer.Handle,
                m_DirectionalLightBuffer.Usage, 0,
                m_DirectionalLightBuffer.Size),
            0,
            m_DirectionalLightBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphPointLightBuffer = FrameGraph::BufferInfo{
            m_FrameGraph.importBuffer(
                m_PointLightBuffer.Handle,
                m_PointLightBuffer.Usage, 0,
                m_PointLightBuffer.Size),
            0,
            m_PointLightBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphSpotLightBuffer = FrameGraph::BufferInfo{
            m_FrameGraph.importBuffer(
                m_SpotLightBuffer.Handle,
                m_SpotLightBuffer.Usage, 0,
                m_SpotLightBuffer.Size),
            0,
            m_SpotLightBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphLightDataBuffer = FrameGraph::BufferInfo{
            m_FrameGraph.importBuffer(
                m_LightDataBuffer.Handle,
                m_LightDataBuffer.Usage, 0,
                m_LightDataBuffer.Size),
            0,
            m_LightDataBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(1, m_MaterialBuffer.Handle, 0, m_MaterialBuffer.Size)
            .writeUniformBuffer(2, m_CameraBuffer.Handle, 0, m_CameraBuffer.Size)
            .update(m_MaterialDescriptor);

        Vulkan::DescriptorSetWriter()
            .writeUniformBuffer(0, m_DirectionalLightBuffer.Handle, 0, m_DirectionalLightBuffer.Size)
            .writeStorageBuffer(1, m_PointLightBuffer.Handle, 0, m_PointLightBuffer.Size)
            .writeStorageBuffer(2, m_SpotLightBuffer.Handle, 0, m_SpotLightBuffer.Size)
            .writeUniformBuffer(3, m_LightDataBuffer.Handle, 0, m_LightDataBuffer.Size)
            .update(m_LightDescriptor);
    }

    BPR::~BPR() {
        m_FrameGraph.removeBuffer(m_FrameGraphMaterialBuffer.Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphCameraBuffer.Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphDirectionalLightBuffer.Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphPointLightBuffer.Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphSpotLightBuffer.Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphLightDataBuffer.Buffer);

        for (const auto  loaded_static_models = m_LoadedStaticModels;
             const auto &model : std::views::values(loaded_static_models))
            unloadStaticModel(model);

        DIGNIS_ASSERT(m_LoadedTextures.empty());
        DIGNIS_ASSERT(m_LoadedMaterials.empty());

        DIGNIS_ASSERT(m_LoadedStaticModels.empty());

        Vulkan::DestroyDescriptorPool(m_DescriptorPool);

        Vulkan::DestroySampler(m_Sampler);

        Vulkan::DestroyBuffer(m_MaterialStagingBuffer);
        Vulkan::DestroyBuffer(m_MaterialBuffer);
        Vulkan::DestroyBuffer(m_CameraBuffer);

        Vulkan::DestroyBuffer(m_PointLightStagingBuffer);
        Vulkan::DestroyBuffer(m_SpotLightStagingBuffer);
        Vulkan::DestroyBuffer(m_DirectionalLightBuffer);
        Vulkan::DestroyBuffer(m_PointLightBuffer);
        Vulkan::DestroyBuffer(m_SpotLightBuffer);
        Vulkan::DestroyBuffer(m_LightDataBuffer);

        Vulkan::DestroyPipeline(m_StaticPipeline);
        Vulkan::DestroyPipelineLayout(m_StaticPipelineLayout);
        Vulkan::DestroyDescriptorSetLayout(m_StaticDescriptorLayout);
        Vulkan::DestroyDescriptorSetLayout(m_LightDescriptorLayout);
        Vulkan::DestroyDescriptorSetLayout(m_MaterialDescriptorLayout);
    }

    void BPR::onRenderPass(FrameGraph::RenderPass &render_pass) {
        render_pass
            .readImages(m_FrameGraphImages.getData())
            .readBuffers({
                m_FrameGraphMaterialBuffer,
                m_FrameGraphCameraBuffer,
                m_FrameGraphDirectionalLightBuffer,
                m_FrameGraphPointLightBuffer,
                m_FrameGraphSpotLightBuffer,
                m_FrameGraphLightDataBuffer,
            })
            .readBuffers(m_FrameGraphStaticModelIndexBuffers.getData())
            .readBuffers(m_FrameGraphStaticModelVertexBuffers.getData())
            .readBuffers(m_FrameGraphStaticModelInstanceBuffers.getData());
    }

    void BPR::onDraw(const vk::CommandBuffer command_buffer) {
        StaticDrawData static_draw_data{};

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_StaticPipeline);
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_StaticPipelineLayout, 0,
            {m_MaterialDescriptor, m_LightDescriptor, m_StaticDescriptor}, {});

        for (const auto &[model_id, model] : m_StaticModels) {
            if (0 == model.InstanceCount)
                continue;
            static_draw_data.Instance = model_id;
            command_buffer.bindVertexBuffers(0, {model.VertexBuffer.Handle}, {0});
            command_buffer.bindIndexBuffer(model.IndexBuffer.Handle, 0, vk::IndexType::eUint32);
            for (const auto &mesh : model.Meshes) {
                static_draw_data.Material = mesh.Material;
                command_buffer.pushConstants(
                    m_StaticPipelineLayout,
                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                    0, sizeof(StaticDrawData), &static_draw_data);
                command_buffer.drawIndexed(
                    mesh.IndexCount,
                    model.InstanceCount,
                    mesh.IndexOffset,
                    static_cast<int32_t>(mesh.VertexOffset),
                    0);
            }
        }
    }

    void BPR::updateCamera(const Camera &camera) const {
        Vulkan::CopyMemoryToAllocation(&camera, m_CameraBuffer.Allocation, 0, m_CameraBuffer.Size);

        const auto inverse  = glm::inverse(camera.View);
        const auto position = glm::vec3{inverse[3]};

        Vulkan::CopyMemoryToAllocation(
            &position,
            m_LightDataBuffer.Allocation,
            offsetof(LightData, ViewPosition),
            sizeof(glm::vec3));
    }

    void BPR::setDirectionalLight(const DirectionalLight &light) const {
        Vulkan::CopyMemoryToAllocation(&light, m_DirectionalLightBuffer.Allocation, 0, m_DirectionalLightBuffer.Size);
    }

    BPR::PointLightID BPR::addPointLight(const PointLight &light) {
        uint32_t point_light_count = k_InvalidPointLightID;

        Vulkan::CopyAllocationToMemory(
            m_LightDataBuffer.Allocation,
            offsetof(LightData, PointLightCount),
            &point_light_count,
            sizeof(uint32_t));
        const uint32_t index = point_light_count++;
        Vulkan::CopyMemoryToAllocation(
            &point_light_count,
            m_LightDataBuffer.Allocation,
            offsetof(LightData, PointLightCount),
            sizeof(uint32_t));

        PointLightID id;
        if (m_FreePointLightIDs.empty()) {
            id = m_NextPointLightID++;
        } else {
            id = m_FreePointLightIDs.back();
            m_FreePointLightIDs.pop_back();
        }

        if (sizeof(PointLight) * index >= m_PointLightBuffer.Size) {
            m_FrameGraph.removeBuffer(m_FrameGraphPointLightBuffer.Buffer);

            const Vulkan::Buffer old_buffer = m_PointLightBuffer;

            m_PointLightBuffer = Vulkan::AllocateBuffer(
                old_buffer.AllocationFlags,
                old_buffer.MemoryUsage,
                old_buffer.CreateFlags,
                old_buffer.Size * 2,
                old_buffer.Usage);

            m_FrameGraphPointLightBuffer = FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(
                    m_PointLightBuffer.Handle,
                    m_PointLightBuffer.Usage, 0,
                    m_PointLightBuffer.Size),
                0,
                m_PointLightBuffer.Size,
            };

            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    old_buffer.Handle,
                    m_PointLightBuffer.Handle,
                    0, 0,
                    old_buffer.Size,
                    command_buffer);
            });

            Vulkan::DestroyBuffer(old_buffer);
        }

        Vulkan::CopyMemoryToAllocation(
            &light,
            m_PointLightStagingBuffer.Allocation,
            0,
            m_PointLightStagingBuffer.Size);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_PointLightStagingBuffer.Handle,
                m_PointLightBuffer.Handle,
                0,
                sizeof(PointLight) * index,
                m_PointLightStagingBuffer.Size,
                command_buffer);
        });

        m_PointLightToIndex[id]    = index;
        m_IndexToPointLight[index] = id;

        return id;
    }

    BPR::SpotLightID BPR::addSpotLight(const SpotLight &light) {
        uint32_t spot_light_count = k_InvalidSpotLightID;

        Vulkan::CopyAllocationToMemory(
            m_LightDataBuffer.Allocation,
            offsetof(LightData, SpotLightCount),
            &spot_light_count,
            sizeof(uint32_t));
        const uint32_t index = spot_light_count++;
        Vulkan::CopyMemoryToAllocation(
            &spot_light_count,
            m_LightDataBuffer.Allocation,
            offsetof(LightData, SpotLightCount),
            sizeof(uint32_t));

        SpotLightID id;
        if (m_FreeSpotLightIDs.empty()) {
            id = m_NextSpotLightID++;
        } else {
            id = m_FreeSpotLightIDs.back();
            m_FreeSpotLightIDs.pop_back();
        }

        if (sizeof(SpotLight) * index >= m_SpotLightBuffer.Size) {
            m_FrameGraph.removeBuffer(m_FrameGraphSpotLightBuffer.Buffer);

            const Vulkan::Buffer old_buffer = m_SpotLightBuffer;

            m_SpotLightBuffer = Vulkan::AllocateBuffer(
                old_buffer.AllocationFlags,
                old_buffer.MemoryUsage,
                old_buffer.CreateFlags,
                old_buffer.Size * 2,
                old_buffer.Usage);

            m_FrameGraphSpotLightBuffer = FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(
                    m_SpotLightBuffer.Handle,
                    m_SpotLightBuffer.Usage, 0,
                    m_SpotLightBuffer.Size),
                0,
                m_SpotLightBuffer.Size,
            };

            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    old_buffer.Handle,
                    m_SpotLightBuffer.Handle,
                    0, 0,
                    old_buffer.Size,
                    command_buffer);
            });

            Vulkan::DestroyBuffer(old_buffer);
        }

        Vulkan::CopyMemoryToAllocation(
            &light,
            m_SpotLightStagingBuffer.Allocation,
            0,
            m_SpotLightStagingBuffer.Size);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_SpotLightStagingBuffer.Handle,
                m_SpotLightBuffer.Handle,
                0,
                sizeof(SpotLight) * index,
                m_SpotLightStagingBuffer.Size,
                command_buffer);
        });

        m_SpotLightToIndex[id]    = index;
        m_IndexToSpotLight[index] = id;

        return id;
    }

    void BPR::removePointLight(const PointLightID id) {
        DIGNIS_ASSERT(m_PointLightToIndex.contains(id));

        uint32_t point_light_count = k_InvalidPointLightID;

        Vulkan::CopyAllocationToMemory(
            m_LightDataBuffer.Allocation,
            offsetof(LightData, PointLightCount),
            &point_light_count,
            sizeof(uint32_t));
        const uint32_t last_index = --point_light_count;
        Vulkan::CopyMemoryToAllocation(
            &point_light_count,
            m_LightDataBuffer.Allocation,
            offsetof(LightData, PointLightCount),
            sizeof(uint32_t));

        if (const uint32_t index = m_PointLightToIndex[id];
            index != last_index) {
            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    m_PointLightBuffer.Handle,
                    m_PointLightBuffer.Handle,
                    sizeof(PointLight) * last_index,
                    sizeof(PointLight) * index,
                    sizeof(PointLight),
                    command_buffer);
            });

            const PointLightID last_id = m_IndexToPointLight[last_index];

            m_PointLightToIndex[last_id] = index;
            m_IndexToPointLight[index]   = last_id;
        }

        m_PointLightToIndex.erase(id);
        m_IndexToPointLight.erase(last_index);

        m_FreePointLightIDs.push_back(id);
    }

    void BPR::removeSpotLight(const SpotLightID id) {
        DIGNIS_ASSERT(m_SpotLightToIndex.contains(id));

        uint32_t spot_light_count = k_InvalidSpotLightID;

        Vulkan::CopyAllocationToMemory(
            m_LightDataBuffer.Allocation,
            offsetof(LightData, SpotLightCount),
            &spot_light_count,
            sizeof(uint32_t));
        const uint32_t last_index = --spot_light_count;
        Vulkan::CopyMemoryToAllocation(
            &spot_light_count,
            m_LightDataBuffer.Allocation,
            offsetof(LightData, SpotLightCount),
            sizeof(uint32_t));

        if (const uint32_t index = m_SpotLightToIndex[id];
            index != last_index) {
            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    m_SpotLightBuffer.Handle,
                    m_SpotLightBuffer.Handle,
                    sizeof(SpotLight) * last_index,
                    sizeof(SpotLight) * index,
                    sizeof(SpotLight),
                    command_buffer);
            });

            const SpotLightID last_id = m_IndexToSpotLight[last_index];

            m_SpotLightToIndex[last_id] = index;
            m_IndexToSpotLight[index]   = last_id;
        }

        m_SpotLightToIndex.erase(id);
        m_IndexToSpotLight.erase(last_index);

        m_FreeSpotLightIDs.push_back(id);
    }

    void BPR::setPointLight(const PointLightID id, const PointLight &light) {
        DIGNIS_ASSERT(m_PointLightToIndex.contains(id));
        const uint32_t index = m_PointLightToIndex[id];
        Vulkan::CopyMemoryToAllocation(&light, m_PointLightStagingBuffer.Allocation, 0, m_PointLightStagingBuffer.Size);
        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_PointLightStagingBuffer.Handle,
                m_PointLightBuffer.Handle,
                0,
                sizeof(PointLight) * index,
                m_PointLightStagingBuffer.Size,
                command_buffer);
        });
    }

    void BPR::setSpotLight(const SpotLightID id, const SpotLight &light) {
        DIGNIS_ASSERT(m_SpotLightToIndex.contains(id));
        const uint32_t index = m_SpotLightToIndex[id];
        Vulkan::CopyMemoryToAllocation(&light, m_SpotLightStagingBuffer.Allocation, 0, m_SpotLightStagingBuffer.Size);
        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_SpotLightStagingBuffer.Handle,
                m_SpotLightBuffer.Handle,
                0,
                sizeof(SpotLight) * index,
                m_SpotLightStagingBuffer.Size,
                command_buffer);
        });
    }

    BPR::StaticModelID BPR::loadStaticModel(const std::filesystem::path &path) {
        DIGNIS_ASSERT(std::filesystem::exists(path));
        const std::string spath = path;

        if (m_LoadedStaticModels.contains(spath))
            return m_LoadedStaticModels[spath];

        StaticModelID id;

        if (m_FreeStaticModelIDs.empty()) {
            id = m_NextStaticModelID++;
        } else {
            id = m_FreeStaticModelIDs.back();
            m_FreeStaticModelIDs.pop_back();
        }

        const std::filesystem::path directory = path.parent_path();

        Assimp::Importer importer{};

        const aiScene *ai_scene =
            importer.ReadFile(
                spath,
                aiProcess_Triangulate |
                    aiProcess_GenNormals |
                    aiProcess_GenUVCoords |
                    aiProcess_FlipUVs);
        DIGNIS_ASSERT(nullptr != ai_scene, "Failed to load asset from path: '{}'. Assimp error: '{}'", spath, importer.GetErrorString());
        DIGNIS_LOG_ENGINE_INFO("Loaded an Ignis::AssimpAsset from path: '{}'", spath);

        const aiNode *ai_node = ai_scene->mRootNode;

        StaticModel model{};
        model.Path = spath;

        std::vector<StaticVertex> vertices{};
        std::vector<uint32_t>     indices{};

        processStaticNode(directory, ai_scene, ai_node, vertices, indices, model);

        model.VertexBuffer =
            Vulkan::AllocateBuffer(
                {}, vma::MemoryUsage::eGpuOnly, {},
                sizeof(StaticVertex) * vertices.size(),
                vk::BufferUsageFlagBits::eVertexBuffer |
                    vk::BufferUsageFlagBits::eTransferDst);
        model.IndexBuffer =
            Vulkan::AllocateBuffer(
                {}, vma::MemoryUsage::eGpuOnly, {},
                sizeof(uint32_t) * indices.size(),
                vk::BufferUsageFlagBits::eIndexBuffer |
                    vk::BufferUsageFlagBits::eTransferDst);

        const Vulkan::Buffer staging_buffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            model.VertexBuffer.Size + model.IndexBuffer.Size,
            vk::BufferUsageFlagBits::eTransferSrc);

        Vulkan::CopyMemoryToAllocation(vertices.data(), staging_buffer.Allocation, 0, model.VertexBuffer.Size);
        Vulkan::CopyMemoryToAllocation(indices.data(), staging_buffer.Allocation, model.VertexBuffer.Size, model.IndexBuffer.Size);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                staging_buffer.Handle,
                model.VertexBuffer.Handle,
                0, 0,
                model.VertexBuffer.Size,
                command_buffer);
            Vulkan::CopyBufferToBuffer(
                staging_buffer.Handle,
                model.IndexBuffer.Handle,
                model.VertexBuffer.Size, 0,
                model.IndexBuffer.Size,
                command_buffer);
        });

        Vulkan::DestroyBuffer(staging_buffer);

        model.InstanceBuffer =
            Vulkan::AllocateBuffer(
                vma::AllocationCreateFlagBits::eMapped |
                    vma::AllocationCreateFlagBits::eHostAccessRandom,
                vma::MemoryUsage::eAutoPreferHost, {},
                sizeof(StaticInstance) * 2,
                vk::BufferUsageFlagBits::eStorageBuffer);

        m_FrameGraphStaticModelVertexBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(
                    model.VertexBuffer.Handle,
                    model.VertexBuffer.Usage, 0,
                    model.VertexBuffer.Size),
                0,
                model.VertexBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexAttributeInput | vk::PipelineStageFlagBits2::eVertexInput,
            });

        m_FrameGraphStaticModelIndexBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(
                    model.IndexBuffer.Handle,
                    model.IndexBuffer.Usage, 0,
                    model.IndexBuffer.Size),
                0,
                model.IndexBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexInput,
            });

        m_FrameGraphStaticModelInstanceBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(
                    model.InstanceBuffer.Handle,
                    model.InstanceBuffer.Usage, 0,
                    model.InstanceBuffer.Size),
                0,
                model.InstanceBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader,
            });

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(0, id, model.InstanceBuffer.Handle, 0, model.InstanceBuffer.Size)
            .update(m_StaticDescriptor);

        model.InstanceCount     = 0;
        model.NextInstanceIndex = 0;
        model.InstanceToIndex.clear();
        model.IndexToInstance.clear();

        m_StaticModels.insert(id, model);

        m_LoadedStaticModels[spath] = id;

        return id;
    }

    BPR::StaticInstanceID BPR::addStaticInstance(const StaticModelID model_id, StaticInstance instance) {
        DIGNIS_ASSERT(m_StaticModels.contains(model_id));

        instance.NormalTransform = glm::transpose(glm::inverse(instance.ModelTransform));

        auto &model = m_StaticModels[model_id];

        Vulkan::WaitDeviceIdle();

        StaticInstanceID id;
        if (m_FreeStaticInstanceIDs.empty()) {
            id = m_NextStaticInstanceID++;
        } else {
            id = m_FreeStaticInstanceIDs.back();
            m_FreeStaticInstanceIDs.pop_back();
        }

        uint32_t index;
        if (model.NextInstanceIndex != model.InstanceCount) {
            index = model.InstanceCount;
        } else {
            index = model.NextInstanceIndex++;
        }

        if (sizeof(StaticInstance) * index >= model.InstanceBuffer.Size) {
            m_FrameGraph.removeBuffer(m_FrameGraphStaticModelInstanceBuffers[model_id].Buffer);

            const Vulkan::Buffer old_buffer = model.InstanceBuffer;

            model.InstanceBuffer =
                Vulkan::AllocateBuffer(
                    old_buffer.AllocationFlags,
                    old_buffer.MemoryUsage,
                    old_buffer.CreateFlags,
                    old_buffer.Size * 2,
                    old_buffer.Usage);

            m_FrameGraphStaticModelInstanceBuffers[model_id] = FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(
                    model.InstanceBuffer.Handle,
                    model.InstanceBuffer.Usage, 0,
                    model.InstanceBuffer.Size),
                0,
                model.InstanceBuffer.Size,
                m_FrameGraphStaticModelInstanceBuffers[model_id].StageMask,
            };

            Vulkan::InvalidateAllocation(old_buffer.Allocation, 0, old_buffer.Size);
            const vma::AllocationInfo old_allocation_info = Vulkan::GetAllocationInfo(old_buffer);
            Vulkan::CopyMemoryToAllocation(
                old_allocation_info.pMappedData,
                model.InstanceBuffer.Allocation,
                0, old_buffer.Size);

            Vulkan::DestroyBuffer(old_buffer);

            Vulkan::DescriptorSetWriter()
                .writeStorageBuffer(0, model_id, model.InstanceBuffer.Handle, 0, model.InstanceBuffer.Size)
                .update(m_StaticDescriptor);
        }

        Vulkan::CopyMemoryToAllocation(
            &instance,
            model.InstanceBuffer.Allocation,
            sizeof(StaticInstance) * index,
            sizeof(StaticInstance));

        model.InstanceToIndex[id]    = index;
        model.IndexToInstance[index] = id;

        model.InstanceCount++;

        m_StaticInstanceToStaticModel[id] = model_id;

        return id;
    }

    void BPR::unloadStaticModel(const StaticModelID id) {
        DIGNIS_ASSERT(m_StaticModels.contains(id));

        const auto &model = m_StaticModels[id];

        std::vector<StaticInstanceID> instances_to_remove{};
        instances_to_remove.reserve(model.InstanceToIndex.size());

        for (const auto &instance : std::views::keys(model.InstanceToIndex))
            instances_to_remove.emplace_back(instance);

        for (const auto &instance : instances_to_remove)
            removeStaticInstance(instance);

        DIGNIS_ASSERT(0u == model.InstanceCount);

        for (const auto &mesh : model.Meshes)
            removeMaterial(mesh.Material);

        m_FrameGraph.removeBuffer(m_FrameGraphStaticModelVertexBuffers[id].Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphStaticModelIndexBuffers[id].Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphStaticModelInstanceBuffers[id].Buffer);

        Vulkan::DestroyBuffer(model.VertexBuffer);
        Vulkan::DestroyBuffer(model.IndexBuffer);
        Vulkan::DestroyBuffer(model.InstanceBuffer);

        m_LoadedStaticModels.erase(model.Path);

        m_FrameGraphStaticModelVertexBuffers.remove(id);
        m_FrameGraphStaticModelIndexBuffers.remove(id);
        m_FrameGraphStaticModelInstanceBuffers.remove(id);

        m_StaticModels.remove(id);

        m_FreeStaticModelIDs.push_back(id);
    }

    void BPR::removeStaticInstance(const StaticInstanceID id) {
        DIGNIS_ASSERT(m_StaticInstanceToStaticModel.contains(id));

        Vulkan::WaitDeviceIdle();

        const auto model_id = m_StaticInstanceToStaticModel[id];

        auto &model = m_StaticModels[model_id];

        const uint32_t last_index = model.InstanceCount - 1;

        if (const uint32_t index = model.InstanceToIndex[id];
            index != last_index) {
            Vulkan::InvalidateAllocation(model.InstanceBuffer.Allocation, 0, model.InstanceBuffer.Size);
            const vma::AllocationInfo allocation_info = Vulkan::GetAllocationInfo(model.InstanceBuffer);
            Vulkan::CopyMemoryToAllocation(
                &static_cast<StaticInstance *>(allocation_info.pMappedData)[last_index],
                model.InstanceBuffer.Allocation,
                sizeof(StaticInstance) * index,
                sizeof(StaticInstance));

            const StaticInstanceID last_id = model.IndexToInstance[last_index];

            model.InstanceToIndex[last_id] = index;
            model.IndexToInstance[index]   = last_id;
        }

        model.InstanceToIndex.erase(id);
        model.IndexToInstance.erase(last_index);

        m_StaticInstanceToStaticModel.erase(id);

        m_FreeStaticInstanceIDs.push_back(id);

        model.InstanceCount--;
    }

    const BPR::StaticModel &BPR::getStaticModelConstRef(const StaticModelID id) const {
        DIGNIS_ASSERT(m_StaticModels.contains(id));
        return m_StaticModels[id];
    }

    BPR::StaticInstance BPR::getStaticInstance(const StaticInstanceID id) const {
        DIGNIS_ASSERT(m_StaticInstanceToStaticModel.contains(id));
        const auto &model_id = m_StaticInstanceToStaticModel.at(id);
        const auto &model    = m_StaticModels[model_id];

        DIGNIS_ASSERT(model.InstanceToIndex.contains(id));
        const uint32_t index = model.InstanceToIndex.at(id);

        StaticInstance instance{};

        Vulkan::CopyAllocationToMemory(
            model.InstanceBuffer.Allocation,
            sizeof(StaticInstance) * index,
            &instance,
            sizeof(StaticInstance));

        return instance;
    }

    void BPR::setStaticInstance(const StaticInstanceID id, StaticInstance instance) {
        DIGNIS_ASSERT(m_StaticInstanceToStaticModel.contains(id));

        instance.NormalTransform = glm::transpose(glm::inverse(instance.ModelTransform));

        const auto &model_id = m_StaticInstanceToStaticModel.at(id);
        const auto &model    = m_StaticModels[model_id];

        DIGNIS_ASSERT(model.InstanceToIndex.contains(id));
        const uint32_t index = model.InstanceToIndex.at(id);

        Vulkan::CopyMemoryToAllocation(
            &instance,
            model.InstanceBuffer.Allocation,
            sizeof(StaticInstance) * index,
            sizeof(StaticInstance));
    }

    void BPR::processStaticNode(
        const std::filesystem::path &directory,
        const aiScene *ai_scene, const aiNode *ai_node,
        std::vector<StaticVertex> &vertices,
        std::vector<uint32_t>     &indices,
        StaticModel               &model) {
        for (uint32_t i = 0; i < ai_node->mNumMeshes; i++)
            model.Meshes.push_back(processStaticMesh(
                directory, ai_scene,
                ai_scene->mMeshes[ai_node->mMeshes[i]],
                vertices, indices));
        for (uint32_t i = 0; i < ai_node->mNumChildren; i++)
            processStaticNode(directory, ai_scene, ai_node->mChildren[i], vertices, indices, model);
    }

    BPR::StaticMesh BPR::processStaticMesh(
        const std::filesystem::path &directory,
        const aiScene *ai_scene, const aiMesh *ai_mesh,
        std::vector<StaticVertex> &vertices,
        std::vector<uint32_t>     &indices) {
        const uint32_t vertex_offset = vertices.size();
        const uint32_t index_offset  = indices.size();

        for (uint32_t i = 0; i < ai_mesh->mNumVertices; i++) {
            StaticVertex vertex{};

            const aiVector3D &position = ai_mesh->mVertices[i];
            const aiVector3D &normal   = ai_mesh->mNormals[i];
            const aiVector3D &uv       = ai_mesh->mTextureCoords[0][i];

            vertex.Position = glm::vec3(position.x, position.y, position.z);
            vertex.Normal   = glm::vec3(normal.x, normal.y, normal.z);
            vertex.UV       = glm::vec2(uv.x, uv.y);

            vertices.push_back(vertex);
        }

        for (uint32_t i = 0; i < ai_mesh->mNumFaces; i++) {
            const aiFace &face = ai_mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        const uint32_t vertex_count = vertices.size() - vertex_offset;
        const uint32_t index_count  = indices.size() - index_offset;

        const aiMaterial *ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
        DIGNIS_ASSERT(nullptr != ai_material);
        const MaterialID material_id = processMaterial(directory, ai_scene, ai_material);

        StaticMesh static_mesh{};
        static_mesh.IndexOffset  = index_offset;
        static_mesh.VertexOffset = vertex_offset;
        static_mesh.IndexCount   = index_count;
        static_mesh.VertexCount  = vertex_count;
        static_mesh.Material     = material_id;
        return static_mesh;
    }

    BPR::MaterialID BPR::processMaterial(
        const std::filesystem::path &directory,
        const aiScene *ai_scene, const aiMaterial *ai_material) {
        const char       *material_name = ai_material->GetName().C_Str();
        const std::string material_path = directory / material_name;

        if (m_LoadedMaterials.contains(material_path)) {
            const auto material_id = m_LoadedMaterials[material_path];
            m_LoadedMaterialRCs[material_id]++;
            return material_id;
        }

        const TextureID diffuse_texture  = processTexture(directory, ai_scene, ai_material, aiTextureType_DIFFUSE);
        const TextureID specular_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_SPECULAR);

        float shininess = 1.0f;
        DIGNIS_ASSERT(AI_SUCCESS == ai_material->Get(AI_MATKEY_SHININESS, shininess));

        Material material{};
        material.DiffuseTexture  = diffuse_texture;
        material.SpecularTexture = specular_texture;
        material.Shininess       = shininess;

        MaterialID id;
        if (m_FreeMaterialIDs.empty()) {
            id = m_NextMaterialID++;
        } else {
            id = m_FreeMaterialIDs.back();
            m_FreeMaterialIDs.pop_back();
        }

        if (sizeof(Material) * id >= m_MaterialBuffer.Size) {
            m_FrameGraph.removeBuffer(m_FrameGraphMaterialBuffer.Buffer);

            const Vulkan::Buffer old_buffer = m_MaterialBuffer;

            m_MaterialBuffer = Vulkan::AllocateBuffer(
                old_buffer.AllocationFlags,
                old_buffer.MemoryUsage,
                old_buffer.CreateFlags,
                old_buffer.Size * 2,
                old_buffer.Usage);

            m_FrameGraphMaterialBuffer = FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(m_MaterialBuffer.Handle, m_MaterialBuffer.Usage, 0, m_MaterialBuffer.Size),
                0,
                m_MaterialBuffer.Size,
                m_FrameGraphMaterialBuffer.StageMask,
            };

            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    old_buffer.Handle,
                    m_MaterialBuffer.Handle,
                    0, 0,
                    old_buffer.Size,
                    command_buffer);
            });

            Vulkan::DestroyBuffer(old_buffer);

            Vulkan::DescriptorSetWriter()
                .writeStorageBuffer(1, m_MaterialBuffer.Handle, 0, m_MaterialBuffer.Size)
                .update(m_MaterialDescriptor);
        }

        Vulkan::CopyMemoryToAllocation(&material, m_MaterialStagingBuffer.Allocation, 0, sizeof(Material));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_MaterialStagingBuffer.Handle,
                m_MaterialBuffer.Handle,
                0,
                sizeof(Material) * id,
                sizeof(Material),
                command_buffer);
        });

        m_MaterialIDs.insert(id);
        m_LoadedMaterialRCs[id] = 1;

        m_LoadedMaterials[material_path] = id;

        m_LoadedMaterialPaths[id] = material_path;
        return id;
    }

    BPR::TextureID BPR::processTexture(
        const std::filesystem::path &directory,
        const aiScene *ai_scene, const aiMaterial *ai_material,
        const aiTextureType ai_texture_type) {
        aiString ai_path{};
        DIGNIS_ASSERT(AI_SUCCESS == ai_material->GetTexture(ai_texture_type, 0, &ai_path));
        const std::string texture_path = directory / ai_path.C_Str();
        if (m_LoadedTextures.contains(texture_path)) {
            const auto texture_id = m_LoadedTextures[texture_path];
            m_LoadedTextureRCs[texture_id]++;
            return texture_id;
        }

        const aiTexture *ai_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());

        const std::optional<TextureAsset> texture_asset_opt =
            nullptr != ai_texture
                ? TextureAsset::LoadFromMemory(ai_texture->pcData, ai_texture->mWidth * ai_texture->mHeight * sizeof(aiTexel), TextureAsset::Type::eRGBA8u)
                : TextureAsset::LoadFromPath(texture_path, TextureAsset::Type::eRGBA8u);
        DIGNIS_ASSERT(texture_asset_opt.has_value());
        const TextureAsset &texture_asset = texture_asset_opt.value();

        const Vulkan::Image image = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            vk::Extent2D{texture_asset.getWidth(), texture_asset.getHeight()});
        const vk::ImageView view = Vulkan::CreateImageColorView2D(image.Handle, image.Format);

        const Vulkan::Buffer staging_buffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(uint8_t) * texture_asset.getData().size(),
            vk::BufferUsageFlagBits::eTransferSrc);
        Vulkan::CopyMemoryToAllocation(texture_asset.getData().data(), staging_buffer.Allocation, 0, staging_buffer.Size);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::BarrierMerger merger{};
            merger.putImageBarrier(
                image.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.flushBarriers(command_buffer);
            Vulkan::CopyBufferToImage(
                staging_buffer.Handle,
                image.Handle,
                0,
                vk::Offset3D{0, 0, 0},
                vk::Extent2D{0, 0},
                image.Extent,
                command_buffer);
            merger.putImageBarrier(
                image.Handle,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.flushBarriers(command_buffer);
        });

        Vulkan::DestroyBuffer(staging_buffer);

        TextureID id;

        if (m_FreeTextureIDs.empty()) {
            id = m_NextTextureID++;
        } else {
            id = m_FreeTextureIDs.back();
            m_FreeTextureIDs.pop_back();
        }

        m_Textures[id]     = image;
        m_TextureViews[id] = view;

        m_LoadedTextures[texture_path] = id;

        m_LoadedTexturePaths[id] = texture_path;

        m_LoadedTextureRCs[id] = 1;

        m_FrameGraphImages.insert(
            id,
            m_FrameGraph.importImage(
                image.Handle, view,
                image.Format, image.Usage, image.Extent,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal));

        Vulkan::DescriptorSetWriter()
            .writeCombinedImageSampler(0, id, view, vk::ImageLayout::eShaderReadOnlyOptimal, m_Sampler)
            .update(m_MaterialDescriptor);

        return id;
    }

    void BPR::removeMaterial(const MaterialID id) {
        DIGNIS_ASSERT(m_MaterialIDs.contains(id));

        m_LoadedMaterialRCs[id]--;

        if (0 != m_LoadedMaterialRCs[id])
            return;

        const Material material = getMaterial(id);

        removeTexture(material.DiffuseTexture);
        removeTexture(material.SpecularTexture);

        const std::string path = m_LoadedMaterialPaths[id];

        m_MaterialIDs.erase(id);
        m_LoadedMaterials.erase(path);
        m_LoadedMaterialPaths.erase(id);
        m_LoadedMaterialRCs.erase(id);

        m_FreeMaterialIDs.push_back(id);
    }

    void BPR::removeTexture(const TextureID id) {
        DIGNIS_ASSERT(m_Textures.contains(id));
        DIGNIS_ASSERT(m_TextureViews.contains(id));

        m_LoadedTextureRCs[id]--;
        if (0 != m_LoadedTextureRCs[id])
            return;

        const std::string path = m_LoadedTexturePaths[id];

        m_FrameGraph.removeImage(m_FrameGraphImages[id]);
        m_FrameGraphImages.remove(id);

        const Vulkan::Image image = m_Textures[id];
        const vk::ImageView view  = m_TextureViews[id];

        Vulkan::DestroyImageView(view);
        Vulkan::DestroyImage(image);

        m_LoadedTextures.erase(path);
        m_LoadedTexturePaths.erase(id);
        m_LoadedTextureRCs.erase(id);
        m_Textures.erase(id);
        m_TextureViews.erase(id);

        Vulkan::DescriptorSetWriter()
            .writeCombinedImageSampler(0, id, nullptr, vk::ImageLayout::eUndefined, m_Sampler)
            .update(m_MaterialDescriptor);

        m_FreeTextureIDs.push_back(id);
    }

    BPR::Material BPR::getMaterial(const MaterialID id) const {
        DIGNIS_ASSERT(m_MaterialIDs.contains(id));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_MaterialBuffer.Handle,
                m_MaterialStagingBuffer.Handle,
                sizeof(Material) * id,
                0,
                sizeof(Material),
                command_buffer);
        });

        Vulkan::InvalidateAllocation(m_MaterialStagingBuffer.Allocation, 0, sizeof(Material));
        return static_cast<Material *>(Vulkan::GetAllocationInfo(m_MaterialStagingBuffer).pMappedData)[0];
    }

}  // namespace Ignis