#include <Ignis/Render.hpp>

namespace Ignis {
    void Render::SetDirectionalLight(const DirectionalLight &light) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");

        Vulkan::CopyMemoryToAllocation(
            &light,
            s_pInstance->m_DirectionalLightStagingBuffer.Allocation,
            0,
            sizeof(DirectionalLight));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                s_pInstance->m_DirectionalLightStagingBuffer.Handle,
                s_pInstance->m_DirectionalLightBuffer.Handle,
                0, 0,
                sizeof(DirectionalLight),
                command_buffer);
        });
    }

    void Render::SetPointLight(const PointLightID id, const PointLight &light) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        DIGNIS_ASSERT(s_pInstance->m_PointLights.contains(id));
        Vulkan::CopyMemoryToAllocation(&light, s_pInstance->m_PointLightStagingBuffer.Allocation, 0, sizeof(PointLight));

        const uint32_t &index = s_pInstance->m_PointLightToIndex.at(id);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                s_pInstance->m_PointLightStagingBuffer.Handle,
                s_pInstance->m_PointLightBuffer.Handle, 0,
                sizeof(PointLight) * index,
                sizeof(PointLight),
                command_buffer);
        });
    }

    void Render::SetSpotLight(const SpotLightID id, const SpotLight &light) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        DIGNIS_ASSERT(s_pInstance->m_SpotLights.contains(id));
        Vulkan::CopyMemoryToAllocation(&light, s_pInstance->m_SpotLightStagingBuffer.Allocation, 0, sizeof(SpotLight));

        const uint32_t &index = s_pInstance->m_SpotLightToIndex.at(id);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                s_pInstance->m_SpotLightStagingBuffer.Handle,
                s_pInstance->m_SpotLightBuffer.Handle, 0,
                sizeof(SpotLight) * index,
                sizeof(SpotLight),
                command_buffer);
        });
    }

    Render::PointLightID Render::AddPointLight(const PointLight &light) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        PointLightID id{k_InvalidPointLightID};
        if (s_pInstance->m_FreePointLightIDs.empty()) {
            id = s_pInstance->m_NextPointLightID;
            s_pInstance->m_NextPointLightID.ID++;
        } else {
            id = s_pInstance->m_FreePointLightIDs.back();
            s_pInstance->m_FreePointLightIDs.pop_back();
        }

        uint32_t index;

        {
            const vma::AllocationInfo light_data_allocation_info = Vulkan::GetAllocationInfo(s_pInstance->m_LightDataBuffer);

            auto *light_data = static_cast<LightData *>(light_data_allocation_info.pMappedData);

            index = light_data->PointLightCount++;

            Vulkan::FlushAllocation(s_pInstance->m_LightDataBuffer.Allocation, offsetof(LightData, PointLightCount), sizeof(uint32_t));
        }

        if (sizeof(PointLight) * index >= s_pInstance->m_PointLightBuffer.Size) {
            s_pInstance->m_pFrameGraph->removeBuffer(s_pInstance->m_FrameGraphPointLightBuffer.Buffer);

            const Vulkan::Buffer old_buffer = s_pInstance->m_PointLightBuffer;

            s_pInstance->m_PointLightBuffer = Vulkan::AllocateBuffer(
                old_buffer.AllocationFlags,
                old_buffer.MemoryUsage,
                old_buffer.CreateFlags,
                old_buffer.Size * 2,
                old_buffer.Usage);

            s_pInstance->m_FrameGraphPointLightBuffer = FrameGraph::BufferInfo{
                s_pInstance->m_pFrameGraph->importBuffer(s_pInstance->m_PointLightBuffer.Handle, s_pInstance->m_PointLightBuffer.Usage, 0, s_pInstance->m_PointLightBuffer.Size),
                0,
                s_pInstance->m_PointLightBuffer.Size,
                vk::PipelineStageFlagBits2::eFragmentShader,
            };

            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    old_buffer.Handle,
                    s_pInstance->m_PointLightBuffer.Handle,
                    0, 0,
                    old_buffer.Size,
                    command_buffer);
            });

            Vulkan::DestroyBuffer(old_buffer);

            Vulkan::DescriptorSetWriter()
                .writeStorageBuffer(2, s_pInstance->m_PointLightBuffer.Handle, 0, s_pInstance->m_PointLightBuffer.Size)
                .update(s_pInstance->m_LightDescriptorSet);
        }

        Vulkan::CopyMemoryToAllocation(&light, s_pInstance->m_PointLightStagingBuffer.Allocation, 0, sizeof(PointLight));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                s_pInstance->m_PointLightStagingBuffer.Handle,
                s_pInstance->m_PointLightBuffer.Handle, 0,
                sizeof(PointLight) * index,
                sizeof(PointLight),
                command_buffer);
        });

        s_pInstance->m_PointLights.emplace(id);
        s_pInstance->m_PointLightToIndex.emplace(id, index);
        s_pInstance->m_IndexToPointLight.emplace(index, id);

        return id;
    }

    Render::SpotLightID Render::AddSpotLight(const SpotLight &light) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        SpotLightID id;
        if (s_pInstance->m_FreeSpotLightIDs.empty()) {
            id = s_pInstance->m_NextSpotLightID;
            s_pInstance->m_NextSpotLightID.ID++;
        } else {
            id = s_pInstance->m_FreeSpotLightIDs.back();
            s_pInstance->m_FreeSpotLightIDs.pop_back();
        }

        uint32_t index;

        {
            const vma::AllocationInfo light_data_allocation_info = Vulkan::GetAllocationInfo(s_pInstance->m_LightDataBuffer);

            auto *light_data = static_cast<LightData *>(light_data_allocation_info.pMappedData);

            index = light_data->SpotLightCount++;

            Vulkan::FlushAllocation(s_pInstance->m_LightDataBuffer.Allocation, offsetof(LightData, SpotLightCount), sizeof(uint32_t));
        }

        if (sizeof(SpotLight) * index >= s_pInstance->m_SpotLightBuffer.Size) {
            s_pInstance->m_pFrameGraph->removeBuffer(s_pInstance->m_FrameGraphSpotLightBuffer.Buffer);

            const Vulkan::Buffer old_buffer = s_pInstance->m_SpotLightBuffer;

            s_pInstance->m_SpotLightBuffer = Vulkan::AllocateBuffer(
                old_buffer.AllocationFlags,
                old_buffer.MemoryUsage,
                old_buffer.CreateFlags,
                old_buffer.Size * 2,
                old_buffer.Usage);

            s_pInstance->m_FrameGraphSpotLightBuffer = FrameGraph::BufferInfo{
                s_pInstance->m_pFrameGraph->importBuffer(s_pInstance->m_SpotLightBuffer.Handle, s_pInstance->m_SpotLightBuffer.Usage, 0, s_pInstance->m_SpotLightBuffer.Size),
                0,
                s_pInstance->m_SpotLightBuffer.Size,
                vk::PipelineStageFlagBits2::eFragmentShader,
            };

            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    old_buffer.Handle,
                    s_pInstance->m_SpotLightBuffer.Handle,
                    0, 0,
                    old_buffer.Size,
                    command_buffer);
            });

            Vulkan::DestroyBuffer(old_buffer);

            Vulkan::DescriptorSetWriter()
                .writeStorageBuffer(2, s_pInstance->m_SpotLightBuffer.Handle, 0, s_pInstance->m_SpotLightBuffer.Size)
                .update(s_pInstance->m_LightDescriptorSet);
        }

        Vulkan::CopyMemoryToAllocation(&light, s_pInstance->m_SpotLightStagingBuffer.Allocation, 0, sizeof(SpotLight));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                s_pInstance->m_SpotLightStagingBuffer.Handle,
                s_pInstance->m_SpotLightBuffer.Handle, 0,
                sizeof(SpotLight) * index,
                sizeof(SpotLight),
                command_buffer);
        });

        s_pInstance->m_SpotLights.emplace(id);
        s_pInstance->m_SpotLightToIndex.emplace(id, index);
        s_pInstance->m_IndexToSpotLight.emplace(index, id);

        return id;
    }

    void Render::RemovePointLight(const PointLightID id) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        DIGNIS_ASSERT(s_pInstance->m_PointLights.contains(id));

        uint32_t last_index;

        {
            const vma::AllocationInfo light_data_allocation_info = Vulkan::GetAllocationInfo(s_pInstance->m_LightDataBuffer);

            auto *light_data = static_cast<LightData *>(light_data_allocation_info.pMappedData);

            last_index = --light_data->PointLightCount;

            Vulkan::FlushAllocation(s_pInstance->m_LightDataBuffer.Allocation, offsetof(LightData, PointLightCount), sizeof(uint32_t));
        }

        if (const uint32_t &index = s_pInstance->m_PointLightToIndex.at(id);
            last_index != index) {
            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    s_pInstance->m_PointLightBuffer.Handle,
                    s_pInstance->m_PointLightBuffer.Handle,
                    sizeof(PointLight) * last_index,
                    sizeof(PointLight) * index,
                    sizeof(PointLight),
                    command_buffer);
            });

            const PointLightID last_id = s_pInstance->m_IndexToPointLight.at(last_index);

            s_pInstance->m_PointLightToIndex[last_id] = index;
            s_pInstance->m_IndexToPointLight[index]   = last_id;
        }

        s_pInstance->m_PointLights.erase(id);
        s_pInstance->m_PointLightToIndex.erase(id);
        s_pInstance->m_IndexToPointLight.erase(last_index);

        s_pInstance->m_FreePointLightIDs.push_back(id);
    }

    void Render::RemoveSpotLight(const SpotLightID id) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        DIGNIS_ASSERT(s_pInstance->m_SpotLights.contains(id));

        uint32_t last_index;

        {
            const vma::AllocationInfo light_data_allocation_info = Vulkan::GetAllocationInfo(s_pInstance->m_LightDataBuffer);

            auto *light_data = static_cast<LightData *>(light_data_allocation_info.pMappedData);

            last_index = --light_data->SpotLightCount;

            Vulkan::FlushAllocation(s_pInstance->m_LightDataBuffer.Allocation, offsetof(LightData, SpotLightCount), sizeof(uint32_t));
        }

        if (const uint32_t &index = s_pInstance->m_SpotLightToIndex.at(id);
            last_index != index) {
            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    s_pInstance->m_SpotLightBuffer.Handle,
                    s_pInstance->m_SpotLightBuffer.Handle,
                    sizeof(SpotLight) * last_index,
                    sizeof(SpotLight) * index,
                    sizeof(SpotLight),
                    command_buffer);
            });

            const SpotLightID last_id = s_pInstance->m_IndexToSpotLight.at(last_index);

            s_pInstance->m_SpotLightToIndex[last_id] = index;
            s_pInstance->m_IndexToSpotLight[index]   = last_id;
        }

        s_pInstance->m_SpotLights.erase(id);
        s_pInstance->m_SpotLightToIndex.erase(id);
        s_pInstance->m_IndexToSpotLight.erase(last_index);

        s_pInstance->m_FreeSpotLightIDs.push_back(id);
    }

    Render::DirectionalLight Render::GetDirectionalLight() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                s_pInstance->m_DirectionalLightBuffer.Handle,
                s_pInstance->m_DirectionalLightStagingBuffer.Handle,
                0, 0,
                sizeof(DirectionalLight),
                command_buffer);
        });
        DirectionalLight light{};
        Vulkan::CopyAllocationToMemory(s_pInstance->m_DirectionalLightStagingBuffer.Allocation, 0, &light, sizeof(DirectionalLight));
        return light;
    }

    Render::PointLight Render::GetPointLight(PointLightID id) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        DIGNIS_ASSERT(s_pInstance->m_PointLights.contains(id));

        const uint32_t index = s_pInstance->m_PointLightToIndex.at(id);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                s_pInstance->m_PointLightBuffer.Handle,
                s_pInstance->m_PointLightStagingBuffer.Handle,
                sizeof(PointLight) * index, 0,
                sizeof(PointLight),
                command_buffer);
        });
        PointLight light{};
        Vulkan::CopyAllocationToMemory(s_pInstance->m_PointLightStagingBuffer.Allocation, 0, &light, sizeof(PointLight));
        return light;
    }

    Render::SpotLight Render::GetSpotLight(SpotLightID id) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        DIGNIS_ASSERT(s_pInstance->m_SpotLights.contains(id));

        const uint32_t index = s_pInstance->m_SpotLightToIndex.at(id);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                s_pInstance->m_SpotLightBuffer.Handle,
                s_pInstance->m_SpotLightStagingBuffer.Handle,
                sizeof(SpotLight) * index, 0,
                sizeof(SpotLight),
                command_buffer);
        });
        SpotLight light{};
        Vulkan::CopyAllocationToMemory(s_pInstance->m_SpotLightStagingBuffer.Allocation, 0, &light, sizeof(SpotLight));
        return light;
    }

    void Render::initializeLights() {
        m_LightDescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addUniformBuffer(0, vk::ShaderStageFlagBits::eFragment)
                .addStorageBuffer(1, vk::ShaderStageFlagBits::eFragment)
                .addStorageBuffer(2, vk::ShaderStageFlagBits::eFragment)
                .addUniformBuffer(3, vk::ShaderStageFlagBits::eFragment)
                .build();

        m_LightDescriptorSet = Vulkan::AllocateDescriptorSet(m_LightDescriptorLayout, m_DescriptorPool);

        m_DirectionalLightStagingBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(DirectionalLight),
            vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_PointLightStagingBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(PointLight),
            vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_SpotLightStagingBuffer = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(SpotLight),
            vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_DirectionalLightBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(DirectionalLight),
            vk::BufferUsageFlagBits::eUniformBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

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
            vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(LightData),
            vk::BufferUsageFlagBits::eUniformBuffer |
                vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst);

        m_FrameGraphDirectionalLightBuffer = FrameGraph::BufferInfo{
            m_pFrameGraph->importBuffer(m_DirectionalLightBuffer.Handle, m_DirectionalLightBuffer.Usage, 0, m_DirectionalLightBuffer.Size),
            0,
            m_DirectionalLightBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphPointLightBuffer = FrameGraph::BufferInfo{
            m_pFrameGraph->importBuffer(m_PointLightBuffer.Handle, m_PointLightBuffer.Usage, 0, m_PointLightBuffer.Size),
            0,
            m_PointLightBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphSpotLightBuffer = FrameGraph::BufferInfo{
            m_pFrameGraph->importBuffer(m_SpotLightBuffer.Handle, m_SpotLightBuffer.Usage, 0, m_SpotLightBuffer.Size),
            0,
            m_SpotLightBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphLightDataBuffer = FrameGraph::BufferInfo{
            m_pFrameGraph->importBuffer(m_LightDataBuffer.Handle, m_LightDataBuffer.Usage, 0, m_LightDataBuffer.Size),
            0,
            m_LightDataBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_NextPointLightID.ID = 0u;
        m_NextSpotLightID.ID  = 0u;

        m_FreePointLightIDs.clear();
        m_FreeSpotLightIDs.clear();

        Vulkan::DescriptorSetWriter()
            .writeUniformBuffer(0, m_DirectionalLightBuffer.Handle, 0, m_DirectionalLightBuffer.Size)
            .writeStorageBuffer(1, m_PointLightBuffer.Handle, 0, m_PointLightBuffer.Size)
            .writeStorageBuffer(2, m_SpotLightBuffer.Handle, 0, m_SpotLightBuffer.Size)
            .writeUniformBuffer(3, m_LightDataBuffer.Handle, 0, m_LightDataBuffer.Size)
            .update(m_LightDescriptorSet);
    }

    void Render::releaseLights() {
        m_pFrameGraph->removeBuffer(m_FrameGraphDirectionalLightBuffer.Buffer);
        m_pFrameGraph->removeBuffer(m_FrameGraphPointLightBuffer.Buffer);
        m_pFrameGraph->removeBuffer(m_FrameGraphSpotLightBuffer.Buffer);
        m_pFrameGraph->removeBuffer(m_FrameGraphLightDataBuffer.Buffer);

        Vulkan::DestroyDescriptorSetLayout(m_LightDescriptorLayout);

        Vulkan::DestroyBuffer(m_DirectionalLightStagingBuffer);
        Vulkan::DestroyBuffer(m_PointLightStagingBuffer);
        Vulkan::DestroyBuffer(m_SpotLightStagingBuffer);

        Vulkan::DestroyBuffer(m_DirectionalLightBuffer);
        Vulkan::DestroyBuffer(m_PointLightBuffer);
        Vulkan::DestroyBuffer(m_SpotLightBuffer);
        Vulkan::DestroyBuffer(m_LightDataBuffer);

        m_FrameGraphDirectionalLightBuffer.Buffer = FrameGraph::k_InvalidBufferID;
        m_FrameGraphPointLightBuffer.Buffer       = FrameGraph::k_InvalidBufferID;
        m_FrameGraphSpotLightBuffer.Buffer        = FrameGraph::k_InvalidBufferID;
        m_FrameGraphLightDataBuffer.Buffer        = FrameGraph::k_InvalidBufferID;
    }

    void Render::readLightBuffers(FrameGraph::RenderPass &render_pass) {
        render_pass
            .readBuffers({
                m_FrameGraphDirectionalLightBuffer,
                m_FrameGraphPointLightBuffer,
                m_FrameGraphSpotLightBuffer,
                m_FrameGraphLightDataBuffer,
            });
    }
}  // namespace Ignis