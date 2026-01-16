#include <Ignis/Render.hpp>

namespace Ignis {
    vk::ShaderModule g_ModelShader = nullptr;

    struct SMikkTSpaceMesh {
        uint32_t IndexCount = 0;

        uint32_t       *Indices  = nullptr;
        Render::Vertex *Vertices = nullptr;
    };

    SMikkTSpaceInterface g_SMikkTSpaceInterface{};

    int32_t GetSMikkTSpaceNumFaces(const SMikkTSpaceContext *context);
    int32_t GetSMikkTSpaceNumVerticesPerFace(const SMikkTSpaceContext *context, int32_t i_face);

    void GetSMikkTSpacePosition(const SMikkTSpaceContext *context, glm::f32 o_fv_position[3], int32_t i_face, int32_t i_vertex);
    void GetSMikkTSpaceNormal(const SMikkTSpaceContext *context, glm::f32 o_fv_normal[3], int32_t i_face, int32_t i_vertex);
    void GetSMikkTSpaceUV(const SMikkTSpaceContext *context, glm::f32 o_fv_uv[2], int32_t i_face, int32_t i_vertex);

    void SetSMikkTSpaceBasic(const SMikkTSpaceContext *context, const glm::f32 fv_tangent[3], glm::f32 f_sign, int32_t i_face, int32_t i_vertex);

