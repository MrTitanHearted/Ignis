#include <Ignis/BlinnPhong.hpp>

namespace Ignis {
    BlinnPhong::BlinnPhong(const Settings &settings)
        : m_ColorAttachmentFormat{settings.ColorAttachmentFormat},
          m_DepthAttachmentFormat{settings.DepthAttachmentFormat},
          m_MaxBindingCount{settings.MaxBindingCount},
          m_NextSamplerHandle{0},
          m_NextTextureHandle{0},
          m_NextMaterialHandle{0},
          m_NextStaticMeshHandle{0},
          m_NextStaticModelHandle{0},
          m_MaterialStagingBuffer{},
          m_MaterialBuffer{},
          m_CameraBuffer{},
          m_FrameGraphMaterialBuffer{},
          m_FrameGraphCameraBuffer{},
          m_Sampler{k_InvalidSamplerHandle} {}

    void BlinnPhong::onAttach(FrameGraph &frame_graph) {
        const FileAsset static_shader_file = FileAsset::LoadBinaryFromPath("Assets/Shaders/Ignis/BlinnPhong/Static.spv").value();

        std::vector<uint32_t> static_shader_code{};
        static_shader_code.resize(static_shader_file.getSize() * sizeof(char) / sizeof(uint32_t));

        std::memcpy(static_shader_code.data(), static_shader_file.getContent().data(), static_shader_file.getSize());

        m_DescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addSampler(0, m_MaxBindingCount, vk::ShaderStageFlagBits::eFragment)
                .addSampledImage(1, m_MaxBindingCount, vk::ShaderStageFlagBits::eFragment)
                .addStorageBuffer(2, vk::ShaderStageFlagBits::eFragment)
                .addStorageBuffer(3, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .addUniformBuffer(4, vk::ShaderStageFlagBits::eVertex)
                .build();
        m_PipelineLayout = Vulkan::CreatePipelineLayout(
            {vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(StaticDrawData)}},
            {m_DescriptorLayout});

        m_DescriptorPool = Vulkan::CreateDescriptorPool(
            {},
            100,
            {
                vk::DescriptorPoolSize{vk::DescriptorType::eSampler, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, m_MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, m_MaxBindingCount},
            });

        m_DescriptorSet = Vulkan::AllocateDescriptorSet(m_DescriptorLayout, m_DescriptorPool);

        m_StaticShader = Vulkan::CreateShaderModuleFromSPV(static_shader_code);

        m_StaticPipeline =
            Vulkan::GraphicsPipelineBuilder()
                .setVertexShader("vs_main", m_StaticShader)
                .setFragmentShader("fs_main", m_StaticShader)
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
                .setCullMode(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
                .setColorAttachmentFormats({m_ColorAttachmentFormat})
                .setDepthAttachmentFormat(m_DepthAttachmentFormat)
                .setDepthTest(vk::True, vk::CompareOp::eLess)
                .setNoStencilTest()
                .setNoMultisampling()
                .setNoBlending()
                .build(m_PipelineLayout);

        m_MaterialStagingBuffer = Vulkan::AllocateBuffer(
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

        m_FrameGraphMaterialBuffer = FrameGraph::BufferInfo{
            frame_graph.importBuffer(m_MaterialBuffer.Handle, m_MaterialBuffer.UsageFlags, 0, m_MaterialBuffer.Size),
            0,
            m_MaterialBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_CameraBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eCpuOnly, {},
            sizeof(Camera),
            vk::BufferUsageFlagBits::eUniformBuffer);

        m_FrameGraphCameraBuffer = FrameGraph::BufferInfo{
            frame_graph.importBuffer(m_CameraBuffer.Handle, m_CameraBuffer.UsageFlags, 0, m_CameraBuffer.Size),
            0,
            m_CameraBuffer.Size,
            vk::PipelineStageFlagBits2::eVertexShader |
                vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_NullSampler = Vulkan::CreateSampler(vk::SamplerCreateInfo{});

        m_Sampler = addSampler(Vulkan::CreateSampler(vk::SamplerCreateInfo{}));

        m_LoadedSamplerRCs[m_Sampler] = 1;

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(2, m_MaterialBuffer.Handle, 0, m_MaterialBuffer.Size)
            .writeUniformBuffer(4, m_CameraBuffer.Handle, 0, m_CameraBuffer.Size)
            .update(m_DescriptorSet);
    }

    void BlinnPhong::onDetach(FrameGraph &frame_graph) {
        frame_graph.removeBuffer(m_FrameGraphCameraBuffer.Buffer);
        frame_graph.removeBuffer(m_FrameGraphMaterialBuffer.Buffer);

        m_LoadedSamplerRCs[m_Sampler]--;
        if (0 == m_LoadedSamplerRCs[m_Sampler]) {
            m_LoadedSamplerRCs.erase(m_Sampler);
            removeSampler(m_Sampler);
        }

        for (const auto &model : std::views::keys(m_StaticModels))
            unloadStaticModel(model, frame_graph);

        DIGNIS_ASSERT(m_LoadedSamplerRCs.empty());
        DIGNIS_ASSERT(m_LoadedTextureRCs.empty());
        DIGNIS_ASSERT(m_LoadedMaterialRCs.empty());

        Vulkan::DestroySampler(m_NullSampler);

        Vulkan::DestroyBuffer(m_CameraBuffer);
        Vulkan::DestroyBuffer(m_MaterialBuffer);
        Vulkan::DestroyBuffer(m_MaterialStagingBuffer);

        Vulkan::DestroyDescriptorPool(m_DescriptorPool);

        Vulkan::DestroyPipeline(m_StaticPipeline);
        Vulkan::DestroyShaderModule(m_StaticShader);

        Vulkan::DestroyPipelineLayout(m_PipelineLayout);
        Vulkan::DestroyDescriptorSetLayout(m_DescriptorLayout);
    }

    void BlinnPhong::onRenderPass(FrameGraph::RenderPass &render_pass) {
        render_pass
            .readImages(m_FrameGraphImages.getData())
            .readBuffers({m_FrameGraphMaterialBuffer, m_FrameGraphCameraBuffer})
            .readBuffers(m_FrameGraphStaticMeshVertexBuffers.getData())
            .readBuffers(m_FrameGraphStaticMeshIndexBuffers.getData())
            .readBuffers(m_FrameGraphStaticInstanceBuffers.getData());
    }

    void BlinnPhong::onDraw(const vk::CommandBuffer command_buffer) {
        StaticDrawData draw_data{};

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_StaticPipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineLayout, 0, {m_DescriptorSet}, {});

        for (const auto &[type, model] : m_StaticModels) {
            draw_data.Instance = type;
            for (const auto &mesh_handle : model.Meshes) {
                const auto &mesh   = m_StaticMeshes[mesh_handle];
                draw_data.Material = mesh.Material;
                command_buffer.pushConstants(
                    m_PipelineLayout,
                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                    0, sizeof(StaticDrawData), &draw_data);
                command_buffer.bindVertexBuffers(0, {mesh.VertexBuffer.Handle}, {0});
                command_buffer.bindIndexBuffer(mesh.IndexBuffer.Handle, 0, vk::IndexType::eUint32);
                command_buffer.drawIndexed(mesh.IndexCount, model.InstanceCount, 0, 0, 0);
            }
        }
    }

    void BlinnPhong::updateCamera(const Camera &camera) const {
        Vulkan::CopyMemoryToAllocation(&camera, m_CameraBuffer.Allocation, 0, m_CameraBuffer.Size);
    }

    BlinnPhong::StaticModel &BlinnPhong::getStaticModelRef(const StaticModelHandle handle) {
        DIGNIS_ASSERT(m_StaticModels.contains(handle));
        return m_StaticModels[handle];
    }

    const BlinnPhong::StaticModel &BlinnPhong::getStaticModelConstRef(StaticModelHandle handle) const {
        DIGNIS_ASSERT(m_StaticModels.contains(handle));
        return m_StaticModels[handle];
    }

    const gtl::flat_hash_map<std::string, BlinnPhong::StaticModelHandle> &BlinnPhong::getStaticModelMap() const {
        return m_LoadedStaticModels;
    }

    BlinnPhong::StaticModelHandle BlinnPhong::loadStaticModel(const std::filesystem::path &path, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(!m_LoadedStaticModels.contains(std::string{path}));
        StaticModelHandle handle = k_InvalidStaticModelHandle;

        if (m_FreeStaticModelHandles.empty()) {
            handle = m_NextStaticModelHandle++;
        } else {
            handle = m_FreeStaticModelHandles.back();
            m_FreeStaticModelHandles.pop_back();
        }

        const auto assimp_asset = AssimpAsset::LoadFromPath(path).value();

        const std::filesystem::path directory = path.parent_path();

        const aiScene *ai_scene = assimp_asset->getScene();
        const aiNode  *ai_node  = ai_scene->mRootNode;

        StaticModel model{};
        model.Path          = path;
        model.InstanceCount = 0;

        processStaticNode(directory, ai_scene, ai_node, model, frame_graph);

        m_StaticModels.insert(handle, model);

        m_NextStaticInstanceHandles[handle] = 0;
        m_FreeStaticInstanceHandles[handle].clear();
        m_NextStaticInstanceIndices[handle] = 0;
        m_FreeStaticInstanceIndices[handle].clear();

        m_StaticInstanceToIndices[handle].clear();
        m_IndicesToStaticInstance[handle].clear();

        m_StaticInstanceBuffers.insert(
            handle,
            Vulkan::AllocateBuffer(
                vma::AllocationCreateFlagBits::eMapped |
                    vma::AllocationCreateFlagBits::eHostAccessRandom,
                vma::MemoryUsage::eAutoPreferHost, {},
                sizeof(StaticInstance) * 2,
                vk::BufferUsageFlagBits::eStorageBuffer |
                    vk::BufferUsageFlagBits::eTransferSrc |
                    vk::BufferUsageFlagBits::eTransferDst));

        const Vulkan::Buffer &instance = m_StaticInstanceBuffers[handle];

        m_FrameGraphStaticInstanceBuffers.insert(
            handle,
            FrameGraph::BufferInfo{
                frame_graph.importBuffer(instance.Handle, instance.UsageFlags, 0, instance.Size),
                0,
                instance.Size,
                vk::PipelineStageFlagBits2::eVertexShader |
                    vk::PipelineStageFlagBits2::eFragmentShader,
            });

        m_LoadedStaticModels[model.Path] = handle;

        return handle;
    }

    void BlinnPhong::unloadStaticModel(const StaticModelHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_StaticModels.contains(handle));

        const auto &model = m_StaticModels[handle];
        DIGNIS_ASSERT(m_LoadedStaticModels.contains(model.Path));

        m_LoadedStaticModels.erase(model.Path);

        for (const auto &mesh : model.Meshes)
            removeStaticMesh(mesh, frame_graph);

        frame_graph.removeBuffer(m_FrameGraphStaticInstanceBuffers[handle].Buffer);

        Vulkan::DestroyBuffer(m_StaticInstanceBuffers[handle]);

        m_NextStaticInstanceIndices.erase(handle);
        m_FreeStaticInstanceIndices.erase(handle);
        m_StaticInstanceToIndices.erase(handle);
        m_IndicesToStaticInstance.erase(handle);

        m_NextStaticInstanceHandles.erase(handle);
        m_FreeStaticInstanceHandles.erase(handle);

        m_StaticInstanceBuffers.remove(handle);
        m_StaticModels.remove(handle);

        m_FreeStaticModelHandles.push_back(handle);
    }

    BlinnPhong::StaticInstance BlinnPhong::getStaticInstance(const StaticModelHandle model, const StaticInstanceHandle handle) {
        DIGNIS_ASSERT(m_StaticModels.contains(model));
        DIGNIS_ASSERT(m_StaticInstanceBuffers.contains(model));
        DIGNIS_ASSERT(m_StaticInstanceToIndices[model].contains(handle));

        const uint32_t index = m_StaticInstanceToIndices[model][handle];

        StaticInstance instance{};

        Vulkan::CopyAllocationToMemory(
            m_StaticInstanceBuffers[model].Allocation,
            sizeof(StaticInstance) * index,
            &instance,
            sizeof(StaticInstance));

        return instance;
    }

    void BlinnPhong::setStaticInstance(
        const StaticModelHandle    model,
        const StaticInstanceHandle handle,
        const StaticInstance      &instance) {
        DIGNIS_ASSERT(m_StaticModels.contains(model));
        DIGNIS_ASSERT(m_StaticInstanceBuffers.contains(model));
        DIGNIS_ASSERT(m_StaticInstanceToIndices[model].contains(handle));

        const uint32_t index = m_StaticInstanceToIndices[model][handle];

        Vulkan::CopyMemoryToAllocation(
            &instance,
            m_StaticInstanceBuffers[model].Allocation,
            sizeof(StaticInstance) * index,
            sizeof(StaticInstance));
    }

    BlinnPhong::StaticInstanceHandle BlinnPhong::addStaticInstance(
        const StaticModelHandle model,
        const StaticInstance   &instance,
        FrameGraph             &frame_graph) {
        DIGNIS_ASSERT(m_StaticModels.contains(model));

        Vulkan::WaitDeviceIdle();

        StaticInstanceHandle handle = k_InvalidStaticInstanceHandle;

        if (m_FreeStaticInstanceHandles[model].empty()) {
            handle = m_NextStaticInstanceHandles[model]++;
        } else {
            handle = m_FreeStaticInstanceHandles[model].back();
            m_FreeStaticInstanceHandles[model].pop_back();
        }

        uint32_t index = k_InvalidStaticInstanceHandle;

        if (m_FreeStaticInstanceIndices[model].empty()) {
            index = m_NextStaticInstanceIndices[model]++;
        } else {
            index = m_FreeStaticInstanceIndices[model].back();
            m_FreeStaticInstanceIndices[model].pop_back();
        }

        if (sizeof(StaticInstance) * index >= m_StaticInstanceBuffers[model].Size) {
            frame_graph.removeBuffer(m_FrameGraphStaticInstanceBuffers[model].Buffer);

            const Vulkan::Buffer old_buffer = m_StaticInstanceBuffers[model];

            m_StaticInstanceBuffers[model] = Vulkan::AllocateBuffer(
                vma::AllocationCreateFlagBits::eMapped |
                    vma::AllocationCreateFlagBits::eHostAccessRandom,
                vma::MemoryUsage::eAutoPreferHost, {},
                2 * old_buffer.Size,
                old_buffer.UsageFlags);

            m_FrameGraphStaticInstanceBuffers[model] = FrameGraph::BufferInfo{
                frame_graph
                    .importBuffer(
                        m_StaticInstanceBuffers[model].Handle,
                        m_StaticInstanceBuffers[model].UsageFlags, 0,
                        m_StaticInstanceBuffers[model].Size),
                0,
                m_FrameGraphStaticInstanceBuffers[model].Size,
                vk::PipelineStageFlagBits2::eVertexShader,
            };

            const vma::Allocation &old_allocation = old_buffer.Allocation;
            Vulkan::InvalidateAllocation(old_allocation, 0, old_buffer.Size);
            const vma::AllocationInfo old_allocation_info = Vulkan::GetAllocationInfo(old_allocation);
            Vulkan::CopyMemoryToAllocation(
                old_allocation_info.pMappedData,
                m_StaticInstanceBuffers[model].Allocation, 0,
                old_buffer.Size);

            Vulkan::DestroyBuffer(old_buffer);

            Vulkan::DescriptorSetWriter()
                .writeStorageBuffer(3, model, m_StaticInstanceBuffers[model].Handle, 0, m_StaticInstanceBuffers[model].Size)
                .update(m_DescriptorSet);
        }

        Vulkan::CopyMemoryToAllocation(
            &instance,
            m_StaticInstanceBuffers[model].Allocation,
            sizeof(StaticInstance) * index,
            sizeof(StaticInstance));

        m_StaticInstanceToIndices[model][handle] = index;
        m_IndicesToStaticInstance[model][index]  = handle;

        m_StaticModels[model].InstanceCount++;

        return handle;
    }

    void BlinnPhong::removeStaticInstance(
        const StaticModelHandle    model,
        const StaticInstanceHandle handle) {
        DIGNIS_ASSERT(m_StaticModels.contains(model));
        DIGNIS_ASSERT(m_StaticInstanceToIndices[model].contains(handle));
        const uint32_t last_slot = m_StaticModels[model].InstanceCount - 1;

        if (const uint32_t slot = m_IndicesToStaticInstance[model][handle];
            slot != last_slot) {
            StaticInstance last_instance{};
            Vulkan::CopyAllocationToMemory(
                m_StaticInstanceBuffers[model].Allocation,
                sizeof(StaticInstance) * last_slot,
                &last_instance,
                sizeof(StaticInstance));
            Vulkan::CopyMemoryToAllocation(
                &last_instance,
                m_StaticInstanceBuffers[model].Allocation,
                sizeof(StaticInstance) * handle,
                sizeof(StaticInstance));

            m_StaticInstanceToIndices[model][m_IndicesToStaticInstance[model][last_slot]] = slot;

            m_IndicesToStaticInstance[model][m_StaticInstanceToIndices[model][slot]] = m_StaticInstanceToIndices[model][slot];
        }

        m_IndicesToStaticInstance[model].erase(last_slot);
        m_StaticInstanceToIndices[model].erase(handle);

        m_FreeStaticInstanceIndices[model].push_back(last_slot);
        m_FreeStaticInstanceHandles[model].push_back(handle);

        m_StaticModels[model].InstanceCount--;
    }

    void BlinnPhong::processStaticNode(
        const std::filesystem::path &directory,
        const aiScene *ai_scene, const aiNode *ai_node,
        StaticModel &model, FrameGraph &frame_graph) {
        for (uint32_t i = 0; i < ai_node->mNumMeshes; i++)
            model.Meshes.push_back(processStaticMesh(directory, ai_scene, ai_scene->mMeshes[ai_node->mMeshes[i]], frame_graph));

        for (uint32_t i = 0; i < ai_node->mNumChildren; i++)
            processStaticNode(directory, ai_scene, ai_node->mChildren[i], model, frame_graph);
    }

    BlinnPhong::StaticMeshHandle BlinnPhong::processStaticMesh(
        const std::filesystem::path &directory,
        const aiScene *ai_scene, const aiMesh *ai_mesh,
        FrameGraph &frame_graph) {
        StaticMeshHandle handle = k_InvalidStaticMeshHandle;

        if (m_FreeStaticMeshHandles.empty()) {
            handle = m_NextStaticMeshHandle++;
        } else {
            handle = m_FreeStaticMeshHandles.back();
            m_FreeStaticMeshHandles.pop_back();
        }

        std::vector<StaticVertex> vertices{};
        std::vector<uint32_t>     indices{};

        vertices.reserve(ai_mesh->mNumVertices);
        indices.reserve(ai_mesh->mNumFaces * 3);

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

        const uint32_t vertex_count = vertices.size();
        const uint32_t index_count  = indices.size();

        const Vulkan::Buffer vertex_buffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(StaticVertex) * vertex_count,
            vk::BufferUsageFlagBits::eVertexBuffer |
                vk::BufferUsageFlagBits::eTransferDst);
        const Vulkan::Buffer index_buffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(uint32_t) * index_count,
            vk::BufferUsageFlagBits::eIndexBuffer |
                vk::BufferUsageFlagBits::eTransferDst);

