#pragma once

#include <Ignis/Vulkan.hpp>

namespace Ignis {
    class FrameGraph final {
       public:
        typedef uint32_t ImageID;
        typedef uint32_t BufferID;

        typedef fu2::function<void(vk::CommandBuffer)> ExecuteFn;

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
            explicit RenderPass(
                std::string_view            label,
                const std::array<float, 4> &label_color = {0.0f, 0.0f, 0.0f, 1.0f});
            ~RenderPass() = default;

            RenderPass &readImages(const vk::ArrayProxy<ImageID> &images);
            RenderPass &readBuffers(const vk::ArrayProxy<BufferInfo> &buffers);

            RenderPass &setColorAttachments(const vk::ArrayProxy<Attachment> &attachments);
            RenderPass &setDepthAttachment(const std::optional<Attachment> &attachment);

            RenderPass &setExecute(const ExecuteFn &execute_fn);

           private:
            std::string m_Label;

            std::array<float, 4> m_LabelColor;

            std::vector<ImageID>    m_ReadImages;
            std::vector<BufferInfo> m_ReadBuffers;

            std::vector<Attachment>   m_ColorAttachments;
            std::optional<Attachment> m_DepthAttachment;

            ExecuteFn m_ExecuteFn;

           private:
            friend class FrameGraph;
        };

        class ComputePass {
           public:
            explicit ComputePass(
                std::string_view            label,
                const std::array<float, 4> &label_color = {0.0f, 0.0f, 0.0f, 1.0f});
            ~ComputePass() = default;

            ComputePass &readImage(const ImageInfo &info);
            ComputePass &writeImage(const ImageInfo &info);
            ComputePass &readBuffer(const BufferInfo &info);
            ComputePass &writeBuffer(const BufferInfo &info);

            ComputePass &setExecute(const ExecuteFn &execute_fn);

           private:
            enum class AccessType {
                eRead,
                eWrite,
            };

            struct BarrierInfo {
                AccessType Type;
                uint32_t   Index;
            };

           private:
            std::string m_Label;

            std::array<float, 4> m_LabelColor;

            std::vector<ImageInfo>   m_ReadImages;
            std::vector<ImageInfo>   m_WriteImages;
            std::vector<BarrierInfo> m_ImageBarriers;

            std::vector<BufferInfo>  m_ReadBuffers;
            std::vector<BufferInfo>  m_WriteBuffers;
            std::vector<BarrierInfo> m_BufferBarriers;

            ExecuteFn m_ExecuteFn;

           private:
            friend class FrameGraph;
        };

       public:
        static bool IsWriteAccess(vk::AccessFlags2 access);
        static bool NeedsMemoryBarrier(vk::AccessFlags2 src, vk::AccessFlags2 dst);
        static bool NeedsImageBarrier(
            vk::AccessFlags2 src_access,
            vk::AccessFlags2 dst_access,
            vk::ImageLayout  src_layout,
            vk::ImageLayout  dst_layout);
        static bool NeedsBufferBarrier(vk::AccessFlags2 src, vk::AccessFlags2 dst);

        static vk::AccessFlags2 GetImageReadAccess(vk::ImageUsageFlags usage, vk::PipelineStageFlags2 stages);
        static vk::AccessFlags2 GetImageWriteAccess(vk::ImageUsageFlags usage, vk::PipelineStageFlags2 stages);
        static vk::AccessFlags2 GetBufferReadAccess(vk::BufferUsageFlags usage, vk::PipelineStageFlags2 stages);
        static vk::AccessFlags2 GetBufferWriteAccess(vk::BufferUsageFlags usage, vk::PipelineStageFlags2 stages);

       public:
        ~FrameGraph() = default;

        void removeImage(ImageID image);
        void removeBuffer(BufferID buffer);

        ImageID importImage(
            vk::Image           image,
            vk::ImageView       image_view,
            vk::Format          format,
            vk::ImageUsageFlags usage,
            const vk::Extent3D &extent,
            vk::ImageLayout     current_layout,
            vk::ImageLayout     final_layout);
        ImageID importImage(
            vk::Image           image,
            vk::ImageView       image_view,
            vk::Format          format,
            vk::ImageUsageFlags usage,
            const vk::Extent2D &extent,
            vk::ImageLayout     current_layout,
            vk::ImageLayout     final_layout);
        BufferID importBuffer(
            vk::Buffer           buffer,
            vk::BufferUsageFlags usage,
            uint64_t             offset,
            uint64_t             size);

