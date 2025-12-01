#pragma once

#include <Ignis/Vulkan.hpp>

namespace Ignis {
    class FrameGraph final {
       public:
        typedef uint32_t ImageID;
        typedef uint32_t BufferID;

        static constexpr ImageID  INVALID_IMAGE_ID  = UINT32_MAX;
        static constexpr BufferID INVALID_BUFFER_ID = UINT32_MAX;

       public:
        struct ImageInfo {
            ImageID Image;

            vk::PipelineStageFlags2 StageMask;
        };

        struct BufferInfo {
            BufferID Buffer;
            uint64_t Offset;
            uint64_t Size;

            vk::PipelineStageFlags2 StageMask;
        };

        struct Attachment {
            ImageID Image;

            vk::ClearValue ClearValue = {};

            vk::AttachmentLoadOp  LoadOp  = vk::AttachmentLoadOp::eLoad;
            vk::AttachmentStoreOp StoreOp = vk::AttachmentStoreOp::eStore;
        };

        class RenderPass {
           public:
            typedef fu2::function<void(vk::CommandBuffer command_buffer)> ExecuteFn;

           public:
            explicit RenderPass(
                std::string_view            label,
                const std::array<float, 4> &label_color = {0.0f, 0.0f, 0.0f, 1.0f});
            ~RenderPass() = default;

            RenderPass &readImages(const vk::ArrayProxy<ImageInfo> &images);
            RenderPass &writeImages(const vk::ArrayProxy<ImageInfo> &images);

            RenderPass &readBuffers(const vk::ArrayProxy<BufferInfo> &buffers);
            RenderPass &writeBuffers(const vk::ArrayProxy<BufferInfo> &buffers);

            RenderPass &setColorAttachments(const vk::ArrayProxy<Attachment> &attachments);
            RenderPass &setDepthAttachment(const Attachment &attachment);

            RenderPass &setExecute(const ExecuteFn &execute_fn);

           private:
            std::string m_Label;

            std::array<float, 4> m_LabelColor;

            gtl::vector<ImageInfo>  m_ReadImages;
            gtl::vector<ImageInfo>  m_WriteImages;
            gtl::vector<BufferInfo> m_ReadBuffers;
            gtl::vector<BufferInfo> m_WriteBuffers;

            gtl::vector<Attachment>   m_ColorAttachments;
            std::optional<Attachment> m_DepthAttachment;

            ExecuteFn m_ExecuteFn;

           private:
            friend class FrameGraph;
        };

       public:
        ~FrameGraph() = default;

        ImageID importImage3D(
            vk::Image           image,
            vk::ImageView       image_view,
            const vk::Extent3D &extent,
            vk::ImageLayout     current_layout,
            vk::ImageLayout     final_layout);
        ImageID importImage2D(
            vk::Image           image,
            vk::ImageView       image_view,
            const vk::Extent2D &extent,
            vk::ImageLayout     current_layout,
            vk::ImageLayout     final_layout);
        BufferID importBuffer(vk::Buffer buffer, uint64_t offset, uint64_t size);

        vk::Image     getImage(ImageID id) const;
        vk::ImageView getImageView(ImageID id) const;
        vk::Extent3D  getImageExtent(ImageID id) const;

        vk::Buffer getBuffer(BufferID id) const;

        ImageID getSwapchainImageID() const;

        void addRenderPass(RenderPass &&render_pass);

       private:
        struct ExecutorPass {
            Vulkan::BarrierMerger MemoryBarriers;
            RenderPass::ExecuteFn ExecuteFn;
        };

        struct Executor {
            gtl::vector<ExecutorPass> Passes;
            Vulkan::BarrierMerger     FinalBarriers;
        };

       private:
        static void Execute(Executor &&executor, vk::CommandBuffer command_buffer);

       private:
        FrameGraph(
            vk::Image           swapchain_image,
            vk::ImageView       swapchain_image_view,
            const vk::Extent2D &swapchain_extent);

        Executor build();

       private:
        struct ImageState {
            vk::Image       Image;
            vk::ImageView   View;
            vk::ImageLayout Layout;
            vk::Extent3D    Extent;
        };

        struct BufferState {
            vk::Buffer Buffer;
            uint64_t   Offset;
            uint64_t   Size;
        };

       private:
        ImageID m_SwapchainImageID;

        gtl::vector<RenderPass> m_RenderPasses;

        ImageID  m_NextImageID  = 0;
        BufferID m_NextBufferID = 0;

        gtl::flat_hash_map<ImageID, ImageState>   m_ImageStates;
        gtl::flat_hash_map<BufferID, BufferState> m_BufferStates;

        IGNIS_IF_DEBUG(
            gtl::flat_hash_set<VkImage>     m_ImageSet;
            gtl::flat_hash_set<VkImageView> m_ViewSet;
            gtl::flat_hash_set<VkBuffer>    m_BufferSet;)

        gtl::flat_hash_map<ImageID, vk::ImageLayout> m_FinalImageLayouts;

       private:
        friend class Render;
        friend class Engine;
    };
}  // namespace Ignis