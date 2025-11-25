#include <Ignis/Vulkan/Context.hpp>

VkResult ignisVkCreateDebugUtilsMessengerEXT(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
    VkDebugUtilsMessengerEXT                 *p_debug_utils_messenger) {
    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func == nullptr)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    return func(instance, p_create_info, nullptr, p_debug_utils_messenger);
}

VkResult ignisVkDestroyDebugUtilsMessengerEXT(
    VkInstance               instance,
    VkDebugUtilsMessengerEXT debug_utils_messenger) {
    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func == nullptr)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    func(instance, debug_utils_messenger, nullptr);
    return VK_SUCCESS;
}

namespace Ignis::Vulkan {
    Context *Context::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Context::State Context::s_State{};)

    void Context::Initialize(Context *context, const Settings &settings) {
        DIGNIS_ASSERT(s_pInstance == nullptr, "Ignis::Vulkan::Context is already initialized");

        s_pInstance = context;

        uint32_t     glfw_required_extensions_count = 0;
        const char **glfw_required_extensions       = glfwGetRequiredInstanceExtensions(&glfw_required_extensions_count);

        std::vector<const char *> instance_extensions{
            glfw_required_extensions,
            glfw_required_extensions + glfw_required_extensions_count,
        };
        std::vector<const char *> instance_layers{};

#if defined(IGNIS_BUILD_TYPE_DEBUG)
        instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instance_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

        const std::vector<const char *> device_extensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        IGNIS_ASSERT(CheckInstanceLayerSupport(instance_layers),
                     "This Vulkan Instance does not support required layers");

        vk::ApplicationInfo vk_application_info{};
        vk_application_info
            .setPApplicationName("Ignis::Vulkan")
            .setPEngineName("Ignis::Engine")
            .setApiVersion(VK_API_VERSION_1_4)
            .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));

        vk::InstanceCreateInfo vk_instance_create_info{};
        vk_instance_create_info
            .setPApplicationInfo(&vk_application_info)
            .setPEnabledExtensionNames(instance_extensions)
            .setPEnabledLayerNames(instance_layers);

#if defined(IGNIS_BUILD_TYPE_DEBUG)
        VkDebugUtilsMessengerCreateInfoEXT vk_debug_messenger_create_info{};
        vk_debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        vk_debug_messenger_create_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        vk_debug_messenger_create_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        vk_debug_messenger_create_info.pfnUserCallback = DebugUtilsMessengerCallback;
        vk_debug_messenger_create_info.pUserData       = s_pInstance;

        vk_instance_create_info.setPNext(&vk_debug_messenger_create_info);
#endif

        {
            auto [result, vk_instance] = vk::createInstance(vk_instance_create_info);
            IGNIS_VK_CHECK(result);
            s_pInstance->m_Instance = vk_instance;
        }

#if defined(IGNIS_BUILD_TYPE_DEBUG)
        VkDebugUtilsMessengerEXT vk_debug_utils_messenger = VK_NULL_HANDLE;
        IGNIS_VK_CHECK(
            ignisVkCreateDebugUtilsMessengerEXT(
                s_pInstance->m_Instance,
                &vk_debug_messenger_create_info,
                &vk_debug_utils_messenger));

        s_pInstance->m_DebugMessenger = vk_debug_utils_messenger;
