#include <Ignis/Render.hpp>

#include "Ignis/Engine.hpp"

namespace Ignis {
    Render *Render::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Render::State Render::s_State{});

    void Render::SetViewport(
        const FrameGraph::ImageID color_image,
        const FrameGraph::ImageID depth_image) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");

        s_pInstance->m_ColorImage = color_image;
        s_pInstance->m_DepthImage = depth_image;

        s_pInstance->setSkyboxViewport(color_image, depth_image);
        s_pInstance->setModelViewport(color_image, depth_image);
    }

    void Render::SetCamera(const Camera &camera) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");

        s_pInstance->m_Camera = camera;
    }

    glm::mat4x4 Render::GetNormalTransform(const glm::mat4x4 &model_transform) {
        return glm::mat4x4(glm::transpose(glm::inverse(glm::mat3x3(model_transform))));
    }

    void Render::initialize(const Settings &settings) {
        DIGNIS_ASSERT(nullptr == s_pInstance, "Ignis::Render is already initialized.");
        m_pFrameGraph = settings.pFrameGraph;

        uint32_t sampler_pool_size                = 0;
        uint32_t sampled_image_pool_size          = 0;
        uint32_t combined_image_sampler_pool_size = 0;
        uint32_t storage_image_pool_size          = 0;
        uint32_t uniform_buffer_pool_size         = 0;
        uint32_t storage_buffer_pool_size         = 0;

        {
            const auto properties = Vulkan::GetPhysicalDevice().getProperties();

            sampler_pool_size                = properties.limits.maxDescriptorSetSamplers;
            sampled_image_pool_size          = properties.limits.maxDescriptorSetSampledImages;
            combined_image_sampler_pool_size = glm::min(sampler_pool_size, sampled_image_pool_size);
            storage_image_pool_size          = properties.limits.maxDescriptorSetStorageImages;
            uniform_buffer_pool_size         = properties.limits.maxDescriptorSetUniformBuffers;
            storage_buffer_pool_size         = properties.limits.maxDescriptorSetStorageBuffers;

            uint32_t max = glm::max(sampler_pool_size, sampled_image_pool_size);

            max = glm::max(max, storage_image_pool_size);
            max = glm::max(max, uniform_buffer_pool_size);
            max = glm::max(max, storage_buffer_pool_size);

            DIGNIS_ASSERT(settings.MaxBindingCount <= max);
        }

        m_DescriptorPool = Vulkan::CreateDescriptorPool(
            vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind |
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            1 << 8,
            {
                vk::DescriptorPoolSize{vk::DescriptorType::eSampler, sampler_pool_size},
                vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, sampled_image_pool_size},
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, combined_image_sampler_pool_size},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, storage_image_pool_size},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, uniform_buffer_pool_size},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, storage_buffer_pool_size},
            });

        m_Sampler = Vulkan::CreateSampler(
            vk::SamplerCreateInfo()
                .setMinLod(0.0f)
                .setMaxLod(vk::LodClampNone)
                .setAnisotropyEnable(vk::True)
                .setMaxAnisotropy(16.0f));

        m_Camera = Camera{glm::mat4x4{1.0f}, glm::mat4x4{1.0f}, glm::vec3{0.0f}};

        initializeSkybox(settings.SkyboxFacePaths);
        initializeMaterials(settings.MaxBindingCount);
        initializeLights();
        initializeModels(settings.MaxBindingCount);

        s_pInstance = this;
        DIGNIS_LOG_ENGINE_INFO("Ignis::Render Initialized");
    }

    void Render::shutdown() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");

        releaseModels();
        releaseLights();
        releaseMaterials();
        releaseSkybox();

        Vulkan::DestroySampler(m_Sampler);
        Vulkan::DestroyDescriptorPool(m_DescriptorPool);

        m_pFrameGraph = nullptr;

        DIGNIS_LOG_ENGINE_INFO("Ignis::Render Shutdown");
        s_pInstance = nullptr;
    }

    void Render::onRender(FrameGraph &frame_graph) {
        FrameGraph::RenderPass model_render_pass{
            "Ignis::Render::Model Pass",
            {1.0f, 0.0f, 0.0f, 1.0f},
        };

        model_render_pass
            .setColorAttachments(FrameGraph::Attachment{
                m_ColorImage,
                vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
            })
            .setDepthAttachment(FrameGraph::Attachment{
                m_DepthImage,
                vk::ClearDepthStencilValue{1.0f},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
            })
            .setExecute([this](const vk::CommandBuffer command_buffer) {
                onModelDraw(command_buffer);
            });

        readMaterialImages(model_render_pass);
        readMaterialBuffers(model_render_pass);
        readLightBuffers(model_render_pass);
        readModelBuffers(model_render_pass);

        frame_graph.addRenderPass(model_render_pass);

        FrameGraph::RenderPass skybox_render_pass{
            "Ignis::Render::Skybox Pass",
            {1.0f, 1.0f, 0.0f, 1.0f},
        };

        skybox_render_pass
            .setColorAttachments(FrameGraph::Attachment{
                m_ColorImage,
                vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f},
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eStore,
            })
            .setDepthAttachment(FrameGraph::Attachment{
                m_DepthImage,
                vk::ClearDepthStencilValue{1.0f},
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eStore,
            })
            .setExecute([this](const vk::CommandBuffer command_buffer) {
                onSkyboxDraw(command_buffer);
            });

        readSkyboxImage(skybox_render_pass);
        readSkyboxBuffers(skybox_render_pass);

        frame_graph.addRenderPass(skybox_render_pass);
    }

    IGNIS_IF_DEBUG(Render::State::~State() {
        assert(s_pInstance == nullptr && "Forgot to shutdown Ignis::Render.");
    });
}  // namespace Ignis