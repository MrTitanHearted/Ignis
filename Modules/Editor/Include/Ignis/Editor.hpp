#pragma once

#include <Ignis/Engine.hpp>
#include <Ignis/ImGuiSystem.hpp>

namespace Ignis {
    class Editor final : public Layer<Editor> {
       public:
        struct Settings {
        };

        struct ModelUIState {
            glm::vec3 Position{0.0f};
            glm::vec3 Rotation{0.0f};
            glm::f32  Scale{1.0f};

            std::vector<Render::InstanceID> Instances{};
        };

        struct InstanceUIState {
            glm::vec3 Position{0.0f};
            glm::vec3 Rotation{0.0f};
            glm::f32  Scale{1.0f};
        };

        struct ModelPanelState {
            std::vector<Render::ModelID> Models{};

            gtl::flat_hash_map<Render::ModelID, ModelUIState>       ModelToUIState{};
            gtl::flat_hash_map<Render::InstanceID, InstanceUIState> InstanceToUIState{};
        };

        struct LightPanelState {
            Render::DirectionalLight DirectionalLight{};

            std::vector<Render::PointLightID> PointLights{};
            std::vector<Render::SpotLightID>  SpotLights{};

            gtl::flat_hash_map<Render::PointLightID, Render::PointLight> PointLightValues{};
            gtl::flat_hash_map<Render::SpotLightID, Render::SpotLight>   SpotLightValues{};
        };

       public:
        static bool OnWindowClose(const WindowCloseEvent &);

        static void RenderModelPanel(ModelPanelState &state);
        static void RenderLightPanel(LightPanelState &state);

       public:
        explicit Editor(const Settings &settings);
        ~Editor() override;

        void onUpdate(double dt) override;

        void onGUI(IGUISystem *ui_system) override;
        void onRender(FrameGraph &frame_graph) override;

       private:
        void createViewportImage(ImGuiSystem *im_gui, uint32_t width, uint32_t height, FrameGraph &frame_graph);
        void destroyViewportImage(ImGuiSystem *im_gui, FrameGraph &frame_graph);

        void updateSpotLight();

        bool onKeyEvent(const WindowKeyEvent &event);
        bool onMouseMove(const WindowMouseMoveEvent &event);
        bool onMouseScroll(const WindowMouseScrollEvent &event);
        bool onMouseButtonEvent(const WindowMouseButtonEvent &event);

       private:
        float  m_LastMouseX;
        float  m_LastMouseY;
        Camera m_Camera;

        bool m_Play;

        Vulkan::Image m_ViewportImage;
        vk::ImageView m_ViewportView;
        vk::Extent2D  m_ViewportExtent;

        Vulkan::Image m_DepthImage;
        vk::ImageView m_DepthView;

        vk::DescriptorSet m_ViewportDescriptor;

        FrameGraph::ImageID m_ColorImageID{FrameGraph::k_InvalidImageID};
        FrameGraph::ImageID m_DepthImageID{FrameGraph::k_InvalidImageID};

        ModelPanelState m_ModelPanel;
        LightPanelState m_LightPanel;

        glm::vec3 m_SpotLightColor{0.0f};
        glm::f32  m_SpotLightPower{0.0f};
        glm::f32  m_SpotLightCutOff{0.0f};
        glm::f32  m_SpotLightOuterCutOff{0.0f};

        Render::SpotLightID m_SpotLightID{Render::k_InvalidSpotLightID};
    };
}  // namespace Ignis