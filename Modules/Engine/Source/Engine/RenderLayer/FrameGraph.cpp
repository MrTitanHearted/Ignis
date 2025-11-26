#include <Ignis/Engine/RenderLayer/FrameGraph.hpp>

namespace Ignis {
    FrameGraph::RenderPass &FrameGraph::RenderPass::read_images(const vk::ArrayProxy<ImageInfo> &images) {
        m_ReadImages.clear();
        m_ReadImages.insert(
            std::begin(m_ReadImages),
            std::begin(images),
            std::end(images));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::write_images(const vk::ArrayProxy<ImageInfo> &images) {
        m_WriteImages.clear();
        m_WriteImages.insert(
            std::begin(m_WriteImages),
            std::begin(images),
            std::end(images));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::read_buffers(const vk::ArrayProxy<BufferInfo> &buffers) {
        m_ReadBuffers.clear();
        m_ReadBuffers.insert(
            std::begin(m_ReadBuffers),
            std::begin(buffers),
            std::end(buffers));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::write_buffers(const vk::ArrayProxy<BufferInfo> &buffers) {
        m_WriteBuffers.clear();
        m_WriteBuffers.insert(
            std::begin(m_WriteBuffers),
            std::begin(buffers),
            std::end(buffers));

        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::set_color_attachment(const Attachment &attachment) {
        m_ColorAttachment = attachment;
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::set_depth_attachment(const Attachment &attachment) {
        m_DepthAttachment = attachment;
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::execute(
        const fu2::function<void(vk::CommandBuffer command_buffer)> &execute_fn) {
        m_ExecuteFn = execute_fn;
        return *this;
    }

    FrameGraph::RenderPass::RenderPass(const std::string_view name) {
        m_Name = name;
    }

    void FrameGraph::Builder::clear() {
        m_ImageStates.clear();
        m_BufferStates.clear();
        m_NextImageID  = 0;
        m_NextBufferID = 0;
        m_RenderPasses.clear();
    }

    FrameGraph::ImageID FrameGraph::Builder::import_image(
        const vk::Image       image,
        const vk::ImageView   image_view,
        const vk::Extent3D   &extent,
        const vk::ImageLayout current_layout,
        const vk::ImageLayout final_layout) {
        const ImageID image_id = m_NextImageID++;

        ImageState image_state{};
        image_state.Image  = image;
        image_state.View   = image_view;
        image_state.Extent = extent;
        image_state.Layout = current_layout;

        m_ImageStates[image_id] = image_state;

        m_FinalImageLayouts[image_id] = final_layout;

        return image_id;
    }

    FrameGraph::BufferID FrameGraph::Builder::import_buffer(
        const vk::Buffer buffer,
        const uint64_t   offset,
        const uint64_t   size) {
        const BufferID buffer_id = m_NextBufferID++;

        BufferState buffer_state{};
        buffer_state.Buffer = buffer;
        buffer_state.Offset = offset;
        buffer_state.Size   = size;

        m_BufferStates[buffer_id] = buffer_state;
        return buffer_id;
    }

    FrameGraph::RenderPass &FrameGraph::Builder::add_render_pass(const std::string_view pass_name) {
        m_RenderPasses.push_back(RenderPass(pass_name));
        return m_RenderPasses.back();
    }

    FrameGraph FrameGraph::Builder::build() {
        gtl::vector<Pass> passes{};
        passes.reserve(m_RenderPasses.size());

        gtl::flat_hash_map<ImageID, vk::ImageLayout>         last_image_layouts;
        gtl::flat_hash_map<ImageID, vk::PipelineStageFlags2> last_image_stages;
        gtl::flat_hash_map<ImageID, vk::AccessFlags2>        last_image_access;

        gtl::flat_hash_map<BufferID, vk::PipelineStageFlags2> last_buffer_stages;
        gtl::flat_hash_map<BufferID, vk::AccessFlags2>        last_buffer_access;

        for (const auto &[id, state] : m_ImageStates) {
            last_image_layouts[id] = state.Layout;
            last_image_stages[id]  = vk::PipelineStageFlagBits2::eTopOfPipe;
            last_image_access[id]  = vk::AccessFlagBits2::eNone;
        }

        for (const auto &id : std::views::keys(m_BufferStates)) {
            last_buffer_stages[id] = vk::PipelineStageFlagBits2::eTopOfPipe;
            last_buffer_access[id] = vk::AccessFlagBits2::eNone;
        }

        for (const RenderPass render_pass : m_RenderPasses) {
            Vulkan::CommandBuffer::BarrierMerger barrier_merger{};

            {
                for (const auto &read_info : render_pass.m_ReadImages) {
                    DIGNIS_ASSERT(m_ImageStates.contains(read_info.Image));

                    const auto &image_state = m_ImageStates[read_info.Image];

                    vk::ImageLayout         src_layout = last_image_layouts[read_info.Image];
                    vk::PipelineStageFlags2 src_stage  = last_image_stages[read_info.Image];
                    vk::AccessFlags2        src_access = last_image_access[read_info.Image];

                    vk::ImageLayout         dst_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
                    vk::PipelineStageFlags2 dst_stage  = read_info.StageMask;
                    vk::AccessFlags2        dst_access = vk::AccessFlagBits2::eShaderRead |
                                                  vk::AccessFlagBits2::eShaderSampledRead |
                                                  vk::AccessFlagBits2::eShaderStorageRead |
                                                  vk::AccessFlagBits2::eMemoryRead;

                    if (src_layout != dst_layout ||
                        src_stage != dst_stage ||
                        src_access != dst_access) {
                        barrier_merger.transition_image_layout(
                            image_state.Image,
                            src_layout,
                            dst_layout,
                            src_stage,
                            src_access,
                            dst_stage,
                            dst_access);

                        last_image_layouts[read_info.Image] = dst_layout;
                        last_image_stages[read_info.Image]  = dst_stage;
                        last_image_access[read_info.Image]  = dst_access;
                    }
                }

                for (const auto &write_info : render_pass.m_WriteImages) {
                    DIGNIS_ASSERT(m_ImageStates.contains(write_info.Image));

                    const auto &image_state = m_ImageStates[write_info.Image];

                    vk::ImageLayout         src_layout = last_image_layouts[write_info.Image];
                    vk::PipelineStageFlags2 src_stage  = last_image_stages[write_info.Image];
                    vk::AccessFlags2        src_access = last_image_access[write_info.Image];

                    vk::ImageLayout         dst_layout = vk::ImageLayout::eGeneral;
                    vk::PipelineStageFlags2 dst_stage  = write_info.StageMask;
                    vk::AccessFlags2        dst_access = vk::AccessFlagBits2::eShaderWrite |
                                                  vk::AccessFlagBits2::eShaderStorageWrite |
                                                  vk::AccessFlagBits2::eMemoryWrite;

                    if (src_layout != dst_layout ||
                        src_stage != dst_stage ||
                        src_access != dst_access) {
                        barrier_merger.transition_image_layout(
                            image_state.Image,
                            src_layout,
                            dst_layout,
                            src_stage,
                            src_access,
                            dst_stage,
                            dst_access);

                        last_image_layouts[write_info.Image] = dst_layout;
                        last_image_stages[write_info.Image]  = dst_stage;
                        last_image_access[write_info.Image]  = dst_access;
                    }
                }

                if (render_pass.m_ColorAttachment.has_value()) {
                    const auto &attachment = render_pass.m_ColorAttachment.value();
                    DIGNIS_ASSERT(m_ImageStates.contains(attachment.Image));
                    const auto &image_state = m_ImageStates[attachment.Image];

                    vk::ImageLayout         src_layout = last_image_layouts[attachment.Image];
                    vk::PipelineStageFlags2 src_stage  = last_image_stages[attachment.Image];
                    vk::AccessFlags2        src_access = last_image_access[attachment.Image];

                    vk::ImageLayout         dst_layout = vk::ImageLayout::eColorAttachmentOptimal;
                    vk::PipelineStageFlags2 dst_stage  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    vk::AccessFlags2        dst_access = vk::AccessFlagBits2::eColorAttachmentWrite;

                    if (vk::AttachmentLoadOp::eLoad == attachment.LoadOp) {
                        dst_access |= vk::AccessFlagBits2::eColorAttachmentRead;
                    }

                    if (src_layout != dst_layout ||
                        src_stage != dst_stage ||
                        src_access != dst_access) {
                        barrier_merger.transition_image_layout(
                            image_state.Image,
                            src_layout,
                            dst_layout,
                            src_stage,
                            src_access,
                            dst_stage,
                            dst_access);

                        last_image_layouts[attachment.Image] = dst_layout;
                        last_image_stages[attachment.Image]  = dst_stage;
                        last_image_access[attachment.Image]  = dst_access;
                    }
                }

                if (render_pass.m_DepthAttachment.has_value()) {
                    const auto &attachment = render_pass.m_DepthAttachment.value();
                    DIGNIS_ASSERT(m_ImageStates.contains(attachment.Image));
                    const auto &image_state = m_ImageStates[attachment.Image];

                    vk::ImageLayout         src_layout = last_image_layouts[attachment.Image];
                    vk::PipelineStageFlags2 src_stage  = last_image_stages[attachment.Image];
                    vk::AccessFlags2        src_access = last_image_access[attachment.Image];

                    vk::ImageLayout         dst_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
                    vk::PipelineStageFlags2 dst_stage  = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                                        vk::PipelineStageFlagBits2::eLateFragmentTests;
                    vk::AccessFlags2 dst_access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;

                    if (vk::AttachmentLoadOp::eLoad == attachment.LoadOp) {
                        dst_access |= vk::AccessFlagBits2::eDepthStencilAttachmentRead;
                    }

                    if (src_layout != dst_layout ||
                        src_stage != dst_stage ||
                        src_access != dst_access) {
                        barrier_merger.transition_image_layout(
                            image_state.Image,
                            src_layout,
                            dst_layout,
                            src_stage,
                            src_access,
                            dst_stage,
                            dst_access);

                        last_image_layouts[attachment.Image] = dst_layout;
                        last_image_stages[attachment.Image]  = dst_stage;
                        last_image_access[attachment.Image]  = dst_access;
                    }
                }

                for (const auto &read_info : render_pass.m_ReadBuffers) {
                    DIGNIS_ASSERT(m_BufferStates.contains(read_info.Buffer));
                    const auto &buffer_state = m_BufferStates[read_info.Buffer];

                    vk::PipelineStageFlags2 src_stage  = last_buffer_stages[read_info.Buffer];
                    vk::AccessFlags2        src_access = last_buffer_access[read_info.Buffer];

                    vk::PipelineStageFlags2 dst_stage  = read_info.StageMask;
                    vk::AccessFlags2        dst_access = vk::AccessFlagBits2::eShaderRead |
                                                  vk::AccessFlagBits2::eShaderStorageRead;

                    if (src_stage != dst_stage ||
                        src_access != dst_access) {
                        barrier_merger.put_buffer_barrier(
                            buffer_state.Buffer,
                            buffer_state.Offset,
                            buffer_state.Size,
                            src_stage,
                            src_access,
                            dst_stage,
                            dst_access);

                        last_buffer_stages[read_info.Buffer] = dst_stage;
                        last_buffer_access[read_info.Buffer] = dst_access;
                    }
                }

                for (const auto &write_info : render_pass.m_WriteBuffers) {
                    DIGNIS_ASSERT(m_BufferStates.contains(write_info.Buffer));
                    const auto &buffer_state = m_BufferStates[write_info.Buffer];

                    vk::PipelineStageFlags2 src_stage  = last_buffer_stages[write_info.Buffer];
                    vk::AccessFlags2        src_access = last_buffer_access[write_info.Buffer];

                    vk::PipelineStageFlags2 dst_stage  = write_info.StageMask;
                    vk::AccessFlags2        dst_access = vk::AccessFlagBits2::eShaderWrite |
                                                  vk::AccessFlagBits2::eShaderStorageWrite;

                    if (src_stage != dst_stage ||
                        src_access != dst_access) {
                        barrier_merger.put_buffer_barrier(
                            buffer_state.Buffer,
                            buffer_state.Offset,
                            buffer_state.Size,
                            src_stage,
                            src_access,
                            dst_stage,
                            dst_access);

                        last_buffer_stages[write_info.Buffer] = dst_stage;
                        last_buffer_access[write_info.Buffer] = dst_access;
                    }
                }
            }

            std::optional<vk::RenderingAttachmentInfo> render_pass_color_attachment = std::nullopt;
            std::optional<vk::RenderingAttachmentInfo> render_pass_depth_attachment = std::nullopt;

            vk::Extent2D render_pass_extent{0, 0};

            if (render_pass.m_ColorAttachment.has_value()) {
                const auto &render_pass_attachment = render_pass.m_ColorAttachment.value();
                DIGNIS_ASSERT(m_ImageStates.contains(render_pass_attachment.Image));
                const auto &image_state = m_ImageStates[render_pass_attachment.Image];

                render_pass_color_attachment = Vulkan::RenderPass::GetAttachmentInfo(
                    image_state.View,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    render_pass_attachment.LoadOp,
                    render_pass_attachment.StoreOp,
                    render_pass_attachment.ClearValue);

                render_pass_extent
                    .setWidth(image_state.Extent.width)
                    .setHeight(image_state.Extent.height);
            }

            if (render_pass.m_DepthAttachment.has_value()) {
                const auto &render_pass_attachment = render_pass.m_DepthAttachment.value();
                DIGNIS_ASSERT(m_ImageStates.contains(render_pass_attachment.Image));
                const auto &image_state = m_ImageStates[render_pass_attachment.Image];

                render_pass_depth_attachment = Vulkan::RenderPass::GetAttachmentInfo(
                    image_state.View,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    render_pass_attachment.LoadOp,
                    render_pass_attachment.StoreOp,
                    render_pass_attachment.ClearValue);

                if (!render_pass_color_attachment.has_value()) {
                    render_pass_extent
                        .setWidth(image_state.Extent.width)
                        .setHeight(image_state.Extent.height);
                }

                DIGNIS_ASSERT(
                    image_state.Extent.width == render_pass_extent.width &&
                    image_state.Extent.height == render_pass_extent.height);
            }

            fu2::function<void(vk::CommandBuffer command_buffer)> render_pass_execute_fn = render_pass.m_ExecuteFn;
            fu2::function<void(vk::CommandBuffer command_buffer)> execute_fn =
                [=](const vk::CommandBuffer command_buffer) mutable {
                    if (render_pass_color_attachment.has_value() &&
                        render_pass_depth_attachment.has_value()) {
                        Vulkan::RenderPass::Begin(
                            render_pass_extent,
                            {render_pass_color_attachment.value()},
                            render_pass_depth_attachment.value(),
                            command_buffer);
                    } else if (render_pass_color_attachment.has_value()) {
                        Vulkan::RenderPass::Begin(
                            render_pass_extent,
                            {render_pass_color_attachment.value()},
                            command_buffer);
                    } else if (render_pass_depth_attachment.has_value()) {
                        Vulkan::RenderPass::Begin(
                            render_pass_extent,
                            {},
                            render_pass_depth_attachment.value(),
                            command_buffer);
                    } else {
                        Vulkan::RenderPass::Begin(
                            render_pass_extent,
                            {},
                            command_buffer);
                    }

                    vk::Viewport viewport{};
                    viewport
                        .setX(0)
                        .setY(render_pass_extent.height)
                        .setWidth(static_cast<float>(render_pass_extent.width))
                        .setHeight(-static_cast<float>(render_pass_extent.height))
                        .setMinDepth(0.0f)
                        .setMaxDepth(1.0f);
                    vk::Rect2D scissor{};
                    scissor
                        .setOffset(vk::Offset2D{0, 0})
                        .setExtent(render_pass_extent);

                    command_buffer.setViewport(0, viewport);
                    command_buffer.setScissor(0, scissor);

                    render_pass_execute_fn(command_buffer);

                    Vulkan::RenderPass::End(command_buffer);
                };

            Pass frame_graph_pass{};
            frame_graph_pass.MemoryBarriers = barrier_merger;
            frame_graph_pass.ExecuteFn      = execute_fn;
            passes.push_back(frame_graph_pass);
        }

        Vulkan::CommandBuffer::BarrierMerger final_barriers{};

        for (const auto [image_id, final_layout] : m_FinalImageLayouts) {
            DIGNIS_ASSERT(m_ImageStates.contains(image_id));
            const auto &image_state = m_ImageStates[image_id];

            vk::ImageLayout         src_layout = last_image_layouts[image_id];
            vk::PipelineStageFlags2 src_stage  = last_image_stages[image_id];
            vk::AccessFlags2        src_access = last_image_access[image_id];

            vk::ImageLayout         dst_layout = final_layout;
            vk::PipelineStageFlags2 dst_stage  = vk::PipelineStageFlagBits2::eAllCommands;
            vk::AccessFlags2        dst_access = vk::AccessFlagBits2::eMemoryRead |
                                          vk::AccessFlagBits2::eMemoryWrite;

            if (src_layout != dst_layout) {
                final_barriers
                    .transition_image_layout(
                        image_state.Image,
                        src_layout,
                        dst_layout,
                        src_stage,
                        src_access,
                        dst_stage,
                        dst_access);

                last_image_layouts[image_id] = src_layout;
                last_image_stages[image_id]  = src_stage;
                last_image_access[image_id]  = dst_access;
            }
        }

        return FrameGraph(passes, final_barriers);
    }

    void FrameGraph::clear() {
        m_Passes.clear();
        m_FinalBarriers.clear();
    }

    void FrameGraph::execute(const vk::CommandBuffer command_buffer) {
        for (auto &[barriers, execute_fn] : m_Passes) {
            auto memory_barriers = barriers;
            memory_barriers.flushBarriers(command_buffer);
            execute_fn(command_buffer);
        }

        auto final_barriers = m_FinalBarriers;

        final_barriers.flushBarriers(command_buffer);
    }

    FrameGraph::FrameGraph(
        const std::span<Pass>                       passes,
        const Vulkan::CommandBuffer::BarrierMerger &final_barriers) {
        m_Passes.insert(
            std::begin(m_Passes),
            std::begin(passes),
            std::end(passes));
        m_FinalBarriers = final_barriers;
    }
}  // namespace Ignis