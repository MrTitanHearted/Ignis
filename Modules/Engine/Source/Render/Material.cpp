#include <Ignis/Render.hpp>

namespace Ignis {
    void Render::initializeMaterials(const uint32_t max_binding_count) {
        m_MaterialDescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addCombinedImageSampler(0, max_binding_count, vk::ShaderStageFlagBits::eFragment)
                .addCombinedImageSampler(1, vk::ShaderStageFlagBits::eFragment)
                .addCombinedImageSampler(2, vk::ShaderStageFlagBits::eFragment)
                .addCombinedImageSampler(3, vk::ShaderStageFlagBits::eFragment)
                .addStorageBuffer(4, vk::ShaderStageFlagBits::eFragment)
                .build();

        m_MaterialDescriptorSet = Vulkan::AllocateDescriptorSet(m_MaterialDescriptorLayout, m_DescriptorPool);

        m_MaterialStagingBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(Material),
            vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_MaterialBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(Material) * 2,
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_FrameGraphMaterialBuffer = FrameGraph::BufferInfo{
            m_pFrameGraph->importBuffer(m_MaterialBuffer.Handle, m_MaterialBuffer.Usage, 0, m_MaterialBuffer.Size),
            0,
            m_MaterialBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader};

        m_NextTextureID.ID  = 0u;
        m_NextMaterialID.ID = 0u;

        m_FreeTextureIDs.clear();
        m_FreeMaterialIDs.clear();

        m_FrameGraphImages.clear();

        Vulkan::DescriptorSetWriter()
            .writeCombinedImageSampler(1, m_BRDFLUTImageView, vk::ImageLayout::eShaderReadOnlyOptimal, m_Sampler)
            .writeCombinedImageSampler(2, m_PrefilterImageView, vk::ImageLayout::eShaderReadOnlyOptimal, m_Sampler)
            .writeCombinedImageSampler(3, m_IrradianceImageView, vk::ImageLayout::eShaderReadOnlyOptimal, m_Sampler)
            .writeStorageBuffer(4, m_MaterialBuffer.Handle, 0, m_MaterialBuffer.Size)
            .update(m_MaterialDescriptorSet);

        initializeDefaultMaps();
    }

    void Render::releaseMaterials() {
        while (!m_Materials.empty()) {
            for (const auto  materials = m_Materials;
                 const auto &material_id : materials) {
                removeMaterialRC(material_id);
            }
        }
        while (!m_Textures.empty()) {
            for (const auto  textures = m_Textures;
                 const auto &texture_id : std::views::keys(textures))
                removeTextureRC(texture_id);
        }

        m_pFrameGraph->removeBuffer(m_FrameGraphMaterialBuffer.Buffer);

        Vulkan::DestroyBuffer(m_MaterialStagingBuffer);
        Vulkan::DestroyBuffer(m_MaterialBuffer);

        Vulkan::DestroyDescriptorSetLayout(m_MaterialDescriptorLayout);
    }

    Render::TextureID Render::addBlackTexture() {
        m_LoadedTextureRCs[m_BlackTexture]++;
        return m_BlackTexture;
    }

    Render::TextureID Render::addWhiteTexture() {
        m_LoadedTextureRCs[m_WhiteTexture]++;
        return m_WhiteTexture;
    }

    Render::TextureID Render::addNormalTexture() {
        m_LoadedTextureRCs[m_NormalTexture]++;
        return m_NormalTexture;
    }

    Render::TextureID Render::addMetallicRoughnessTexture() {
        m_LoadedTextureRCs[m_MetallicRoughnessTexture]++;
        return m_MetallicRoughnessTexture;
    }

    Render::TextureID Render::addTexture(const Vulkan::Image &image, const vk::ImageView view) {
        TextureID id;

        if (m_FreeTextureIDs.empty()) {
            id = m_NextTextureID;
            m_NextTextureID.ID++;
        } else {
            id = m_FreeTextureIDs.back();
            m_FreeTextureIDs.pop_back();
        }

        m_FrameGraphImages.insert(
            id,
            m_pFrameGraph->importImage(
                image.Handle, view,
                image.Format, image.Usage, image.Extent,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal));

        Vulkan::DescriptorSetWriter()
            .writeCombinedImageSampler(0, id.ID, view, vk::ImageLayout::eShaderReadOnlyOptimal, m_Sampler)
            .update(m_MaterialDescriptorSet);

        m_Textures.emplace(id, image);
        m_TextureViews.emplace(id, view);

        return id;
    }

