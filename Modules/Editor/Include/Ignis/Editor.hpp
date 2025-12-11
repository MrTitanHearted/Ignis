#pragma once

#include <Ignis/Engine.hpp>

#include <Ignis/RenderModule.hpp>

#include <Ignis/ImGuiSystem.hpp>

namespace Ignis {
    class Editor final : public Layer<Editor> {
       public:
        struct Settings {
        };

        struct AddUIState {
            glm::vec3 Position;
            glm::vec3 Rotation;
            float     Scale;
        };

        struct InstanceUIState {
            glm::vec3 Position;
            glm::vec3 Rotation;

            float Scale;
            bool  IsInitialized{false};
        };

        struct LightPanelState {
            BPR::DirectionalLight DirectionalLight;

            std::vector<BPR::PointLightID> PointLights;
            std::vector<BPR::SpotLightID>  SpotLights;

            gtl::flat_hash_map<BPR::PointLightID, BPR::PointLight> PointLightValues;
            gtl::flat_hash_map<BPR::SpotLightID, BPR::SpotLight>   SpotLightValues;
        };

        struct StaticModelPanelState {
            std::vector<BPR::StaticModelID> StaticModels;

            gtl::flat_hash_map<BPR::StaticModelID, AddUIState> StaticModelUIStates;

            gtl::flat_hash_map<BPR::StaticModelID, std::vector<BPR::StaticInstanceID>> StaticInstances;

            gtl::flat_hash_map<BPR::StaticInstanceID, InstanceUIState> StaticInstanceUIStates;
        };

       public:
        static bool OnWindowClose(const WindowCloseEvent &);

        static glm::mat4x4 ComposeTRS(
            const glm::vec3 &position,
            const glm::vec3 &rotation,
            float            scale);

        static void RenderLightPanel(LightPanelState &state, BPR *bpr);
        static void RenderStaticModelPanel(StaticModelPanelState &state, BPR *bpr);

       public:
        explicit Editor(const Settings &settings);
        ~Editor() override;

        void onUpdate(double dt) override;

        void onGUI(IGUISystem *ui_system) override;
        void onRender(FrameGraph &frame_graph) override;

       private:
        void createViewportImage(ImGuiSystem *im_gui, uint32_t width, uint32_t height, FrameGraph &frame_graph);
        void destroyViewportImage(ImGuiSystem *im_gui, FrameGraph &frame_graph);

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

        vk::DescriptorSet   m_ViewportDescriptor;
        FrameGraph::ImageID m_ViewportImageID{FrameGraph::k_InvalidImageID};
        FrameGraph::ImageID m_DepthImageID{FrameGraph::k_InvalidImageID};

        std::unique_ptr<BPR> m_pBPR;

        LightPanelState m_LightPanelState;

        StaticModelPanelState m_StaticModelPanelState;
    };
}  // namespace Ignis