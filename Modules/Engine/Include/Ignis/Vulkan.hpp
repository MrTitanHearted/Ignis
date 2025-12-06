#pragma once

#include <Ignis/Core.hpp>

#define IGNIS_VK_CHECK(result)                                        \
    do {                                                              \
        vk::Result ignis_vk_result = static_cast<vk::Result>(result); \
        IGNIS_ASSERT(                                                 \
            vk::Result::eSuccess == ignis_vk_result,                  \
            "Ignis::Vulkan vk::Result failed: '{}'",                  \
            vk::to_string(ignis_vk_result));                          \
    } while (false);

#if defined(IGNIS_BUILD_TYPE_DEBUG)
    #define DIGNIS_VK_CHECK(result) IGNIS_VK_CHECK(result)
#else
    #define DIGNIS_VK_CHECK(result) \
        do {                        \
            (void)result;           \
        } while (false);
#endif

namespace Ignis {
    class Vulkan {
       public:
        struct Settings {
            std::vector<vk::Format> PreferredSurfaceFormats{vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Srgb};

            std::vector<vk::PresentModeKHR> PreferredPresentModes{vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifoRelaxed};

            uint32_t PreferredImageCount = 3;

            vk::ImageUsageFlags SurfaceUsageFlags = {};

            bool ValidationLayers = true;
        };

        struct Image {
            vk::Image    Handle;
            vk::Format   Format;
            vk::Extent3D Extent;

            vma::Allocation Allocation;

            vk::ImageUsageFlags UsageFlags;
        };

        struct Buffer {
            vk::Buffer Handle;
            uint64_t   Size;

            vma::Allocation Allocation;

            vk::BufferUsageFlags UsageFlags;
        };

        struct VertexLayout {
            vk::VertexInputBindingDescription Binding;

            std::vector<vk::VertexInputAttributeDescription> Attributes;
        };

#pragma region Command

        class BarrierMerger {
           public:
            BarrierMerger()  = default;
            ~BarrierMerger() = default;

            void clear();

            void putImageBarrier(
                vk::Image               image,
                vk::ImageLayout         old_layout,
                vk::ImageLayout         new_layout,
                vk::PipelineStageFlags2 src_stage,
                vk::AccessFlags2        src_access,
                vk::PipelineStageFlags2 dst_stage,
                vk::AccessFlags2        dst_access);

            void putBufferBarrier(
                vk::Buffer              buffer,
                uint64_t                offset,
                uint64_t                size,
                vk::PipelineStageFlags2 src_stage,
                vk::AccessFlags2        src_access,
                vk::PipelineStageFlags2 dst_stage,
                vk::AccessFlags2        dst_access);

            void flushBarriers(vk::CommandBuffer command_buffer);

           private:
            std::vector<vk::ImageMemoryBarrier2>  m_ImageBarriers;
            std::vector<vk::BufferMemoryBarrier2> m_BufferBarriers;
        };

#pragma endregion

#pragma region Descriptor

        class DescriptorSetLayoutBuilder {
           public:
            DescriptorSetLayoutBuilder()  = default;
            ~DescriptorSetLayoutBuilder() = default;

            void clear();

            DescriptorSetLayoutBuilder &setFlags(vk::DescriptorSetLayoutCreateFlags flags);