    Render::MaterialID Render::addMaterial(const Material &material) {
        MaterialID id{};

        if (m_FreeMaterialIDs.empty()) {
            id = m_NextMaterialID;
            m_NextMaterialID.ID++;
        } else {
            id = m_FreeMaterialIDs.back();
            m_FreeMaterialIDs.pop_back();
        }

        if (sizeof(Material) * id.ID >= m_MaterialBuffer.Size) {
            m_pFrameGraph->removeBuffer(m_FrameGraphMaterialBuffer.Buffer);

            const Vulkan::Buffer old_buffer = m_MaterialBuffer;

            const auto old_stage_mask = m_FrameGraphMaterialBuffer.StageMask;

            m_MaterialBuffer = Vulkan::AllocateBuffer(
                old_buffer.AllocationFlags,
                old_buffer.MemoryUsage,
                old_buffer.CreateFlags,
                old_buffer.Size * 2,
                old_buffer.Usage);

            m_FrameGraphMaterialBuffer = FrameGraph::BufferInfo{
                m_pFrameGraph->importBuffer(m_MaterialBuffer.Handle, m_MaterialBuffer.Usage, 0, m_MaterialBuffer.Size),
                0,
                m_MaterialBuffer.Size,
                old_stage_mask,
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
                .writeStorageBuffer(4, m_MaterialBuffer.Handle, 0, m_MaterialBuffer.Size)
                .update(m_MaterialDescriptorSet);
        }

        Vulkan::CopyMemoryToAllocation(&material, m_MaterialStagingBuffer.Allocation, 0, sizeof(Material));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_MaterialStagingBuffer.Handle,
                m_MaterialBuffer.Handle, 0,
                sizeof(Material) * id.ID,
                sizeof(Material),
                command_buffer);
        });

        m_Materials.insert(id);

        return id;
    }

    void Render::removeTextureRC(const TextureID id) {
        DIGNIS_ASSERT(m_Textures.contains(id));

        m_LoadedTextureRCs.at(id)--;
        if (0 != m_LoadedTextureRCs.at(id))
            return;

        const std::string path = m_LoadedTexturePaths.at(id);

        m_pFrameGraph->removeImage(m_FrameGraphImages[id]);
        m_FrameGraphImages.remove(id);

        const Vulkan::Image image = m_Textures.at(id);
        const vk::ImageView view  = m_TextureViews.at(id);

        Vulkan::DescriptorSetWriter()
            .writeCombinedImageSampler(0, id.ID, nullptr, vk::ImageLayout::eUndefined, m_Sampler)
            .update(m_MaterialDescriptorSet);

        Vulkan::DestroyImageView(view);
        Vulkan::DestroyImage(image);

        m_Textures.erase(id);
        m_TextureViews.erase(id);

        m_LoadedTextures.erase(path);
        m_LoadedTexturePaths.erase(id);
        m_LoadedTextureRCs.erase(id);

        m_FreeTextureIDs.emplace_back(id);
    }

    void Render::removeMaterialRC(const MaterialID id) {
        DIGNIS_ASSERT(m_Materials.contains(id));

        m_LoadedMaterialRCs.at(id)--;
        if (0 != m_LoadedMaterialRCs.at(id))
            return;

        const Material material = getMaterial(id);

        if (k_InvalidTextureID != material.AlbedoTexture)
            removeTextureRC(material.AlbedoTexture);
        if (k_InvalidTextureID != material.NormalTexture)
            removeTextureRC(material.NormalTexture);
        if (k_InvalidTextureID != material.EmissiveTexture)
            removeTextureRC(material.EmissiveTexture);
        if (k_InvalidTextureID != material.AmbientOcclusionTexture)
            removeTextureRC(material.AmbientOcclusionTexture);
        if (k_InvalidTextureID != material.MetallicRoughnessTexture)
            removeTextureRC(material.MetallicRoughnessTexture);
        if (k_InvalidTextureID != material.MetallicTexture)
            removeTextureRC(material.MetallicTexture);
        if (k_InvalidTextureID != material.RoughnessTexture)
            removeTextureRC(material.RoughnessTexture);

        const std::string path = m_LoadedMaterialPaths.at(id);

        m_Materials.erase(id);

        m_LoadedMaterials.erase(path);
        m_LoadedMaterialPaths.erase(id);
        m_LoadedMaterialRCs.erase(id);

        m_FreeMaterialIDs.emplace_back(id);
    }

    Render::Material Render::getMaterial(const MaterialID id) const {
        DIGNIS_ASSERT(m_Materials.contains(id));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_MaterialBuffer.Handle,
                m_MaterialStagingBuffer.Handle,
                sizeof(Material) * id.ID, 0,
                sizeof(Material),
                command_buffer);
        });

        Material material{};

        Vulkan::CopyAllocationToMemory(m_MaterialStagingBuffer.Allocation, 0, &material, sizeof(Material));

        return material;
    }

    void Render::readMaterialImages(FrameGraph::RenderPass &render_pass) {
        render_pass.readImages(m_FrameGraphImages.getData());
    }

    void Render::readMaterialBuffers(FrameGraph::RenderPass &render_pass) const {
        render_pass.readBuffers(m_FrameGraphMaterialBuffer);
    }
}  // namespace Ignis