#include <Ignis/Engine/VulkanLayer/RenderPass.hpp>

namespace Ignis::Vulkan::RenderPass {

    vk::RenderingAttachmentInfo GetAttachmentInfo(
        const vk::ImageView         target_view,
        const vk::ImageLayout       target_layout,
        const vk::AttachmentLoadOp  load_op,
        const vk::AttachmentStoreOp store_op,
        const vk::ClearValue       &clear_value) {
        return vk::RenderingAttachmentInfo{}
            .setImageView(target_view)
            .setImageLayout(target_layout)
            .setLoadOp(load_op)
            .setStoreOp(store_op)
            .setResolveMode(vk::ResolveModeFlagBits::eNone)
            .setClearValue(clear_value);
    }

    vk::RenderingAttachmentInfo GetAttachmentInfo(
        const vk::ImageView   target_view,
        const vk::ImageLayout target_layout,
        const vk::ClearValue &clear_value) {
        return vk::RenderingAttachmentInfo{}
            .setImageView(target_view)
            .setImageLayout(target_layout)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setResolveMode(vk::ResolveModeFlagBits::eNone)
            .setClearValue(clear_value);
    }

    vk::RenderingAttachmentInfo GetAttachmentInfo(
        const vk::ImageView   target_view,
        const vk::ImageLayout target_layout) {
        return vk::RenderingAttachmentInfo{}
            .setImageView(target_view)
            .setImageLayout(target_layout)
            .setLoadOp(vk::AttachmentLoadOp::eLoad)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setResolveMode(vk::ResolveModeFlagBits::eNone);
    }

    void Begin(
        const vk::Extent2D                                &extent,
        const vk::ArrayProxy<vk::RenderingAttachmentInfo> &color_attachments,
        const vk::RenderingAttachmentInfo                 &depth_stencil_attachment,
        const vk::CommandBuffer                            command_buffer) {
        vk::RenderingInfo vk_rendering_info{};
        vk_rendering_info
            .setRenderArea(vk::Rect2D{vk::Offset2D{}, extent})
            .setLayerCount(1)
            .setViewMask(0)
            .setColorAttachments(color_attachments)
            .setPDepthAttachment(&depth_stencil_attachment);

        command_buffer.beginRendering(vk_rendering_info);
    }

    void Begin(
        const vk::Extent2D                                &extent,
        const vk::ArrayProxy<vk::RenderingAttachmentInfo> &color_attachments,
        const vk::CommandBuffer                            command_buffer) {
        vk::RenderingInfo vk_rendering_info{};
        vk_rendering_info
            .setRenderArea(vk::Rect2D{vk::Offset2D{}, extent})
            .setLayerCount(1)
            .setViewMask(0)
            .setColorAttachments(color_attachments);

        command_buffer.beginRendering(vk_rendering_info);
    }

    void End(const vk::CommandBuffer command_buffer) {
        command_buffer.endRendering();
    }

}  // namespace Ignis::Vulkan::RenderPass