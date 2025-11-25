#pragma once

#include <Ignis/Vulkan/Types.hpp>

namespace Ignis::Vulkan::Memory {
    void Destroy(vma::Allocation allocation, vma::Allocator allocator);

    vma::Allocation AllocateForBuffer(
        vma::AllocationCreateFlags allocation_flags,
        vma::MemoryUsage           memory_usage,
        vk::Buffer                 buffer,
        vma::Allocator             allocator);

    vma::Allocation AllocateForImage(
        vma::AllocationCreateFlags allocation_flags,
        vma::MemoryUsage           memory_usage,
        vk::Image                  image,
        vma::Allocator             allocator);

}  // namespace Ignis::Vulkan::Memory