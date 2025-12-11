#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Frame.hpp>

#include <Ignis/Engine/GUISystem.hpp>

namespace Ignis {
    class Engine;

    class ILayer {
       public:
        ILayer(Engine *engine, std::type_index layer_id);
        virtual ~ILayer() = default;

        template <typename TLayer, typename... Args>
            requires(std::is_base_of_v<ILayer, TLayer>)
        void transition(Args &&...args);

       protected:
        virtual void onUpdate(double dt) {}

        virtual void onGUI(IGUISystem *ui_system) {}

        virtual void onRender(FrameGraph &frame_graph) {}

        template <typename TEvent, typename TLayer>
            requires(std::is_base_of_v<IEvent, TEvent> &&
                     std::is_base_of_v<ILayer, TLayer>)
        void attachCallback(bool (TLayer::*method)(const TEvent &)) {
            attachCallback<TEvent>([this, method](const TEvent &event) {
                return (static_cast<TLayer *>(this)->*method)(event);
            });
        }

        template <typename TEvent, typename TLayer>
            requires(std::is_base_of_v<IEvent, TEvent> &&
                     std::is_base_of_v<ILayer, TLayer>)
        void attachCallback(bool (TLayer::*method)(const TEvent &) const) const {
            attachCallback<TEvent>([this, method](const TEvent &event) {
                return (static_cast<const TLayer *>(this)->*method)(event);
            });
        }

        template <typename TEvent, typename Callback>
            requires(std::is_base_of_v<IEvent, TEvent>)
        void attachCallback(Callback callback) const;

        template <typename TEvent>
            requires(std::is_base_of_v<IEvent, TEvent>)
        void detachCallback() const;

        template <typename TEvent, typename... Args>
            requires(std::is_base_of_v<IEvent, TEvent>)
        void postEvent(Args &&...args) const;

        void postTask(fu2::function<void()> &&task) const;

       private:
        Engine *m_pEngine;

        std::type_index m_LayerID;

       private:
        friend class Engine;
    };

    template <typename TLayer>
    class Layer : public ILayer {
       public:
        Layer();
        ~Layer() override = default;
    };
}  // namespace Ignis