#include <Ignis/GPUScenes/BlinnPhong.hpp>

namespace Ignis {
    BlinnPhongScene::BlinnPhongScene(
        const vk::Format color_attachment_format,
        const vk::Format depth_attachment_format)
        : m_NextTextureHandle(0),
          m_NextSamplerHandle(0),
          m_NextVertexBufferHandle(0),
          m_NextIndexBufferHandle(0),
          m_NextMeshHandle(0),
          m_NextMaterialHandle(0),
          m_NextCameraHandle(0),
          m_NextModelHandle(0),
          m_SceneSampler(0),
          m_SceneCamera(0) {
        m_ColorAttachmentFormat = color_attachment_format;
        m_DepthAttachmentFormat = depth_attachment_format;
    }

    void BlinnPhongScene::initialize(FrameGraph &frame_graph) {
        const FileAsset shader_file = FileAsset::BinaryFromPath("Assets/Shaders/Ignis/BlinnPhong.spv").value();

        std::vector<uint32_t> shader_code{};
        shader_code.resize(shader_file.getContent().size() / sizeof(uint32_t));

        std::memcpy(shader_code.data(), shader_file.getContent().data(), shader_file.getContent().size());

        constexpr uint32_t binding_count = 2 << 16;

        m_DescriptorSetLayout =
            Vulkan::DescriptorSetLayoutBuilder{}
                .addSampledImage(0, binding_count, vk::ShaderStageFlagBits::eFragment)
                .addSampler(1, binding_count, vk::ShaderStageFlagBits::eFragment)
                .addStorageBuffer(2, binding_count, vk::ShaderStageFlagBits::eVertex)
                .addStorageBuffer(3, binding_count, vk::ShaderStageFlagBits::eVertex)
                .addUniformBuffer(4, binding_count, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .addUniformBuffer(5, binding_count, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .addUniformBuffer(6, binding_count, vk::ShaderStageFlagBits::eVertex)
                .build();

        m_BlinnPhongShader = Vulkan::CreateShaderModuleFromSPV(shader_code);

        m_PipelineLayout = Vulkan::CreatePipelineLayout(
            {vk::PushConstantRange{
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0,
                sizeof(DrawData),
            }},
            {m_DescriptorSetLayout});
        m_Pipeline = Vulkan::GraphicsPipelineBuilder{}
                         .setVertexShader("vsmain", m_BlinnPhongShader)
                         .setFragmentShader("fsmain", m_BlinnPhongShader)
                         .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                         .setPolygonMode(vk::PolygonMode::eFill)
                         .setCullMode(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
                         .setColorAttachmentFormats(m_ColorAttachmentFormat)
                         .setDepthAttachmentFormat(m_DepthAttachmentFormat)
                         .setDepthTest(true, vk::CompareOp::eLess)
                         .setNoMultisampling()
                         .setNoBlending()
                         .build(m_PipelineLayout);

        m_DescriptorPool =
            Vulkan::CreateDescriptorPool(
                {},
                binding_count,
                {
                    vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, binding_count},
                    vk::DescriptorPoolSize{vk::DescriptorType::eSampler, binding_count},
                    vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, binding_count},
                    vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, binding_count},
                    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, binding_count},
                    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, binding_count},
                    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, binding_count},
                });
        m_DescriptorSet = Vulkan::AllocateDescriptorSet(m_DescriptorSetLayout, m_DescriptorPool);

        m_NextTextureHandle      = 0;
        m_NextSamplerHandle      = 0;
        m_NextVertexBufferHandle = 0;
        m_NextIndexBufferHandle  = 0;
        m_NextMeshHandle         = 0;
        m_NextMaterialHandle     = 0;
        m_NextCameraHandle       = 0;
        m_NextModelHandle        = 0;

        m_FreeTextureHandles.clear();
        m_FreeSamplerHandles.clear();
        m_FreeVertexBufferHandles.clear();
        m_FreeIndexBufferHandles.clear();
        m_FreeMeshHandles.clear();
        m_FreeMaterialHandles.clear();
        m_FreeCameraHandles.clear();
        m_FreeModelHandles.clear();

        m_Textures.clear();
        m_TextureViews.clear();
        m_Samplers.clear();
        m_VertexBuffers.clear();
        m_IndexBuffers.clear();
        m_Meshes.clear();
        m_Materials.clear();
        m_Cameras.clear();

        m_Models.clear();

        m_FrameGraphImageIDs.clear();
        m_FrameGraphVertexBufferIDs.clear();
        m_FrameGraphIndexBufferIDs.clear();
        m_FrameGraphMaterialIDs.clear();
        m_FrameGraphMeshIDs.clear();
        m_FrameGraphCameraIDs.clear();

        m_LoadedTextures.clear();

        m_SceneSampler = addSampler(Vulkan::CreateSampler({}));

        m_SceneCamera = addCamera(
            Vulkan::AllocateBuffer(
                vma::AllocationCreateFlagBits::eMapped,
                vma::MemoryUsage::eCpuOnly, {},
                sizeof(Camera),
                vk::BufferUsageFlagBits::eUniformBuffer),
            frame_graph);
    }

    void BlinnPhongScene::release(FrameGraph &frame_graph) {
        for (const auto &handle : std::views::keys(m_Models))
            removeModel(handle, frame_graph);

        for (const auto &handle : std::views::keys(m_Samplers))
            removeSampler(handle);

        for (const auto &handle : std::views::keys(m_Cameras))
            removeCamera(handle, frame_graph);

        Vulkan::DestroyDescriptorPool(m_DescriptorPool);
        Vulkan::DestroyPipeline(m_Pipeline);
        Vulkan::DestroyPipelineLayout(m_PipelineLayout);
        Vulkan::DestroyDescriptorSetLayout(m_DescriptorSetLayout);
        Vulkan::DestroyShaderModule(m_BlinnPhongShader);
    }

    void BlinnPhongScene::onRender(FrameGraph::RenderPass &render_pass) {
        render_pass
            .readImages(m_FrameGraphImageIDs.getData())
            .readBuffers(m_FrameGraphVertexBufferIDs.getData())
            .readBuffers(m_FrameGraphIndexBufferIDs.getData())
            .readBuffers(m_FrameGraphMeshIDs.getData())
            .readBuffers(m_FrameGraphMaterialIDs.getData())
            .readBuffers(m_FrameGraphCameraIDs.getData());
    }

    void BlinnPhongScene::onDraw(const vk::CommandBuffer command_buffer) {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineLayout, 0, {m_DescriptorSet}, {});

        for (const auto &model : std::views::values(m_Models))
            drawModel(model, command_buffer);
    }

    void BlinnPhongScene::updateSceneCamera(const Camera &camera) {
        DIGNIS_ASSERT(m_Cameras.contains(m_SceneCamera));

        const Vulkan::Buffer &buffer = m_Cameras[m_SceneCamera];

        Vulkan::CopyMemoryToAllocation(&camera, buffer.Allocation, 0, sizeof(Camera));
    }

    BlinnPhongScene::ModelHandle BlinnPhongScene::loadModel(const std::filesystem::path &path, FrameGraph &frame_graph) {
        const std::filesystem::path directory = path.parent_path();

        const auto asset = AssimpAsset::LoadFromPath(path).value();

        const aiScene *ai_scene = asset->getScene();

        Model model{};
        model.Transform = glm::mat4(1.0f);
        model.RootNode  = processNode(directory, frame_graph, ai_scene, ai_scene->mRootNode);

        ModelHandle handle = k_InvalidModelHandle;
        if (!m_FreeModelHandles.empty()) {
            handle = m_FreeModelHandles.back();
            m_FreeModelHandles.pop_back();
        } else {
            m_NextModelHandle++;
        }

        m_Models.insert(handle, model);

        return handle;
    }

    void BlinnPhongScene::removeModel(const ModelHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_Models.contains(handle));

        const auto &[transform, root_node] = m_Models[handle];

        removeNode(root_node, frame_graph);
    }

    BlinnPhongScene::Node BlinnPhongScene::processNode(const std::filesystem::path &directory, FrameGraph &frame_graph, const aiScene *ai_scene, const aiNode *ai_node) {
        Node node{};

        for (uint32_t i = 0; i < ai_node->mNumMeshes; i++) {
            const aiMesh *ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];

            VertexBufferHandle vertex_buffer = k_InvalidVertexBufferHandle;
            IndexBufferHandle  index_buffer  = k_InvalidIndexBufferHandle;

            uint32_t vertices_count;
            uint32_t indices_count;

            {
                std::vector<Vertex>   vertices{};
                std::vector<uint32_t> indices{};

                vertices.reserve(ai_mesh->mNumVertices);
                indices.reserve(ai_mesh->mNumFaces * 3);

                for (uint32_t j = 0; j < ai_mesh->mNumVertices; j++) {
                    Vertex vertex{};

                    const aiVector3D &ai_position   = ai_mesh->mVertices[j];
                    const aiVector3D &ai_normal     = ai_mesh->mNormals[j];
                    const aiVector3D &ai_tex_coords = ai_mesh->mTextureCoords[0][j];

                    vertex.Position = glm::vec3(ai_position.x, ai_position.y, ai_position.z);
                    vertex.Normal   = glm::vec3(ai_normal.x, ai_normal.y, ai_normal.z);
                    vertex.UV       = glm::vec2(ai_tex_coords.x, ai_tex_coords.y);

                    vertices.push_back(vertex);
                }

                for (uint32_t j = 0; j < ai_mesh->mNumFaces; j++) {
                    const aiFace &ai_face = ai_mesh->mFaces[j];
                    for (uint32_t k = 0; k < ai_face.mNumIndices; k++) {
                        indices.push_back(ai_face.mIndices[k]);
                    }
                }

                vertices_count = vertices.size();
                indices_count  = indices.size();

                Vulkan::Buffer vk_vertex_buffer = Vulkan::AllocateBuffer(
                    {}, vma::MemoryUsage::eGpuOnly, {},
                    vertices_count * sizeof(Vertex),
                    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
                Vulkan::Buffer vk_index_buffer = Vulkan::AllocateBuffer(
                    {}, vma::MemoryUsage::eGpuOnly, {},
                    indices_count * sizeof(uint32_t),
                    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

                Vulkan::Buffer vk_staging_buffer = Vulkan::AllocateBuffer(
                    vma::AllocationCreateFlagBits::eMapped,
                    vma::MemoryUsage::eCpuOnly, {},
                    vk_vertex_buffer.Size + vk_index_buffer.Size,
                    vk::BufferUsageFlagBits::eTransferSrc);

                Vulkan::CopyMemoryToAllocation(vertices.data(), vk_staging_buffer.Allocation, 0, vk_vertex_buffer.Size);
                Vulkan::CopyMemoryToAllocation(indices.data(), vk_staging_buffer.Allocation, vk_vertex_buffer.Size, vk_index_buffer.Size);

                Render::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                    Vulkan::CopyBufferToBuffer(
                        vk_staging_buffer.Handle,
                        vk_vertex_buffer.Handle,
                        0,
                        0,
                        vk_vertex_buffer.Size,
                        command_buffer);
                    Vulkan::CopyBufferToBuffer(
                        vk_staging_buffer.Handle,
                        vk_index_buffer.Handle,
                        vk_vertex_buffer.Size,
                        0,
                        vk_index_buffer.Size,
                        command_buffer);
                });

                Vulkan::DestroyBuffer(vk_staging_buffer);
            }

            const aiMaterial *ai_material = ai_scene->mMaterials[i];

            TextureHandle diffuse_texture  = k_InvalidTextureHandle;
            TextureHandle specular_texture = k_InvalidTextureHandle;

            float specular_power = 1.0f;

            {
                aiString ai_path{};
                DIGNIS_ASSERT(AI_SUCCESS == ai_material->GetTexture(aiTextureType_DIFFUSE, 0, &ai_path));
                std::string tpath = (directory / ai_path.C_Str()).string();
                if (m_LoadedTextures.find(tpath) == m_LoadedTextures.end()) {
                    const aiTexture *ai_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());

                    TextureAsset texture =
                        nullptr == ai_texture
                            ? TextureAsset::LoadFromPath(tpath).value()
                            : TextureAsset::LoadFromMemory(ai_texture->pcData, ai_texture->mWidth * ai_texture->mHeight * sizeof(aiTexel)).value();

                    Vulkan::Image image =
                        Vulkan::AllocateImage2D(
                            {}, vma::MemoryUsage::eGpuOnly, {},
                            vk::Format::eR8G8B8A8Unorm,
                            vk::ImageUsageFlagBits::eSampled |
                                vk::ImageUsageFlagBits::eTransferDst,
                            vk::Extent2D{texture.getWidth(), texture.getHeight()});
                    vk::ImageView view = Vulkan::CreateImageColorView2D(image.Handle, image.Format);

                    Vulkan::Buffer staging_buffer =
                        Vulkan::AllocateBuffer(
                            vma::AllocationCreateFlagBits::eMapped,
                            vma::MemoryUsage::eCpuOnly, {},
                            texture.getWidth() * texture.getHeight() * TextureAsset::GetChannelSize(texture.getType()),
                            vk::BufferUsageFlagBits::eTransferSrc);

                    Vulkan::CopyMemoryToAllocation(texture.getData().data(), staging_buffer.Allocation, 0, staging_buffer.Size);

                    Render::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                        Vulkan::BarrierMerger merger{};

                        merger.put_image_barrier(
                            image.Handle,
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eTransferDstOptimal,
                            vk::PipelineStageFlagBits2::eNone,
                            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                            vk::PipelineStageFlagBits2::eNone,
                            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
                        merger.flushBarriers(command_buffer);
                        Vulkan::CopyBufferToImage(
                            staging_buffer.Handle,
                            image.Handle,
                            0,
                            vk::Offset3D{0, 0, 0},
                            vk::Extent2D{0, 0},
                            image.Extent,
                            command_buffer);
                        merger.put_image_barrier(
                            image.Handle,
                            vk::ImageLayout::eTransferDstOptimal,
                            vk::ImageLayout::eShaderReadOnlyOptimal,
                            vk::PipelineStageFlagBits2::eNone,
                            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                            vk::PipelineStageFlagBits2::eNone,
                            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
                        merger.flushBarriers(command_buffer);
                    });

                    Vulkan::DestroyBuffer(staging_buffer);

                    diffuse_texture = addTexture(image, view, frame_graph);

                    m_LoadedTextures[tpath]             = diffuse_texture;
                    m_LoadedTextureRCs[diffuse_texture] = 1;
                } else {
                    diffuse_texture = m_LoadedTextures[tpath];
                    m_LoadedTextureRCs[diffuse_texture]++;
                }

                DIGNIS_ASSERT(AI_SUCCESS == ai_material->GetTexture(aiTextureType_SPECULAR, 0, &ai_path));
                tpath = (directory / ai_path.C_Str()).string();
                if (m_LoadedTextures.find(tpath) == m_LoadedTextures.end()) {
                    const aiTexture *ai_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());

                    TextureAsset texture =
                        nullptr == ai_texture
                            ? TextureAsset::LoadFromPath(tpath).value()
                            : TextureAsset::LoadFromMemory(ai_texture->pcData, ai_texture->mWidth * ai_texture->mHeight * sizeof(aiTexel)).value();

                    Vulkan::Image image =
                        Vulkan::AllocateImage2D(
                            {}, vma::MemoryUsage::eGpuOnly, {},
                            vk::Format::eR8G8B8A8Unorm,
                            vk::ImageUsageFlagBits::eSampled |
                                vk::ImageUsageFlagBits::eTransferDst,
                            vk::Extent2D{texture.getWidth(), texture.getHeight()});
                    vk::ImageView view = Vulkan::CreateImageColorView2D(image.Handle, image.Format);

                    Vulkan::Buffer staging_buffer =
                        Vulkan::AllocateBuffer(
                            vma::AllocationCreateFlagBits::eMapped,
                            vma::MemoryUsage::eCpuOnly, {},
                            texture.getWidth() * texture.getHeight() * TextureAsset::GetChannelSize(texture.getType()),
                            vk::BufferUsageFlagBits::eTransferSrc);

                    Vulkan::CopyMemoryToAllocation(texture.getData().data(), staging_buffer.Allocation, 0, staging_buffer.Size);

                    Render::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                        Vulkan::BarrierMerger merger{};

                        merger.put_image_barrier(
                            image.Handle,
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eTransferDstOptimal,
                            vk::PipelineStageFlagBits2::eNone,
                            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                            vk::PipelineStageFlagBits2::eNone,
                            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
                        merger.flushBarriers(command_buffer);
                        Vulkan::CopyBufferToImage(
                            staging_buffer.Handle,
                            image.Handle,
                            0,
                            vk::Offset3D{0, 0, 0},
                            vk::Extent2D{0, 0},
                            image.Extent,
                            command_buffer);
                        merger.put_image_barrier(
                            image.Handle,
                            vk::ImageLayout::eTransferDstOptimal,
                            vk::ImageLayout::eShaderReadOnlyOptimal,
                            vk::PipelineStageFlagBits2::eNone,
                            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                            vk::PipelineStageFlagBits2::eNone,
                            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
                        merger.flushBarriers(command_buffer);
                    });

                    Vulkan::DestroyBuffer(staging_buffer);

                    specular_texture = addTexture(image, view, frame_graph);

                    m_LoadedTextures[tpath]              = specular_texture;
                    m_LoadedTextureRCs[specular_texture] = 1;
                } else {
                    specular_texture = m_LoadedTextures[tpath];
                    m_LoadedTextureRCs[specular_texture]++;
                }

                DIGNIS_ASSERT(AI_SUCCESS == ai_material->Get(AI_MATKEY_SHININESS_STRENGTH, specular_power));
            }

            MaterialHandle material = k_InvalidMaterialHandle;

            {
                Material material_data{};
                material_data.DiffuseTexture.Texture  = diffuse_texture;
                material_data.DiffuseTexture.Sampler  = m_SceneSampler;
                material_data.SpecularTexture.Texture = specular_texture;
                material_data.SpecularTexture.Sampler = m_SceneSampler;
                material_data.SpecularPower           = specular_power;

                Vulkan::Buffer material_buffer = Vulkan::AllocateBuffer(
                    vma::AllocationCreateFlagBits::eMapped,
                    vma::MemoryUsage::eCpuOnly, {},
                    sizeof(Material),
                    vk::BufferUsageFlagBits::eUniformBuffer);
                Vulkan::CopyMemoryToAllocation(&material_data, material_buffer.Allocation, 0, material_buffer.Size);

                material = addMaterial(material_buffer, frame_graph);
            }

            MeshHandle mesh = k_InvalidMeshHandle;

            {
                Mesh mesh_data{};
                mesh_data.VertexBuffer = vertex_buffer;
                mesh_data.IndexBuffer  = index_buffer;
                mesh_data.Material     = material;

                Vulkan::Buffer mesh_buffer = Vulkan::AllocateBuffer(
                    vma::AllocationCreateFlagBits::eMapped,
                    vma::MemoryUsage::eCpuOnly, {},
                    sizeof(Mesh),
                    vk::BufferUsageFlagBits::eUniformBuffer);
                Vulkan::CopyMemoryToAllocation(&mesh_data, mesh_buffer.Allocation, 0, mesh_buffer.Size);

                mesh = addMesh(mesh_buffer, frame_graph);
            }

            NodeMesh node_mesh{};
            node_mesh.Mesh        = mesh;
            node_mesh.VertexCount = vertices_count;
            node_mesh.IndexCount  = indices_count;

            node.Meshes.push_back(node_mesh);
        }

        for (uint32_t i = 0; i < ai_node->mNumChildren; i++) {
            if (const Node child_node = processNode(directory, frame_graph, ai_scene, ai_node->mChildren[i]);
                !child_node.Meshes.empty())
                node.Children.push_back(child_node);
        }

        return node;
    }

    void BlinnPhongScene::removeNode(const Node &node, FrameGraph &frame_graph) {
        const auto &[children, node_meshes] = node;
        for (const auto &node_mesh : node_meshes) {
            const Mesh     mesh     = getMesh(node_mesh.Mesh);
            const Material material = getMaterial(mesh.Material);

            removeMesh(node_mesh.Mesh, frame_graph);
            removeMaterial(mesh.Material, frame_graph);
            m_LoadedTextureRCs[material.DiffuseTexture.Texture]--;
            m_LoadedTextureRCs[material.SpecularTexture.Texture]--;
            if (m_LoadedTextureRCs[material.DiffuseTexture.Texture] == 0)
                removeTexture(material.DiffuseTexture.Texture, frame_graph);
            if (m_LoadedTextureRCs[material.SpecularTexture.Texture] == 0)
                removeTexture(material.SpecularTexture.Texture, frame_graph);
        }

        for (const auto &child : children)
            removeNode(child, frame_graph);
    }

    void BlinnPhongScene::drawModel(const Model &model, const vk::CommandBuffer command_buffer) const {
        DrawData draw_data{};
        draw_data.Camera = m_SceneCamera;
        draw_data.Model  = model.Transform;

        drawNode(draw_data, model.RootNode, command_buffer);
    }

    void BlinnPhongScene::drawNode(DrawData &draw_data, const Node &node, const vk::CommandBuffer command_buffer) const {
        const auto &[children, node_meshes] = node;
        for (const auto &mesh : node_meshes) {
            draw_data.Mesh = mesh.Mesh;
            command_buffer.pushConstants(
                m_PipelineLayout,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0, sizeof(DrawData), &draw_data);
            command_buffer.draw(mesh.IndexCount, 1, 0, 0);
        }

        for (const auto &child : children)
            drawNode(draw_data, child, command_buffer);
    }

    BlinnPhongScene::TextureHandle BlinnPhongScene::addTexture(
        const Vulkan::Image &image,
        const vk::ImageView  view,
        FrameGraph          &frame_graph) {
        TextureHandle handle = UINT32_MAX;
        if (!m_FreeTextureHandles.empty()) {
            handle = m_FreeTextureHandles.back();
            m_FreeTextureHandles.pop_back();
        } else {
            handle = m_NextTextureHandle++;
        }

        m_Textures[handle]     = image;
        m_TextureViews[handle] = view;

        const FrameGraph::ImageID image_id = frame_graph.importImage(
            image.Handle,
            view,
            image.Format,
            image.Extent,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        m_FrameGraphImageIDs.insert(handle, image_id);

        Vulkan::DescriptorSetWriter()
            .writeSampledImage(0, handle, view, vk::ImageLayout::eShaderReadOnlyOptimal)
            .update(m_DescriptorSet);

        return handle;
    }

    BlinnPhongScene::SamplerHandle BlinnPhongScene::addSampler(const vk::Sampler sampler) {
        SamplerHandle handle = UINT32_MAX;
        if (!m_FreeSamplerHandles.empty()) {
            handle = m_FreeSamplerHandles.back();
            m_FreeSamplerHandles.pop_back();
        } else {
            handle = m_NextSamplerHandle++;
        }

        m_Samplers[handle] = sampler;

        Vulkan::DescriptorSetWriter()
            .writeSampler(1, handle, sampler)
            .update(m_DescriptorSet);

        return handle;
    }

    BlinnPhongScene::VertexBufferHandle BlinnPhongScene::addVertexBuffer(const Vulkan::Buffer &buffer, FrameGraph &frame_graph) {
        VertexBufferHandle handle = UINT32_MAX;
        if (!m_FreeVertexBufferHandles.empty()) {
            handle = m_FreeVertexBufferHandles.back();
            m_FreeVertexBufferHandles.pop_back();
        } else {
            handle = m_NextVertexBufferHandle++;
        }

        m_VertexBuffers[handle] = buffer;

        const FrameGraph::BufferID buffer_id = frame_graph.importBuffer(buffer.Handle, 0, buffer.Size);

        m_FrameGraphVertexBufferIDs.insert(
            handle,
            FrameGraph::BufferInfo{
                buffer_id,
                0,
                buffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader,
            });

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(2, handle, buffer.Handle, 0, buffer.Size)
            .update(m_DescriptorSet);

        return handle;
    }

    BlinnPhongScene::IndexBufferHandle BlinnPhongScene::addIndexBuffer(const Vulkan::Buffer &buffer, FrameGraph &frame_graph) {
        IndexBufferHandle handle = UINT32_MAX;
        if (!m_FreeIndexBufferHandles.empty()) {
            handle = m_FreeIndexBufferHandles.back();
            m_FreeIndexBufferHandles.pop_back();
        } else {
            handle = m_NextIndexBufferHandle++;
        }

        m_IndexBuffers[handle] = buffer;

        const FrameGraph::BufferID buffer_id = frame_graph.importBuffer(buffer.Handle, 0, buffer.Size);

        m_FrameGraphIndexBufferIDs.insert(
            handle,
            FrameGraph::BufferInfo{
                buffer_id,
                0,
                buffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader,
            });

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(3, handle, buffer.Handle, 0, buffer.Size)
            .update(m_DescriptorSet);

        return handle;
    }

    BlinnPhongScene::MaterialHandle BlinnPhongScene::addMaterial(const Vulkan::Buffer &buffer, FrameGraph &frame_graph) {
        MaterialHandle handle = UINT32_MAX;
        if (!m_FreeMaterialHandles.empty()) {
            handle = m_FreeMaterialHandles.back();
            m_FreeMaterialHandles.pop_back();
        } else {
            handle = m_NextMaterialHandle++;
        }

        m_Materials[handle] = buffer;

        const FrameGraph::BufferID buffer_id = frame_graph.importBuffer(buffer.Handle, 0, buffer.Size);

        m_FrameGraphMaterialIDs.insert(
            handle,
            FrameGraph::BufferInfo{
                buffer_id,
                0,
                buffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader |
                    vk::PipelineStageFlagBits2::eFragmentShader,
            });

        Vulkan::DescriptorSetWriter()
            .writeUniformBuffer(4, handle, buffer.Handle, 0, buffer.Size)
            .update(m_DescriptorSet);

        return handle;
    }

    BlinnPhongScene::MeshHandle BlinnPhongScene::addMesh(const Vulkan::Buffer &buffer, FrameGraph &frame_graph) {
        MeshHandle handle = UINT32_MAX;
        if (!m_FreeMeshHandles.empty()) {
            handle = m_FreeMeshHandles.back();
            m_FreeMeshHandles.pop_back();
        } else {
            handle = m_NextMeshHandle++;
        }

        m_Meshes[handle] = buffer;

        const FrameGraph::BufferID buffer_id = frame_graph.importBuffer(buffer.Handle, 0, buffer.Size);

        m_FrameGraphMeshIDs.insert(
            handle,
            FrameGraph::BufferInfo{
                buffer_id,
                0,
                buffer.Size,
                vk::PipelineStageFlagBits2::eVertexShader |
                    vk::PipelineStageFlagBits2::eFragmentShader,
            });

        Vulkan::DescriptorSetWriter()
            .writeUniformBuffer(5, handle, buffer.Handle, 0, buffer.Size)
            .update(m_DescriptorSet);

        return handle;
    }

    BlinnPhongScene::CameraHandle BlinnPhongScene::addCamera(const Vulkan::Buffer &buffer, FrameGraph &frame_graph) {
        CameraHandle handle = UINT32_MAX;
        if (!m_FreeCameraHandles.empty()) {
            handle = m_FreeCameraHandles.back();
            m_FreeCameraHandles.pop_back();
        } else {
            handle = m_NextCameraHandle++;
        }

        m_Cameras[handle] = buffer;

        const FrameGraph::BufferID buffer_id = frame_graph.importBuffer(buffer.Handle, 0, buffer.Size);

        m_FrameGraphCameraIDs.insert(
            handle, FrameGraph::BufferInfo{
                        buffer_id,
                        0,
                        buffer.Size,
                        vk::PipelineStageFlagBits2::eVertexShader |
                            vk::PipelineStageFlagBits2::eFragmentShader,
                    });

        Vulkan::DescriptorSetWriter()
            .writeUniformBuffer(6, handle, buffer.Handle, 0, buffer.Size)
            .update(m_DescriptorSet);

        return handle;
    }

    void BlinnPhongScene::removeTexture(const TextureHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_Textures.contains(handle));

        const Vulkan::Image image = m_Textures[handle];
        const vk::ImageView view  = m_TextureViews[handle];

        const FrameGraph::ImageID image_id = m_FrameGraphImageIDs[handle];

        m_Textures.erase(handle);
        m_TextureViews.erase(handle);
        m_FreeTextureHandles.push_back(handle);

        m_FrameGraphImageIDs.remove(handle);
        frame_graph.removeImage(image_id);

        Vulkan::DestroyImageView(view);
        Vulkan::DestroyImage(image);

        Vulkan::DescriptorSetWriter()
            .writeSampledImage(0, handle, VK_NULL_HANDLE, vk::ImageLayout::eShaderReadOnlyOptimal)
            .update(m_DescriptorSet);
    }

    void BlinnPhongScene::removeSampler(const SamplerHandle handle) {
        DIGNIS_ASSERT(m_Samplers.contains(handle));

        const vk::Sampler sampler = m_Samplers[handle];

        m_Samplers.erase(handle);
        m_FreeSamplerHandles.push_back(handle);

        Vulkan::DestroySampler(sampler);

        Vulkan::DescriptorSetWriter()
            .writeSampler(1, handle, VK_NULL_HANDLE)
            .update(m_DescriptorSet);
    }

    void BlinnPhongScene::removeVertexBuffer(const VertexBufferHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_VertexBuffers.contains(handle));

        const Vulkan::Buffer buffer = m_VertexBuffers[handle];

        const FrameGraph::BufferInfo buffer_info = m_FrameGraphVertexBufferIDs[handle];

        m_VertexBuffers.erase(handle);
        m_FreeVertexBufferHandles.push_back(handle);

        m_FrameGraphVertexBufferIDs.remove(handle);
        frame_graph.removeBuffer(buffer_info.Buffer);

        Vulkan::DestroyBuffer(buffer);

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(2, handle, VK_NULL_HANDLE, 0, 0)
            .update(m_DescriptorSet);
    }

    void BlinnPhongScene::removeIndexBuffer(const IndexBufferHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_IndexBuffers.contains(handle));

        const Vulkan::Buffer buffer = m_IndexBuffers[handle];

        const FrameGraph::BufferInfo buffer_info = m_FrameGraphIndexBufferIDs[handle];

        m_IndexBuffers.erase(handle);
        m_FreeIndexBufferHandles.push_back(handle);

        m_FrameGraphIndexBufferIDs.remove(handle);
        frame_graph.removeBuffer(buffer_info.Buffer);

        Vulkan::DestroyBuffer(buffer);

        Vulkan::DescriptorSetWriter()
            .writeStorageBuffer(3, handle, VK_NULL_HANDLE, 0, 0)
            .update(m_DescriptorSet);
    }

    void BlinnPhongScene::removeMaterial(const MaterialHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_Materials.contains(handle));

        const Vulkan::Buffer buffer = m_Materials[handle];

        const FrameGraph::BufferInfo buffer_info = m_FrameGraphMaterialIDs[handle];

        m_Materials.erase(handle);
        m_FreeMaterialHandles.push_back(handle);

        m_FrameGraphMaterialIDs.remove(handle);
        frame_graph.removeBuffer(buffer_info.Buffer);

        Vulkan::DestroyBuffer(buffer);

        Vulkan::DescriptorSetWriter()
            .writeUniformBuffer(4, handle, VK_NULL_HANDLE, 0, 0)
            .update(m_DescriptorSet);
    }

    void BlinnPhongScene::removeMesh(const MeshHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_Meshes.contains(handle));

        const Vulkan::Buffer buffer = m_Meshes[handle];

        const FrameGraph::BufferInfo buffer_info = m_FrameGraphMeshIDs[handle];

        m_Meshes.erase(handle);
        m_FreeMeshHandles.push_back(handle);

        m_FrameGraphMeshIDs.remove(handle);
        frame_graph.removeBuffer(buffer_info.Buffer);

        Vulkan::DestroyBuffer(buffer);

        Vulkan::DescriptorSetWriter()
            .writeUniformBuffer(5, handle, VK_NULL_HANDLE, 0, 0)
            .update(m_DescriptorSet);
    }

    void BlinnPhongScene::removeCamera(const CameraHandle handle, FrameGraph &frame_graph) {
        DIGNIS_ASSERT(m_Cameras.contains(handle));

        const Vulkan::Buffer buffer = m_Cameras[handle];

        const FrameGraph::BufferInfo buffer_info = m_FrameGraphCameraIDs[handle];

        m_Cameras.erase(handle);
        m_FreeCameraHandles.push_back(handle);

        m_FrameGraphCameraIDs.remove(handle);
        frame_graph.removeBuffer(buffer_info.Buffer);

        Vulkan::DestroyBuffer(buffer);

        Vulkan::DescriptorSetWriter()
            .writeUniformBuffer(6, handle, VK_NULL_HANDLE, 0, 0)
            .update(m_DescriptorSet);
    }

    BlinnPhongScene::Mesh BlinnPhongScene::getMesh(const MeshHandle handle) {
        DIGNIS_ASSERT(m_Meshes.contains(handle));

        const Vulkan::Buffer &buffer = m_Meshes[handle];

        Mesh mesh{};

        Vulkan::CopyAllocationToMemory(buffer.Allocation, 0, &mesh, sizeof(Mesh));

        return mesh;
    }

    BlinnPhongScene::Material BlinnPhongScene::getMaterial(const MaterialHandle handle) {
        DIGNIS_ASSERT(m_Materials.contains(handle));

        const Vulkan::Buffer &buffer = m_Materials[handle];

        Material material{};

        Vulkan::CopyAllocationToMemory(buffer.Allocation, 0, &material, sizeof(Material));

        return material;
    }
}  // namespace Ignis