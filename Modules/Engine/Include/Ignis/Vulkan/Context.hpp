#pragma once

#include <Ignis/Window.hpp>

#include <Ignis/Vulkan/Types.hpp>

namespace Ignis::Vulkan {
    class Context {
       public:
        struct Settings {
            std::vector<vk::Format>         PreferredSurfaceFormats{vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Srgb};
            std::vector<vk::PresentModeKHR> PreferredPresentModes{vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifoRelaxed};

            std::uint32_t       PreferredImageCount{3u};
            vk::ImageUsageFlags SurfaceUsageFlags{0u};
        };

       public:
        static void Initialize(Context *context, const Settings &settings);
        static void Shutdown();

        static void ResizeSwapchain(uint32_t width, uint32_t height);

        static vk::Instance               GetInstance();
        static vk::DebugUtilsMessengerEXT GetDebugMessenger();
        static vk::SurfaceKHR             GetSurface();
        static vk::PhysicalDevice         GetPhysicalDevice();
        static vk::Device                 GetDevice();
        static uint32_t                   GetQueueFamilyIndex();
        static uint32_t                   GetQueueFamilyQueueCount();
        static vk::Queue                  GetGraphicsQueue();
        static vk::Queue                  GetComputeQueue();
        static vk::Queue                  GetPresentQueue();
        static uint32_t                   GetSwapchainImageCount();
        static vk::SurfaceFormatKHR       GetSwapchainFormat();
        static vk::PresentModeKHR         GetSwapchainPresentMode();
        static vk::SwapchainKHR           GetSwapchain();
        static vk::Extent2D               GetSwapchainExtent();
        static vk::ImageUsageFlags        GetSwapchainUsageFlags();
        static vk::Image                  GetSwapchainImage(uint32_t index);
        static vk::ImageView              GetSwapchainImageView(uint32_t index);
        static vma::Allocator             GetVmaAllocator();

       public:
        Context()  = default;
        ~Context() = default;

       private:
        IGNIS_IF_DEBUG(class State {
           public:
            ~State();
        };)

       private:
        static void CreateSwapchain(uint32_t width, uint32_t height);
        static void DestroySwapchain();

        static void SelectPhysicalDevice(
            const std::vector<const char *> &required_device_extensions,
            vk::ImageUsageFlags              required_usage_flags);
        static void SelectQueueFamilyIndex();
        static void SelectSwapchainImageCount(uint32_t preferred_image_count);
        static void SelectSwapchainFormat(std::span<const vk::Format> preferred_formats);
        static void SelectSwapchainPresentMode(std::span<const vk::PresentModeKHR> preferred_present_modes);

        static bool CheckInstanceLayerSupport(const std::vector<const char *> &required_instance_layers);
        static bool CheckPhysicalDeviceFeatureSupport(const vk::PhysicalDevice &physical_device);
        static bool CheckPhysicalDeviceSwapchainSupport(
            const vk::PhysicalDevice &physical_device,
            vk::ImageUsageFlags       required_usage_flags);
        static bool CheckPhysicalDeviceExtensionSupport(
            const vk::PhysicalDevice        &physical_device,
            const std::vector<const char *> &required_extensions);

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
            VkDebugUtilsMessengerCreateFlagsEXT         type,
            const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
            void                                       *p_userdata);

       private:
        vk::Instance               m_Instance;
        vk::DebugUtilsMessengerEXT m_DebugMessenger;

        vk::SurfaceKHR     m_Surface;
        vk::PhysicalDevice m_PhysicalDevice;

        vk::Device m_Device;
        uint32_t   m_QueueFamilyIndex;
        uint32_t   m_QueueFamilyQueueCount;
        vk::Queue  m_GraphicsQueue;
        vk::Queue  m_ComputeQueue;
        vk::Queue  m_PresentQueue;

        uint32_t             m_SwapchainMinImageCount;
        vk::SurfaceFormatKHR m_SwapchainFormat;
        vk::PresentModeKHR   m_SwapchainPresentMode;

        vk::SwapchainKHR    m_Swapchain;
        vk::Extent2D        m_SwapchainExtent;
        vk::ImageUsageFlags m_SwapchainUsageFlags;
        uint32_t            m_SwapchainImageCount;

        std::vector<vk::Image>     m_SwapchainImages;
        std::vector<vk::ImageView> m_SwapchainImageViews;

        vma::Allocator m_VmaAllocator;

       private:
        static Context *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State;)
    };
}  // namespace Ignis::Vulkan