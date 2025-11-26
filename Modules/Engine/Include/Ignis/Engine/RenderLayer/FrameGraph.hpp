#pragma once

#include <Ignis/Engine/VulkanLayer.hpp>

namespace Ignis {
    class FrameGraph {
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

        class Builder;

        class RenderPass {
           public:
            ~RenderPass() = default;

            RenderPass &read_images(const vk::ArrayProxy<ImageInfo> &images);
            RenderPass &write_images(const vk::ArrayProxy<ImageInfo> &images);

            RenderPass &read_buffers(const vk::ArrayProxy<BufferInfo> &buffers);
            RenderPass &write_buffers(const vk::ArrayProxy<BufferInfo> &buffers);

            RenderPass &set_color_attachment(const Attachment &attachment);
            RenderPass &set_depth_attachment(const Attachment &attachment);

            RenderPass &execute(const fu2::function<void(vk::CommandBuffer command_buffer)> &execute_fn);

           private:
            explicit RenderPass(std::string_view name);

           private:
            std::string m_Name;

            gtl::vector<ImageInfo>  m_ReadImages;
            gtl::vector<ImageInfo>  m_WriteImages;
            gtl::vector<BufferInfo> m_ReadBuffers;
            gtl::vector<BufferInfo> m_WriteBuffers;

            std::optional<Attachment> m_ColorAttachment;
            std::optional<Attachment> m_DepthAttachment;

            fu2::function<void(vk::CommandBuffer command_buffer)> m_ExecuteFn;

           private:
            friend class Builder;
        };

        class Builder {
           public:
            Builder()  = default;
            ~Builder() = default;

            void clear();

            ImageID import_image(
                vk::Image           image,
                vk::ImageView       image_view,
                const vk::Extent3D &extent,
                vk::ImageLayout     current_layout,
                vk::ImageLayout     final_layout);
            BufferID import_buffer(vk::Buffer buffer, uint64_t offset, uint64_t size);

            RenderPass &add_render_pass(std::string_view pass_name);

            FrameGraph build();

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
            gtl::vector<RenderPass> m_RenderPasses;

            ImageID  m_NextImageID  = 0;
            BufferID m_NextBufferID = 0;

            gtl::flat_hash_map<ImageID, ImageState>   m_ImageStates;
            gtl::flat_hash_map<BufferID, BufferState> m_BufferStates;

            gtl::flat_hash_map<ImageID, vk::ImageLayout> m_FinalImageLayouts;

           private:
            friend class RenderPass;
        };

       public:
        ~FrameGraph() = default;

        void clear();
        void execute(vk::CommandBuffer command_buffer);

       private:
        struct Pass {
            Vulkan::CommandBuffer::BarrierMerger MemoryBarriers;

            fu2::function<void(vk::CommandBuffer command_buffer)> ExecuteFn;
        };

       private:
        FrameGraph(std::span<Pass> passes, const Vulkan::CommandBuffer::BarrierMerger &final_barriers);

       private:
        gtl::vector<Pass> m_Passes;

        Vulkan::CommandBuffer::BarrierMerger m_FinalBarriers;

       private:
        friend class Builder;
    };
}  // namespace Ignis