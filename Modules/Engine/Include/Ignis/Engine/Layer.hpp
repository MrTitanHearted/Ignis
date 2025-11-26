#pragma once

#include <Ignis/Engine/Event.hpp>

namespace Ignis {
    class ILayer {
       public:
        virtual ~ILayer() = default;

        virtual void onEvent(AEvent &event) {}

        virtual void onPreUpdate() {}

        virtual void onUpdate(double dt) {}

        virtual void onPostUpdate() {}

        virtual void onPreRender() {}

        virtual void onRender() {}

        virtual void onPostRender() {}
    };
}  // namespace Ignis