    void Render::SetInstance(const InstanceID id, const glm::mat4x4 &transform) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return s_pInstance->setInstance(id, transform);
    }

    Render::ModelID Render::AddModel(const std::filesystem::path &path) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return s_pInstance->addModel(path);
    }

    Render::InstanceID Render::AddInstance(const ModelID model_id, const glm::mat4x4 &transform) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return s_pInstance->addInstance(model_id, transform);
    }

    void Render::RemoveModel(const ModelID id) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return s_pInstance->removeModel(id);
    }

    void Render::RemoveInstance(const InstanceID id) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return s_pInstance->removeInstance(id);
    }

    std::string_view Render::GetModelPath(const ModelID id) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return s_pInstance->getModelPath(id);
    }

    glm::mat4x4 Render::GetInstance(const InstanceID id) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return s_pInstance->getInstance(id);
    }

    void Render::initializeModels(const uint32_t max_binding_count) {
        m_ModelDescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addStorageBuffer(0, max_binding_count, vk::ShaderStageFlagBits::eVertex)
                .addStorageBuffer(1, max_binding_count, vk::ShaderStageFlagBits::eVertex)
                .build();

        m_ModelPipelineLayout = Vulkan::CreatePipelineLayout(
            vk::PushConstantRange{
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0, sizeof(CameraPC) + sizeof(ModelID)},
            {m_MaterialDescriptorLayout, m_LightDescriptorLayout, m_ModelDescriptorLayout});

        m_ModelDescriptorSet = Vulkan::AllocateDescriptorSet(m_ModelDescriptorLayout, m_DescriptorPool);

        m_NextMeshID.ID     = 0;
        m_NextInstanceID.ID = 0;
        m_NextModelID.ID    = 0;

        m_FreeMeshIDs.clear();
        m_FreeInstanceIDs.clear();
        m_FreeModelIDs.clear();

        const FileAsset model_shader_file = FileAsset::LoadBinaryFromPath("Assets/Shaders/Ignis/Model.spv").value();

        std::vector<uint32_t> model_shader_code{};
        model_shader_code.resize(model_shader_file.getSize() * sizeof(char) / sizeof(uint32_t));

        std::memcpy(model_shader_code.data(), model_shader_file.getContent().data(), model_shader_file.getSize());

        g_ModelShader = Vulkan::CreateShaderModuleFromSPV(model_shader_code);

        g_SMikkTSpaceInterface.m_getNumFaces          = GetSMikkTSpaceNumFaces;
        g_SMikkTSpaceInterface.m_getNumVerticesOfFace = GetSMikkTSpaceNumVerticesPerFace;
        g_SMikkTSpaceInterface.m_getPosition          = GetSMikkTSpacePosition;
        g_SMikkTSpaceInterface.m_getNormal            = GetSMikkTSpaceNormal;
        g_SMikkTSpaceInterface.m_getTexCoord          = GetSMikkTSpaceUV;
        g_SMikkTSpaceInterface.m_setTSpaceBasic       = SetSMikkTSpaceBasic;
        g_SMikkTSpaceInterface.m_setTSpace            = nullptr;
    }

    void Render::releaseModels() {
        while (!m_LoadedModels.empty()) {
            for (const auto  loaded_models = m_LoadedModels;
                 const auto &model : std::views::values(loaded_models))
                RemoveModel(model);
        }

        Vulkan::DestroyPipeline(m_ModelPipeline);
        Vulkan::DestroyShaderModule(g_ModelShader);
        Vulkan::DestroyPipelineLayout(m_ModelPipelineLayout);
        Vulkan::DestroyDescriptorSetLayout(m_ModelDescriptorLayout);

        m_FreeMeshIDs.clear();
        m_FreeInstanceIDs.clear();
        m_FreeModelIDs.clear();
    }

    void Render::setModelViewport(
        const FrameGraph::ImageID color_image,
        const FrameGraph::ImageID depth_image) {
        if (nullptr == m_ModelPipeline) {
            m_ModelPipeline =
                Vulkan::GraphicsPipelineBuilder()
                    .setVertexShader("vs_main", g_ModelShader)
                    .setFragmentShader("fs_main", g_ModelShader)
                    .setVertexLayouts({Vulkan::VertexLayout{
                        vk::VertexInputBindingDescription{0, sizeof(Vertex), vk::VertexInputRate::eVertex},
                        {
                            vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Position)},
                            vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Normal)},
                            vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, UV)},
                            vk::VertexInputAttributeDescription{3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, Tangent)},
                        },
                    }})
                    .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                    .setPolygonMode(vk::PolygonMode::eFill)
                    .setCullMode(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
                    .setColorAttachmentFormats({m_pFrameGraph->getImageFormat(color_image)})
                    .setDepthAttachmentFormat(m_pFrameGraph->getImageFormat(depth_image))
                    .setDepthTest(vk::True, vk::CompareOp::eLess)
                    .setNoStencilTest()
                    .setNoMultisampling()
                    .setNoBlending()
                    .build(m_ModelPipelineLayout);
        }
    }

    void Render::readModelBuffers(FrameGraph::RenderPass &render_pass) {
        render_pass
            .readBuffers(m_FrameGraphModelVertexBuffers.getData())
            .readBuffers(m_FrameGraphModelIndexBuffers.getData())
            .readBuffers(m_FrameGraphModelMeshBuffers.getData())
            .readBuffers(m_FrameGraphModelInstanceBuffers.getData())
            .readBuffers(m_FrameGraphModelIndirectBuffers.getData());
    }

    void Render::onModelDraw(const vk::CommandBuffer command_buffer) {
        const CameraPC camera_pc{
            m_Camera.Projection * m_Camera.View,
            m_Camera.Position,
        };

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_ModelPipeline);
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_ModelPipelineLayout, 0,
            {m_MaterialDescriptorSet, m_LightDescriptorSet, m_ModelDescriptorSet}, {});

        command_buffer.pushConstants(
            m_ModelPipelineLayout,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof(CameraPC),
            &camera_pc);

        for (const auto &[model_id, model] : m_Models) {
            command_buffer.pushConstants(
                m_ModelPipelineLayout,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                sizeof(CameraPC),
                sizeof(ModelID),
                &model_id);
            command_buffer.bindIndexBuffer(model.IndexBuffer.Handle, 0, vk::IndexType::eUint32);
            command_buffer.bindVertexBuffers(0, {model.VertexBuffer.Handle}, {0});
            command_buffer.drawIndexedIndirect(
                model.IndirectBuffer.Handle, 0,
                model.MeshCount,
                sizeof(vk::DrawIndexedIndirectCommand));
        }
    }

    void Render::setInstance(const InstanceID id, const glm::mat4x4 &transform) {
        DIGNIS_ASSERT(m_Instances.contains(id));
        const auto  model_id = m_InstanceToModel.at(id);
        const auto &model    = m_Models.at(model_id);
        const auto  index    = model.InstanceToIndex.at(id);

        Instance instance{};
        instance.VertexTransform = transform;
        instance.NormalTransform = glm::transpose(glm::inverse(transform));

        Vulkan::CopyMemoryToAllocation(
            &instance,
            model.InstanceBuffer.Allocation,
            sizeof(Instance) * index,
            sizeof(Instance));
    }

    Render::ModelID Render::addModel(const std::filesystem::path &path) {
        std::string spath = path;

        if (m_LoadedModels.contains(spath))
            return m_LoadedModels.at(spath);

        DIGNIS_ASSERT(std::filesystem::exists(path));

        Assimp::Importer importer{};

        const aiScene *ai_scene =
            importer.ReadFile(
                spath,
                // aiProcess_CalcTangentSpace |
                aiProcess_Triangulate |
                    aiProcess_GenNormals |
                    aiProcess_GenUVCoords |
                    aiProcess_OptimizeMeshes |
                    aiProcess_OptimizeGraph |
                    aiProcess_JoinIdenticalVertices |
                    aiProcess_FixInfacingNormals |
                    aiProcess_RemoveRedundantMaterials |
                    aiProcess_FindDegenerates |
                    aiProcess_FindInvalidData |
                    aiProcess_ImproveCacheLocality |
                    aiProcess_ValidateDataStructure |
                    aiProcess_FlipUVs);
        if (nullptr == ai_scene) {
            DIGNIS_LOG_ENGINE_WARN("Failed to load asset from path: '{}'. Assimp error: '{}'", spath, importer.GetErrorString());
            return k_InvalidModelID;
        }
        DIGNIS_LOG_ENGINE_INFO("Loaded an Ignis::AssimpAsset from path: '{}'", spath);

        const aiNode *ai_node = ai_scene->mRootNode;

        std::vector<Vertex>   vertices{};
        std::vector<uint32_t> indices{};
        std::vector<Mesh>     meshes{};

        std::vector<vk::DrawIndexedIndirectCommand> indirect_commands{};

        Vulkan::WaitDeviceIdle();

        processNode(path, ai_scene, ai_node, glm::mat4x4{1.0f}, vertices, indices, meshes, indirect_commands);

        DIGNIS_ASSERT(meshes.size() == indirect_commands.size());

        Model model{};

        model.Path = spath;

        model.MeshCount = meshes.size();

        model.InstanceCount = 0;

        model.VertexBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(Vertex) * vertices.size(),
            vk::BufferUsageFlagBits::eVertexBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        model.IndexBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(uint32_t) * indices.size(),
            vk::BufferUsageFlagBits::eIndexBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        model.MeshBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(Mesh) * meshes.size(),
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        {
            const Vulkan::Buffer staging_buffer = Vulkan::AllocateBuffer(
                vma::AllocationCreateFlagBits::eMapped,
                vma::MemoryUsage::eCpuOnly, {},
                model.VertexBuffer.Size + model.IndexBuffer.Size + model.MeshBuffer.Size,
                vk::BufferUsageFlagBits::eTransferSrc);

            {
                uint64_t offset = 0;
                Vulkan::CopyMemoryToAllocation(vertices.data(), staging_buffer.Allocation, offset, model.VertexBuffer.Size);
                offset += model.VertexBuffer.Size;
                Vulkan::CopyMemoryToAllocation(indices.data(), staging_buffer.Allocation, offset, model.IndexBuffer.Size);
                offset += model.IndexBuffer.Size;
                Vulkan::CopyMemoryToAllocation(meshes.data(), staging_buffer.Allocation, offset, model.MeshBuffer.Size);
                offset += model.MeshBuffer.Size;
            }

            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                uint64_t offset = 0;
                Vulkan::CopyBufferToBuffer(
                    staging_buffer.Handle,
                    model.VertexBuffer.Handle,
                    offset, 0,
                    model.VertexBuffer.Size,
                    command_buffer);
                offset += model.VertexBuffer.Size;

                Vulkan::CopyBufferToBuffer(
                    staging_buffer.Handle,
                    model.IndexBuffer.Handle,
                    offset, 0,
                    model.IndexBuffer.Size,
                    command_buffer);
                offset += model.IndexBuffer.Size;

                Vulkan::CopyBufferToBuffer(
                    staging_buffer.Handle,
                    model.MeshBuffer.Handle,
                    offset, 0,
                    model.MeshBuffer.Size,
                    command_buffer);
                offset += model.MeshBuffer.Size;
            });

            Vulkan::DestroyBuffer(staging_buffer);
        }

        model.InstanceBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(Instance) * 2,
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        model.IndirectBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(vk::DrawIndexedIndirectCommand) * model.MeshCount,
            vk::BufferUsageFlagBits::eIndirectBuffer |
                vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        Vulkan::CopyMemoryToAllocation(indirect_commands.data(), model.IndirectBuffer.Allocation, 0, model.IndirectBuffer.Size);

        ModelID id{};
        if (m_FreeModelIDs.empty()) {
            id = m_NextModelID;
            m_NextModelID.ID++;
        } else {
            id = m_FreeModelIDs.back();
            m_FreeModelIDs.pop_back();
        }

        m_Models.emplace(id, model);

        m_LoadedModels.emplace(spath, id);

        m_FrameGraphModelIndexBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_pFrameGraph->importBuffer(model.IndexBuffer.Handle, model.IndexBuffer.Usage, 0, model.IndexBuffer.Size),
                0,
                model.IndexBuffer.Size,
                vk::PipelineStageFlagBits2::eIndexInput,
            });

        m_FrameGraphModelVertexBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_pFrameGraph->importBuffer(model.VertexBuffer.Handle, model.VertexBuffer.Usage, 0, model.VertexBuffer.Size),
                0,
                model.VertexBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexInput,
            });

        m_FrameGraphModelMeshBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_pFrameGraph->importBuffer(model.MeshBuffer.Handle, model.MeshBuffer.Usage, 0, model.MeshBuffer.Size),
                0,
                model.MeshBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader,
            });

        m_FrameGraphModelInstanceBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_pFrameGraph->importBuffer(model.InstanceBuffer.Handle, model.InstanceBuffer.Usage, 0, model.InstanceBuffer.Size),
                0,
                model.InstanceBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader,
            });

        m_FrameGraphModelIndirectBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_pFrameGraph->importBuffer(model.IndirectBuffer.Handle, model.IndirectBuffer.Usage, 0, model.IndirectBuffer.Size),
                0,
                model.IndirectBuffer.Size,
                vk::PipelineStageFlagBits2::eDrawIndirect,
            });

        for (uint32_t i = 0; i < model.MeshCount; i++) {
            MeshID mesh_id{};
            if (m_FreeMeshIDs.empty()) {
                mesh_id = m_NextMeshID;
                m_NextMeshID.ID++;
            } else {
                mesh_id = m_FreeMeshIDs.back();
                m_FreeMeshIDs.pop_back();
            }

            model.MeshToIndex.emplace(mesh_id, i);
            model.IndexToMesh.emplace(i, mesh_id);
        }

        model.InstanceToIndex.clear();
        model.IndexToInstance.clear();

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(0, id.ID, model.MeshBuffer.Handle, 0, model.MeshBuffer.Size)
            .writeStorageBuffer(1, id.ID, model.InstanceBuffer.Handle, 0, model.InstanceBuffer.Size)
            .update(m_ModelDescriptorSet);

        return id;
    }

    Render::InstanceID Render::addInstance(const ModelID model_id, const glm::mat4x4 &transform) {
        DIGNIS_ASSERT(m_Models.contains(model_id));

        Model &model = m_Models.at(model_id);

        Instance instance{};
        instance.VertexTransform = transform;
        instance.NormalTransform = GetNormalTransform(transform);

        InstanceID id{};

        if (m_FreeInstanceIDs.empty()) {
            id = m_NextInstanceID;
            m_NextInstanceID.ID++;
        } else {
            id = m_FreeInstanceIDs.back();
            m_FreeInstanceIDs.pop_back();
        }

        const uint32_t index = model.InstanceCount++;

        if (sizeof(Instance) * index >= model.InstanceBuffer.Size) {
            m_pFrameGraph->removeBuffer(m_FrameGraphModelInstanceBuffers[model_id].Buffer);

            const Vulkan::Buffer old_buffer = model.InstanceBuffer;

            model.InstanceBuffer = Vulkan::AllocateBuffer(
                old_buffer.AllocationFlags,
                old_buffer.MemoryUsage,
                old_buffer.CreateFlags,
                old_buffer.Size * 2,
                old_buffer.Usage);

            m_FrameGraphModelInstanceBuffers[model_id] = FrameGraph::BufferInfo{
                m_pFrameGraph->importBuffer(model.InstanceBuffer.Handle, model.InstanceBuffer.Usage, 0, model.InstanceBuffer.Size),
                0,
                model.InstanceBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader,
            };

            Vulkan::InvalidateAllocation(old_buffer.Allocation, 0, old_buffer.Size);
            Vulkan::CopyMemoryToAllocation(
                Vulkan::GetAllocationInfo(old_buffer).pMappedData,
                model.InstanceBuffer.Allocation, 0,
                old_buffer.Size);

            Vulkan::DestroyBuffer(old_buffer);

            Vulkan::DescriptorSetWriter()
                .writeStorageBuffer(1, model_id.ID, model.InstanceBuffer.Handle, 0, model.InstanceBuffer.Size)
                .update(m_ModelDescriptorSet);
        }

        Vulkan::CopyMemoryToAllocation(
            &instance,
            model.InstanceBuffer.Allocation,
            sizeof(Instance) * index,
            sizeof(Instance));

        m_Instances.insert(id);

        model.InstanceToIndex.emplace(id, index);
        model.IndexToInstance.emplace(index, id);

        m_InstanceToModel.emplace(id, model_id);

        {
            Vulkan::InvalidateAllocation(model.IndirectBuffer.Allocation, 0, model.IndirectBuffer.Size);
            const auto info = Vulkan::GetAllocationInfo(model.IndirectBuffer);

            auto *indirect_commands = static_cast<vk::DrawIndexedIndirectCommand *>(info.pMappedData);
            for (uint32_t i = 0; i < model.MeshCount; i++) {
                indirect_commands[i].instanceCount++;
            }
            Vulkan::FlushAllocation(model.IndirectBuffer.Allocation, 0, model.IndirectBuffer.Size);
        }

        return id;
    }

    void Render::removeModel(const ModelID id) {
        DIGNIS_ASSERT(m_Models.contains(id));

        const auto &model = m_Models.at(id);

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(0, id.ID, nullptr, 0, vk::WholeSize)
            .writeStorageBuffer(1, id.ID, nullptr, 0, vk::WholeSize)
            .update(m_ModelDescriptorSet);

        for (const auto &instance : std::views::values(model.IndexToInstance))
            m_InstanceToModel.erase(instance);

        const Vulkan::Buffer mesh_buffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eCpuOnly, {},
            model.MeshBuffer.Size,
            vk::BufferUsageFlagBits::eTransferDst);
        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                model.MeshBuffer.Handle,
                mesh_buffer.Handle,
                0, 0,
                mesh_buffer.Size,
                command_buffer);
        });
        Vulkan::InvalidateAllocation(mesh_buffer.Allocation, 0, mesh_buffer.Size);

        const auto *meshes = static_cast<Mesh *>(Vulkan::GetAllocationInfo(mesh_buffer).pMappedData);

        for (uint32_t i = 0; i < model.MeshCount; i++)
            removeMaterialRC(meshes[i].Material);

        Vulkan::DestroyBuffer(mesh_buffer);

        m_pFrameGraph->removeBuffer(m_FrameGraphModelIndexBuffers[id].Buffer);
        m_pFrameGraph->removeBuffer(m_FrameGraphModelVertexBuffers[id].Buffer);
        m_pFrameGraph->removeBuffer(m_FrameGraphModelMeshBuffers[id].Buffer);
        m_pFrameGraph->removeBuffer(m_FrameGraphModelInstanceBuffers[id].Buffer);
        m_pFrameGraph->removeBuffer(m_FrameGraphModelIndirectBuffers[id].Buffer);

        m_FrameGraphModelIndexBuffers.remove(id);
        m_FrameGraphModelVertexBuffers.remove(id);
        m_FrameGraphModelMeshBuffers.remove(id);
        m_FrameGraphModelInstanceBuffers.remove(id);
        m_FrameGraphModelIndirectBuffers.remove(id);

        Vulkan::DestroyBuffer(model.VertexBuffer);
        Vulkan::DestroyBuffer(model.IndexBuffer);
        Vulkan::DestroyBuffer(model.MeshBuffer);
        Vulkan::DestroyBuffer(model.InstanceBuffer);
        Vulkan::DestroyBuffer(model.IndirectBuffer);

        m_LoadedModels.erase(model.Path);

        m_Models.erase(id);

        m_FreeModelIDs.emplace_back(id);
    }

    void Render::removeInstance(const InstanceID id) {
        DIGNIS_ASSERT(m_Instances.contains(id));

        const auto model_id = m_InstanceToModel.at(id);

        auto &model = m_Models.at(model_id);

        const uint32_t last_index = --model.InstanceCount;
        if (const uint32_t index = model.InstanceToIndex.at(id);
            index != last_index) {
            Vulkan::InvalidateAllocation(model.InstanceBuffer.Allocation, 0, model.InstanceBuffer.Size);
            const vma::AllocationInfo allocation_info = Vulkan::GetAllocationInfo(model.InstanceBuffer);
            Vulkan::CopyMemoryToAllocation(
                &static_cast<Instance *>(allocation_info.pMappedData)[last_index],
                model.InstanceBuffer.Allocation,
                sizeof(Instance) * index,
                sizeof(Instance));

            const InstanceID last_id = model.IndexToInstance[last_index];

            model.InstanceToIndex[last_id] = index;
            model.IndexToInstance[index]   = last_id;
        }

        model.InstanceToIndex.erase(id);
        model.IndexToInstance.erase(last_index);

        m_InstanceToModel.erase(id);

        m_FreeInstanceIDs.push_back(id);

        {
            Vulkan::InvalidateAllocation(model.IndirectBuffer.Allocation, 0, model.IndirectBuffer.Size);
            const auto info = Vulkan::GetAllocationInfo(model.IndirectBuffer);

            auto *indirect_commands = static_cast<vk::DrawIndexedIndirectCommand *>(info.pMappedData);
            for (uint32_t i = 0; i < model.MeshCount; i++) {
                indirect_commands[i].instanceCount--;
            }
            Vulkan::FlushAllocation(model.IndirectBuffer.Allocation, 0, model.IndirectBuffer.Size);
        }
    }

    std::string_view Render::getModelPath(const ModelID id) {
        DIGNIS_ASSERT(m_Models.contains(id));
        return m_Models.at(id).Path;
    }

    glm::mat4x4 Render::getInstance(const InstanceID id) {
        DIGNIS_ASSERT(m_Instances.contains(id));

        const auto  model_id = m_InstanceToModel.at(id);
        const auto &model    = m_Models.at(model_id);
        const auto  index    = model.InstanceToIndex.at(id);

        glm::mat4x4 transform{};
        Vulkan::CopyAllocationToMemory(
            model.InstanceBuffer.Allocation,
            sizeof(Instance) * index,
            &transform,
            sizeof(glm::mat4x4));
        return transform;
    }

    void Render::processNode(
        const std::filesystem::path &path,

        const aiScene     *ai_scene,
        const aiNode      *ai_node,
        const glm::mat4x4 &transform,

        std::vector<Vertex>   &vertices,
        std::vector<uint32_t> &indices,
        std::vector<Mesh>     &meshes,

        std::vector<vk::DrawIndexedIndirectCommand> &indirect_commands) {
        const glm::mat4x4 node_transform = transform * AssimpToGlm(ai_node->mTransformation);

        for (uint32_t i = 0; i < ai_node->mNumMeshes; i++)
            processMesh(
                path, ai_scene,
                ai_scene->mMeshes[ai_node->mMeshes[i]],
                node_transform,
                vertices, indices, meshes,
                indirect_commands);

        for (uint32_t i = 0; i < ai_node->mNumChildren; i++)
            processNode(
                path, ai_scene,
                ai_node->mChildren[i],
                node_transform,
                vertices, indices, meshes,
                indirect_commands);
    }

    void Render::processMesh(
        const std::filesystem::path &path,

        const aiScene     *ai_scene,
        const aiMesh      *ai_mesh,
        const glm::mat4x4 &transform,

        std::vector<Vertex>   &vertices,
        std::vector<uint32_t> &indices,
        std::vector<Mesh>     &meshes,

        std::vector<vk::DrawIndexedIndirectCommand> &indirect_commands) {
        const uint32_t index_offset  = indices.size();
        const uint32_t vertex_offset = vertices.size();

        for (uint32_t i = 0; i < ai_mesh->mNumFaces; i++) {
            const aiFace &face = ai_mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        for (uint32_t i = 0; i < ai_mesh->mNumVertices; i++) {
            Vertex vertex{};

            const auto position = AssimpToGlm(ai_mesh->mVertices[i]);
            const auto normal   = AssimpToGlm(ai_mesh->mNormals[i]);
            const auto uv       = AssimpToGlm(ai_mesh->mTextureCoords[0][i]);
            // const auto tangent  = AssimpToGlm(ai_mesh->mTangents[i]);
            constexpr auto tangent = glm::vec3{0.0f};

            vertex.Position = position;
            vertex.Normal   = normal;
            vertex.UV       = uv;
            vertex.Tangent  = glm::vec4{tangent, 0.0f};

            vertices.push_back(vertex);
        }

        const uint32_t index_count = indices.size() - index_offset;

        SMikkTSpaceMesh s_mikk_tspace_mesh{};
        s_mikk_tspace_mesh.IndexCount = index_count;
        s_mikk_tspace_mesh.Indices    = &indices[index_offset];
        s_mikk_tspace_mesh.Vertices   = &vertices[vertex_offset];

        SMikkTSpaceContext s_mikk_tspace_context{};
        s_mikk_tspace_context.m_pInterface = &g_SMikkTSpaceInterface;
        s_mikk_tspace_context.m_pUserData  = &s_mikk_tspace_mesh;

        genTangSpaceDefault(&s_mikk_tspace_context);

        const aiMaterial *ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
        DIGNIS_ASSERT(nullptr != ai_material);
        const MaterialID material_id = processMaterial(path, ai_scene, ai_material, ai_mesh->mMaterialIndex);

        Mesh mesh{};
        mesh.VertexTransform = transform;
        mesh.NormalTransform = GetNormalTransform(transform);
        mesh.Material        = material_id;

        vk::DrawIndexedIndirectCommand indirect_command{};
        indirect_command
            .setFirstIndex(index_offset)
            .setIndexCount(index_count)
            .setVertexOffset(static_cast<int32_t>(vertex_offset))
            .setFirstInstance(0)
            .setInstanceCount(0);

        meshes.emplace_back(mesh);
        indirect_commands.emplace_back(indirect_command);
    }

    Render::MaterialID Render::processMaterial(
        const std::filesystem::path &path,

        const aiScene    *ai_scene,
        const aiMaterial *ai_material,

        const uint32_t ai_material_index) {
        const std::string material_name = ai_material->GetName().C_Str();
        const std::string material_path = path / material_name / std::to_string(ai_material_index);

        if (m_LoadedMaterials.contains(material_path)) {
            const auto material_id = m_LoadedMaterials.at(material_path);
            m_LoadedMaterialRCs.at(material_id)++;
            return material_id;
        }

        const std::filesystem::path directory = path.parent_path();

        aiVector3D albedo_factor;
        aiVector3D emissive_factor;

        ai_real metallic_factor;
        ai_real roughness_factor;

        if (AI_SUCCESS != ai_material->Get(AI_MATKEY_BASE_COLOR, albedo_factor))
            if (AI_SUCCESS != ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, albedo_factor))
                albedo_factor = aiVector3D(1.0f, 1.0f, 1.0f);
        if (AI_SUCCESS != ai_material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive_factor))
            emissive_factor = aiVector3D(0.0f, 0.0f, 0.0f);
        if (AI_SUCCESS != ai_material->Get(AI_MATKEY_METALLIC_FACTOR, metallic_factor))
            metallic_factor = 1.0f;
        if (AI_SUCCESS != ai_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_factor))
            roughness_factor = 1.0f;

        auto albedo_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_BASE_COLOR, true);
        if (k_InvalidTextureID == albedo_texture) {
            albedo_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_DIFFUSE, true);
        }
        auto normal_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_NORMALS, false);
        if (k_InvalidTextureID == normal_texture) {
            normal_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_HEIGHT, false);
        }
        const auto emissive_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_EMISSIVE, false);
        const auto ambient_occlusion_texture =
            processTexture(directory, ai_scene, ai_material, aiTextureType_AMBIENT_OCCLUSION, false);
        const auto metallic_roughness_texture =
            processTexture(directory, ai_scene, ai_material, aiTextureType_GLTF_METALLIC_ROUGHNESS, false);
        auto metallic_texture  = k_InvalidTextureID;
        auto roughness_texture = k_InvalidTextureID;
        if (k_InvalidTextureID == metallic_roughness_texture) {
            metallic_texture  = processTexture(directory, ai_scene, ai_material, aiTextureType_METALNESS, false);
            roughness_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_DIFFUSE_ROUGHNESS, false);
        }

        Material material{};
        material.AlbedoFactor    = AssimpToGlm(albedo_factor);
        material.MetallicFactor  = metallic_factor;
        material.EmissiveFactor  = AssimpToGlm(emissive_factor);
        material.RoughnessFactor = roughness_factor;

        material.AlbedoTexture           = albedo_texture;
        material.NormalTexture           = normal_texture;
        material.EmissiveTexture         = emissive_texture;
        material.AmbientOcclusionTexture = ambient_occlusion_texture;

        material.MetallicRoughnessTexture = metallic_roughness_texture;
        material.MetallicTexture          = metallic_texture;
        material.RoughnessTexture         = roughness_texture;

        const MaterialID id = addMaterial(material);
        m_LoadedMaterials.emplace(material_path, id);
        m_LoadedMaterialPaths.emplace(id, material_path);
        m_LoadedMaterialRCs.emplace(id, 1);

        return id;
    }

    Render::TextureID Render::processTexture(
        const std::filesystem::path &directory,

        const aiScene    *ai_scene,
        const aiMaterial *ai_material,

        const aiTextureType ai_texture_type,

        const bool is_srgb) {
        aiString ai_path{};

        if (AI_SUCCESS != ai_material->GetTexture(ai_texture_type, 0, &ai_path))
            return k_InvalidTextureID;

        const std::string texture_path = directory / ai_path.C_Str();

        if (m_LoadedTextures.contains(texture_path)) {
            const auto texture_id = m_LoadedTextures.at(texture_path);
            m_LoadedTextureRCs.at(texture_id)++;
            return texture_id;
        }

        const aiTexture *ai_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());

        const std::optional<TextureAsset> texture_asset_opt =
            nullptr != ai_texture
                ? TextureAsset::LoadFromMemory(ai_texture->pcData, ai_texture->mWidth, TextureAsset::Type::eRGBA8u)
                : TextureAsset::LoadFromPath(texture_path, TextureAsset::Type::eRGBA8u);
        DIGNIS_ASSERT(texture_asset_opt.has_value());
        const TextureAsset &texture_asset = texture_asset_opt.value();

        const vk::Format format =
            is_srgb
                ? vk::Format::eR8G8B8A8Srgb
                : vk::Format::eR8G8B8A8Unorm;

        const Vulkan::Image image = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            format,
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

        const TextureID id = addTexture(image, view);

        m_LoadedTextures.emplace(texture_path, id);
        m_LoadedTexturePaths.emplace(id, texture_path);
        m_LoadedTextureRCs.emplace(id, 1);

        return id;
    }

    int32_t GetSMikkTSpaceNumFaces(const SMikkTSpaceContext *context) {
        return static_cast<int32_t>(static_cast<const SMikkTSpaceMesh *>(context->m_pUserData)->IndexCount) / 3;
    }

    int32_t GetSMikkTSpaceNumVerticesPerFace(const SMikkTSpaceContext *, const int32_t) {
        return 3;
    }

    void GetSMikkTSpacePosition(const SMikkTSpaceContext *context, glm::f32 o_fv_position[3], const int32_t i_face, const int32_t i_vertex) {
        const auto *mesh     = static_cast<const SMikkTSpaceMesh *>(context->m_pUserData);
        const auto &index    = mesh->Indices[i_face * 3 + i_vertex];
        const auto &vertex   = mesh->Vertices[index];
        const auto &position = vertex.Position;

        o_fv_position[0] = position.x;
        o_fv_position[1] = position.y;
        o_fv_position[2] = position.z;
    }

    void GetSMikkTSpaceNormal(const SMikkTSpaceContext *context, glm::f32 o_fv_normal[3], const int32_t i_face, const int32_t i_vertex) {
        const auto *mesh   = static_cast<const SMikkTSpaceMesh *>(context->m_pUserData);
        const auto &index  = mesh->Indices[i_face * 3 + i_vertex];
        const auto &vertex = mesh->Vertices[index];
        const auto &normal = vertex.Normal;

        o_fv_normal[0] = normal.x;
        o_fv_normal[1] = normal.y;
        o_fv_normal[2] = normal.z;
    }

    void GetSMikkTSpaceUV(const SMikkTSpaceContext *context, glm::f32 o_fv_uv[2], const int32_t i_face, const int32_t i_vertex) {
        const auto *mesh   = static_cast<const SMikkTSpaceMesh *>(context->m_pUserData);
        const auto &index  = mesh->Indices[i_face * 3 + i_vertex];
        const auto &vertex = mesh->Vertices[index];
        const auto &uv     = vertex.UV;

        o_fv_uv[0] = uv.x;
        o_fv_uv[1] = uv.y;
    }

    void SetSMikkTSpaceBasic(const SMikkTSpaceContext *context, const glm::f32 fv_tangent[3], const glm::f32 f_sign, const int32_t i_face, const int32_t i_vertex) {
        const auto *mesh  = static_cast<SMikkTSpaceMesh *>(context->m_pUserData);
        const auto &index = mesh->Indices[i_face * 3 + i_vertex];

        auto &vertex  = mesh->Vertices[index];
        auto &tangent = vertex.Tangent;

        tangent.x = fv_tangent[0];
        tangent.y = fv_tangent[1];
        tangent.z = fv_tangent[2];
        tangent.w = f_sign;
    }

}  // namespace Ignis