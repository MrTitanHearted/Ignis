#include <Ignis/Vulkan/Memory.hpp>

namespace Ignis::Vulkan::Memory {
    void Destroy(
        const vma::Allocation allocation,
        const vma::Allocator  allocator) {
        allocator.freeMemory(allocation);
    }

    vma::Allocation AllocateForBuffer(
        const vma::AllocationCreateFlags allocation_flags,
        const vma::MemoryUsage           memory_usage,
        const vk::Buffer                 buffer,
        const vma::Allocator             allocator) {
        vma::AllocationCreateInfo vma_allocation_create_info{};
        vma_allocation_create_info
            .setFlags(allocation_flags)
            .setUsage(memory_usage);

        auto [result, allocation] = allocator.allocateMemoryForBuffer(
            buffer,
            vma_allocation_create_info);

        DIGNIS_VK_CHECK(result);

        return allocation;
    }

    vma::Allocation AllocateForImage(
        const vma::AllocationCreateFlags allocation_flags,
        const vma::MemoryUsage           memory_usage,
        const vk::Image                  image,
        const vma::Allocator             allocator) {
        vma::AllocationCreateInfo vma_allocation_create_info{};
        vma_allocation_create_info
            .setFlags(allocation_flags)
            .setUsage(memory_usage);

        auto [result, allocation] = allocator.allocateMemoryForImage(
            image,
            vma_allocation_create_info);

        DIGNIS_VK_CHECK(result);

        return allocation;
    }

}  // namespace Ignis::Vulkan::Memory