#endif

        VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
        IGNIS_VK_CHECK(glfwCreateWindowSurface(s_pInstance->m_Instance, Window::Get(), nullptr, &vk_surface));
        s_pInstance->m_Surface = vk_surface;

        SelectPhysicalDevice(device_extensions, settings.SurfaceUsageFlags);
        SelectQueueFamilyIndex();
        SelectSwapchainImageCount(settings.PreferredImageCount);
        SelectSwapchainFormat(settings.PreferredSurfaceFormats);
        SelectSwapchainPresentMode(settings.PreferredPresentModes);

        s_pInstance->m_SwapchainUsageFlags =
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst |
            settings.SurfaceUsageFlags;

        vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT dynamic_state2_features{};
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT  dynamic_state_features{};

        vk::PhysicalDeviceVulkan13Features vulkan13_features{};
        vk::PhysicalDeviceVulkan12Features vulkan12_features{};
        vk::PhysicalDeviceVulkan11Features vulkan11_features{};
        vk::PhysicalDeviceFeatures2        features{};

        dynamic_state2_features
            .setExtendedDynamicState2(vk::True);
        dynamic_state_features
            .setExtendedDynamicState(vk::True)
            .setPNext(&dynamic_state2_features);
        vulkan13_features
            .setDynamicRendering(vk::True)
            .setSynchronization2(vk::True)
            .setPNext(&dynamic_state_features);
        vulkan12_features
            .setBufferDeviceAddress(vk::True)
            .setDescriptorIndexing(vk::True)
            .setDescriptorBindingPartiallyBound(vk::True)
            .setDescriptorBindingSampledImageUpdateAfterBind(vk::True)
            .setDescriptorBindingStorageBufferUpdateAfterBind(vk::True)
            .setDescriptorBindingStorageImageUpdateAfterBind(vk::True)
            .setDescriptorBindingStorageTexelBufferUpdateAfterBind(vk::True)
            .setDescriptorBindingUniformBufferUpdateAfterBind(vk::True)
            .setDescriptorBindingUniformTexelBufferUpdateAfterBind(vk::True)
            .setDescriptorBindingUpdateUnusedWhilePending(vk::True)
            .setDescriptorBindingVariableDescriptorCount(vk::True)
            .setRuntimeDescriptorArray(vk::True)
            .setTimelineSemaphore(vk::True)
            .setPNext(&vulkan13_features);
        vulkan11_features
            .setShaderDrawParameters(vk::True)
            .setPNext(&vulkan12_features);
        features.setPNext(&vulkan11_features);

        std::vector<float> queue_priorities{};
        queue_priorities.reserve(3);

        for (uint32_t i = 0; i < s_pInstance->m_QueueFamilyQueueCount; i++) {
            queue_priorities.push_back(1.0f - static_cast<float>(i) / 6.0f);
        }

        vk::DeviceQueueCreateInfo vk_queue_create_info{};
        vk_queue_create_info
            .setQueueFamilyIndex(s_pInstance->m_QueueFamilyIndex)
            .setQueuePriorities(queue_priorities);

        vk::DeviceCreateInfo vk_device_create_info{};
        vk_device_create_info
            .setQueueCreateInfos(vk_queue_create_info)
            .setPEnabledExtensionNames(device_extensions)
            .setPNext(&features);

        {
            auto [result, vk_device] = s_pInstance->m_PhysicalDevice.createDevice(vk_device_create_info);
            IGNIS_VK_CHECK(result);
            s_pInstance->m_Device = vk_device;
        }

        uint32_t queue_index         = 0;
        s_pInstance->m_GraphicsQueue = s_pInstance->m_Device.getQueue(s_pInstance->m_QueueFamilyIndex, queue_index);
        if (queue_index + 1 < s_pInstance->m_QueueFamilyQueueCount) queue_index++;
        s_pInstance->m_PresentQueue = s_pInstance->m_Device.getQueue(s_pInstance->m_QueueFamilyIndex, queue_index);
        if (queue_index + 1 < s_pInstance->m_QueueFamilyQueueCount) queue_index++;
        s_pInstance->m_ComputeQueue = s_pInstance->m_Device.getQueue(s_pInstance->m_QueueFamilyIndex, queue_index);

        const auto [width, height] = Window::GetSize();

        CreateSwapchain(width, height);

        vma::AllocatorCreateInfo vma_allocator_create_info{};
        vma_allocator_create_info
            .setFlags(vma::AllocatorCreateFlagBits::eBufferDeviceAddress)
            .setInstance(s_pInstance->m_Instance)
            .setPhysicalDevice(s_pInstance->m_PhysicalDevice)
            .setDevice(s_pInstance->m_Device)
            .setVulkanApiVersion(VK_API_VERSION_1_4);

        auto [result, vma_allocator] = vma::createAllocator(vma_allocator_create_info);

        IGNIS_VK_CHECK(result);

        s_pInstance->m_VmaAllocator = vma_allocator;

        DIGNIS_LOG_ENGINE_INFO("Ignis::Vulkan::Context Initialized");
    }

    void Context::Shutdown() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");

        DIGNIS_LOG_ENGINE_INFO("Ignis::Vulkan::Context Shutdown");

        DestroySwapchain();

        s_pInstance->m_VmaAllocator.destroy();

        s_pInstance->m_Device.destroy();

        s_pInstance->m_Instance.destroySurfaceKHR(s_pInstance->m_Surface);

#if defined(IGNIS_BUILD_TYPE_DEBUG)
        ignisVkDestroyDebugUtilsMessengerEXT(s_pInstance->m_Instance, s_pInstance->m_DebugMessenger);
