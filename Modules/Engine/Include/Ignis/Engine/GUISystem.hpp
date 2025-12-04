#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Render.hpp>

namespace Ignis {
    class AGUISystem {
       public:
        explicit AGUISystem(std::type_index type);

        virtual ~AGUISystem() {}

        virtual void onAttach() {}

        virtual void onDetach() {}

        virtual void onGUIBegin() {}

        virtual void onGUIEnd() {}

        virtual void onRender(FrameGraph &frame_graph) {}

       protected:
        template <typename TEvent, typename TGUISystem>
            requires(std::is_base_of_v<IEvent, TEvent> &&
                     std::is_base_of_v<AGUISystem, TGUISystem>)
        void attachCallback(bool (TGUISystem::*method)(const TEvent &)) {
            attachCallback<TEvent>([this, method](const TEvent &event) {
                return (static_cast<TGUISystem *>(this)->*method)(event);
            });
        }

        template <typename TEvent, typename TGUISystem>
            requires(std::is_base_of_v<IEvent, TEvent> &&
                     std::is_base_of_v<AGUISystem, TGUISystem>)
        void attachCallback(bool (TGUISystem::*method)(const TEvent &) const) const {
            attachCallback<TEvent>([this, method](const TEvent &event) {
                return (static_cast<const TGUISystem *>(this)->*method)(event);
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

       private:
        std::type_index m_ID;

       private:
        friend class Engine;
    };

    template <typename TGUISystem>
    class IGUISystem : public AGUISystem {
       public:
        explicit IGUISystem();

        ~IGUISystem() override {}
    };
}  // namespace Ignis