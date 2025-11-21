#pragma once

#include <Ignis/Core.hpp>

#define IGNIS_VK_CHECK(result)                                                                    \
    do {                                                                                          \
        vk::Result ignis_vk_result = static_cast<vk::Result>(result);                             \
        IGNIS_ASSERT(vk::Result::eSuccess == ignis_vk_result, "Ignis::Vulkan vk::Result failed"); \
    } while (false);

#if defined(IGNIS_BUILD_TYPE_DEBUG)
    #define DIGNIS_VK_CHECK(result) IGNIS_VK_CHECK(result)
#else
    #define DIGNIS_VK_CHECK(result) \
        do {                        \
            (void)result;           \
        } while (false);
#endif

namespace Ignis::Vulkan {
    namespace Image {
        struct Allocation {
            vk::Image       Image;
            vk::Extent3D    Extent;
            vk::Format      Format;
            vma::Allocation VmaAllocation;

            vk::ImageUsageFlags UsageFlags;
        };
    }  // namespace Image

    namespace Buffer {
        struct Allocation {
            vk::Buffer      Buffer;
            uint64_t        Size;
            vma::Allocation VmaAllocation;

            vk::BufferUsageFlags UsageFlags;
        };
    }  // namespace Buffer
}  // namespace Ignis::Vulkan
