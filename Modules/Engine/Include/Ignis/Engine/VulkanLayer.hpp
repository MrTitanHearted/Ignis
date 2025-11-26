#pragma once

#include <Ignis/Engine/Layer.hpp>

#include <Ignis/Engine/WindowLayer.hpp>

#include <Ignis/Engine/VulkanLayer/Buffer.hpp>
#include <Ignis/Engine/VulkanLayer/Command.hpp>
#include <Ignis/Engine/VulkanLayer/Descriptor.hpp>
#include <Ignis/Engine/VulkanLayer/Image.hpp>
#include <Ignis/Engine/VulkanLayer/Memory.hpp>
#include <Ignis/Engine/VulkanLayer/Pipeline.hpp>
#include <Ignis/Engine/VulkanLayer/Queue.hpp>
#include <Ignis/Engine/VulkanLayer/RenderPass.hpp>
#include <Ignis/Engine/VulkanLayer/Sampler.hpp>
#include <Ignis/Engine/VulkanLayer/Shader.hpp>
#include <Ignis/Engine/VulkanLayer/Synchronization.hpp>
#include <Ignis/Engine/VulkanLayer/Types.hpp>

namespace Ignis {
    class VulkanLayer final : public ILayer {
       public:
        struct Settings {
            std::vector<vk::Format> PreferredSurfaceFormats{vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Srgb};

            std::vector<vk::PresentModeKHR> PreferredPresentModes{vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifoRelaxed};

            std::uint32_t PreferredImageCount{3u};

            vk::ImageUsageFlags SurfaceUsageFlags{0u};
        };

       public:
        explicit VulkanLayer(WindowLayer *window_layer, const Settings &settings);
        ~VulkanLayer() override;

        void resizeSwapchain(uint32_t width, uint32_t height);

        vk::Instance getInstance() const;

        vk::DebugUtilsMessengerEXT getDebugMessenger() const;

        vk::SurfaceKHR     getSurface() const;
        vk::PhysicalDevice getPhysicalDevice() const;

        vk::Device getDevice() const;
        uint32_t   getQueueFamilyIndex() const;
        uint32_t   getQueueFamilyQueueCount() const;
        vk::Queue  getGraphicsQueue() const;
        vk::Queue  getComputeQueue() const;
        vk::Queue  getPresentQueue() const;

        uint32_t getSwapchainMinImageCount() const;

        vk::SurfaceFormatKHR getSwapchainFormat() const;
        vk::PresentModeKHR   getSwapchainPresentMode() const;

        vk::SwapchainKHR    getSwapchain() const;
        vk::Extent2D        getSwapchainExtent() const;
        vk::ImageUsageFlags getSwapchainUsageFlags() const;

        uint32_t getSwapchainImageCount() const;

        vk::Image     getSwapchainImage(uint32_t index) const;
        vk::ImageView getSwapchainImageView(uint32_t index) const;

        vma::Allocator getVmaAllocator() const;

        vk::Format &getSwapchainFormatRef();

        const vk::Format &getSwapchainFormatConstRef() const;

       private:
        static bool CheckInstanceLayerSupport(const std::vector<const char *> &required_instance_layers);
        static bool CheckPhysicalDeviceSwapchainSupport(
            vk::PhysicalDevice  physical_device,
            vk::SurfaceKHR      surface,
            vk::ImageUsageFlags required_usage_flags);

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
            VkDebugUtilsMessengerCreateFlagsEXT         type,
            const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
            void                                       *p_userdata);

       private:
        void createSwapchain(uint32_t width, uint32_t height);
        void destroySwapchain();

        void selectPhysicalDevice(vk::ImageUsageFlags required_usage_flags);
        void selectQueueFamilyIndex();
        void selectSwapchainImageCount(uint32_t preferred_image_count);
        void selectSwapchainFormat(std::span<const vk::Format> preferred_formats);
        void selectSwapchainPresentMode(std::span<const vk::PresentModeKHR> preferred_present_modes);

       private:
        vk::Instance m_Instance;

        vk::DebugUtilsMessengerEXT m_DebugMessenger;

        vk::SurfaceKHR     m_Surface;
        vk::PhysicalDevice m_PhysicalDevice;

        vk::Device m_Device;
        uint32_t   m_QueueFamilyIndex;
        uint32_t   m_QueueFamilyQueueCount;
        vk::Queue  m_GraphicsQueue;
        vk::Queue  m_ComputeQueue;
        vk::Queue  m_PresentQueue;

        uint32_t m_SwapchainMinImageCount;

        vk::SurfaceFormatKHR m_SwapchainFormat;
        vk::PresentModeKHR   m_SwapchainPresentMode;

        vk::SwapchainKHR    m_Swapchain;
        vk::Extent2D        m_SwapchainExtent;
        vk::ImageUsageFlags m_SwapchainUsageFlags;

        uint32_t m_SwapchainImageCount;

        std::vector<vk::Image>     m_SwapchainImages;
        std::vector<vk::ImageView> m_SwapchainImageViews;

        vma::Allocator m_VmaAllocator;
    };
}  // namespace Ignis