#endif

        s_pInstance->m_Instance.destroy();

        s_pInstance = nullptr;
    }

    void Context::ResizeSwapchain(const uint32_t width, const uint32_t height) {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        CreateSwapchain(width, height);
    }

    vk::Instance Context::GetInstance() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_Instance;
    }

    vk::DebugUtilsMessengerEXT Context::GetDebugMessenger() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_DebugMessenger;
    }

    vk::SurfaceKHR Context::GetSurface() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_Surface;
    }

    vk::PhysicalDevice Context::GetPhysicalDevice() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_PhysicalDevice;
    }

    vk::Device Context::GetDevice() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_Device;
    }

    uint32_t Context::GetQueueFamilyIndex() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_QueueFamilyIndex;
    }

    uint32_t Context::GetQueueFamilyQueueCount() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_QueueFamilyQueueCount;
    }

    vk::Queue Context::GetGraphicsQueue() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_GraphicsQueue;
    }

    vk::Queue Context::GetComputeQueue() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_ComputeQueue;
    }

    vk::Queue Context::GetPresentQueue() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_PresentQueue;
    }

    uint32_t Context::GetSwapchainImageCount() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_SwapchainImageCount;
    }

    vk::SurfaceFormatKHR Context::GetSwapchainFormat() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_SwapchainFormat;
    }

    vk::PresentModeKHR Context::GetSwapchainPresentMode() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_SwapchainPresentMode;
    }

    vk::SwapchainKHR Context::GetSwapchain() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_Swapchain;
    }

    vk::Extent2D Context::GetSwapchainExtent() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_SwapchainExtent;
    }

    vk::ImageUsageFlags Context::GetSwapchainUsageFlags() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_SwapchainUsageFlags;
    }

    vk::Image Context::GetSwapchainImage(const uint32_t index) {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_SwapchainImages[index];
    }

    vk::ImageView Context::GetSwapchainImageView(const uint32_t index) {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_SwapchainImageViews[index];
    }

    vma::Allocator Context::GetVmaAllocator() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Vulkan::Context is not initialized");
        return s_pInstance->m_VmaAllocator;
    }

    void Context::CreateSwapchain(const uint32_t width, const uint32_t height) {
        vk::SurfaceCapabilitiesKHR surface_capabilities{};
        {
            auto [result, capabilities] = s_pInstance->m_PhysicalDevice.getSurfaceCapabilitiesKHR(
                s_pInstance->m_Surface);
            IGNIS_VK_CHECK(result);
            surface_capabilities = capabilities;
        }

        s_pInstance->m_SwapchainExtent.width  = width;
        s_pInstance->m_SwapchainExtent.height = height;

        const vk::SwapchainKHR old_swapchain = s_pInstance->m_Swapchain;

        for (const vk::ImageView &view : s_pInstance->m_SwapchainImageViews) {
            s_pInstance->m_Device.destroyImageView(view);
        }

        s_pInstance->m_SwapchainImageViews.clear();
        s_pInstance->m_SwapchainImages.clear();

        {
            vk::SwapchainCreateInfoKHR swapchain_create_info{};
            swapchain_create_info
                .setOldSwapchain(old_swapchain)
                .setSurface(s_pInstance->m_Surface)
                .setMinImageCount(s_pInstance->m_SwapchainMinImageCount)
                .setImageFormat(s_pInstance->m_SwapchainFormat.format)
                .setImageColorSpace(s_pInstance->m_SwapchainFormat.colorSpace)
                .setImageExtent(s_pInstance->m_SwapchainExtent)
                .setImageArrayLayers(1)
                .setImageUsage(s_pInstance->m_SwapchainUsageFlags)
                .setImageSharingMode(vk::SharingMode::eExclusive)
                .setQueueFamilyIndices({s_pInstance->m_QueueFamilyIndex})
                .setPreTransform(surface_capabilities.currentTransform)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                .setPresentMode(s_pInstance->m_SwapchainPresentMode)
                .setClipped(vk::True);
            auto [result, swapchain] = s_pInstance->m_Device.createSwapchainKHR(swapchain_create_info);
            IGNIS_VK_CHECK(result);
            s_pInstance->m_Swapchain = swapchain;

            if (old_swapchain) {
                s_pInstance->m_Device.destroySwapchainKHR(old_swapchain);
            }
        }
        {
            auto [result, swapchain_images] = s_pInstance->m_Device.getSwapchainImagesKHR(s_pInstance->m_Swapchain);

            IGNIS_VK_CHECK(result);

            s_pInstance->m_SwapchainImageCount = swapchain_images.size();
            s_pInstance->m_SwapchainImages     = swapchain_images;
        }

        for (const vk::Image &image : s_pInstance->m_SwapchainImages) {
            vk::ImageViewCreateInfo image_view_create_info{};
            image_view_create_info
                .setViewType(vk::ImageViewType::e2D)
                .setImage(image)
                .setFormat(s_pInstance->m_SwapchainFormat.format)
                .setComponents(vk::ComponentMapping{}
                                   .setR(vk::ComponentSwizzle::eIdentity)
                                   .setG(vk::ComponentSwizzle::eIdentity)
                                   .setB(vk::ComponentSwizzle::eIdentity)
                                   .setA(vk::ComponentSwizzle::eIdentity))
                .setSubresourceRange(
                    vk::ImageSubresourceRange{}
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        .setBaseMipLevel(0)
                        .setLevelCount(1)
                        .setBaseArrayLayer(0)
                        .setLayerCount(1));

            auto [result, view] = s_pInstance->m_Device.createImageView(image_view_create_info);
            IGNIS_VK_CHECK(result);
            s_pInstance->m_SwapchainImageViews.push_back(view);
        }
    }

    void Context::DestroySwapchain() {
        for (const vk::ImageView &view : s_pInstance->m_SwapchainImageViews) {
            s_pInstance->m_Device.destroyImageView(view);
        }

        s_pInstance->m_SwapchainImageViews.clear();
        s_pInstance->m_SwapchainImages.clear();

        s_pInstance->m_Device.destroySwapchainKHR(s_pInstance->m_Swapchain);
    }

    void Context::SelectPhysicalDevice(
        const std::vector<const char *> &required_device_extensions,
        const vk::ImageUsageFlags        required_usage_flags) {
        auto [result, physical_devices] = s_pInstance->m_Instance.enumeratePhysicalDevices();
        IGNIS_VK_CHECK(result);
        IGNIS_ASSERT(!physical_devices.empty(), "You need a GPU that supports VP_KHR_ROADMAP_2024 profile");

        gtl::flat_hash_map<vk::PhysicalDeviceType, std::vector<vk::PhysicalDevice> > physical_device_map{};

        for (const vk::PhysicalDevice &physical_device : physical_devices) {
            if (!(CheckPhysicalDeviceSwapchainSupport(physical_device, required_usage_flags) &&
                  CheckPhysicalDeviceExtensionSupport(physical_device, required_device_extensions))) {
                continue;
            }

            vk::PhysicalDeviceProperties properties = physical_device.getProperties();

            physical_device_map[properties.deviceType].push_back(physical_device);
        }

        std::optional<vk::PhysicalDevice> vk_physical_device_opt = std::nullopt;

        for (const auto &physical_device : physical_device_map[vk::PhysicalDeviceType::eDiscreteGpu]) {
            vk_physical_device_opt = physical_device;
            break;
        }

        if (!vk_physical_device_opt) {
            for (const auto &physical_device : physical_device_map[vk::PhysicalDeviceType::eIntegratedGpu]) {
                vk_physical_device_opt = physical_device;
                break;
            }
        }

        if (!vk_physical_device_opt) {
            for (const auto &physical_device : physical_device_map[vk::PhysicalDeviceType::eVirtualGpu]) {
                vk_physical_device_opt = physical_device;
                break;
            }
        }

        if (!vk_physical_device_opt) {
            for (const auto &physical_device : physical_device_map[vk::PhysicalDeviceType::eCpu]) {
                vk_physical_device_opt = physical_device;
                break;
            }
        }

        if (!vk_physical_device_opt) {
            for (const auto &physical_device : physical_device_map[vk::PhysicalDeviceType::eOther]) {
                vk_physical_device_opt = physical_device;
                break;
            }
        }

        IGNIS_ASSERT(vk_physical_device_opt.has_value(), "No GPU supports VP_KHR_ROADMAP_2024");

        const vk::PhysicalDevice vk_physical_device = vk_physical_device_opt.value();

        const vk::PhysicalDeviceProperties properties = vk_physical_device.getProperties();

        const uint32_t api_version    = properties.apiVersion;
        const uint32_t driver_version = properties.driverVersion;

        uint32_t api_version_major = VK_VERSION_MAJOR(api_version);
        uint32_t api_version_minor = VK_VERSION_MINOR(api_version);
        uint32_t api_version_patch = VK_VERSION_PATCH(api_version);

        auto [_, surface_capabilities] = vk_physical_device.getSurfaceCapabilitiesKHR(s_pInstance->m_Surface);

        DIGNIS_LOG_ENGINE_INFO("GPU Selected:");
        DIGNIS_LOG_ENGINE_INFO("\tAPI Version: {}.{}.{}", api_version_major, api_version_minor, api_version_patch);
        DIGNIS_LOG_ENGINE_INFO("\tDevice ID: {}", properties.deviceID);
        DIGNIS_LOG_ENGINE_INFO("\tVendor ID: {}", properties.vendorID);
        DIGNIS_LOG_ENGINE_INFO("\tDevice Name: {}", properties.deviceName.data());
        DIGNIS_LOG_ENGINE_INFO("\tDevice Type: {}", vk::to_string(properties.deviceType));
        DIGNIS_LOG_ENGINE_INFO("\tDriver Version: {}", driver_version);
        DIGNIS_LOG_ENGINE_INFO(
            "\tMin Surface Size: {}x{}",
            surface_capabilities.minImageExtent.width,
            surface_capabilities.minImageExtent.height);
        DIGNIS_LOG_ENGINE_INFO(
            "\tMax Surface Size: {}x{}",
            surface_capabilities.maxImageExtent.width,
            surface_capabilities.maxImageExtent.height);

        s_pInstance->m_PhysicalDevice = vk_physical_device;
    }

    void Context::SelectQueueFamilyIndex() {
        const auto queue_families = s_pInstance->m_PhysicalDevice.getQueueFamilyProperties();

        IGNIS_ASSERT(!queue_families.empty(), "There are no Vulkan Queue Families for this GPU (HOW?!!)");

        constexpr vk::QueueFlags required_queue_flags =
            vk::QueueFlagBits::eGraphics |
            vk::QueueFlagBits::eCompute |
            vk::QueueFlagBits::eTransfer;

        uint32_t index = 0;
        for (const auto &queue_family : queue_families) {
            auto [result, presentation_support] = s_pInstance->m_PhysicalDevice.getSurfaceSupportKHR(
                index, s_pInstance->m_Surface);
            IGNIS_VK_CHECK(result);

            const bool supports_graphics_compute_transfer =
                (queue_family.queueFlags & required_queue_flags) == required_queue_flags;

            if (vk::True == presentation_support &&
                supports_graphics_compute_transfer &&
                queue_family.queueCount >= 3) {
                s_pInstance->m_QueueFamilyIndex      = index;
                s_pInstance->m_QueueFamilyQueueCount = 3;
                return;
            }
            index++;
        }

        index = 0;
        for (const auto &queue_family : queue_families) {
            auto [result, presentation_support] = s_pInstance->m_PhysicalDevice.getSurfaceSupportKHR(
                index, s_pInstance->m_Surface);
            IGNIS_VK_CHECK(result);

            const bool supports_graphics_compute_transfer =
                (queue_family.queueFlags & required_queue_flags) == required_queue_flags;

            if (vk::True == presentation_support &&
                supports_graphics_compute_transfer) {
                s_pInstance->m_QueueFamilyIndex      = index;
                s_pInstance->m_QueueFamilyQueueCount = queue_family.queueCount;
                return;
            }
            index++;
        }

        s_pInstance->m_QueueFamilyIndex      = 0;
        s_pInstance->m_QueueFamilyQueueCount = queue_families[0].queueCount;
    }

    void Context::SelectSwapchainImageCount(const uint32_t preferred_image_count) {
        auto [result, surface_capabilities] = s_pInstance->m_PhysicalDevice.getSurfaceCapabilitiesKHR(
            s_pInstance->m_Surface);
        IGNIS_VK_CHECK(result);

        uint32_t image_count = preferred_image_count;
        if (surface_capabilities.minImageCount > 0 && image_count < surface_capabilities.minImageCount) {
            image_count = surface_capabilities.minImageCount;
        }
        if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
            image_count = surface_capabilities.maxImageCount;
        }

        s_pInstance->m_SwapchainMinImageCount = image_count;
    }

    void Context::SelectSwapchainFormat(const std::span<const vk::Format> preferred_formats) {
        auto [result, surface_formats] = s_pInstance->m_PhysicalDevice.getSurfaceFormatsKHR(s_pInstance->m_Surface);
        IGNIS_VK_CHECK(result);

        for (const vk::Format &preferred_format : preferred_formats) {
            for (const vk::SurfaceFormatKHR &surface_format : surface_formats) {
                if (surface_format.format == preferred_format &&
                    surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                    s_pInstance->m_SwapchainFormat = surface_format;
                    return;
                }
            }
        }

        s_pInstance->m_SwapchainFormat = surface_formats[0];
    }

    void Context::SelectSwapchainPresentMode(const std::span<const vk::PresentModeKHR> preferred_present_modes) {
        auto [result, present_modes] = s_pInstance->m_PhysicalDevice.getSurfacePresentModesKHR(s_pInstance->m_Surface);
        IGNIS_VK_CHECK(result);

        for (const vk::PresentModeKHR &preferred_mode : preferred_present_modes) {
            for (const vk::PresentModeKHR &mode : present_modes) {
                if (mode == preferred_mode) {
                    s_pInstance->m_SwapchainPresentMode = mode;
                    return;
                }
            }
        }

        s_pInstance->m_SwapchainPresentMode = vk::PresentModeKHR::eFifo;
    }

    bool Context::CheckInstanceLayerSupport(const std::vector<const char *> &required_instance_layers) {
        auto [result, available_layers] = vk::enumerateInstanceLayerProperties();
        IGNIS_VK_CHECK(result);

        for (const char *const &layer_name : required_instance_layers) {
            bool layer_found = false;

            for (const auto &layer_property : available_layers) {
                if (strcmp(layer_name, layer_property.layerName.data()) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found) {
                return false;
            }
        }

        return true;
    }

    bool Context::CheckPhysicalDeviceSwapchainSupport(
        const vk::PhysicalDevice &physical_device,
        vk::ImageUsageFlags       required_usage_flags) {
        auto [result, capabilities] = physical_device.getSurfaceCapabilitiesKHR(s_pInstance->m_Surface);
        IGNIS_VK_CHECK(result);

        required_usage_flags |=
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eTransferSrc;

        return (capabilities.supportedUsageFlags & required_usage_flags) == required_usage_flags;
    }

    bool Context::CheckPhysicalDeviceExtensionSupport(
        const vk::PhysicalDevice        &physical_device,
        const std::vector<const char *> &required_extensions) {
        auto [result, available_extensions] = physical_device.enumerateDeviceExtensionProperties();
        IGNIS_VK_CHECK(result);

        gtl::flat_hash_set<std::string> extensions{required_extensions.begin(), required_extensions.end()};
        for (const auto &extension : available_extensions) {
            extensions.erase(extension.extensionName.data());
        }

        return extensions.empty();
    }

    VkBool32 Context::DebugUtilsMessengerCallback(
        const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessengerCreateFlagsEXT          type,
        const VkDebugUtilsMessengerCallbackDataEXT  *p_callback_data,
        void                                        *p_userdata) {
        switch (severity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {  // NOLINT(*-branch-clone)
                // IGNIS_LOG_ENGINE_TRACE(
                //     "[Vulkan Validation Layers]: {} - {}: {}",
                //     p_callback_data->messageIdNumber,
                //     p_callback_data->pMessageIdName,
                //     p_callback_data->pMessage);
            } break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
                // IGNIS_LOG_ENGINE_INFO(
                //     "[Vulkan Validation Layers]: {} - {}: {}",
                //     p_callback_data->messageIdNumber,
                //     p_callback_data->pMessageIdName,
                //     p_callback_data->pMessage);
            } break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
                IGNIS_LOG_ENGINE_WARN(
                    "[Vulkan Validation Layers]: {} - {}: {}",
                    p_callback_data->messageIdNumber,
                    p_callback_data->pMessageIdName,
                    p_callback_data->pMessage);
            } break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
                IGNIS_LOG_ENGINE_ERROR(
                    "[Vulkan Validation Layers]: {} - {}: {}",
                    p_callback_data->messageIdNumber,
                    p_callback_data->pMessageIdName,
                    p_callback_data->pMessage);
            } break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
                break;
        }

        return VK_FALSE;
    }

    IGNIS_IF_DEBUG(Context::State::~State() {
        assert(s_pInstance == nullptr && "Forgot to shutdown Ignis::Vulkan::Context");
    })
}  // namespace Ignis::Vulkan
