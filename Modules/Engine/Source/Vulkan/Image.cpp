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

    Vulkan::Image Vulkan::AllocateImageCube(
        const vma::AllocationCreateFlags allocation_flags,
        const vma::MemoryUsage           memory_usage,
        const vk::ImageCreateFlagBits    image_flags,
        const vk::Format                 format,
        const vk::ImageUsageFlags        usage_flags,
        const uint32_t                   mip_level_count,
        const vk::Extent2D              &extent) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageCreateInfo image_create_info{};
        image_create_info
            .setFlags(image_flags | vk::ImageCreateFlagBits::eCubeCompatible)
            .setImageType(vk::ImageType::e2D)
            .setFormat(format)
            .setExtent(vk::Extent3D{extent, 1})
            .setArrayLayers(6)
            .setMipLevels(mip_level_count)
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
        image.Handle        = handle;
        image.Format        = format;
        image.Extent        = vk::Extent3D{extent, 1};
        image.Allocation    = allocation;
        image.Usage         = usage_flags;
        image.MipLevelCount = mip_level_count;

        image.CreateFlags = image_flags;
        image.MemoryUsage = memory_usage;

        image.AllocationFlags = allocation_flags;

        return image;
    }

    Vulkan::Image Vulkan::AllocateImageCube(
        const vma::AllocationCreateFlags allocation_flags,
        const vma::MemoryUsage           memory_usage,
        const vk::ImageCreateFlagBits    image_flags,
        const vk::Format                 format,
        const vk::ImageUsageFlags        usage_flags,
        const vk::Extent2D              &extent) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageCreateInfo image_create_info{};
        image_create_info
            .setFlags(image_flags | vk::ImageCreateFlagBits::eCubeCompatible)
            .setImageType(vk::ImageType::e2D)
            .setFormat(format)
            .setExtent(vk::Extent3D{extent, 1})
            .setArrayLayers(6)
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
        image.Handle        = handle;
        image.Format        = format;
        image.Extent        = vk::Extent3D{extent, 1};
        image.Allocation    = allocation;
        image.Usage         = usage_flags;
        image.MipLevelCount = 1;

        image.CreateFlags = image_flags;
        image.MemoryUsage = memory_usage;

        image.AllocationFlags = allocation_flags;

        return image;
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
        image.Handle        = handle;
        image.Format        = format;
        image.Extent        = extent;
        image.Allocation    = allocation;
        image.Usage         = usage_flags;
        image.MipLevelCount = 1;

        image.CreateFlags = image_flags;
        image.MemoryUsage = memory_usage;

        image.AllocationFlags = allocation_flags;

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
        image.Handle        = handle;
        image.Format        = format;
        image.Extent        = vk::Extent3D{extent, 1};
        image.Allocation    = allocation;
        image.Usage         = usage_flags;
        image.MipLevelCount = 1;

        image.CreateFlags = image_flags;
        image.MemoryUsage = memory_usage;

        image.AllocationFlags = allocation_flags;

        return image;
    }

    vk::ImageView Vulkan::CreateImageColorViewCube(const vk::Image image, const vk::Format format, const uint32_t base_layer) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::eCube)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(base_layer)
                    .setBaseMipLevel(0)
                    .setLayerCount(6)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageDepthViewCube(const vk::Image image, const vk::Format format, const uint32_t base_layer) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::eCube)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                    .setBaseArrayLayer(base_layer)
                    .setBaseMipLevel(0)
                    .setLayerCount(6)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageColorView2DArray(
        const vk::Image  image,
        const vk::Format format,
        const uint32_t   base_mip_level,
        const uint32_t   mip_level_count,
        const uint32_t   base_layer,
        const uint32_t   layer_count) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e2DArray)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(base_layer)
                    .setBaseMipLevel(base_mip_level)
                    .setLayerCount(layer_count)
                    .setLevelCount(mip_level_count));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageDepthView2DArray(
        const vk::Image  image,
        const vk::Format format,
        const uint32_t   base_mip_level,
        const uint32_t   mip_level_count,
        const uint32_t   base_layer,
        const uint32_t   layer_count) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e2DArray)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                    .setBaseArrayLayer(base_layer)
                    .setBaseMipLevel(base_mip_level)
                    .setLayerCount(layer_count)
                    .setLevelCount(mip_level_count));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageColorView2DArray(
        const vk::Image  image,
        const vk::Format format,
        const uint32_t   base_layer,
        const uint32_t   layer_count) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e2DArray)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(base_layer)
                    .setBaseMipLevel(0)
                    .setLayerCount(layer_count)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageDepthView2DArray(
        const vk::Image  image,
        const vk::Format format,
        const uint32_t   base_layer,
        const uint32_t   layer_count) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e2DArray)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                    .setBaseArrayLayer(base_layer)
                    .setBaseMipLevel(0)
                    .setLayerCount(layer_count)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageColorView2D(
        const vk::Image  image,
        const vk::Format format,
        const uint32_t   base_layer,
        const uint32_t   layer_count) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(base_layer)
                    .setBaseMipLevel(0)
                    .setLayerCount(layer_count)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageDepthView2D(
        const vk::Image  image,
        const vk::Format format,
        const uint32_t   base_layer,
        const uint32_t   layer_count) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                    .setBaseArrayLayer(base_layer)
                    .setBaseMipLevel(0)
                    .setLayerCount(layer_count)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageColorViewCube(
        const vk::Image  image,
        const vk::Format format,
        const uint32_t   base_mip_level,
        const uint32_t   mip_level_count) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::eCube)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setBaseMipLevel(base_mip_level)
                    .setLayerCount(6)
                    .setLevelCount(mip_level_count));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageDepthViewCube(
        const vk::Image  image,
        const vk::Format format,
        const uint32_t   base_mip_level,
        const uint32_t   mip_level_count) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::eCube)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                    .setBaseArrayLayer(0)
                    .setBaseMipLevel(base_mip_level)
                    .setLayerCount(6)
                    .setLevelCount(mip_level_count));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageColorViewCube(const vk::Image image, const vk::Format format) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::eCube)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setBaseMipLevel(0)
                    .setLayerCount(6)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
    }

    vk::ImageView Vulkan::CreateImageDepthViewCube(const vk::Image image, const vk::Format format) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ImageViewCreateInfo create_info{};
        create_info
            .setViewType(vk::ImageViewType::eCube)
            .setFormat(format)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                    .setBaseArrayLayer(0)
                    .setBaseMipLevel(0)
                    .setLayerCount(6)
                    .setLevelCount(1));
        auto [result, view] = s_pInstance->m_Device.createImageView(create_info);
        DIGNIS_VK_CHECK(result);
        return view;
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
