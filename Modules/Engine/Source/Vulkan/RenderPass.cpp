#include <Ignis/Vulkan.hpp>

namespace Ignis {
    void Vulkan::BeginRenderPass(
        const vk::Extent2D                                &extent,
        const vk::ArrayProxy<vk::RenderingAttachmentInfo> &color_attachments,
        const std::optional<vk::RenderingAttachmentInfo>  &depth_attachment_opt,
        const vk::CommandBuffer                            command_buffer) {
        vk::RenderingInfo rendering_info{};
        rendering_info
            .setRenderArea(vk::Rect2D{vk::Offset2D{0, 0}, extent})
            .setLayerCount(1)
            .setViewMask(0)
            .setColorAttachments(color_attachments);
        if (depth_attachment_opt.has_value()) {
            rendering_info.setPDepthAttachment(&depth_attachment_opt.value());
        }
        command_buffer.beginRendering(rendering_info);
    }

    void Vulkan::EndRenderPass(const vk::CommandBuffer command_buffer) {
        command_buffer.endRendering();
    }

    vk::RenderingAttachmentInfo Vulkan::GetRenderingAttachmentInfo(
        const vk::ImageView         view,
        const vk::ImageLayout       layout,
        const vk::AttachmentLoadOp  load_op,
        const vk::AttachmentStoreOp store_op,
        const vk::ClearValue       &clear_value) {
        return vk::RenderingAttachmentInfo{}
            .setImageView(view)
            .setImageLayout(layout)
            .setLoadOp(load_op)
            .setStoreOp(store_op)
            .setResolveMode(vk::ResolveModeFlagBits::eNone)
            .setClearValue(clear_value);
    }
}  // namespace Ignis