#include <Ignis/Engine/VulkanLayer.hpp>

VkResult ignisVkCreateDebugUtilsMessengerEXT(
    const VkInstance                          instance,
    const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
    VkDebugUtilsMessengerEXT                 *p_debug_utils_messenger) {
    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func == nullptr)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    return func(instance, p_create_info, nullptr, p_debug_utils_messenger);
}

VkResult ignisVkDestroyDebugUtilsMessengerEXT(
    const VkInstance               instance,
    const VkDebugUtilsMessengerEXT debug_utils_messenger) {
    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func == nullptr)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    func(instance, debug_utils_messenger, nullptr);
    return VK_SUCCESS;
}

namespace Ignis {
    VulkanLayer::VulkanLayer(WindowLayer *window_layer, const Settings &settings) {
        DIGNIS_ASSERT(nullptr != window_layer);
        uint32_t glfw_required_extensions_count = 0;

        const char **glfw_required_extensions = glfwGetRequiredInstanceExtensions(&glfw_required_extensions_count);

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
        vk_debug_messenger_create_info.pUserData       = this;

        vk_instance_create_info.setPNext(&vk_debug_messenger_create_info);
#endif

        {
            auto [result, vk_instance] = vk::createInstance(vk_instance_create_info);
            IGNIS_VK_CHECK(result);
            m_Instance = vk_instance;
        }

#if defined(IGNIS_BUILD_TYPE_DEBUG)
        VkDebugUtilsMessengerEXT vk_debug_utils_messenger = VK_NULL_HANDLE;
        IGNIS_VK_CHECK(
            ignisVkCreateDebugUtilsMessengerEXT(
                m_Instance,
                &vk_debug_messenger_create_info,
                &vk_debug_utils_messenger));

        m_DebugMessenger = vk_debug_utils_messenger;
#endif

        VkSurfaceKHR vk_surface = VK_NULL_HANDLE;

        DIGNIS_ASSERT(window_layer != nullptr, "Ignis::WindowLayer is not initialized");

        IGNIS_VK_CHECK(glfwCreateWindowSurface(m_Instance, window_layer->get(), nullptr, &vk_surface));
        m_Surface = vk_surface;

        selectPhysicalDevice(settings.SurfaceUsageFlags);
        selectQueueFamilyIndex();
        selectSwapchainImageCount(settings.PreferredImageCount);
        selectSwapchainFormat(settings.PreferredSurfaceFormats);
        selectSwapchainPresentMode(settings.PreferredPresentModes);

        m_SwapchainUsageFlags =
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

        for (uint32_t i = 0; i < m_QueueFamilyQueueCount; i++) {
            queue_priorities.push_back(1.0f - static_cast<float>(i) / 6.0f);
        }

        vk::DeviceQueueCreateInfo vk_queue_create_info{};
        vk_queue_create_info
            .setQueueFamilyIndex(m_QueueFamilyIndex)
            .setQueuePriorities(queue_priorities);

        vk::DeviceCreateInfo vk_device_create_info{};
        vk_device_create_info
            .setQueueCreateInfos(vk_queue_create_info)
            .setPEnabledExtensionNames(device_extensions)
            .setPNext(&features);

        {
            auto [result, vk_device] = m_PhysicalDevice.createDevice(vk_device_create_info);
            IGNIS_VK_CHECK(result);
            m_Device = vk_device;
        }

        uint32_t queue_index = 0;
        m_GraphicsQueue      = m_Device.getQueue(m_QueueFamilyIndex, queue_index);
        if (queue_index + 1 < m_QueueFamilyQueueCount) queue_index++;
        m_PresentQueue = m_Device.getQueue(m_QueueFamilyIndex, queue_index);
        if (queue_index + 1 < m_QueueFamilyQueueCount) queue_index++;
        m_ComputeQueue = m_Device.getQueue(m_QueueFamilyIndex, queue_index);

        const auto [width, height] = window_layer->getSize();

        createSwapchain(width, height);

        vma::AllocatorCreateInfo vma_allocator_create_info{};
        vma_allocator_create_info
            .setFlags(vma::AllocatorCreateFlagBits::eBufferDeviceAddress)
            .setInstance(m_Instance)
            .setPhysicalDevice(m_PhysicalDevice)
            .setDevice(m_Device)
            .setVulkanApiVersion(VK_API_VERSION_1_4);

        auto [result, vma_allocator] = vma::createAllocator(vma_allocator_create_info);

        IGNIS_VK_CHECK(result);

        m_VmaAllocator = vma_allocator;

        DIGNIS_LOG_ENGINE_INFO("Ignis::VulkanLayer Initialized");
    }

