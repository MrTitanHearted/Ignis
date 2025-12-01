#include <Ignis/Render/FrameGraph.hpp>

namespace Ignis {
    FrameGraph::RenderPass::RenderPass(
        const std::string_view      label,
        const std::array<float, 4> &label_color) {
        m_Label      = label;
        m_LabelColor = label_color;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::readImages(const vk::ArrayProxy<ImageInfo> &images) {
        m_ReadImages.clear();
        m_ReadImages.insert(
            std::begin(m_ReadImages),
            std::begin(images),
            std::end(images));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::writeImages(const vk::ArrayProxy<ImageInfo> &images) {
        m_WriteImages.clear();
        m_WriteImages.insert(
            std::begin(m_WriteImages),
            std::begin(images),
            std::end(images));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::readBuffers(const vk::ArrayProxy<BufferInfo> &buffers) {
        m_ReadBuffers.clear();
        m_ReadBuffers.insert(
            std::begin(m_ReadBuffers),
            std::begin(buffers),
            std::end(buffers));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::writeBuffers(const vk::ArrayProxy<BufferInfo> &buffers) {
        m_WriteBuffers.clear();
        m_WriteBuffers.insert(
            std::begin(m_WriteBuffers),
            std::begin(buffers),
            std::end(buffers));

        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::setColorAttachments(const vk::ArrayProxy<Attachment> &attachments) {
        m_ColorAttachments.clear();
        m_ColorAttachments.insert(
            std::begin(m_ColorAttachments),
            std::begin(attachments),
            std::end(attachments));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::setDepthAttachment(const Attachment &attachment) {
        m_DepthAttachment = attachment;
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::setExecute(
        const fu2::function<void(vk::CommandBuffer command_buffer)> &execute_fn) {
        m_ExecuteFn = execute_fn;
        return *this;
    }

    FrameGraph::ImageID FrameGraph::importImage3D(
        const vk::Image       image,
        const vk::ImageView   image_view,
        const vk::Extent3D   &extent,
        const vk::ImageLayout current_layout,
        const vk::ImageLayout final_layout) {
        IGNIS_IF_DEBUG(
            IGNIS_ASSERT(!m_ImageSet.contains(image), "vk::Image already imported to the frame graph.");
            IGNIS_ASSERT(!m_ViewSet.contains(image_view), "vk::ImageView already imported to the frame graph.");

            m_ImageSet.insert(image);
            m_ViewSet.insert(image_view);)

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

    FrameGraph::ImageID FrameGraph::importImage2D(
        const vk::Image       image,
        const vk::ImageView   image_view,
        const vk::Extent2D   &extent,
        const vk::ImageLayout current_layout,
        const vk::ImageLayout final_layout) {
        IGNIS_IF_DEBUG(
            IGNIS_ASSERT(!m_ImageSet.contains(image), "vk::Image already imported to the frame graph.");
            IGNIS_ASSERT(!m_ViewSet.contains(image_view), "vk::ImageView already imported to the frame graph.");

            m_ImageSet.insert(image);
            m_ViewSet.insert(image_view);)

        const ImageID image_id = m_NextImageID++;

        ImageState image_state{};
        image_state.Image  = image;
        image_state.View   = image_view;
        image_state.Extent = vk::Extent3D{extent, 1};
        image_state.Layout = current_layout;

        m_ImageStates[image_id] = image_state;

        m_FinalImageLayouts[image_id] = final_layout;

        return image_id;
    }

    FrameGraph::BufferID FrameGraph::importBuffer(
        const vk::Buffer buffer,
        const uint64_t   offset,
        const uint64_t   size) {
        IGNIS_IF_DEBUG(
            IGNIS_ASSERT(!m_BufferSet.contains(buffer), "vk::Buffer already imported to the frame graph.");

            m_BufferSet.insert(buffer);)

        const BufferID buffer_id = m_NextBufferID++;

        BufferState buffer_state{};
        buffer_state.Buffer = buffer;
        buffer_state.Offset = offset;
        buffer_state.Size   = size;

        m_BufferStates[buffer_id] = buffer_state;
        return buffer_id;
    }

    vk::Image FrameGraph::getImage(const ImageID id) const {
        DIGNIS_ASSERT(id < m_NextImageID, "FrameGraph::ImageID is incorrect image id.");
        return m_ImageStates.at(id).Image;
    }

    vk::ImageView FrameGraph::getImageView(const ImageID id) const {
        DIGNIS_ASSERT(id < m_NextImageID, "FrameGraph::ImageID is incorrect image id.");
        return m_ImageStates.at(id).View;
    }

    vk::Extent3D FrameGraph::getImageExtent(const ImageID id) const {
        DIGNIS_ASSERT(id < m_NextImageID, "FrameGraph::ImageID is incorrect image id.");
        return m_ImageStates.at(id).Extent;
    }

    vk::Buffer FrameGraph::getBuffer(const BufferID id) const {
        DIGNIS_ASSERT(id < m_NextBufferID, "FrameGraph::BufferID is incorrect buffer id.");
        return m_BufferStates.at(id).Buffer;
    }

    FrameGraph::ImageID FrameGraph::getSwapchainImageID() const {
        return m_SwapchainImageID;
    }

    void FrameGraph::addRenderPass(RenderPass &&render_pass) {
        IGNIS_IF_DEBUG(
            for (const auto &[image, _] : render_pass.m_ReadImages)
                DIGNIS_ASSERT(m_ImageStates.contains(image), "Ignis::FrameGraph does not have image with id: {}", image);
            for (const auto &[image, _] : render_pass.m_WriteImages)
                DIGNIS_ASSERT(m_ImageStates.contains(image), "Ignis::FrameGraph does not have image with id: {}", image);

            for (const auto &info : render_pass.m_ReadBuffers)
                DIGNIS_ASSERT(m_BufferStates.contains(info.Buffer), "Ignis::FrameGraph does not have buffer with id: {}", info.Buffer);
            for (const auto &info : render_pass.m_WriteBuffers)
                DIGNIS_ASSERT(m_BufferStates.contains(info.Buffer), "Ignis::FrameGraph does not have buffer with id: {}", info.Buffer);

            for (const auto &attachment : render_pass.m_ColorAttachments)
                DIGNIS_ASSERT(m_ImageStates.contains(attachment.Image), "Ignis::FrameGraph does not have image with id: {}", attachment.Image);

            if (render_pass.m_DepthAttachment.has_value()) {
                const Attachment &attachment = render_pass.m_DepthAttachment.value();
                DIGNIS_ASSERT(m_ImageStates.contains(attachment.Image), "Ignis::FrameGraph does not have image with id: {}", attachment.Image);
            }

            DIGNIS_ASSERT(
                !render_pass.m_ColorAttachments.empty() || render_pass.m_DepthAttachment.has_value(),
                "Ignis::FrameGraph::RenderPass must have at least one color attachment or a depth attachment"););

        m_RenderPasses.push_back(std::move(render_pass));
    }

    void FrameGraph::Execute(Executor &&executor, const vk::CommandBuffer command_buffer) {
        for (auto &[barriers, execute_fn] : executor.Passes) {
            barriers.flushBarriers(command_buffer);
            execute_fn(command_buffer);
        }
        executor.FinalBarriers.flushBarriers(command_buffer);
    }

    FrameGraph::FrameGraph(
        const vk::Image     swapchain_image,
        const vk::ImageView swapchain_image_view,
        const vk::Extent2D &swapchain_extent) {
        m_ImageStates.clear();
        m_BufferStates.clear();
        m_NextImageID  = 0;
        m_NextBufferID = 0;
        m_RenderPasses.clear();

        IGNIS_IF_DEBUG(
            m_ImageSet.clear();
            m_ViewSet.clear();
            m_BufferSet.clear();)

        m_SwapchainImageID = importImage2D(
            swapchain_image,
            swapchain_image_view,
            swapchain_extent,
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageLayout::ePresentSrcKHR);
    }

    FrameGraph::Executor FrameGraph::build() {
        gtl::vector<ExecutorPass> passes{};
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

        for (const RenderPass &render_pass : m_RenderPasses) {
            std::string render_pass_label = render_pass.m_Label;

            std::array<float, 4> render_pass_label_color = render_pass.m_LabelColor;

            Vulkan::BarrierMerger barrier_merger{};

            {
                for (const auto &read_info : render_pass.m_ReadImages) {
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
                        barrier_merger.put_image_barrier(
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
                        barrier_merger.put_image_barrier(
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

                for (const auto &attachment : render_pass.m_ColorAttachments) {
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
                        barrier_merger.put_image_barrier(
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
                    const auto &attachment  = render_pass.m_DepthAttachment.value();
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
                        barrier_merger.put_image_barrier(
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
                    const auto &buffer_state = m_BufferStates[read_info.Buffer];

                    vk::PipelineStageFlags2 src_stage  = last_buffer_stages[read_info.Buffer];
                    vk::AccessFlags2        src_access = last_buffer_access[read_info.Buffer];

                    vk::PipelineStageFlags2 dst_stage  = read_info.StageMask;
                    vk::AccessFlags2        dst_access = vk::AccessFlagBits2::eMemoryRead |
                                                  vk::AccessFlagBits2::eShaderRead |
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
                    const auto &buffer_state = m_BufferStates[write_info.Buffer];

                    vk::PipelineStageFlags2 src_stage  = last_buffer_stages[write_info.Buffer];
                    vk::AccessFlags2        src_access = last_buffer_access[write_info.Buffer];

                    vk::PipelineStageFlags2 dst_stage  = write_info.StageMask;
                    vk::AccessFlags2        dst_access = vk::AccessFlagBits2::eMemoryWrite |
                                                  vk::AccessFlagBits2::eShaderWrite |
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

            gtl::vector<vk::RenderingAttachmentInfo>   render_pass_color_attachments{};
            std::optional<vk::RenderingAttachmentInfo> render_pass_depth_attachment = std::nullopt;

            vk::Extent2D render_pass_extent{0, 0};

            render_pass_color_attachments.reserve(render_pass.m_ColorAttachments.size());
            for (const auto &render_pass_attachment : render_pass.m_ColorAttachments) {
                const auto &image_state = m_ImageStates[render_pass_attachment.Image];

                const auto attachment_info = Vulkan::GetRenderingAttachmentInfo(
                    image_state.View,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    render_pass_attachment.LoadOp,
                    render_pass_attachment.StoreOp,
                    render_pass_attachment.ClearValue);

                if (render_pass_extent.width == 0 &&
                    render_pass_extent.height == 0) {
                    render_pass_extent
                        .setWidth(image_state.Extent.width)
                        .setHeight(image_state.Extent.height);
                }

                DIGNIS_ASSERT(
                    image_state.Extent.width == render_pass_extent.width &&
                        image_state.Extent.height == render_pass_extent.height,
                    "In a FrameGraph RenderPass, Color Attachments should be the same size.");

                render_pass_color_attachments.push_back(attachment_info);
            }

            if (render_pass.m_DepthAttachment.has_value()) {
                const auto &render_pass_attachment = render_pass.m_DepthAttachment.value();

                const auto &image_state = m_ImageStates[render_pass_attachment.Image];

                render_pass_depth_attachment = Vulkan::GetRenderingAttachmentInfo(
                    image_state.View,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    render_pass_attachment.LoadOp,
                    render_pass_attachment.StoreOp,
                    render_pass_attachment.ClearValue);

                if (render_pass_extent.width == 0 &&
                    render_pass_extent.height == 0) {
                    render_pass_extent
                        .setWidth(image_state.Extent.width)
                        .setHeight(image_state.Extent.height);
                }

                DIGNIS_ASSERT(
                    image_state.Extent.width == render_pass_extent.width &&
                        image_state.Extent.height == render_pass_extent.height,
                    "In a FrameGraph RenderPass, Depth Attachment should be the same as Color Attachments' size");
            }

            RenderPass::ExecuteFn render_pass_execute_fn = render_pass.m_ExecuteFn;
            RenderPass::ExecuteFn execute_fn =
                [=](const vk::CommandBuffer command_buffer) mutable {
                    Vulkan::BeginDebugUtilsLabel(command_buffer, render_pass_label, render_pass_label_color);

                    Vulkan::BeginRenderPass(
                        render_pass_extent,
                        render_pass_color_attachments,
                        render_pass_depth_attachment,
                        command_buffer);

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

                    Vulkan::EndRenderPass(command_buffer);

                    Vulkan::EndDebugUtilsLabel(command_buffer);
                };

            ExecutorPass executor_render_pass{};
            executor_render_pass.MemoryBarriers = barrier_merger;
            executor_render_pass.ExecuteFn      = execute_fn;
            passes.push_back(executor_render_pass);
        }

        Vulkan::BarrierMerger final_barriers{};

        for (const auto [image_id, final_layout] : m_FinalImageLayouts) {
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
                    .put_image_barrier(
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

        return Executor{passes, final_barriers};
    }
}  // namespace Ignis