#include <Ignis/Vulkan.hpp>

namespace Ignis {
    void Vulkan::DestroyImage(const Image &image) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        s_pInstance->m_VmaAllocator.destroyImage(image.Handle, image.Allocation);
    }

    void Vulkan::DestroyImageView(const vk::ImageView view) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        s_pInstance->m_Device.destroyImageView(view);
    }

    Vulkan::Image Vulkan::AllocateImage3D(
        const vma::AllocationCreateFlags allocation_flags,
        const vma::MemoryUsage           memory_usage,
        const vk::ImageCreateFlagBits    image_flags,
        const vk::Format                 format,
        const vk::ImageUsageFlags        usage_flags,
        const vk::Extent3D              &extent) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageCreateInfo image_create_info{};
        image_create_info
            .setFlags(image_flags)
            .setImageType(vk::ImageType::e3D)
            .setFormat(format)
            .setExtent(extent)
            .setArrayLayers(1)
            .setMipLevels(1)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(usage_flags);
        vma::AllocationCreateInfo create_info{};
        create_info
            .setFlags(allocation_flags)
            .setUsage(memory_usage);

        const auto [result, image_allocation] =
            s_pInstance->m_VmaAllocator.createImage(image_create_info, create_info);
        DIGNIS_VK_CHECK(result);
        const auto [handle, allocation] = image_allocation;

        Image image{};
        image.Handle     = handle;
        image.Format     = format;
        image.Extent     = extent;
        image.Allocation = allocation;
        image.UsageFlags = usage_flags;

        return image;
    }

    Vulkan::Image Vulkan::AllocateImage2D(
        const vma::AllocationCreateFlags allocation_flags,
        const vma::MemoryUsage           memory_usage,
        const vk::ImageCreateFlagBits    image_flags,
        const vk::Format                 format,
        const vk::ImageUsageFlags        usage_flags,
        const vk::Extent2D              &extent) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageCreateInfo image_create_info{};
        image_create_info
            .setFlags(image_flags)
            .setImageType(vk::ImageType::e2D)
            .setFormat(format)
            .setExtent(vk::Extent3D{extent, 1})
            .setArrayLayers(1)
            .setMipLevels(1)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(usage_flags);
        vma::AllocationCreateInfo create_info{};
        create_info
            .setFlags(allocation_flags)
            .setUsage(memory_usage);

        const auto [result, image_allocation] =
            s_pInstance->m_VmaAllocator.createImage(image_create_info, create_info);
        DIGNIS_VK_CHECK(result);
        const auto [handle, allocation] = image_allocation;

        Image image{};
        image.Handle     = handle;
        image.Format     = format;
        image.Extent     = vk::Extent3D{extent, 1};
        image.Allocation = allocation;
        image.UsageFlags = usage_flags;

        return image;
    }

    vk::ImageView Vulkan::CreateImageColorView3D(const vk::Image image, const vk::Format format) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e3D)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setBaseMipLevel(0)
                    .setLayerCount(1)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageDepthView3D(const vk::Image image, const vk::Format format) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e3D)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                    .setBaseArrayLayer(0)
                    .setBaseMipLevel(0)
                    .setLayerCount(1)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageColorView2D(const vk::Image image, const vk::Format format) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setBaseMipLevel(0)
                    .setLayerCount(1)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageDepthView2D(const vk::Image image, const vk::Format format) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                    .setBaseArrayLayer(0)
                    .setBaseMipLevel(0)
                    .setLayerCount(1)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageAspectFlags Vulkan::GetImageAspectMask(const vk::ImageLayout layout) {
        switch (layout) {
            case vk::ImageLayout::eUndefined:
                return vk::ImageAspectFlagBits::eNone;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
            case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
            case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
                return vk::ImageAspectFlagBits::eDepth |
                       vk::ImageAspectFlagBits::eStencil;
            case vk::ImageLayout::eDepthAttachmentOptimal:
            case vk::ImageLayout::eDepthReadOnlyOptimal:
                return vk::ImageAspectFlagBits::eDepth;
            case vk::ImageLayout::eStencilAttachmentOptimal:
            case vk::ImageLayout::eStencilReadOnlyOptimal:
                return vk::ImageAspectFlagBits::eStencil;
            case vk::ImageLayout::eColorAttachmentOptimal:
            default:
                return vk::ImageAspectFlagBits::eColor;
        }
    }
}  // namespace Ignis
