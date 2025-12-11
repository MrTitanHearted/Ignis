#include <Ignis/Vulkan.hpp>

namespace Ignis {
    void Vulkan::DestroyBuffer(const Buffer &buffer) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        s_pInstance->m_VmaAllocator.destroyBuffer(buffer.Handle, buffer.Allocation);
    }

    Vulkan::Buffer Vulkan::AllocateBuffer(
        const vma::AllocationCreateFlags allocation_flags,
        const vma::MemoryUsage           memory_usage,
        const vk::BufferCreateFlags      buffer_flags,
        const uint64_t                   size,
        const vk::BufferUsageFlags       usage_flags) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::BufferCreateInfo buffer_create_info{};
        buffer_create_info
            .setFlags(buffer_flags)
            .setSize(size)
            .setUsage(usage_flags);
        vma::AllocationCreateInfo create_info{};
        create_info
            .setFlags(allocation_flags)
            .setUsage(memory_usage);

        auto [result, buffer_allocation] =
            s_pInstance->m_VmaAllocator.createBuffer(buffer_create_info, create_info);
        DIGNIS_VK_CHECK(result);
        auto [handle, allocation] = buffer_allocation;

        Buffer buffer{};
        buffer.Handle     = handle;
        buffer.Size       = size;
        buffer.Allocation = allocation;
        buffer.Usage      = usage_flags;

        buffer.CreateFlags = buffer_flags;
        buffer.MemoryUsage = memory_usage;

        buffer.AllocationFlags = allocation_flags;

        return buffer;
    }
}  // namespace Ignis