    VulkanLayer::~VulkanLayer() {
        DIGNIS_LOG_ENGINE_INFO("Ignis::VulkanLayer Shutdown");

        destroySwapchain();

        m_VmaAllocator.destroy();

        m_Device.destroy();

        m_Instance.destroySurfaceKHR(m_Surface);

#if defined(IGNIS_BUILD_TYPE_DEBUG)
        ignisVkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger);
#endif

        m_Instance.destroy();
    }

    void VulkanLayer::resizeSwapchain(const uint32_t width, const uint32_t height) {
        createSwapchain(width, height);
    }

    vk::Instance VulkanLayer::getInstance() const {
        return m_Instance;
    }

    vk::DebugUtilsMessengerEXT VulkanLayer::getDebugMessenger() const {
        return m_DebugMessenger;
    }

    vk::SurfaceKHR VulkanLayer::getSurface() const {
        return m_Surface;
    }

    vk::PhysicalDevice VulkanLayer::getPhysicalDevice() const {
        return m_PhysicalDevice;
    }

    vk::Device VulkanLayer::getDevice() const {
        return m_Device;
    }

    uint32_t VulkanLayer::getQueueFamilyIndex() const {
        return m_QueueFamilyIndex;
    }

    uint32_t VulkanLayer::getQueueFamilyQueueCount() const {
        return m_QueueFamilyQueueCount;
    }

    vk::Queue VulkanLayer::getGraphicsQueue() const {
        return m_GraphicsQueue;
    }

    vk::Queue VulkanLayer::getComputeQueue() const {
        return m_ComputeQueue;
    }

    vk::Queue VulkanLayer::getPresentQueue() const {
        return m_PresentQueue;
    }

    uint32_t VulkanLayer::getSwapchainMinImageCount() const {
        return m_SwapchainMinImageCount;
    }

    vk::SurfaceFormatKHR VulkanLayer::getSwapchainFormat() const {
        return m_SwapchainFormat;
    }

    vk::PresentModeKHR VulkanLayer::getSwapchainPresentMode() const {
        return m_SwapchainPresentMode;
    }

    vk::SwapchainKHR VulkanLayer::getSwapchain() const {
        return m_Swapchain;
    }

    vk::Extent2D VulkanLayer::getSwapchainExtent() const {
        return m_SwapchainExtent;
    }

    vk::ImageUsageFlags VulkanLayer::getSwapchainUsageFlags() const {
        return m_SwapchainUsageFlags;
    }

    uint32_t VulkanLayer::getSwapchainImageCount() const {
        return m_SwapchainImageCount;
    }

    vk::Image VulkanLayer::getSwapchainImage(const uint32_t index) const {
        return m_SwapchainImages[index];
    }

    vk::ImageView VulkanLayer::getSwapchainImageView(const uint32_t index) const {
        return m_SwapchainImageViews[index];
    }

    vma::Allocator VulkanLayer::getVmaAllocator() const {
        return m_VmaAllocator;
    }

    vk::Format &VulkanLayer::getSwapchainFormatRef() {
        return m_SwapchainFormat.format;
    }

    const vk::Format &VulkanLayer::getSwapchainFormatConstRef() const {
        return m_SwapchainFormat.format;
    }

    bool VulkanLayer::CheckInstanceLayerSupport(const std::vector<const char *> &required_instance_layers) {
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

    bool VulkanLayer::CheckPhysicalDeviceSwapchainSupport(
        const vk::PhysicalDevice physical_device,
        const vk::SurfaceKHR     surface,

        vk::ImageUsageFlags required_usage_flags) {
        auto [result, capabilities] = physical_device.getSurfaceCapabilitiesKHR(surface);
        IGNIS_VK_CHECK(result);

        required_usage_flags |=
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eTransferSrc;

        return (capabilities.supportedUsageFlags & required_usage_flags) == required_usage_flags;
    }

    VkBool32 VulkanLayer::DebugUtilsMessengerCallback(
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

    void VulkanLayer::createSwapchain(const uint32_t width, const uint32_t height) {
        vk::SurfaceCapabilitiesKHR surface_capabilities{};
        {
            auto [result, capabilities] = m_PhysicalDevice.getSurfaceCapabilitiesKHR(
                m_Surface);
            IGNIS_VK_CHECK(result);
            surface_capabilities = capabilities;
        }

        m_SwapchainExtent.width  = width;
        m_SwapchainExtent.height = height;

        const vk::SwapchainKHR old_swapchain = m_Swapchain;

        for (const vk::ImageView &view : m_SwapchainImageViews) {
            m_Device.destroyImageView(view);
        }

        m_SwapchainImageViews.clear();
        m_SwapchainImages.clear();

        {
            vk::SwapchainCreateInfoKHR swapchain_create_info{};
            swapchain_create_info
                .setOldSwapchain(old_swapchain)
                .setSurface(m_Surface)
                .setMinImageCount(m_SwapchainMinImageCount)
                .setImageFormat(m_SwapchainFormat.format)
                .setImageColorSpace(m_SwapchainFormat.colorSpace)
                .setImageExtent(m_SwapchainExtent)
                .setImageArrayLayers(1)
                .setImageUsage(m_SwapchainUsageFlags)
                .setImageSharingMode(vk::SharingMode::eExclusive)
                .setQueueFamilyIndices({m_QueueFamilyIndex})
                .setPreTransform(surface_capabilities.currentTransform)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                .setPresentMode(m_SwapchainPresentMode)
                .setClipped(vk::True);
            auto [result, swapchain] = m_Device.createSwapchainKHR(swapchain_create_info);
            IGNIS_VK_CHECK(result);
            m_Swapchain = swapchain;

            if (old_swapchain) {
                m_Device.destroySwapchainKHR(old_swapchain);
            }
        }
        {
            auto [result, swapchain_images] = m_Device.getSwapchainImagesKHR(m_Swapchain);

            IGNIS_VK_CHECK(result);

            m_SwapchainImageCount = swapchain_images.size();
            m_SwapchainImages     = swapchain_images;
        }

        for (const vk::Image &image : m_SwapchainImages) {
            vk::ImageViewCreateInfo image_view_create_info{};
            image_view_create_info
                .setViewType(vk::ImageViewType::e2D)
                .setImage(image)
                .setFormat(m_SwapchainFormat.format)
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

            auto [result, view] = m_Device.createImageView(image_view_create_info);
            IGNIS_VK_CHECK(result);
            m_SwapchainImageViews.push_back(view);
        }
    }

    void VulkanLayer::destroySwapchain() {
        for (const vk::ImageView &view : m_SwapchainImageViews) {
            m_Device.destroyImageView(view);
        }

        m_SwapchainImageViews.clear();
        m_SwapchainImages.clear();

        m_Device.destroySwapchainKHR(m_Swapchain);
    }

    void VulkanLayer::selectPhysicalDevice(const vk::ImageUsageFlags required_usage_flags) {
        auto [result, physical_devices] = m_Instance.enumeratePhysicalDevices();
        IGNIS_VK_CHECK(result);
        IGNIS_ASSERT(!physical_devices.empty(), "You need a GPU that supports VP_KHR_ROADMAP_2024 profile");

        gtl::flat_hash_map<vk::PhysicalDeviceType, std::vector<vk::PhysicalDevice> > physical_device_map{};

        for (const vk::PhysicalDevice &physical_device : physical_devices) {
            if (!CheckPhysicalDeviceSwapchainSupport(physical_device, m_Surface, required_usage_flags)) {
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

        auto [_, surface_capabilities] = vk_physical_device.getSurfaceCapabilitiesKHR(m_Surface);

        DIGNIS_LOG_ENGINE_INFO("GPU Selected:");
        DIGNIS_LOG_ENGINE_INFO("\tAPI Version: {}.{}.{}", api_version_major, api_version_minor, api_version_patch);
        DIGNIS_LOG_ENGINE_INFO("\tDevice ID: {}", properties.deviceID);
        DIGNIS_LOG_ENGINE_INFO("\tVendor ID: {}", properties.vendorID);
        DIGNIS_LOG_ENGINE_INFO("\tDevice Name: {}", properties.deviceName.data());
        DIGNIS_LOG_ENGINE_INFO("\tDevice Type: {}", vk::to_string(properties.deviceType));
        DIGNIS_LOG_ENGINE_INFO("\tDriver Version: {}", driver_version);
        DIGNIS_LOG_ENGINE_INFO("\tSurface Usages: {}", vk::to_string(surface_capabilities.supportedUsageFlags));

        m_PhysicalDevice = vk_physical_device;
    }

    void VulkanLayer::selectQueueFamilyIndex() {
        const auto queue_families = m_PhysicalDevice.getQueueFamilyProperties();

        IGNIS_ASSERT(!queue_families.empty(), "There are no Vulkan Queue Families for this GPU (HOW?!!)");

        constexpr vk::QueueFlags required_queue_flags =
            vk::QueueFlagBits::eGraphics |
            vk::QueueFlagBits::eCompute |
            vk::QueueFlagBits::eTransfer;

        uint32_t index = 0;
        for (const auto &queue_family : queue_families) {
            auto [result, presentation_support] = m_PhysicalDevice.getSurfaceSupportKHR(
                index, m_Surface);
            IGNIS_VK_CHECK(result);

            const bool supports_graphics_compute_transfer =
                (queue_family.queueFlags & required_queue_flags) == required_queue_flags;

            if (vk::True == presentation_support &&
                supports_graphics_compute_transfer &&
                queue_family.queueCount >= 3) {
                m_QueueFamilyIndex      = index;
                m_QueueFamilyQueueCount = 3;
                return;
            }
            index++;
        }

        index = 0;
        for (const auto &queue_family : queue_families) {
            auto [result, presentation_support] = m_PhysicalDevice.getSurfaceSupportKHR(
                index, m_Surface);
            IGNIS_VK_CHECK(result);

            const bool supports_graphics_compute_transfer =
                (queue_family.queueFlags & required_queue_flags) == required_queue_flags;

            if (vk::True == presentation_support &&
                supports_graphics_compute_transfer) {
                m_QueueFamilyIndex      = index;
                m_QueueFamilyQueueCount = queue_family.queueCount;
                return;
            }
            index++;
        }

        m_QueueFamilyIndex      = 0;
        m_QueueFamilyQueueCount = queue_families[0].queueCount;
    }

    void VulkanLayer::selectSwapchainImageCount(const uint32_t preferred_image_count) {
        auto [result, surface_capabilities] = m_PhysicalDevice.getSurfaceCapabilitiesKHR(
            m_Surface);
        IGNIS_VK_CHECK(result);

        uint32_t image_count = preferred_image_count;
        if (surface_capabilities.minImageCount > 0 && image_count < surface_capabilities.minImageCount) {
            image_count = surface_capabilities.minImageCount;
        }
        if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
            image_count = surface_capabilities.maxImageCount;
        }

        m_SwapchainMinImageCount = image_count;
    }

    void VulkanLayer::selectSwapchainFormat(const std::span<const vk::Format> preferred_formats) {
        auto [result, surface_formats] = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface);
        IGNIS_VK_CHECK(result);

        for (const vk::Format &preferred_format : preferred_formats) {
            for (const vk::SurfaceFormatKHR &surface_format : surface_formats) {
                if (surface_format.format == preferred_format &&
                    surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                    m_SwapchainFormat = surface_format;
                    return;
                }
            }
        }

        m_SwapchainFormat = surface_formats[0];
    }

    void VulkanLayer::selectSwapchainPresentMode(const std::span<const vk::PresentModeKHR> preferred_present_modes) {
        auto [result, present_modes] = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface);
        IGNIS_VK_CHECK(result);

        for (const vk::PresentModeKHR &preferred_mode : preferred_present_modes) {
            for (const vk::PresentModeKHR &mode : present_modes) {
                if (mode == preferred_mode) {
                    m_SwapchainPresentMode = mode;
                    return;
                }
            }
        }

        m_SwapchainPresentMode = vk::PresentModeKHR::eFifo;
    }
}  // namespace Ignis
