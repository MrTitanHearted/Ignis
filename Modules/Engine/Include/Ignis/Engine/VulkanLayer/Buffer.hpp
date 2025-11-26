#pragma once

#include <Ignis/Engine/VulkanLayer/Types.hpp>

namespace Ignis::Vulkan::Buffer {
    void Destroy(const Allocation &allocation, vma::Allocator allocator);

    Allocation Allocate(
        vma::AllocationCreateFlags allocation_flags,
        vma::MemoryUsage           memory_usage,
        vk::BufferCreateFlags      buffer_flags,
        uint64_t                   size,
        vk::BufferUsageFlags       usage_flags,
        vma::Allocator             allocator);
    Allocation Allocate(
        vma::AllocationCreateFlags allocation_flags,
        vma::MemoryUsage           memory_usage,
        uint64_t                   size,
        vk::BufferUsageFlags       usage_flags,
        vma::Allocator             allocator);
    Allocation Allocate(
        vma::MemoryUsage     memory_usage,
        uint64_t             size,
        vk::BufferUsageFlags usage_flags,
        vma::Allocator       allocator);
}  // namespace Ignis::Vulkan::Buffer