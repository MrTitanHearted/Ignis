#pragma once

#include <Ignis/Engine/VulkanLayer/Types.hpp>

namespace Ignis::Vulkan::RenderPass {
    vk::RenderingAttachmentInfo GetAttachmentInfo(
        vk::ImageView         target_view,
        vk::ImageLayout       target_layout,
        vk::AttachmentLoadOp  load_op,
        vk::AttachmentStoreOp store_op,
        const vk::ClearValue &clear_value);

    vk::RenderingAttachmentInfo GetAttachmentInfo(
        vk::ImageView         target_view,
        vk::ImageLayout       target_layout,
        const vk::ClearValue &clear_value);
    vk::RenderingAttachmentInfo GetAttachmentInfo(
        vk::ImageView   target_view,
        vk::ImageLayout target_layout);

    void Begin(
        const vk::Extent2D                                &extent,
        const vk::ArrayProxy<vk::RenderingAttachmentInfo> &color_attachments,
        const vk::RenderingAttachmentInfo                 &depth_stencil_attachment,
        vk::CommandBuffer                                  command_buffer);
    void Begin(
        const vk::Extent2D                                &extent,
        const vk::ArrayProxy<vk::RenderingAttachmentInfo> &color_attachments,
        vk::CommandBuffer                                  command_buffer);

    void End(vk::CommandBuffer command_buffer);

}  // namespace Ignis::Vulkan::RenderPass