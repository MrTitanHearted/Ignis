#include <Ignis/Vulkan/Buffer.hpp>

namespace Ignis::Vulkan::Buffer {
    void Destroy(
        const Allocation    &allocation,
        const vma::Allocator allocator) {
        allocator.destroyBuffer(allocation.Buffer, allocation.VmaAllocation);
    }

    Allocation Allocate(
        const vma::AllocationCreateFlags allocation_flags,
        const vma::MemoryUsage           memory_usage,
        const vk::BufferCreateFlags      buffer_flags,
        const uint64_t                   size,
        const vk::BufferUsageFlags       usage_flags,
        const vma::Allocator             allocator) {
        vk::BufferCreateInfo vk_buffer_create_info{};
        vk_buffer_create_info
            .setFlags(buffer_flags)
            .setSize(size)
            .setUsage(usage_flags);
        vma::AllocationCreateInfo vma_allocation_create_info{};
        vma_allocation_create_info
            .setFlags(allocation_flags)
            .setUsage(memory_usage);

        auto [result, buffer_allocation] = allocator.createBuffer(vk_buffer_create_info, vma_allocation_create_info);
        DIGNIS_VK_CHECK(result);
        auto [vk_buffer, vma_allocation] = buffer_allocation;

        Allocation allocation{};
        allocation.Buffer        = vk_buffer;
        allocation.Size          = size;
        allocation.VmaAllocation = vma_allocation;
        allocation.UsageFlags    = usage_flags;

        return allocation;
    }

    Allocation Allocate(
        const vma::AllocationCreateFlags allocation_flags,
        const vma::MemoryUsage           memory_usage,
        const uint64_t                   size,
        const vk::BufferUsageFlags       usage_flags,
        const vma::Allocator             allocator) {
        return Allocate(allocation_flags, memory_usage, {}, size, usage_flags, allocator);
    }

    Allocation Allocate(
        const vma::MemoryUsage     memory_usage,
        const uint64_t             size,
        const vk::BufferUsageFlags usage_flags,
        const vma::Allocator       allocator) {
        return Allocate({}, memory_usage, size, usage_flags, allocator);
    }
}  // namespace Ignis::Vulkan::Buffer