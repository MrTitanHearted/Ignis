#include <Ignis/RenderModule/PBR.hpp>

namespace Ignis {
    void PBR::setDirectionalLight(const DirectionalLight &light) const {
        Vulkan::CopyMemoryToAllocation(&light, m_DirectionalLightStagingBuffer.Allocation, 0, sizeof(DirectionalLight));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_DirectionalLightStagingBuffer.Handle,
                m_DirectionalLightBuffer.Handle,
                0, 0,
                sizeof(DirectionalLight),
                command_buffer);
        });
    }

    void PBR::setPointLight(const PointLightID id, const PointLight &light) const {
        DIGNIS_ASSERT(m_PointLights.contains(id));
        Vulkan::CopyMemoryToAllocation(&light, m_PointLightStagingBuffer.Allocation, 0, sizeof(PointLight));

        const uint32_t &index = m_PointLightToIndex.at(id);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_PointLightStagingBuffer.Handle,
                m_PointLightBuffer.Handle, 0,
                sizeof(PointLight) * index,
                sizeof(PointLight),
                command_buffer);
        });
    }

    void PBR::setSpotLight(const SpotLightID id, const SpotLight &light) const {
        DIGNIS_ASSERT(m_SpotLights.contains(id));
        Vulkan::CopyMemoryToAllocation(&light, m_SpotLightStagingBuffer.Allocation, 0, sizeof(SpotLight));

        const uint32_t &index = m_SpotLightToIndex.at(id);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_SpotLightStagingBuffer.Handle,
                m_SpotLightBuffer.Handle, 0,
                sizeof(SpotLight) * index,
                sizeof(SpotLight),
                command_buffer);
        });
    }

    PBR::PointLightID PBR::addPointLight(const PointLight &light) {
        PointLightID id;
        if (m_FreePointLightIDs.empty()) {
            id = m_NextPointLightID;
            m_NextPointLightID.ID++;
        } else {
            id = m_FreePointLightIDs.back();
            m_FreePointLightIDs.pop_back();
        }

        uint32_t index;

        {
            const vma::AllocationInfo light_data_allocation_info = Vulkan::GetAllocationInfo(m_LightDataBuffer);

            auto *light_data = static_cast<LightData *>(light_data_allocation_info.pMappedData);

            index = light_data->PointLightCount++;

            Vulkan::FlushAllocation(m_LightDataBuffer.Allocation, offsetof(LightData, PointLightCount), sizeof(uint32_t));
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
                m_FrameGraph.importBuffer(m_PointLightBuffer.Handle, m_PointLightBuffer.Usage, 0, m_PointLightBuffer.Size),
                0,
                m_PointLightBuffer.Size,
                vk::PipelineStageFlagBits2::eFragmentShader,
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

            Vulkan::DescriptorSetWriter()
                .writeStorageBuffer(2, m_PointLightBuffer.Handle, 0, m_PointLightBuffer.Size)
                .update(m_LightDescriptorSet);
        }

        Vulkan::CopyMemoryToAllocation(&light, m_PointLightStagingBuffer.Allocation, 0, sizeof(PointLight));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_PointLightStagingBuffer.Handle,
                m_PointLightBuffer.Handle, 0,
                sizeof(PointLight) * index,
                sizeof(PointLight),
                command_buffer);
        });

        m_PointLights.emplace(id);
        m_PointLightToIndex.emplace(id, index);
        m_IndexToPointLight.emplace(index, id);

        return id;
    }

    PBR::SpotLightID PBR::addSpotLight(const SpotLight &light) {
        SpotLightID id;
        if (m_FreeSpotLightIDs.empty()) {
            id = m_NextSpotLightID;
            m_NextSpotLightID.ID++;
        } else {
            id = m_FreeSpotLightIDs.back();
            m_FreeSpotLightIDs.pop_back();
        }

        uint32_t index;

        {
            const vma::AllocationInfo light_data_allocation_info = Vulkan::GetAllocationInfo(m_LightDataBuffer);

            auto *light_data = static_cast<LightData *>(light_data_allocation_info.pMappedData);

            index = light_data->SpotLightCount++;

            Vulkan::FlushAllocation(m_LightDataBuffer.Allocation, offsetof(LightData, SpotLightCount), sizeof(uint32_t));
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
                m_FrameGraph.importBuffer(m_SpotLightBuffer.Handle, m_SpotLightBuffer.Usage, 0, m_SpotLightBuffer.Size),
                0,
                m_SpotLightBuffer.Size,
                vk::PipelineStageFlagBits2::eFragmentShader,
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

            Vulkan::DescriptorSetWriter()
                .writeStorageBuffer(2, m_SpotLightBuffer.Handle, 0, m_SpotLightBuffer.Size)
                .update(m_LightDescriptorSet);
        }

        Vulkan::CopyMemoryToAllocation(&light, m_SpotLightStagingBuffer.Allocation, 0, sizeof(SpotLight));

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_SpotLightStagingBuffer.Handle,
                m_SpotLightBuffer.Handle, 0,
                sizeof(SpotLight) * index,
                sizeof(SpotLight),
                command_buffer);
        });

        m_SpotLights.emplace(id);
        m_SpotLightToIndex.emplace(id, index);
        m_IndexToSpotLight.emplace(index, id);

        return id;
    }

    void PBR::removePointLight(const PointLightID id) {
        DIGNIS_ASSERT(m_PointLights.contains(id));

        uint32_t last_index;

        {
            const vma::AllocationInfo light_data_allocation_info = Vulkan::GetAllocationInfo(m_LightDataBuffer);

            auto *light_data = static_cast<LightData *>(light_data_allocation_info.pMappedData);

            last_index = --light_data->PointLightCount;

            Vulkan::FlushAllocation(m_LightDataBuffer.Allocation, offsetof(LightData, PointLightCount), sizeof(uint32_t));
        }

        if (const uint32_t &index = m_PointLightToIndex.at(id);
            last_index != index) {
            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    m_PointLightBuffer.Handle,
                    m_PointLightBuffer.Handle,
                    sizeof(PointLight) * last_index,
                    sizeof(PointLight) * index,
                    sizeof(PointLight),
                    command_buffer);
            });

            const PointLightID last_id = m_IndexToPointLight.at(last_index);

            m_PointLightToIndex[last_id] = index;
            m_IndexToPointLight[index]   = last_id;
        }

        m_PointLights.erase(id);
        m_PointLightToIndex.erase(id);
        m_IndexToPointLight.erase(last_index);

        m_FreePointLightIDs.push_back(id);
    }

    void PBR::removeSpotLight(const SpotLightID id) {
        DIGNIS_ASSERT(m_SpotLights.contains(id));

        uint32_t last_index;

        {
            const vma::AllocationInfo light_data_allocation_info = Vulkan::GetAllocationInfo(m_LightDataBuffer);

            auto *light_data = static_cast<LightData *>(light_data_allocation_info.pMappedData);

            last_index = --light_data->SpotLightCount;

            Vulkan::FlushAllocation(m_LightDataBuffer.Allocation, offsetof(LightData, SpotLightCount), sizeof(uint32_t));
        }

        if (const uint32_t &index = m_SpotLightToIndex.at(id);
            last_index != index) {
            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    m_SpotLightBuffer.Handle,
                    m_SpotLightBuffer.Handle,
                    sizeof(SpotLight) * last_index,
                    sizeof(SpotLight) * index,
                    sizeof(SpotLight),
                    command_buffer);
            });

            const SpotLightID last_id = m_IndexToSpotLight.at(last_index);

            m_SpotLightToIndex[last_id] = index;
            m_IndexToSpotLight[index]   = last_id;
        }

        m_SpotLights.erase(id);
        m_SpotLightToIndex.erase(id);
        m_IndexToSpotLight.erase(last_index);

        m_FreeSpotLightIDs.push_back(id);
    }

    PBR::DirectionalLight PBR::getDirectionalLight() const {
        Vulkan::ImmediateSubmit([this](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_DirectionalLightBuffer.Handle,
                m_DirectionalLightStagingBuffer.Handle,
                0, 0,
                sizeof(DirectionalLight),
                command_buffer);
        });
        DirectionalLight light{};
        Vulkan::CopyAllocationToMemory(m_DirectionalLightStagingBuffer.Allocation, 0, &light, sizeof(DirectionalLight));
        return light;
    }

    PBR::PointLight PBR::getPointLight(const PointLightID id) const {
        DIGNIS_ASSERT(m_PointLights.contains(id));

        const uint32_t index = m_PointLightToIndex.at(id);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_PointLightBuffer.Handle,
                m_PointLightStagingBuffer.Handle,
                sizeof(PointLight) * index, 0,
                sizeof(PointLight),
                command_buffer);
        });
        PointLight light{};
        Vulkan::CopyAllocationToMemory(m_PointLightStagingBuffer.Allocation, 0, &light, sizeof(PointLight));
        return light;
    }

    PBR::SpotLight PBR::getSpotLight(const SpotLightID id) const {
        DIGNIS_ASSERT(m_SpotLights.contains(id));

        const uint32_t index = m_SpotLightToIndex.at(id);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::CopyBufferToBuffer(
                m_SpotLightBuffer.Handle,
                m_SpotLightStagingBuffer.Handle,
                sizeof(SpotLight) * index, 0,
                sizeof(SpotLight),
                command_buffer);
        });
        SpotLight light{};
        Vulkan::CopyAllocationToMemory(m_SpotLightStagingBuffer.Allocation, 0, &light, sizeof(SpotLight));
        return light;
    }

    void PBR::initLights() {
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
            m_FrameGraph.importBuffer(m_DirectionalLightBuffer.Handle, m_DirectionalLightBuffer.Usage, 0, m_DirectionalLightBuffer.Size),
            0,
            m_DirectionalLightBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphPointLightBuffer = FrameGraph::BufferInfo{
            m_FrameGraph.importBuffer(m_PointLightBuffer.Handle, m_PointLightBuffer.Usage, 0, m_PointLightBuffer.Size),
            0,
            m_PointLightBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphSpotLightBuffer = FrameGraph::BufferInfo{
            m_FrameGraph.importBuffer(m_SpotLightBuffer.Handle, m_SpotLightBuffer.Usage, 0, m_SpotLightBuffer.Size),
            0,
            m_SpotLightBuffer.Size,
            vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_FrameGraphLightDataBuffer = FrameGraph::BufferInfo{
            m_FrameGraph.importBuffer(m_LightDataBuffer.Handle, m_LightDataBuffer.Usage, 0, m_LightDataBuffer.Size),
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

    void PBR::releaseLights() {
        m_FrameGraph.removeBuffer(m_FrameGraphDirectionalLightBuffer.Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphPointLightBuffer.Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphSpotLightBuffer.Buffer);
        m_FrameGraph.removeBuffer(m_FrameGraphLightDataBuffer.Buffer);

        Vulkan::DestroyDescriptorSetLayout(m_LightDescriptorLayout);

        Vulkan::DestroyBuffer(m_DirectionalLightStagingBuffer);
        Vulkan::DestroyBuffer(m_PointLightStagingBuffer);
        Vulkan::DestroyBuffer(m_SpotLightStagingBuffer);

        Vulkan::DestroyBuffer(m_DirectionalLightBuffer);
        Vulkan::DestroyBuffer(m_PointLightBuffer);
        Vulkan::DestroyBuffer(m_SpotLightBuffer);
        Vulkan::DestroyBuffer(m_LightDataBuffer);

        m_FrameGraphDirectionalLightBuffer.Buffer = FrameGraph::k_InvalidBufferID;

        m_FrameGraphPointLightBuffer.Buffer = FrameGraph::k_InvalidBufferID;
        m_FrameGraphSpotLightBuffer.Buffer  = FrameGraph::k_InvalidBufferID;
        m_FrameGraphLightDataBuffer.Buffer  = FrameGraph::k_InvalidBufferID;
    }

    void PBR::readLightBuffers(FrameGraph::RenderPass &render_pass) {
        render_pass
            .readBuffers({
                m_FrameGraphDirectionalLightBuffer,
                m_FrameGraphPointLightBuffer,
                m_FrameGraphSpotLightBuffer,
                m_FrameGraphLightDataBuffer,
            });
    }
}  // namespace Ignis