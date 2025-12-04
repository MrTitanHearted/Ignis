#include <Ignis/Render/FrameGraph.hpp>

namespace Ignis {
    FrameGraph::RenderPass::RenderPass(
        const std::string_view      label,
        const std::array<float, 4> &label_color) {
        m_Label      = label;
        m_LabelColor = label_color;

        m_ReadImages.clear();
        m_ReadBuffers.clear();

        m_DepthAttachment = std::nullopt;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::readImages(const vk::ArrayProxy<ImageID> &images) {
        m_ReadImages.insert(
            std::end(m_ReadImages),
            std::begin(images),
            std::end(images));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::readBuffers(const vk::ArrayProxy<BufferInfo> &buffers) {
        m_ReadBuffers.insert(
            std::end(m_ReadBuffers),
            std::begin(buffers),
            std::end(buffers));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::setColorAttachments(const vk::ArrayProxy<Attachment> &attachments) {
        m_ColorAttachments.clear();
        m_ColorAttachments.insert(
            std::end(m_ColorAttachments),
            std::begin(attachments),
            std::end(attachments));
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::setDepthAttachment(const std::optional<Attachment> &attachment) {
        m_DepthAttachment = attachment;
        return *this;
    }

    FrameGraph::RenderPass &FrameGraph::RenderPass::setExecute(
        const fu2::function<void(vk::CommandBuffer command_buffer)> &execute_fn) {
        m_ExecuteFn = execute_fn;
        return *this;
    }

    FrameGraph::ComputePass::ComputePass(
        const std::string_view      label,
        const std::array<float, 4> &label_color) {
        m_Label      = label;
        m_LabelColor = label_color;

        m_ReadImages.clear();
        m_WriteImages.clear();
        m_ImageBarriers.clear();

        m_ReadBuffers.clear();
        m_WriteBuffers.clear();
        m_BufferBarriers.clear();
    }

    FrameGraph::ComputePass &FrameGraph::ComputePass::readImage(const ImageInfo &info) {
        m_ImageBarriers.push_back(BarrierInfo{AccessType::eRead, static_cast<uint32_t>(m_ReadImages.size())});
        m_ReadImages.push_back(info);
        return *this;
    }

    FrameGraph::ComputePass &FrameGraph::ComputePass::writeImage(const ImageInfo &info) {
        m_ImageBarriers.push_back(BarrierInfo{AccessType::eWrite, static_cast<uint32_t>(m_WriteImages.size())});
        m_WriteImages.push_back(info);
        return *this;
    }

    FrameGraph::ComputePass &FrameGraph::ComputePass::readBuffer(const BufferInfo &info) {
        m_BufferBarriers.push_back(BarrierInfo{AccessType::eRead, static_cast<uint32_t>(m_ReadBuffers.size())});
        m_ReadBuffers.push_back(info);
        return *this;
    }

    FrameGraph::ComputePass &FrameGraph::ComputePass::writeBuffer(const BufferInfo &info) {
        m_BufferBarriers.push_back(BarrierInfo{AccessType::eWrite, static_cast<uint32_t>(m_WriteBuffers.size())});
        m_WriteBuffers.push_back(info);
        return *this;
    }

    FrameGraph::ComputePass &FrameGraph::ComputePass::setExecute(const ExecuteFn &execute_fn) {
        m_ExecuteFn = execute_fn;
        return *this;
    }

    vk::AccessFlags2 FrameGraph::GetImageReadAccess(vk::ImageUsageFlags usage, vk::PipelineStageFlags2 stages) {
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;

        // Sampled image reads
        if (usage & vk::ImageUsageFlagBits::eSampled) {
            if (stages & (vk::PipelineStageFlagBits2::eVertexShader |
                          vk::PipelineStageFlagBits2::eFragmentShader |
                          vk::PipelineStageFlagBits2::eComputeShader |
                          vk::PipelineStageFlagBits2::eRayTracingShaderKHR |
                          vk::PipelineStageFlagBits2::eTaskShaderEXT |
                          vk::PipelineStageFlagBits2::eMeshShaderEXT)) {
                access |= vk::AccessFlagBits2::eShaderSampledRead;
            }
        }

        // Storage image reads
        if (usage & vk::ImageUsageFlagBits::eStorage) {
            if (stages & (vk::PipelineStageFlagBits2::eComputeShader |
                          vk::PipelineStageFlagBits2::eFragmentShader |
                          vk::PipelineStageFlagBits2::eVertexShader |
                          vk::PipelineStageFlagBits2::eRayTracingShaderKHR)) {
                access |= vk::AccessFlagBits2::eShaderStorageRead;
            }
        }

        // Input attachment reads
        if (usage & vk::ImageUsageFlagBits::eInputAttachment) {
            if (stages & vk::PipelineStageFlagBits2::eFragmentShader) {
                access |= vk::AccessFlagBits2::eInputAttachmentRead;
            }
        }

        // Color attachment reads (for blending)
        if (usage & vk::ImageUsageFlagBits::eColorAttachment) {
            if (stages & vk::PipelineStageFlagBits2::eColorAttachmentOutput) {
                access |= vk::AccessFlagBits2::eColorAttachmentRead;
            }
        }

        // Depth/stencil attachment reads
        if (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
            if (stages & (vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                          vk::PipelineStageFlagBits2::eLateFragmentTests)) {
                access |= vk::AccessFlagBits2::eDepthStencilAttachmentRead;
            }
        }

        // Transfer source reads
        if (usage & vk::ImageUsageFlagBits::eTransferSrc) {
            if (stages & vk::PipelineStageFlagBits2::eTransfer) {
                access |= vk::AccessFlagBits2::eTransferRead;
            }
        }

        return access;
    }

    vk::AccessFlags2 FrameGraph::GetImageWriteAccess(vk::ImageUsageFlags usage, vk::PipelineStageFlags2 stages) {
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;

        // Storage image writes
        if (usage & vk::ImageUsageFlagBits::eStorage) {
            if (stages & (vk::PipelineStageFlagBits2::eComputeShader |
                          vk::PipelineStageFlagBits2::eFragmentShader |
                          vk::PipelineStageFlagBits2::eVertexShader |
                          vk::PipelineStageFlagBits2::eRayTracingShaderKHR)) {
                access |= vk::AccessFlagBits2::eShaderStorageWrite;
            }
        }

        // Color attachment writes
        if (usage & vk::ImageUsageFlagBits::eColorAttachment) {
            if (stages & vk::PipelineStageFlagBits2::eColorAttachmentOutput) {
                access |= vk::AccessFlagBits2::eColorAttachmentWrite;
            }
        }

        // Depth/stencil attachment writes
        if (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
            if (stages & (vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                          vk::PipelineStageFlagBits2::eLateFragmentTests)) {
                access |= vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
            }
        }

        // Transfer destination writes
        if (usage & vk::ImageUsageFlagBits::eTransferDst) {
            if (stages & vk::PipelineStageFlagBits2::eTransfer) {
                access |= vk::AccessFlagBits2::eTransferWrite;
            }
        }

        return access;
    }

    vk::AccessFlags2 FrameGraph::GetBufferReadAccess(vk::BufferUsageFlags usage, vk::PipelineStageFlags2 stages) {
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;

        // Uniform buffer reads
        if (usage & vk::BufferUsageFlagBits::eUniformBuffer) {
            if (stages & (vk::PipelineStageFlagBits2::eVertexShader |
                          vk::PipelineStageFlagBits2::eFragmentShader |
                          vk::PipelineStageFlagBits2::eComputeShader |
                          vk::PipelineStageFlagBits2::eRayTracingShaderKHR |
                          vk::PipelineStageFlagBits2::eTaskShaderEXT |
                          vk::PipelineStageFlagBits2::eMeshShaderEXT)) {
                access |= vk::AccessFlagBits2::eUniformRead;
            }
        }

        // Storage buffer reads
        if (usage & vk::BufferUsageFlagBits::eStorageBuffer) {
            if (stages & (vk::PipelineStageFlagBits2::eVertexShader |
                          vk::PipelineStageFlagBits2::eFragmentShader |
                          vk::PipelineStageFlagBits2::eComputeShader |
                          vk::PipelineStageFlagBits2::eRayTracingShaderKHR |
                          vk::PipelineStageFlagBits2::eTaskShaderEXT |
                          vk::PipelineStageFlagBits2::eMeshShaderEXT)) {
                access |= vk::AccessFlagBits2::eShaderStorageRead;
            }
        }

        // Vertex buffer reads
        if (usage & vk::BufferUsageFlagBits::eVertexBuffer) {
            if (stages & vk::PipelineStageFlagBits2::eVertexInput) {
                access |= vk::AccessFlagBits2::eVertexAttributeRead;
            }
        }

        // Index buffer reads
        if (usage & vk::BufferUsageFlagBits::eIndexBuffer) {
            if (stages & vk::PipelineStageFlagBits2::eIndexInput) {
                access |= vk::AccessFlagBits2::eIndexRead;
            }
        }

        // Indirect buffer reads
        if (usage & vk::BufferUsageFlagBits::eIndirectBuffer) {
            if (stages & vk::PipelineStageFlagBits2::eDrawIndirect) {
                access |= vk::AccessFlagBits2::eIndirectCommandRead;
            }
        }

        // Transfer source reads
        if (usage & vk::BufferUsageFlagBits::eTransferSrc) {
            if (stages & vk::PipelineStageFlagBits2::eTransfer) {
                access |= vk::AccessFlagBits2::eTransferRead;
            }
        }

        // Acceleration structure reads
        if (usage & vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR) {
            if (stages & vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR) {
                access |= vk::AccessFlagBits2::eAccelerationStructureReadKHR;
            }
        }

        // Shader binding table reads
        if (usage & vk::BufferUsageFlagBits::eShaderBindingTableKHR) {
            if (stages & vk::PipelineStageFlagBits2::eRayTracingShaderKHR) {
                access |= vk::AccessFlagBits2::eShaderBindingTableReadKHR;
            }
        }

        // Acceleration structure build input reads
        if (usage & vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR) {
            if (stages & vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR) {
                access |= vk::AccessFlagBits2::eAccelerationStructureReadKHR;
            }
        }

        return access;
    }

    vk::AccessFlags2 FrameGraph::GetBufferWriteAccess(vk::BufferUsageFlags usage, vk::PipelineStageFlags2 stages) {
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;

        // Storage buffer writes
        if (usage & vk::BufferUsageFlagBits::eStorageBuffer) {
            if (stages & (vk::PipelineStageFlagBits2::eVertexShader |
                          vk::PipelineStageFlagBits2::eFragmentShader |
                          vk::PipelineStageFlagBits2::eComputeShader |
                          vk::PipelineStageFlagBits2::eRayTracingShaderKHR |
                          vk::PipelineStageFlagBits2::eTaskShaderEXT |
                          vk::PipelineStageFlagBits2::eMeshShaderEXT)) {
                access |= vk::AccessFlagBits2::eShaderStorageWrite;
            }
        }

        // Transfer destination writes
        if (usage & vk::BufferUsageFlagBits::eTransferDst) {
            if (stages & vk::PipelineStageFlagBits2::eTransfer) {
                access |= vk::AccessFlagBits2::eTransferWrite;
            }
        }

        // Acceleration structure writes
        if (usage & vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR) {
            if (stages & vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR) {
                access |= vk::AccessFlagBits2::eAccelerationStructureWriteKHR;
            }
        }

        return access;
    }

    void FrameGraph::removeImage(const ImageID image) {
        DIGNIS_ASSERT(m_ImageStates.contains(image), "FrameGraph::removeImage: image {} is not imported.", image);

        const auto handle = m_ImageStates[image].Handle;
        const auto view   = m_ImageStates[image].View;

        m_ImageMap.erase(handle);
        m_ViewMap.erase(view);

        m_ImageStates.remove(image);

        m_FreeImageIDs.push_back(image);
    }

    void FrameGraph::removeBuffer(const BufferID buffer) {
        DIGNIS_ASSERT(m_BufferStates.contains(buffer), "FrameGraph::removeBuffer: buffer {} is not imported.", buffer);

        const auto handle = m_BufferStates[buffer].Handle;

        m_BufferMap.erase(handle);

        m_BufferStates.remove(buffer);

        m_FreeBufferIDs.push_back(buffer);
    }

    FrameGraph::ImageID FrameGraph::importImage(
        const vk::Image           image,
        const vk::ImageView       image_view,
        const vk::Format          format,
        const vk::ImageUsageFlags usage,
        const vk::Extent3D       &extent,
        const vk::ImageLayout     current_layout,
        const vk::ImageLayout     final_layout) {
        IGNIS_IF_DEBUG(
            IGNIS_ASSERT(!m_ImageMap.contains(static_cast<VkImage>(image)), "vk::Image already imported to the frame graph.");
            IGNIS_ASSERT(!m_ViewMap.contains(static_cast<VkImageView>(image_view)), "vk::ImageView already imported to the frame graph.");)

        ImageID image_id;

        if (!m_FreeImageIDs.empty()) {
            image_id = m_FreeImageIDs.back();
            m_FreeImageIDs.pop_back();
        } else {
            image_id = m_NextImageID++;
        }

        m_ImageMap[static_cast<VkImage>(image)]         = image_id;
        m_ViewMap[static_cast<VkImageView>(image_view)] = image_id;

        ImageState image_state{};
        image_state.Handle = image;
        image_state.View   = image_view;
        image_state.Format = format;
        image_state.Extent = extent;
        image_state.Layout = current_layout;
        image_state.Usage  = usage;

        m_ImageStates.insert(image_id, image_state);

        m_FinalImageLayouts[image_id] = final_layout;

        return image_id;
    }

    FrameGraph::ImageID FrameGraph::importImage(
        const vk::Image           image,
        const vk::ImageView       image_view,
        const vk::Format          format,
        const vk::ImageUsageFlags usage,
        const vk::Extent2D       &extent,
        const vk::ImageLayout     current_layout,
        const vk::ImageLayout     final_layout) {
        return importImage(image, image_view, format, usage, vk::Extent3D{extent, 1}, current_layout, final_layout);
    }

    FrameGraph::BufferID FrameGraph::importBuffer(
        const vk::Buffer           buffer,
        const vk::BufferUsageFlags usage,
        const uint64_t             offset,
        const uint64_t             size) {
        IGNIS_IF_DEBUG(IGNIS_ASSERT(!m_BufferMap.contains(buffer), "vk::Buffer already imported to the frame graph."));

        BufferID buffer_id = INVALID_BUFFER_ID;

        if (!m_FreeBufferIDs.empty()) {
            buffer_id = m_FreeBufferIDs.back();
            m_FreeBufferIDs.pop_back();
        } else {
            buffer_id = m_NextBufferID++;
        }

        m_BufferMap[static_cast<VkBuffer>(buffer)] = buffer_id;

        BufferState buffer_state{};
        buffer_state.Handle = buffer;
        buffer_state.Offset = offset;
        buffer_state.Size   = size;
        buffer_state.Usage  = usage;

        m_BufferStates.insert(buffer_id, buffer_state);

        return buffer_id;
    }

    FrameGraph::ImageID FrameGraph::getImageID(const vk::Image image) const {
        DIGNIS_ASSERT(m_ImageMap.contains(image), "FrameGraph does not contain this image.");
        return m_ImageMap.at(image);
    }

    FrameGraph::ImageID FrameGraph::getImageID(const vk::ImageView view) const {
        DIGNIS_ASSERT(m_ViewMap.contains(view), "FrameGraph does not contain this image view.");
        return m_ViewMap.at(view);
    }

    FrameGraph::BufferID FrameGraph::getBufferID(const vk::Buffer buffer) const {
        DIGNIS_ASSERT(m_BufferMap.contains(buffer), "FrameGraph does not contain this buffer.");
        return m_BufferMap.at(buffer);
    }

    vk::Image FrameGraph::getImage(const ImageID id) const {
        DIGNIS_ASSERT(id < m_NextImageID, "FrameGraph::ImageID is incorrect image id.");
        return m_ImageStates[id].Handle;
    }

    vk::ImageView FrameGraph::getImageView(const ImageID id) const {
        DIGNIS_ASSERT(id < m_NextImageID, "FrameGraph::ImageID is incorrect image id.");
        return m_ImageStates[id].View;
    }

    vk::Format FrameGraph::getImageFormat(const ImageID id) const {
        DIGNIS_ASSERT(id < m_NextImageID, "FrameGraph::ImageID is incorrect image id.");
        return m_ImageStates[id].Format;
    }

    vk::Extent3D FrameGraph::getImageExtent(const ImageID id) const {
        DIGNIS_ASSERT(id < m_NextImageID, "FrameGraph::ImageID is incorrect image id.");
        return m_ImageStates[id].Extent;
    }

    vk::Buffer FrameGraph::getBuffer(const BufferID id) const {
        DIGNIS_ASSERT(id < m_NextBufferID, "FrameGraph::BufferID is incorrect buffer id.");
        return m_BufferStates[id].Handle;
    }

    FrameGraph::ImageID FrameGraph::getSwapchainImageID() const {
        return m_SwapchainImageID;
    }

    void FrameGraph::addRenderPass(const RenderPass &render_pass) {
        IGNIS_IF_DEBUG(
            for (const auto &image : render_pass.m_ReadImages)
                DIGNIS_ASSERT(m_ImageStates.contains(image), "Ignis::FrameGraph does not have image with id: {}", image);

            for (const auto &info : render_pass.m_ReadBuffers)
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

        PassIndex &&index{
            PassType::eRender,
            static_cast<uint32_t>(m_RenderPasses.size()),
        };

        m_RenderPasses.push_back(render_pass);
        m_PassIndices.push_back(index);
    }

    void FrameGraph::addComputePass(const ComputePass &compute_pass) {
        IGNIS_IF_DEBUG(
            for (const auto &[image, _] : compute_pass.m_ReadImages)
                DIGNIS_ASSERT(m_ImageStates.contains(image), "Ignis::FrameGraph does not have image with id: {}", image);
            for (const auto &[image, _] : compute_pass.m_WriteImages)
                DIGNIS_ASSERT(m_ImageStates.contains(image), "Ignis::FrameGraph does not have image with id: {}", image);

            for (const auto &info : compute_pass.m_ReadBuffers)
                DIGNIS_ASSERT(m_BufferStates.contains(info.Buffer), "Ignis::FrameGraph does not have buffer with id: {}", info.Buffer);
            for (const auto &info : compute_pass.m_WriteBuffers)
                DIGNIS_ASSERT(m_BufferStates.contains(info.Buffer), "Ignis::FrameGraph does not have buffer with id: {}", info.Buffer););

        PassIndex &&index{
            PassType::eCompute,
            static_cast<uint32_t>(m_ComputePasses.size()),
        };

        m_ComputePasses.push_back(compute_pass);
        m_PassIndices.push_back(index);
    }

    void FrameGraph::Execute(Executor &&executor, const vk::CommandBuffer command_buffer) {
        for (auto &[barriers, execute_fn] : executor.Passes) {
            barriers.flushBarriers(command_buffer);
            execute_fn(command_buffer);
        }
        executor.FinalBarriers.flushBarriers(command_buffer);
    }

    FrameGraph::FrameGraph() {
        clear();
    }

    void FrameGraph::clear() {
        m_SwapchainImageID = 0;

        m_ImageStates.clear();
        m_BufferStates.clear();

        m_NextImageID  = 0;
        m_NextBufferID = 0;

        m_FreeImageIDs.clear();
        m_FreeBufferIDs.clear();

        m_ImageMap.clear();
        m_ViewMap.clear();
        m_BufferMap.clear();

        m_RenderPasses.clear();
        m_ComputePasses.clear();
        m_PassIndices.clear();
    }

    void FrameGraph::beginFrame(
        const vk::Image           swapchain_image,
        const vk::ImageView       swapchain_view,
        const vk::Format          swapchain_format,
        const vk::ImageUsageFlags swapchain_usage,
        const vk::Extent2D       &swapchain_extent) {
        m_SwapchainImageID = importImage(
            swapchain_image,
            swapchain_view,
            swapchain_format,
            swapchain_usage,
            swapchain_extent,
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageLayout::ePresentSrcKHR);
    }

    FrameGraph::Executor FrameGraph::endFrame() {
        std::vector<ExecutorPass> passes{};
        passes.reserve(m_PassIndices.size());

        ResourceTracker resource_tracker{};

        for (const auto &[id, state] : m_ImageStates) {
            resource_tracker.LastImageLayout[id] = state.Layout;
            resource_tracker.LastImageStages[id] = vk::PipelineStageFlagBits2::eTopOfPipe;
            resource_tracker.LastImageAccess[id] = vk::AccessFlagBits2::eNone;
        }

        for (const auto &id : std::views::keys(m_BufferStates)) {
            resource_tracker.LastBufferStages[id] = vk::PipelineStageFlagBits2::eTopOfPipe;
            resource_tracker.LastBufferAccess[id] = vk::AccessFlagBits2::eNone;
        }

        for (const auto &[type, index] : m_PassIndices) {
            switch (type) {
                case PassType::eRender: {
                    passes.push_back(buildRenderPass(resource_tracker, m_RenderPasses[index]));
                } break;
                case PassType::eCompute: {
                    passes.push_back(buildComputePass(resource_tracker, m_ComputePasses[index]));
                } break;
            }
        }

        Vulkan::BarrierMerger final_barriers{};

        for (const auto [image_id, final_layout] : m_FinalImageLayouts) {
            const auto &image_state = m_ImageStates[image_id];

            const auto src_layout = resource_tracker.LastImageLayout[image_id];
            const auto src_stage  = resource_tracker.LastImageStages[image_id];
            const auto src_access = resource_tracker.LastImageAccess[image_id];

            const auto dst_layout = final_layout;

            constexpr auto dst_stage  = vk::PipelineStageFlagBits2::eAllCommands;
            constexpr auto dst_access = vk::AccessFlagBits2::eMemoryRead |
                                        vk::AccessFlagBits2::eMemoryWrite;

            if (src_layout != dst_layout) {
                final_barriers
                    .put_image_barrier(
                        image_state.Handle,
                        src_layout,
                        dst_layout,
                        src_stage,
                        src_access,
                        dst_stage,
                        dst_access);

                resource_tracker.LastImageLayout[image_id] = src_layout;
                resource_tracker.LastImageStages[image_id] = src_stage;
                resource_tracker.LastImageAccess[image_id] = dst_access;
            }
        }

        m_RenderPasses.clear();
        m_ComputePasses.clear();
        m_PassIndices.clear();

        removeImage(m_SwapchainImageID);

        return Executor{passes, final_barriers};
    }

    FrameGraph::ExecutorPass FrameGraph::buildRenderPass(
        ResourceTracker  &resource_tracker,
        const RenderPass &render_pass) {
        std::string render_pass_label = render_pass.m_Label;

        std::array<float, 4> render_pass_label_color = render_pass.m_LabelColor;

        Vulkan::BarrierMerger barrier_merger{};

        for (const auto &image_id : render_pass.m_ReadImages) {
            const auto &image_state = m_ImageStates[image_id];

            const auto src_layout = resource_tracker.LastImageLayout[image_id];
            const auto src_stages = resource_tracker.LastImageStages[image_id];
            const auto src_access = resource_tracker.LastImageAccess[image_id];

            constexpr auto dst_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
            constexpr auto dst_stages = vk::PipelineStageFlagBits2::eFragmentShader;
            const auto     dst_access = GetImageReadAccess(image_state.Usage, dst_stages);

            if (src_layout != dst_layout ||
                src_stages != dst_stages ||
                src_access != dst_access) {
                barrier_merger.put_image_barrier(
                    image_state.Handle,
                    src_layout,
                    dst_layout,
                    src_stages,
                    src_access,
                    dst_stages,
                    dst_access);

                resource_tracker.LastImageLayout[image_id] = dst_layout;
                resource_tracker.LastImageStages[image_id] = dst_stages;
                resource_tracker.LastImageAccess[image_id] = dst_access;
            }
        }

        for (const auto &attachment : render_pass.m_ColorAttachments) {
            const auto &image_state = m_ImageStates[attachment.Image];

            const auto src_layout = resource_tracker.LastImageLayout[attachment.Image];
            const auto src_stages = resource_tracker.LastImageStages[attachment.Image];
            const auto src_access = resource_tracker.LastImageAccess[attachment.Image];

            constexpr auto dst_layout = vk::ImageLayout::eColorAttachmentOptimal;
            constexpr auto dst_stages = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            constexpr auto dst_access = vk::AccessFlagBits2::eColorAttachmentWrite;

            if (src_layout != dst_layout ||
                src_stages != dst_stages ||
                src_access != dst_access) {
                barrier_merger.put_image_barrier(
                    image_state.Handle,
                    src_layout,
                    dst_layout,
                    src_stages,
                    src_access,
                    dst_stages,
                    dst_access);

                resource_tracker.LastImageLayout[attachment.Image] = dst_layout;
                resource_tracker.LastImageStages[attachment.Image] = dst_stages;
                resource_tracker.LastImageAccess[attachment.Image] = dst_access;
            }
        }

        if (render_pass.m_DepthAttachment.has_value()) {
            const auto &attachment  = render_pass.m_DepthAttachment.value();
            const auto &image_state = m_ImageStates[attachment.Image];

            const auto src_layout = resource_tracker.LastImageLayout[attachment.Image];
            const auto src_stages = resource_tracker.LastImageStages[attachment.Image];
            const auto src_access = resource_tracker.LastImageAccess[attachment.Image];

            constexpr auto dst_layout = vk::ImageLayout::eDepthAttachmentOptimal;
            constexpr auto dst_stages = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                        vk::PipelineStageFlagBits2::eLateFragmentTests;

            vk::AccessFlags2 dst_access = vk::AccessFlagBits2::eDepthStencilAttachmentRead;

            if (vk::AttachmentLoadOp::eClear == attachment.LoadOp) {
                dst_access |= vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
            }

            if (src_layout != dst_layout ||
                src_stages != dst_stages ||
                src_access != dst_access) {
                barrier_merger.put_image_barrier(
                    image_state.Handle,
                    src_layout,
                    dst_layout,
                    src_stages,
                    src_access,
                    dst_stages,
                    dst_access);

                resource_tracker.LastImageLayout[attachment.Image] = dst_layout;
                resource_tracker.LastImageStages[attachment.Image] = dst_stages;
                resource_tracker.LastImageAccess[attachment.Image] = dst_access;
            }
        }

        for (const auto &[buffer_id, offset, size, stages] : render_pass.m_ReadBuffers) {
            const auto &buffer_state = m_BufferStates[buffer_id];

            const auto src_stages = resource_tracker.LastBufferStages[buffer_id];
            const auto src_access = resource_tracker.LastBufferAccess[buffer_id];

            const auto dst_stages = stages;
            const auto dst_access = GetBufferReadAccess(buffer_state.Usage, stages);

            if (src_stages != dst_stages ||
                src_access != dst_access) {
                barrier_merger.put_buffer_barrier(
                    buffer_state.Handle,
                    offset,
                    size,
                    src_stages,
                    src_access,
                    dst_stages,
                    dst_access);

                resource_tracker.LastBufferStages[buffer_id] = dst_stages;
                resource_tracker.LastBufferAccess[buffer_id] = dst_access;
            }
        }

        std::vector<vk::RenderingAttachmentInfo>   render_pass_color_attachments{};
        std::optional<vk::RenderingAttachmentInfo> render_pass_depth_attachment = std::nullopt;

        vk::Extent2D render_pass_extent{0, 0};

        render_pass_color_attachments.reserve(render_pass.m_ColorAttachments.size());
        for (const auto &[image_id, clear_value, load_op, store_op] : render_pass.m_ColorAttachments) {
            const auto &image_state = m_ImageStates[image_id];

            const auto attachment_info = Vulkan::GetRenderingAttachmentInfo(
                image_state.View,
                vk::ImageLayout::eColorAttachmentOptimal,
                load_op,
                store_op,
                clear_value);

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
            const auto &[image_id, clear_value, load_op, store_op] = render_pass.m_DepthAttachment.value();

            const auto &image_state = m_ImageStates[image_id];

            render_pass_depth_attachment = Vulkan::GetRenderingAttachmentInfo(
                image_state.View,
                vk::ImageLayout::eDepthAttachmentOptimal,
                load_op,
                store_op,
                clear_value);

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

        ExecuteFn render_pass_execute_fn = render_pass.m_ExecuteFn;
        ExecuteFn execute_fn =
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

        return ExecutorPass{barrier_merger, execute_fn};
    }

    FrameGraph::ExecutorPass FrameGraph::buildComputePass(
        ResourceTracker   &resource_tracker,
        const ComputePass &compute_pass) {
        const std::string compute_pass_label = compute_pass.m_Label;

        const std::array<float, 4> compute_pass_label_color = compute_pass.m_LabelColor;

        Vulkan::BarrierMerger barrier_merger{};

        for (const auto &[type, index] : compute_pass.m_ImageBarriers) {
            switch (type) {
                case ComputePass::AccessType::eRead: {
                    const auto &[image_id, stage] = compute_pass.m_ReadImages[index];

                    const auto &image_state = m_ImageStates[image_id];

                    const auto src_layout = resource_tracker.LastImageLayout[image_id];
                    const auto src_stage  = resource_tracker.LastImageStages[image_id];
                    const auto src_access = resource_tracker.LastImageAccess[image_id];

                    constexpr auto dst_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

                    const auto dst_stages = stage;
                    const auto dst_access = GetImageReadAccess(image_state.Usage, dst_stages);

                    if (src_layout != dst_layout ||
                        src_stage != dst_stages ||
                        src_access != dst_access) {
                        barrier_merger.put_image_barrier(
                            image_state.Handle,
                            src_layout,
                            dst_layout,
                            src_stage,
                            src_access,
                            dst_stages,
                            dst_access);

                        resource_tracker.LastImageLayout[image_id] = dst_layout;
                        resource_tracker.LastImageStages[image_id] = dst_stages;
                        resource_tracker.LastImageAccess[image_id] = dst_access;
                    }
                } break;
                case ComputePass::AccessType::eWrite: {
                    const auto &[image_id, stage] = compute_pass.m_WriteImages[index];

                    const auto &image_state = m_ImageStates[image_id];

                    const auto src_layout = resource_tracker.LastImageLayout[image_id];
                    const auto src_stage  = resource_tracker.LastImageStages[image_id];
                    const auto src_access = resource_tracker.LastImageAccess[image_id];

                    constexpr auto dst_layout = vk::ImageLayout::eGeneral;

                    const auto dst_stages = stage;
                    const auto dst_access = GetImageWriteAccess(image_state.Usage, dst_stages);

                    if (src_layout != dst_layout ||
                        src_stage != dst_stages ||
                        src_access != dst_access) {
                        barrier_merger.put_image_barrier(
                            image_state.Handle,
                            src_layout,
                            dst_layout,
                            src_stage,
                            src_access,
                            dst_stages,
                            dst_access);

                        resource_tracker.LastImageLayout[image_id] = dst_layout;
                        resource_tracker.LastImageStages[image_id] = dst_stages;
                        resource_tracker.LastImageAccess[image_id] = dst_access;
                    }

                } break;
            }
        }

        for (const auto &[type, index] : compute_pass.m_BufferBarriers) {
            switch (type) {
                case ComputePass::AccessType::eRead: {
                    const auto &[buffer_id, offset, size, stages] = compute_pass.m_ReadBuffers[index];

                    const auto &buffer_state = m_BufferStates[buffer_id];

                    const auto src_stages = resource_tracker.LastBufferStages[buffer_id];
                    const auto src_access = resource_tracker.LastBufferAccess[buffer_id];

                    const auto dst_stages = stages;
                    const auto dst_access = GetBufferReadAccess(buffer_state.Usage, dst_stages);

                    if (src_stages != dst_stages ||
                        src_access != dst_access) {
                        barrier_merger.put_buffer_barrier(
                            buffer_state.Handle,
                            offset,
                            size,
                            src_stages,
                            src_access,
                            dst_stages,
                            dst_access);

                        resource_tracker.LastBufferStages[buffer_id] = dst_stages;
                        resource_tracker.LastBufferAccess[buffer_id] = dst_access;
                    }
                } break;
                case ComputePass::AccessType::eWrite: {
                    const auto &[buffer_id, offset, size, stages] = compute_pass.m_WriteBuffers[index];

                    const auto &buffer_state = m_BufferStates[buffer_id];

                    const auto src_stages = resource_tracker.LastBufferStages[buffer_id];
                    const auto src_access = resource_tracker.LastBufferAccess[buffer_id];

                    const auto dst_stages = stages;
                    const auto dst_access = GetBufferWriteAccess(buffer_state.Usage, dst_stages);

                    if (src_stages != dst_stages ||
                        src_access != dst_access) {
                        barrier_merger.put_buffer_barrier(
                            buffer_state.Handle,
                            offset,
                            size,
                            src_stages,
                            src_access,
                            dst_stages,
                            dst_access);

                        resource_tracker.LastBufferStages[buffer_id] = dst_stages;
                        resource_tracker.LastBufferAccess[buffer_id] = dst_access;
                    }

                } break;
            }
        }

        ExecuteFn compute_pass_execute_fn = compute_pass.m_ExecuteFn;

        ExecuteFn execute_fn =
            [=](const vk::CommandBuffer command_buffer) mutable {
                Vulkan::BeginDebugUtilsLabel(command_buffer, compute_pass_label, compute_pass_label_color);
                compute_pass_execute_fn(command_buffer);
                Vulkan::EndDebugUtilsLabel(command_buffer);
            };

        return ExecutorPass{barrier_merger, execute_fn};
    }
}  // namespace Ignis