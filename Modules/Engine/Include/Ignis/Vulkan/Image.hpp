#pragma once

#include <Ignis/Vulkan/Types.hpp>

namespace Ignis::Vulkan {
    namespace Image {
        vk::ImageAspectFlags GetAspectMask(vk::ImageLayout layout);

        void TransitionLayout(
            vk::Image         image,
            vk::ImageLayout   old_layout,
            vk::ImageLayout   new_layout,
            vk::CommandBuffer command_buffer);

        void TransitionLayout(
            vk::Image         image,
            vk::ImageLayout   new_layout,
            vk::CommandBuffer command_buffer);

        void Destroy(const Allocation &allocation, vma::Allocator allocator);

        Allocation Allocate3D(
            vma::AllocationCreateFlags allocation_flags,
            vma::MemoryUsage           memory_usage,
            vk::Format                 format,
            vk::ImageUsageFlags        usage_flags,
            const vk::Extent3D        &extent,
            vma::Allocator             allocator);
        Allocation Allocate3D(
            vma::MemoryUsage    memory_usage,
            vk::Format          format,
            vk::ImageUsageFlags usage_flags,
            const vk::Extent3D &extent,
            vma::Allocator      allocator);

        Allocation Allocate2D(
            vma::AllocationCreateFlags allocation_flags,
            vma::MemoryUsage           memory_usage,
            vk::Format                 format,
            vk::ImageUsageFlags        usage_flags,
            const vk::Extent2D        &extent,
            vma::Allocator             allocator);
        Allocation Allocate2D(
            vma::MemoryUsage    memory_usage,
            vk::Format          format,
            vk::ImageUsageFlags usage_flags,
            const vk::Extent2D &extent,
            vma::Allocator      allocator);
    }  // namespace Image

    namespace ImageView {
        vk::ImageView CreateColor3D(vk::Format format, vk::Image image, vk::Device device);
        vk::ImageView CreateDepth3D(vk::Format format, vk::Image image, vk::Device device);

        vk::ImageView CreateColor2D(vk::Format format, vk::Image image, vk::Device device);
        vk::ImageView CreateDepth2D(vk::Format format, vk::Image image, vk::Device device);
    }  // namespace ImageView
}  // namespace Ignis::Vulkan