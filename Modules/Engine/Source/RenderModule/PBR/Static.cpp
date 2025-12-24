#include <Ignis/RenderModule/PBR.hpp>

namespace Ignis {
    PBR::StaticModelID PBR::addStaticModel(const std::filesystem::path &path) {
        std::string spath = path;

        if (m_LoadedStaticModels.contains(spath))
            return m_LoadedStaticModels.at(spath);

        DIGNIS_ASSERT(std::filesystem::exists(path));

        Assimp::Importer importer{};

        const aiScene *ai_scene =
            importer.ReadFile(
                spath,
                aiProcess_Triangulate |
                    aiProcess_GenNormals |
                    aiProcess_GenUVCoords |
                    aiProcess_CalcTangentSpace |
                    aiProcess_FlipUVs);
        DIGNIS_ASSERT(nullptr != ai_scene, "Failed to load asset from path: '{}'. Assimp error: '{}'", spath, importer.GetErrorString());
        DIGNIS_LOG_ENGINE_INFO("Loaded an Ignis::AssimpAsset from path: '{}'", spath);

        const aiNode *ai_node = ai_scene->mRootNode;

        std::vector<StaticVertex> vertices{};
        std::vector<uint32_t>     indices{};
        std::vector<StaticMesh>   meshes{};

        std::vector<vk::DrawIndexedIndirectCommand> indirect_commands{};

        Vulkan::WaitDeviceIdle();

        processStaticNode(path, ai_scene, ai_node, glm::mat4x4{1.0f}, vertices, indices, meshes, indirect_commands);

        DIGNIS_ASSERT(meshes.size() == indirect_commands.size());

        StaticModel model{};

        model.Path = spath;

        model.MeshCount = meshes.size();

        model.InstanceCount = 0;

        model.VertexBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(StaticVertex) * vertices.size(),
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
            sizeof(StaticMesh) * meshes.size(),
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
            });

            Vulkan::DestroyBuffer(staging_buffer);
        }

        model.InstanceBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(StaticInstance) * 2,
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

        StaticModelID id{};
        if (m_FreeStaticModelIDs.empty()) {
            id = m_NextStaticModelID;
            m_NextStaticModelID.ID++;
        } else {
            id = m_FreeStaticModelIDs.back();
            m_FreeStaticModelIDs.pop_back();
        }

        m_StaticModels.emplace(id, model);

        m_LoadedStaticModels.emplace(spath, id);

        m_FrameGraphStaticModelIndexBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(model.IndexBuffer.Handle, model.IndexBuffer.Usage, 0, model.IndexBuffer.Size),
                0,
                model.IndexBuffer.Size,
                vk::PipelineStageFlagBits2::eIndexInput,
            });

        m_FrameGraphStaticModelVertexBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(model.VertexBuffer.Handle, model.VertexBuffer.Usage, 0, model.VertexBuffer.Size),
                0,
                model.VertexBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexInput,
            });

        m_FrameGraphStaticModelMeshBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(model.MeshBuffer.Handle, model.MeshBuffer.Usage, 0, model.MeshBuffer.Size),
                0,
                model.MeshBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader,
            });

        m_FrameGraphStaticModelInstanceBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(model.InstanceBuffer.Handle, model.InstanceBuffer.Usage, 0, model.InstanceBuffer.Size),
                0,
                model.InstanceBuffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader,
            });

        m_FrameGraphStaticModelIndirectBuffers.insert(
            id,
            FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(model.IndirectBuffer.Handle, model.IndirectBuffer.Usage, 0, model.IndirectBuffer.Size),
                0,
                model.IndirectBuffer.Size,
                vk::PipelineStageFlagBits2::eDrawIndirect,
            });

        for (uint32_t i = 0; i < model.MeshCount; i++) {
            StaticMeshID mesh_id{};
            if (m_FreeStaticMeshIDs.empty()) {
                mesh_id = m_NextStaticMeshID;
                m_NextStaticMeshID.ID++;
            } else {
                mesh_id = m_FreeStaticMeshIDs.back();
                m_FreeStaticMeshIDs.pop_back();
            }

            model.MeshToIndex.emplace(mesh_id, i);
            model.IndexToMesh.emplace(i, mesh_id);
        }

        model.InstanceToIndex.clear();
        model.IndexToInstance.clear();

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(0, id.ID, model.MeshBuffer.Handle, 0, model.MeshBuffer.Size)
            .writeStorageBuffer(1, id.ID, model.InstanceBuffer.Handle, 0, model.InstanceBuffer.Size)
            .update(m_StaticDescriptorSet);

        return id;
    }

    PBR::StaticInstanceID PBR::addStaticInstance(const StaticModelID model_id, const glm::mat4x4 &transform) {
        DIGNIS_ASSERT(m_StaticModels.contains(model_id));

        Vulkan::WaitDeviceIdle();

        StaticModel &model = m_StaticModels.at(model_id);

        StaticInstance instance{};
        instance.ModelTransform  = transform;
        instance.NormalTransform = glm::transpose(glm::inverse(transform));

        StaticInstanceID id{};

        if (m_FreeStaticInstanceIDs.empty()) {
            id = m_NextStaticInstanceID;
            m_NextStaticInstanceID.ID++;
        } else {
            id = m_FreeStaticInstanceIDs.back();
            m_FreeStaticInstanceIDs.pop_back();
        }

        const uint32_t index = model.InstanceCount++;

        if (sizeof(StaticInstance) * index >= model.InstanceBuffer.Size) {
            m_FrameGraph.removeBuffer(m_FrameGraphStaticModelInstanceBuffers[model_id].Buffer);

            const Vulkan::Buffer old_buffer = model.InstanceBuffer;

            model.InstanceBuffer = Vulkan::AllocateBuffer(
                old_buffer.AllocationFlags,
                old_buffer.MemoryUsage,
                old_buffer.CreateFlags,
                old_buffer.Size * 2,
                old_buffer.Usage);

            m_FrameGraphStaticModelInstanceBuffers[model_id] = FrameGraph::BufferInfo{
                m_FrameGraph.importBuffer(model.InstanceBuffer.Handle, model.InstanceBuffer.Usage, 0, model.InstanceBuffer.Size),
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
                .update(m_StaticDescriptorSet);
        }

        Vulkan::CopyMemoryToAllocation(
            &instance,
            model.InstanceBuffer.Allocation,
            sizeof(StaticInstance) * index,
            sizeof(StaticInstance));

        m_StaticInstances.insert(id);

        model.InstanceToIndex.emplace(id, index);
        model.IndexToInstance.emplace(index, id);

        m_StaticInstanceToStaticModel.emplace(id, model_id);

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

    void PBR::removeStaticModel(const StaticModelID id) {
        DIGNIS_ASSERT(m_StaticModels.contains(id));

        Vulkan::WaitDeviceIdle();

        const auto &model = m_StaticModels.at(id);

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(0, id.ID, nullptr, 0, vk::WholeSize)
            .writeStorageBuffer(1, id.ID, nullptr, 0, vk::WholeSize)
            .update(m_StaticDescriptorSet);

        for (const auto &instance : std::views::keys(model.InstanceToIndex))
            m_StaticInstanceToStaticModel.erase(instance);

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

        const auto *meshes = static_cast<StaticMesh *>(Vulkan::GetAllocationInfo(mesh_buffer).pMappedData);

        for (uint32_t i = 0; i < model.MeshCount; i++)
            removeMaterialRC(meshes[i].Material);

        Vulkan::DestroyBuffer(mesh_buffer);

        m_FrameGraph.removeBuffer(m_FrameGraphStaticModelIndexBuffers[id].Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphStaticModelVertexBuffers[id].Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphStaticModelMeshBuffers[id].Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphStaticModelInstanceBuffers[id].Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphStaticModelIndirectBuffers[id].Buffer);

        m_FrameGraphStaticModelIndexBuffers.remove(id);
        m_FrameGraphStaticModelVertexBuffers.remove(id);
        m_FrameGraphStaticModelMeshBuffers.remove(id);
        m_FrameGraphStaticModelInstanceBuffers.remove(id);
        m_FrameGraphStaticModelIndirectBuffers.remove(id);

        Vulkan::DestroyBuffer(model.VertexBuffer);
        Vulkan::DestroyBuffer(model.IndexBuffer);
        Vulkan::DestroyBuffer(model.MeshBuffer);
        Vulkan::DestroyBuffer(model.InstanceBuffer);
        Vulkan::DestroyBuffer(model.IndirectBuffer);

        m_LoadedStaticModels.erase(model.Path);

        m_StaticModels.erase(id);

        m_FreeStaticModelIDs.emplace_back(id);
    }

    void PBR::removeStaticInstance(const StaticInstanceID id) {
        DIGNIS_ASSERT(m_StaticInstances.contains(id));

        Vulkan::WaitDeviceIdle();

        const auto model_id = m_StaticInstanceToStaticModel.at(id);

        auto &model = m_StaticModels.at(model_id);

        const uint32_t last_index = --model.InstanceCount;
        if (const uint32_t index = model.InstanceToIndex.at(id);
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

    void PBR::setStaticInstance(const StaticInstanceID id, const glm::mat4x4 &transform) {
        DIGNIS_ASSERT(m_StaticInstances.contains(id));
        const auto  model_id = m_StaticInstanceToStaticModel.at(id);
        const auto &model    = m_StaticModels.at(model_id);
        const auto  index    = model.InstanceToIndex.at(id);

        StaticInstance instance{};
        instance.ModelTransform  = transform;
        instance.NormalTransform = glm::transpose(glm::inverse(transform));

        Vulkan::CopyMemoryToAllocation(
            &instance,
            model.InstanceBuffer.Allocation,
            sizeof(StaticInstance) * index,
            sizeof(StaticInstance));
    }

    std::string_view PBR::getStaticModelPath(const StaticModelID id) {
        DIGNIS_ASSERT(m_StaticModels.contains(id));
        return m_StaticModels.at(id).Path;
    }

    glm::mat4x4 PBR::getStaticInstance(const StaticInstanceID id) {
        DIGNIS_ASSERT(m_StaticInstances.contains(id));
        const auto  model_id = m_StaticInstanceToStaticModel.at(id);
        const auto &model    = m_StaticModels.at(model_id);
        const auto  index    = model.InstanceToIndex.at(id);
        glm::mat4x4 transform{};
        Vulkan::CopyAllocationToMemory(
            model.InstanceBuffer.Allocation,
            sizeof(StaticInstance) * index,
            &transform,
            sizeof(glm::mat4x4));
        return transform;
    }

    void PBR::initStatic(const uint32_t max_binding_count) {
        const FileAsset static_shader_file = FileAsset::LoadBinaryFromPath("Assets/Shaders/Ignis/PBR/Static.spv").value();

        std::vector<uint32_t> static_shader_code{};
        static_shader_code.resize(static_shader_file.getSize() * sizeof(char) / sizeof(uint32_t));

        std::memcpy(static_shader_code.data(), static_shader_file.getContent().data(), static_shader_file.getSize());

        m_StaticDescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addStorageBuffer(0, max_binding_count / 2, vk::ShaderStageFlagBits::eVertex)
                .addStorageBuffer(1, max_binding_count / 2, vk::ShaderStageFlagBits::eVertex)
                .build();

        m_StaticPipelineLayout = Vulkan::CreatePipelineLayout(
            vk::PushConstantRange{
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0, sizeof(CameraPC) + sizeof(StaticModelID)},
            {m_MaterialDescriptorLayout, m_LightDescriptorLayout, m_StaticDescriptorLayout});

        m_StaticDescriptorSet = Vulkan::AllocateDescriptorSet(m_StaticDescriptorLayout, m_DescriptorPool);

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
                        vk::VertexInputAttributeDescription{3, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticVertex, Tangent)},
                        vk::VertexInputAttributeDescription{4, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticVertex, Tangent)},
                    },
                }})
                .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setCullMode(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
                .setColorAttachmentFormats({m_FrameGraph.getImageFormat(m_FrameGraphViewportImage)})
                .setDepthAttachmentFormat(m_FrameGraph.getImageFormat(m_FrameGraphDepthImage))
                .setDepthTest(vk::True, vk::CompareOp::eLess)
                .setNoStencilTest()
                .setNoMultisampling()
                .setNoBlending()
                .build(m_StaticPipelineLayout);

        Vulkan::DestroyShaderModule(static_shader_module);

        m_NextStaticMeshID.ID     = 0;
        m_NextStaticInstanceID.ID = 0;
        m_NextStaticModelID.ID    = 0;

        m_FreeStaticMeshIDs.clear();
        m_FreeStaticInstanceIDs.clear();
        m_FreeStaticModelIDs.clear();
    }

    void PBR::releaseStatic() {
        for (const auto  loaded_static_models = m_LoadedStaticModels;
             const auto &model : std::views::values(loaded_static_models))
            removeStaticModel(model);

        Vulkan::DestroyPipeline(m_StaticPipeline);
        Vulkan::DestroyPipelineLayout(m_StaticPipelineLayout);
        Vulkan::DestroyDescriptorSetLayout(m_StaticDescriptorLayout);
    }

    void PBR::readStaticBuffers(FrameGraph::RenderPass &render_pass) {
        render_pass
            .readBuffers(m_FrameGraphStaticModelVertexBuffers.getData())
            .readBuffers(m_FrameGraphStaticModelIndexBuffers.getData())
            .readBuffers(m_FrameGraphStaticModelMeshBuffers.getData())
            .readBuffers(m_FrameGraphStaticModelInstanceBuffers.getData())
            .readBuffers(m_FrameGraphStaticModelIndirectBuffers.getData());
    }

    void PBR::onStaticDraw(const vk::CommandBuffer command_buffer) {
        const CameraPC camera_pc{
            m_Camera.Projection * m_Camera.View,
            m_Camera.Position,
        };

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_StaticPipeline);
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_StaticPipelineLayout, 0,
            {m_MaterialDescriptorSet, m_LightDescriptorSet, m_StaticDescriptorSet}, {});

        command_buffer.pushConstants(
            m_StaticPipelineLayout,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof(CameraPC),
            &camera_pc);

        for (const auto &[model_id, model] : m_StaticModels) {
            if (0 == model.InstanceCount)
                continue;
            command_buffer.pushConstants(
                m_StaticPipelineLayout,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                sizeof(CameraPC),
                sizeof(StaticModelID),
                &model_id);
            command_buffer.bindIndexBuffer(model.IndexBuffer.Handle, 0, vk::IndexType::eUint32);
            command_buffer.bindVertexBuffers(0, {model.VertexBuffer.Handle}, {0});
            command_buffer.drawIndexedIndirect(
                model.IndirectBuffer.Handle,
                0, model.MeshCount,
                sizeof(vk::DrawIndexedIndirectCommand));
        }
    }

    void PBR::processStaticNode(
        const std::filesystem::path &path,
        const aiScene *ai_scene, const aiNode *ai_node,
        const glm::mat4x4 &transform,

        std::vector<StaticVertex> &vertices,
        std::vector<uint32_t>     &indices,
        std::vector<StaticMesh>   &meshes,

        std::vector<vk::DrawIndexedIndirectCommand> &indirect_commands) {
        const glm::mat4x4 node_transform = transform * AssimpToGlm(ai_node->mTransformation);

        for (uint32_t i = 0; i < ai_node->mNumMeshes; i++)
            processStaticMesh(
                path, ai_scene,
                ai_scene->mMeshes[ai_node->mMeshes[i]],
                node_transform,
                vertices, indices, meshes,
                indirect_commands);

        for (uint32_t i = 0; i < ai_node->mNumChildren; i++)
            processStaticNode(
                path, ai_scene, ai_node->mChildren[i],
                node_transform,
                vertices, indices, meshes,
                indirect_commands);
    }

    void PBR::processStaticMesh(
        const std::filesystem::path &path,
        const aiScene *ai_scene, const aiMesh *ai_mesh,
        const glm::mat4x4 &transform,

        std::vector<StaticVertex> &vertices,
        std::vector<uint32_t>     &indices,
        std::vector<StaticMesh>   &meshes,

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
            StaticVertex vertex{};

            const auto &position  = AssimpToGlm(ai_mesh->mVertices[i]);
            const auto &normal    = AssimpToGlm(ai_mesh->mNormals[i]);
            const auto &uv        = AssimpToGlm(ai_mesh->mTextureCoords[0][i]);
            const auto &tangent   = AssimpToGlm(ai_mesh->mTangents[i]);
            const auto &bitangent = AssimpToGlm(ai_mesh->mBitangents[i]);

            vertex.Position  = position;
            vertex.Normal    = normal;
            vertex.UV        = uv;
            vertex.Tangent   = tangent;
            vertex.Bitangent = bitangent;

            vertices.push_back(vertex);
        }

        const uint32_t index_count = indices.size() - index_offset;

        const aiMaterial *ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
        DIGNIS_ASSERT(nullptr != ai_material);
        const MaterialID material_id = processMaterial(path, ai_scene, ai_material, ai_mesh->mMaterialIndex);

        StaticMesh mesh{};
        mesh.MeshTransform   = transform;
        mesh.NormalTransform = glm::transpose(glm::inverse(transform));
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

    PBR::MaterialID PBR::processMaterial(
        const std::filesystem::path &path,
        const aiScene *ai_scene, const aiMaterial *ai_material,
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

        ai_real metallic_factor;
        ai_real roughness_factor;

        if (AI_SUCCESS != ai_material->Get(AI_MATKEY_BASE_COLOR, albedo_factor))
            albedo_factor = aiVector3D(1.0f, 1.0f, 1.0f);
        if (AI_SUCCESS != ai_material->Get(AI_MATKEY_METALLIC_FACTOR, metallic_factor))
            metallic_factor = 1.0f;
        if (AI_SUCCESS != ai_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_factor))
            roughness_factor = 1.0f;

        const auto albedo_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_BASE_COLOR);
        const auto normal_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_NORMALS);
        const auto metallic_roughness_texture =
            processTexture(directory, ai_scene, ai_material, aiTextureType_GLTF_METALLIC_ROUGHNESS);

        Material material{};
        material.AlbedoFactor    = AssimpToGlm(albedo_factor);
        material.MetallicFactor  = metallic_factor;
        material.RoughnessFactor = roughness_factor;

        material.AlbedoTexture = albedo_texture;
        material.NormalTexture = normal_texture;

        material.MetallicRoughnessTexture = metallic_roughness_texture;

        const MaterialID id = addMaterial(material);
        m_LoadedMaterials.emplace(material_path, id);
        m_LoadedMaterialPaths.emplace(id, material_path);
        m_LoadedMaterialRCs.emplace(id, 1);

        return id;
    }

    PBR::TextureID PBR::processTexture(
        const std::filesystem::path &directory,
        const aiScene *ai_scene, const aiMaterial *ai_material,
        const aiTextureType ai_texture_type) {
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

        const TextureID id = addTexture(image, view);

        m_LoadedTextures.emplace(texture_path, id);
        m_LoadedTexturePaths.emplace(id, texture_path);
        m_LoadedTextureRCs.emplace(id, 1);

        return id;
    }
}  // namespace Ignis