        {
            const Vulkan::Buffer staging = Vulkan::AllocateBuffer(
                vma::AllocationCreateFlagBits::eMapped,
                vma::MemoryUsage::eCpuOnly, {},
                vertex_buffer.Size + index_buffer.Size,
                vk::BufferUsageFlagBits::eTransferSrc);

            Vulkan::CopyMemoryToAllocation(vertices.data(), staging.Allocation, 0, vertex_buffer.Size);
            Vulkan::CopyMemoryToAllocation(indices.data(), staging.Allocation, vertex_buffer.Size, index_buffer.Size);

            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    staging.Handle,
                    vertex_buffer.Handle,
                    0,
                    0,
                    vertex_buffer.Size,
                    command_buffer);
                Vulkan::CopyBufferToBuffer(
                    staging.Handle,
                    index_buffer.Handle,
                    vertex_buffer.Size,
                    0,
                    index_buffer.Size,
                    command_buffer);
            });

            Vulkan::DestroyBuffer(staging);
        }

        m_FrameGraphStaticMeshVertexBuffers.insert(
            handle,
            FrameGraph::BufferInfo{
                frame_graph.importBuffer(vertex_buffer.Handle, vertex_buffer.UsageFlags, 0, vertex_buffer.Size),
                0,
                vertex_buffer.Size,
                vk::PipelineStageFlagBits2::eVertexInput,
            });

        m_FrameGraphStaticMeshIndexBuffers.insert(
            handle,
            FrameGraph::BufferInfo{
                frame_graph.importBuffer(index_buffer.Handle, index_buffer.UsageFlags, 0, index_buffer.Size),
                0,
                index_buffer.Size,
                vk::PipelineStageFlagBits2::eVertexInput,
            });

        const aiMaterial *ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

        DIGNIS_ASSERT(nullptr != ai_material);
        const MaterialHandle material_handle = processMaterial(directory, ai_scene, ai_material, frame_graph);

        StaticMesh static_mesh{};
        static_mesh.VertexBuffer = vertex_buffer;
        static_mesh.IndexBuffer  = index_buffer;
        static_mesh.VertexCount  = vertex_count;
        static_mesh.IndexCount   = index_count;
        static_mesh.Material     = material_handle;

        m_StaticMeshes.insert(handle, static_mesh);

        return handle;
    }

    BlinnPhong::MaterialHandle BlinnPhong::processMaterial(
        const std::filesystem::path &directory,
        const aiScene *ai_scene, const aiMaterial *ai_material,
        FrameGraph &frame_graph) {
        const char *material_name = ai_material->GetName().C_Str();
        std::string material_path = directory / material_name;

        if (m_LoadedMaterials.find(material_path) != std::end(m_LoadedMaterials)) {
            const MaterialHandle handle = m_LoadedMaterials[material_path];
            DIGNIS_ASSERT(0 == std::strcmp(material_path.data(), m_LoadedMaterialNames[handle].data()));
            m_LoadedMaterialRCs[handle]++;
            return handle;
        }

        const TextureHandle diffuse_texture  = processTexture(directory, ai_scene, ai_material, aiTextureType_DIFFUSE, frame_graph);
        const TextureHandle specular_texture = processTexture(directory, ai_scene, ai_material, aiTextureType_SPECULAR, frame_graph);

        float shininess = 1.0f;
        DIGNIS_ASSERT(AI_SUCCESS == ai_material->Get(AI_MATKEY_SHININESS, shininess));

        m_LoadedTextureRCs[diffuse_texture]++;
        m_LoadedSamplerRCs[m_Sampler]++;
        m_LoadedTextureRCs[specular_texture]++;
        m_LoadedSamplerRCs[m_Sampler]++;

        Material material{};
        material.Diffuse.Texture  = diffuse_texture;
        material.Diffuse.Sampler  = m_Sampler;
        material.Specular.Texture = specular_texture;
        material.Specular.Sampler = m_Sampler;
        material.Shininess        = shininess;

        const MaterialHandle handle = addMaterial(material, frame_graph);

        m_LoadedMaterials[material_path] = handle;
        m_LoadedMaterialRCs[handle]      = 1;
        m_LoadedMaterialNames[handle]    = material_path;

        return handle;
    }

    BlinnPhong::TextureHandle BlinnPhong::processTexture(
        const std::filesystem::path &directory,
        const aiScene *ai_scene, const aiMaterial *ai_material,
        const aiTextureType ai_texture_type,
        FrameGraph         &frame_graph) {
        TextureHandle handle = k_InvalidTextureHandle;

        aiString ai_path{};
        DIGNIS_ASSERT(AI_SUCCESS == ai_material->GetTexture(ai_texture_type, 0, &ai_path));
        const std::string texture_path = directory / ai_path.C_Str();
        if (m_LoadedTextures.find(texture_path) != std::end(m_LoadedTextures)) {
            handle = m_LoadedTextures[texture_path];
            DIGNIS_ASSERT(0 == std::strcmp(texture_path.data(), m_LoadedTextureNames[handle].data()));
            return handle;
        }
        const aiTexture *ai_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());

        const TextureAsset texture_asset =
            nullptr == ai_texture
                ? TextureAsset::LoadFromPath(texture_path, TextureAsset::Type::eRGBA8u).value()
                : TextureAsset::LoadFromMemory(ai_texture->pcData, ai_texture->mWidth * ai_texture->mHeight * sizeof(aiTexel), TextureAsset::Type::eRGBA8u).value();

        const Vulkan::Image image = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst,
            vk::Extent2D{texture_asset.getWidth(), texture_asset.getHeight()});
        const Vulkan::Buffer staging = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            texture_asset.getWidth() * texture_asset.getHeight() * TextureAsset::GetChannelSize(texture_asset.getType()),
            vk::BufferUsageFlagBits::eTransferSrc);
        Vulkan::CopyMemoryToAllocation(texture_asset.getData().data(), staging.Allocation, 0, staging.Size);

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
                staging.Handle,
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

        Vulkan::DestroyBuffer(staging);

        const vk::ImageView view = Vulkan::CreateImageColorView2D(image.Handle, image.Format);

        handle = addTexture(image, view, frame_graph);

        m_LoadedTextures[texture_path] = handle;
        m_LoadedTextureRCs[handle]     = 0;
        m_LoadedTextureNames[handle]   = texture_path;

        return handle;
    }

    void BlinnPhong::removeStaticMesh(const StaticMeshHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_StaticMeshes.contains(handle));

        const StaticMesh mesh = m_StaticMeshes[handle];

        const Material material = getMaterial(mesh.Material);

        m_LoadedSamplerRCs[material.Diffuse.Sampler]--;
        if (0 == m_LoadedSamplerRCs[material.Diffuse.Sampler]) {
            m_LoadedSamplerRCs.erase(material.Diffuse.Sampler);
            removeSampler(material.Diffuse.Sampler);
        }
        m_LoadedTextureRCs[material.Diffuse.Texture]--;
        if (0 == m_LoadedTextureRCs[material.Diffuse.Texture]) {
            m_LoadedTextureRCs.erase(material.Diffuse.Texture);
            removeTexture(material.Diffuse.Texture, frame_graph);
            m_LoadedTextures.erase(m_LoadedTextureNames[material.Diffuse.Texture]);
            m_LoadedTextureNames.erase(material.Diffuse.Texture);
        }
        m_LoadedSamplerRCs[material.Specular.Sampler]--;
        if (0 == m_LoadedSamplerRCs[material.Specular.Sampler]) {
            m_LoadedSamplerRCs.erase(material.Specular.Sampler);
            removeSampler(material.Specular.Sampler);
        }
        m_LoadedTextureRCs[material.Specular.Texture]--;
        if (0 == m_LoadedTextureRCs[material.Specular.Texture]) {
            m_LoadedTextureRCs.erase(material.Specular.Texture);
            removeTexture(material.Specular.Texture, frame_graph);
            m_LoadedTextures.erase(m_LoadedTextureNames[material.Specular.Texture]);
            m_LoadedTextureNames.erase(material.Specular.Texture);
        }

        m_LoadedMaterialRCs[mesh.Material]--;
        if (0 == m_LoadedMaterialRCs[mesh.Material]) {
            m_LoadedMaterialRCs.erase(mesh.Material);
            removeMaterial(mesh.Material);
            m_LoadedMaterials.erase(m_LoadedMaterialNames[mesh.Material]);
            m_LoadedMaterialNames.erase(mesh.Material);
        }

        frame_graph.removeBuffer(m_FrameGraphStaticMeshVertexBuffers[handle].Buffer);
        frame_graph.removeBuffer(m_FrameGraphStaticMeshIndexBuffers[handle].Buffer);

        m_FrameGraphStaticMeshVertexBuffers.remove(handle);
        m_FrameGraphStaticMeshIndexBuffers.remove(handle);

        Vulkan::DestroyBuffer(mesh.VertexBuffer);
        Vulkan::DestroyBuffer(mesh.IndexBuffer);

        m_StaticMeshes.remove(handle);

        m_FreeStaticMeshHandles.push_back(handle);
    }

    BlinnPhong::SamplerHandle BlinnPhong::addSampler(const vk::Sampler sampler) {
        SamplerHandle handle = k_InvalidSamplerHandle;

        if (m_FreeSamplerHandles.empty()) {
            handle = m_NextSamplerHandle++;
        } else {
            handle = m_FreeSamplerHandles.back();
            m_FreeSamplerHandles.pop_back();
        }

        m_Samplers[handle] = sampler;

        Vulkan::DescriptorSetWriter()
            .writeSampler(0, handle, sampler)
            .update(m_DescriptorSet);

        return handle;
    }

    BlinnPhong::TextureHandle BlinnPhong::addTexture(
        const Vulkan::Image &image,
        const vk::ImageView  view,
        FrameGraph          &frame_graph) {
        TextureHandle handle = k_InvalidTextureHandle;

        if (m_FreeTexturesHandles.empty()) {
            handle = m_NextTextureHandle++;
        } else {
            handle = m_FreeTexturesHandles.back();
            m_FreeTexturesHandles.pop_back();
        }

        m_Textures[handle]     = image;
        m_TextureViews[handle] = view;

        m_FrameGraphImages.insert(
            handle,
            frame_graph.importImage(
                image.Handle, view,
                image.Format, image.UsageFlags,
                image.Extent,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal));

        Vulkan::DescriptorSetWriter()
            .writeSampledImage(1, handle, view, vk::ImageLayout::eShaderReadOnlyOptimal)
            .update(m_DescriptorSet);

        return handle;
    }

    BlinnPhong::MaterialHandle BlinnPhong::addMaterial(const Material &material, FrameGraph &frame_graph) {
        MaterialHandle handle = k_InvalidMaterialHandle;

        if (m_FreeMaterialHandles.empty()) {
            handle = m_NextMaterialHandle++;
        } else {
            handle = m_FreeMaterialHandles.back();
            m_FreeMaterialHandles.pop_back();
        }

        m_Materials.insert(handle);

        if (sizeof(Material) * handle >= m_MaterialBuffer.Size) {
            frame_graph.removeBuffer(m_FrameGraphMaterialBuffer.Buffer);

            const Vulkan::Buffer old_material_buffer = m_MaterialBuffer;

            m_MaterialBuffer = Vulkan::AllocateBuffer(
                {}, vma::MemoryUsage::eGpuOnly, {},
                2 * old_material_buffer.Size,
                old_material_buffer.UsageFlags);

            m_FrameGraphMaterialBuffer = FrameGraph::BufferInfo{
                frame_graph.importBuffer(m_MaterialBuffer.Handle, m_MaterialBuffer.UsageFlags, 0, m_MaterialBuffer.Size),
                0,
                m_MaterialBuffer.Size,
                vk::PipelineStageFlagBits2::eFragmentShader,
            };

            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    old_material_buffer.Handle,
                    m_MaterialStagingBuffer.Handle,
                    0,
                    0,
                    old_material_buffer.Size,
                    command_buffer);
            });

            Vulkan::DestroyBuffer(old_material_buffer);

            Vulkan::DescriptorSetWriter()
                .writeStorageBuffer(2, m_MaterialBuffer.Handle, 0, m_MaterialStagingBuffer.Size)
                .update(m_DescriptorSet);
        }

        Vulkan::CopyMemoryToAllocation(&material, m_MaterialStagingBuffer.Allocation, 0, sizeof(Material));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_MaterialStagingBuffer.Handle,
                m_MaterialBuffer.Handle,
                0,
                sizeof(Material) * handle,
                sizeof(Material),
                command_buffer);
        });

        return handle;
    }

    void BlinnPhong::removeSampler(const SamplerHandle handle) {
        DIGNIS_ASSERT(m_Samplers.contains(handle));

        m_FreeSamplerHandles.push_back(handle);

        const vk::Sampler sampler = m_Samplers[handle];
        m_Samplers.erase(handle);
        Vulkan::DestroySampler(sampler);

        Vulkan::DescriptorSetWriter()
            .writeSampler(0, handle, m_NullSampler)
            .update(m_DescriptorSet);
    }

    void BlinnPhong::removeTexture(const TextureHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_Textures.contains(handle));
        DIGNIS_ASSERT(m_TextureViews.contains(handle));
        DIGNIS_ASSERT(m_FrameGraphImages.contains(handle));

        m_FreeTexturesHandles.push_back(handle);

        const Vulkan::Image image = m_Textures[handle];
        const vk::ImageView view  = m_TextureViews[handle];

        const FrameGraph::ImageID id = m_FrameGraphImages[handle];

        m_Textures.erase(handle);
        m_TextureViews.erase(handle);
        m_FrameGraphImages.remove(handle);

        frame_graph.removeImage(id);
        Vulkan::DestroyImageView(view);
        Vulkan::DestroyImage(image);

        Vulkan::DescriptorSetWriter()
            .writeSampledImage(1, handle, nullptr, vk::ImageLayout::eShaderReadOnlyOptimal)
            .update(m_DescriptorSet);
    }

    void BlinnPhong::removeMaterial(const MaterialHandle handle) {
        DIGNIS_ASSERT(m_Materials.contains(handle));

        m_FreeMaterialHandles.push_back(handle);
    }

    BlinnPhong::Material BlinnPhong::getMaterial(const MaterialHandle handle) const {
        DIGNIS_ASSERT(m_Materials.contains(handle));

        Material material{};

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_MaterialBuffer.Handle,
                m_MaterialStagingBuffer.Handle,
                sizeof(Material) * handle,
                0,
                sizeof(Material),
                command_buffer);
        });

        Vulkan::CopyAllocationToMemory(m_MaterialStagingBuffer.Allocation, 0, &material, sizeof(Material));

        return material;
    }
}  // namespace Ignis