            DescriptorSetLayoutBuilder &addBinding(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::DescriptorType         type,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addBinding(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::DescriptorType         type,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addBinding(
                uint32_t             binding,
                uint32_t             count,
                vk::DescriptorType   type,
                vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addBinding(
                uint32_t             binding,
                vk::DescriptorType   type,
                vk::ShaderStageFlags stages);

            DescriptorSetLayoutBuilder &addSampler(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addCombinedImageSampler(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addSampledImage(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addStorageImage(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addUniformBuffer(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addStorageBuffer(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                uint32_t                   count,
                vk::ShaderStageFlags       stages);

            DescriptorSetLayoutBuilder &addSampler(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addCombinedImageSampler(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addSampledImage(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addStorageImage(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addUniformBuffer(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);
            DescriptorSetLayoutBuilder &addStorageBuffer(
                vk::DescriptorBindingFlags flags,
                uint32_t                   binding,
                vk::ShaderStageFlags       stages);

            DescriptorSetLayoutBuilder &addSampler(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addCombinedImageSampler(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addSampledImage(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addStorageImage(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addUniformBuffer(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addStorageBuffer(uint32_t binding, uint32_t count, vk::ShaderStageFlags stages);

            DescriptorSetLayoutBuilder &addSampler(uint32_t binding, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addCombinedImageSampler(uint32_t binding, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addSampledImage(uint32_t binding, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addStorageImage(uint32_t binding, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addUniformBuffer(uint32_t binding, vk::ShaderStageFlags stages);
            DescriptorSetLayoutBuilder &addStorageBuffer(uint32_t binding, vk::ShaderStageFlags stages);

            vk::DescriptorSetLayout build();

           private:
            vk::DescriptorSetLayoutCreateFlags m_Flags;

            std::vector<vk::DescriptorSetLayoutBinding> m_Bindings;
            std::vector<vk::DescriptorBindingFlags>     m_BindingFlags;
        };

        class DescriptorSetWriter {
           public:
            DescriptorSetWriter()  = default;
            ~DescriptorSetWriter() = default;

            void clear();

            void update(vk::DescriptorSet set);

            DescriptorSetWriter &writeSampler(
                uint32_t    binding,
                uint32_t    index,
                vk::Sampler sampler);
            DescriptorSetWriter &writeCombinedImageSampler(
                uint32_t        binding,
                uint32_t        index,
                vk::ImageView   view,
                vk::ImageLayout layout,
                vk::Sampler     sampler);
            DescriptorSetWriter &writeSampledImage(
                uint32_t        binding,
                uint32_t        index,
                vk::ImageView   view,
                vk::ImageLayout layout);
            DescriptorSetWriter &writeStorageImage(
                uint32_t        binding,
                uint32_t        index,
                vk::ImageView   view,
                vk::ImageLayout layout);
            DescriptorSetWriter &writeUniformBuffer(
                uint32_t   binding,
                uint32_t   index,
                vk::Buffer buffer,
                uint64_t   offset,
                uint64_t   size);
            DescriptorSetWriter &writeStorageBuffer(
                uint32_t   binding,
                uint32_t   index,
                vk::Buffer buffer,
                uint64_t   offset,
                uint64_t   size);

            DescriptorSetWriter &writeSampler(
                uint32_t    binding,
                vk::Sampler sampler);
            DescriptorSetWriter &writeCombinedImageSampler(
                uint32_t        binding,
                vk::ImageView   view,
                vk::ImageLayout layout,
                vk::Sampler     sampler);
            DescriptorSetWriter &writeSampledImage(
                uint32_t        binding,
                vk::ImageView   view,
                vk::ImageLayout layout);
            DescriptorSetWriter &writeStorageImage(
                uint32_t        binding,
                vk::ImageView   view,
                vk::ImageLayout layout);
            DescriptorSetWriter &writeUniformBuffer(
                uint32_t   binding,
                vk::Buffer buffer,
                uint64_t   offset,
                uint64_t   size);
            DescriptorSetWriter &writeStorageBuffer(
                uint32_t   binding,
                vk::Buffer buffer,
                uint64_t   offset,
                uint64_t   size);

           private:
            struct Write {
                uint32_t Binding;
                uint32_t Index;
                uint32_t InfoIndex;

                vk::DescriptorType Type;
            };

           private:
            std::vector<vk::DescriptorImageInfo>  m_ImageInfos;
            std::vector<vk::DescriptorBufferInfo> m_BufferInfos;

            std::vector<Write> m_ImageWrites;
            std::vector<Write> m_BufferWrites;
        };

#pragma endregion

#pragma region Pipeline

        class GraphicsPipelineBuilder {
           public:
            GraphicsPipelineBuilder();
            ~GraphicsPipelineBuilder() = default;

            void clear();

            GraphicsPipelineBuilder &setVertexShader(std::string_view entry_point, vk::ShaderModule module);
            GraphicsPipelineBuilder &setFragmentShader(std::string_view entry_point, vk::ShaderModule module);
            GraphicsPipelineBuilder &setVertexLayouts(const vk::ArrayProxy<VertexLayout> &vertex_layouts);
            GraphicsPipelineBuilder &setInputTopology(vk::PrimitiveTopology topology);
            GraphicsPipelineBuilder &setPolygonMode(vk::PolygonMode polygon_mode);
            GraphicsPipelineBuilder &setCullMode(vk::CullModeFlags cull_mode, vk::FrontFace front_face);
            GraphicsPipelineBuilder &setColorAttachmentFormats(const vk::ArrayProxy<vk::Format> &formats);
            GraphicsPipelineBuilder &setDepthAttachmentFormat(vk::Format format);
            GraphicsPipelineBuilder &setDepthTest(vk::Bool32 depth_write, vk::CompareOp op, float min_bounds = 0.0f, float max_bounds = 1.0f);
            GraphicsPipelineBuilder &setBlendingAdditive();
            GraphicsPipelineBuilder &setBlendingAlphaBlended();
            GraphicsPipelineBuilder &setNoMultisampling();
            GraphicsPipelineBuilder &setNoBlending();
            GraphicsPipelineBuilder &setNoDepthTest();
            GraphicsPipelineBuilder &setNoStencilTest();

            vk::Pipeline build(vk::PipelineLayout layout);

           private:
            struct ShaderStageInfo {
                std::string             EntryPoint;
                vk::ShaderModule        Module;
                vk::ShaderStageFlagBits Stage;
            };

           private:
            std::vector<ShaderStageInfo> m_ShaderStages;

            std::vector<vk::VertexInputBindingDescription>   m_VertexInputBindingDescriptions;
            std::vector<vk::VertexInputAttributeDescription> m_VertexInputAttributeDescriptions;

            vk::PipelineInputAssemblyStateCreateInfo m_InputAssembly;
            vk::PipelineRasterizationStateCreateInfo m_Rasterization;
            vk::PipelineMultisampleStateCreateInfo   m_Multisample;
            vk::PipelineDepthStencilStateCreateInfo  m_DepthStencil;
            vk::PipelineRenderingCreateInfo          m_Rendering;

            vk::PipelineColorBlendAttachmentState m_ColorBlendAttachment;

            std::vector<vk::Format> m_ColorAttachmentFormats;
        };

#pragma endregion

       public:
        static Vulkan &GetRef();

        static vk::Instance GetInstance();

        static vk::detail::DispatchLoaderDynamic GetDynamicLoader();

        static vk::DebugUtilsMessengerEXT GetDebugMessenger();

        static vk::SurfaceKHR     GetSurface();
        static vk::PhysicalDevice GetPhysicalDevice();

        static vk::Device GetDevice();
        static uint32_t   GetQueueFamilyIndex();
        static uint32_t   GetQueueFamilyQueueCount();
        static vk::Queue  GetGraphicsQueue();
        static vk::Queue  GetComputeQueue();
        static vk::Queue  GetPresentQueue();

        static uint32_t GetSwapchainMinImageCount();

        static vk::SurfaceFormatKHR GetSwapchainFormat();
        static vk::PresentModeKHR   GetSwapchainPresentMode();

        static vk::SwapchainKHR    GetSwapchain();
        static vk::Extent2D        GetSwapchainExtent();
        static vk::ImageUsageFlags GetSwapchainUsageFlags();

        static uint32_t GetSwapchainImageCount();

        static vk::Image     GetSwapchainImage(uint32_t index);
        static vk::ImageView GetSwapchainImageView(uint32_t index);

        static vma::Allocator GetVmaAllocator();

        static vk::Format &GetSwapchainFormatRef();

        static const vk::Format &GetSwapchainFormatConstRef();

        static void ImmediateSubmit(fu2::function<void(vk::CommandBuffer)> fn);

        static void WaitDeviceIdle();

        static void CopyMemoryToAllocation(const void *src, vma::Allocation dst, uint64_t dst_offset, uint64_t size);
        static void CopyAllocationToMemory(vma::Allocation src, uint64_t src_offset, void *dst, uint64_t size);

        static void InvalidateAllocation(const vma::Allocation &allocation, uint64_t offset, uint64_t size);
        static void FlushAllocation(const vma::Allocation &allocation, uint64_t offset, uint64_t size);

        static vma::AllocationInfo GetAllocationInfo(const vma::Allocation &allocation);

#pragma region Buffer
        static void DestroyBuffer(const Buffer &buffer);

        static Buffer AllocateBuffer(
            vma::AllocationCreateFlags allocation_flags,
            vma::MemoryUsage           memory_usage,
            vk::BufferCreateFlags      buffer_flags,
            uint64_t                   size,
            vk::BufferUsageFlags       usage_flags);
#pragma endregion

#pragma region Command
        static void DestroyCommandPool(vk::CommandPool command_pool);

        static vk::CommandPool CreateCommandPool(vk::CommandPoolCreateFlags flags);

        static std::vector<vk::CommandBuffer> AllocatePrimaryCommandBuffers(
            uint32_t        count,
            vk::CommandPool command_pool);

        static vk::CommandBuffer AllocatePrimaryCommandBuffer(vk::CommandPool command_pool);

        static void ResetCommandBuffer(vk::CommandBuffer command_buffer);

        static void BeginDebugUtilsLabel(
            vk::CommandBuffer           command_buffer,
            std::string_view            label,
            const std::array<float, 4> &color);
        static void EndDebugUtilsLabel(vk::CommandBuffer command_buffer);

        static void BeginCommandBuffer(vk::CommandBufferUsageFlags flags, vk::CommandBuffer command_buffer);
        static void EndCommandBuffer(vk::CommandBuffer command_buffer);

        static void BlitImageToImage(
            vk::Image           src_image,
            vk::Image           dst_image,
            const vk::Offset3D &src_offset,
            const vk::Offset3D &dst_offset,
            const vk::Extent3D &src_extent,
            const vk::Extent3D &dst_extent,
            vk::CommandBuffer   command_buffer);

        static void CopyImageToImage(
            vk::Image           src_image,
            vk::Image           dst_image,
            const vk::Offset3D &src_offset,
            const vk::Offset3D &dst_offset,
            const vk::Extent3D &extent,
            vk::CommandBuffer   command_buffer);
        static void CopyBufferToBuffer(
            vk::Buffer        src_buffer,
            vk::Buffer        dst_buffer,
            uint64_t          src_offset,
            uint64_t          dst_offset,
            uint64_t          size,
            vk::CommandBuffer command_buffer);
        static void CopyBufferToImage(
            vk::Buffer          src_buffer,
            vk::Image           dst_image,
            uint64_t            src_offset,
            const vk::Offset3D &dst_offset,
            const vk::Extent2D &src_extent,
            const vk::Extent3D &dst_extent,
            vk::CommandBuffer   command_buffer);
        static void CopyImageToBuffer(
            vk::Image           src_image,
            vk::Buffer          dst_buffer,
            const vk::Offset3D &src_offset,
            uint64_t            dst_offset,
            const vk::Extent3D &src_extent,
            const vk::Extent2D &dst_extent,
            vk::CommandBuffer   command_buffer);

        static vk::CommandBufferSubmitInfo GetCommandBufferSubmitInfo(vk::CommandBuffer command_buffer);

#pragma endregion

#pragma region Descriptor
        static void DestroyDescriptorPool(vk::DescriptorPool descriptor_pool);
        static void DestroyDescriptorSetLayout(vk::DescriptorSetLayout layout);

        static vk::DescriptorPool CreateDescriptorPool(
            vk::DescriptorPoolCreateFlags                 flags,
            uint32_t                                      max_sets,
            const vk::ArrayProxy<vk::DescriptorPoolSize> &pool_sizes);

        static std::vector<vk::DescriptorSet> AllocateDescriptorSets(
            const vk::ArrayProxy<uint32_t>                &runtime_sizes,
            const vk::ArrayProxy<vk::DescriptorSetLayout> &layouts,
            vk::DescriptorPool                             pool);
        static vk::DescriptorSet AllocateDescriptorSet(
            uint32_t                runtime_size,
            vk::DescriptorSetLayout layout,
            vk::DescriptorPool      pool);

        static std::vector<vk::DescriptorSet> AllocateDescriptorSets(
            const vk::ArrayProxy<vk::DescriptorSetLayout> &layouts,
            vk::DescriptorPool                             pool);
        static vk::DescriptorSet AllocateDescriptorSet(
            vk::DescriptorSetLayout layout,
            vk::DescriptorPool      pool);
#pragma endregion

#pragma region Image
        static void DestroyImage(const Image &image);
        static void DestroyImageView(vk::ImageView view);

        static Image AllocateImage3D(
            vma::AllocationCreateFlags allocation_flags,
            vma::MemoryUsage           memory_usage,
            vk::ImageCreateFlagBits    image_flags,
            vk::Format                 format,
            vk::ImageUsageFlags        usage_flags,
            const vk::Extent3D        &extent);

        static Image AllocateImage2D(
            vma::AllocationCreateFlags allocation_flags,
            vma::MemoryUsage           memory_usage,
            vk::ImageCreateFlagBits    image_flags,
            vk::Format                 format,
            vk::ImageUsageFlags        usage_flags,
            const vk::Extent2D        &extent);

        static vk::ImageView CreateImageColorView3D(vk::Image image, vk::Format format);
        static vk::ImageView CreateImageDepthView3D(vk::Image image, vk::Format format);

        static vk::ImageView CreateImageColorView2D(vk::Image image, vk::Format format);
        static vk::ImageView CreateImageDepthView2D(vk::Image image, vk::Format format);

        static vk::ImageAspectFlags GetImageAspectMask(vk::ImageLayout layout);

#pragma endregion

#pragma region Pipeline
        static void DestroyPipelineLayout(vk::PipelineLayout layout);
        static void DestroyPipeline(vk::Pipeline pipeline);

        static vk::PipelineLayout CreatePipelineLayout(
            const vk::ArrayProxy<vk::PushConstantRange>   &push_constants,
            const vk::ArrayProxy<vk::DescriptorSetLayout> &set_layouts);

        static vk::Pipeline CreateComputePipeline(
            vk::PipelineCreateFlags flags,
            std::string_view        entry,
            vk::ShaderModule        shader_module,
            vk::PipelineLayout      layout);
#pragma endregion

#pragma region Queue
        static vk::Result Present(
            const vk::ArrayProxy<vk::Semaphore> &wait_semaphores,

            uint32_t image_index);

        static void Submit(
            const vk::ArrayProxy<vk::CommandBufferSubmitInfo> &command_buffer_infos,
            const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &wait_semaphore_infos,
            const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &signal_semaphore_infos,

            vk::Fence fence,
            vk::Queue queue);
#pragma endregion

#pragma region RenderPass
        static void BeginRenderPass(
            const vk::Extent2D                                &extent,
            const vk::ArrayProxy<vk::RenderingAttachmentInfo> &color_attachments,
            const std::optional<vk::RenderingAttachmentInfo>  &depth_attachment_opt,
            vk::CommandBuffer                                  command_buffer);
        static void EndRenderPass(vk::CommandBuffer command_buffer);

        static vk::RenderingAttachmentInfo GetRenderingAttachmentInfo(
            vk::ImageView         view,
            vk::ImageLayout       layout,
            vk::AttachmentLoadOp  load_op,
            vk::AttachmentStoreOp store_op,
            const vk::ClearValue &clear_value);
#pragma endregion

#pragma region Sampler
        static void DestroySampler(vk::Sampler sampler);

        static vk::Sampler CreateSampler(const vk::SamplerCreateInfo &create_info);
#pragma endregion

#pragma region Shader
        static void DestroyShaderModule(vk::ShaderModule shader_module);

        static vk::ShaderModule CreateShaderModuleFromSPV(std::span<uint32_t> code);
#pragma endregion

#pragma region Synchronization
        static void DestroyFence(vk::Fence fence);
        static void DestroySemaphore(vk::Semaphore semaphore);

        static vk::Fence CreateFence(vk::FenceCreateFlags flags);

        static vk::Semaphore CreateSemaphore();
        static vk::Semaphore CreateTimelineSemaphore(uint64_t initial_value);

        static void ResetFences(const vk::ArrayProxy<vk::Fence> &fences);
        static void WaitForAllFences(const vk::ArrayProxy<vk::Fence> &fences);

        static vk::SemaphoreSubmitInfo GetSemaphoreSubmitInfo(
            vk::PipelineStageFlags2 stages,
            vk::Semaphore           semaphore);

        static vk::SemaphoreSubmitInfo GetTimelineSemaphoreSubmitInfo(
            uint64_t                wait_value,
            vk::PipelineStageFlags2 stages,
            vk::Semaphore           semaphore);

        static vk::SemaphoreWaitInfo GetTimelineSemaphoreWaitInfo(
            const vk::ArrayProxyNoTemporaries<vk::Semaphore> &semaphores,
            const vk::ArrayProxyNoTemporaries<uint64_t>      &wait_values);
#pragma endregion

       public:
        Vulkan()  = default;
        ~Vulkan() = default;

        void initialize(const Settings &settings);
        void shutdown();

       private:
        IGNIS_IF_DEBUG(class State {
           public:
            ~State();
        });

       private:
        static bool CheckInstanceLayerSupport(const std::vector<const char *> &required_instance_layers);
        static bool CheckPhysicalDeviceSwapchainSupport(
            vk::PhysicalDevice  physical_device,
            vk::SurfaceKHR      surface,
            vk::ImageUsageFlags required_usage_flags);

        static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugUtilsMessengerCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
            vk::DebugUtilsMessengerCreateFlagsEXT         type,
            const vk::DebugUtilsMessengerCallbackDataEXT *p_callback_data,
            void                                         *p_userdata);

        static vk::ResultValue<uint32_t> AcquireNextImage(vk::Semaphore semaphore);

       private:
        void resizeSwapchain(uint32_t width, uint32_t height);
        void createSwapchain(uint32_t width, uint32_t height);
        void destroySwapchain();

        void selectPhysicalDevice(vk::ImageUsageFlags required_usage_flags);
        void selectQueueFamilyIndex();
        void selectSwapchainImageCount(uint32_t preferred_image_count);
        void selectSwapchainFormat(std::span<const vk::Format> preferred_formats);
        void selectSwapchainPresentMode(std::span<const vk::PresentModeKHR> preferred_present_modes);

       private:
        vk::Instance m_Instance;

        vk::detail::DispatchLoaderDynamic m_DynamicLoader;

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

        vk::CommandPool   m_ImmCommandPool;
        vk::CommandBuffer m_ImmCommandBuffer;
        vk::Fence         m_ImmFence;

       private:
        static Vulkan *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State);

       private:
        friend class Frame;
        friend class Engine;
    };
}  // namespace Ignis