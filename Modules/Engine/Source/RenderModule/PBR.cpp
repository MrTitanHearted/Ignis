#include <Ignis/RenderModule/PBR.hpp>

namespace Ignis {
    PBR::PBR(const Settings &settings)
        : m_FrameGraph{settings.FrameGraph} {
        m_FrameGraphViewportImage = settings.ViewportImage;
        m_FrameGraphDepthImage    = settings.DepthImage;

        m_DescriptorPool = Vulkan::CreateDescriptorPool(
            vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind |
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            64,
            {
                vk::DescriptorPoolSize{vk::DescriptorType::eSampler, settings.MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, settings.MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, settings.MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, settings.MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, settings.MaxBindingCount},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, settings.MaxBindingCount},
            });

        m_Sampler = Vulkan::CreateSampler(
            vk::SamplerCreateInfo()
                .setMaxAnisotropy(16.0f));

        m_Camera = Camera{glm::mat4x4{1.0f}, glm::mat4x4{1.0f}, glm::vec3{0.0f}};

        initSkybox(settings.SkyboxFacePaths);
        initMaterials(settings.MaxBindingCount);
        initLights();
        initStatic(settings.MaxBindingCount);
    }

    PBR::~PBR() {
        releaseSkybox();
        releaseStatic();
        releaseLights();
        releaseMaterials();

        Vulkan::DestroyDescriptorPool(m_DescriptorPool);
        Vulkan::DestroySampler(m_Sampler);
    }

    void PBR::setCamera(const Camera &camera) {
        m_Camera = camera;
    }

    void PBR::onResize(
        const FrameGraph::ImageID viewport_image,
        const FrameGraph::ImageID depth_image) {
        m_FrameGraphViewportImage = viewport_image;
        m_FrameGraphDepthImage    = depth_image;
    }

    void PBR::onRender() {
        FrameGraph::RenderPass static_render_pass{
            "Ignis::PBR::Static::RenderPass",
            {1.0f, 0.0f, 0.0f, 1.0f},
        };

        static_render_pass
            .setColorAttachments(FrameGraph::Attachment{
                m_FrameGraphViewportImage,
                vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
            })
            .setDepthAttachment(FrameGraph::Attachment{
                m_FrameGraphDepthImage,
                vk::ClearDepthStencilValue{1.0f},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
            })
            .setExecute([this](const vk::CommandBuffer command_buffer) {
                onStaticDraw(command_buffer);
            });

        readMaterialImages(static_render_pass);
        readMaterialBuffers(static_render_pass);
        readLightBuffers(static_render_pass);
        readStaticBuffers(static_render_pass);

        m_FrameGraph.addRenderPass(static_render_pass);

        FrameGraph::RenderPass skybox_render_pass{
            "Ignis::PBR::Skybox::RenderPass",
            {1.0f, 1.0f, 0.0f, 1.0f},
        };

        skybox_render_pass
            .setColorAttachments(FrameGraph::Attachment{
                m_FrameGraphViewportImage,
                vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f},
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eStore,
            })
            .setDepthAttachment(FrameGraph::Attachment{
                m_FrameGraphDepthImage,
                vk::ClearDepthStencilValue{1.0f},
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eStore,
            })
            .setExecute([this](const vk::CommandBuffer command_buffer) {
                onSkyboxDraw(command_buffer);
            });

        readSkyboxImage(skybox_render_pass);
        readSkyboxBuffers(skybox_render_pass);

        m_FrameGraph.addRenderPass(skybox_render_pass);
    }
}  // namespace Ignis