        ImageID getImageID(vk::Image image) const;
        ImageID getImageID(vk::ImageView view) const;

        BufferID getBufferID(vk::Buffer buffer) const;

        vk::Image           getImage(ImageID id) const;
        vk::ImageView       getImageView(ImageID id) const;
        vk::Format          getImageFormat(ImageID id) const;
        vk::ImageUsageFlags getImageUsage(ImageID id) const;
        vk::Extent3D        getImageExtent(ImageID id) const;

        vk::Buffer           getBuffer(BufferID id) const;
        uint64_t             getBufferOffset(BufferID id) const;
        uint64_t             getBufferSize(BufferID id) const;
        vk::BufferUsageFlags getBufferUsage(BufferID id) const;

        ImageID getSwapchainImageID() const;

        void addRenderPass(const RenderPass &render_pass);
        void addComputePass(const ComputePass &compute_pass);

       private:
        struct ImageState {
            vk::Image       Handle;
            vk::ImageView   View;
            vk::Format      Format;
            vk::ImageLayout Layout;
            vk::Extent3D    Extent;

            vk::ImageUsageFlags Usage;
        };

        struct BufferState {
            vk::Buffer Handle;
            uint64_t   Offset;
            uint64_t   Size;

            vk::BufferUsageFlags Usage;
        };

        enum class PassType {
            eRender,
            eCompute,
        };

        struct PassIndex {
            PassType Type;
            uint32_t Index;
        };

        struct ExecutorPass {
            Vulkan::BarrierMerger MemoryBarriers;

            ExecuteFn Execute;
        };

        struct Executor {
            std::vector<ExecutorPass> Passes;
            Vulkan::BarrierMerger     FinalBarriers;
        };

        struct ResourceTracker {
            gtl::flat_hash_map<ImageID, vk::ImageLayout>         LastImageLayout;
            gtl::flat_hash_map<ImageID, vk::PipelineStageFlags2> LastImageStages;
            gtl::flat_hash_map<ImageID, vk::AccessFlags2>        LastImageAccess;

            gtl::flat_hash_map<BufferID, vk::PipelineStageFlags2> LastBufferStages;
            gtl::flat_hash_map<BufferID, vk::AccessFlags2>        LastBufferAccess;
        };

       private:
        static void Execute(Executor &&executor, vk::CommandBuffer command_buffer);

       private:
        FrameGraph();

        void clear();

        void beginFrame(
            vk::Image           swapchain_image,
            vk::ImageView       swapchain_view,
            vk::Format          swapchain_format,
            vk::ImageUsageFlags swapchain_usage,
            const vk::Extent2D &swapchain_extent);

        Executor endFrame();

        ExecutorPass buildRenderPass(ResourceTracker &resource_tracker, const RenderPass &render_pass);
        ExecutorPass buildComputePass(ResourceTracker &resource_tracker, const ComputePass &compute_pass);

       private:
        ImageID m_SwapchainImageID;

        std::vector<RenderPass>  m_RenderPasses;
        std::vector<ComputePass> m_ComputePasses;

        std::vector<PassIndex> m_PassIndices;

        ImageID  m_NextImageID  = 0;
        BufferID m_NextBufferID = 0;

        SparseVector<ImageID, ImageState>   m_ImageStates;
        SparseVector<BufferID, BufferState> m_BufferStates;

        std::vector<ImageID>  m_FreeImageIDs;
        std::vector<BufferID> m_FreeBufferIDs;

        gtl::node_hash_map<VkImage, ImageID>     m_ImageMap;
        gtl::node_hash_map<VkImageView, ImageID> m_ViewMap;
        gtl::node_hash_map<VkBuffer, BufferID>   m_BufferMap;

        gtl::flat_hash_map<ImageID, vk::ImageLayout> m_FinalImageLayouts;

       private:
        friend class Render;
        friend class Engine;
    };
}  // namespace Ignis