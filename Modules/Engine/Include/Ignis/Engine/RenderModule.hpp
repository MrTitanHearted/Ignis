#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Frame.hpp>

namespace Ignis {
    class IRenderModule {
       public:
        explicit IRenderModule(const std::type_index id) : m_ID{id} {}

        virtual ~IRenderModule() = default;

        virtual void onAttach(FrameGraph &frame_graph) = 0;
        virtual void onDetach(FrameGraph &frame_graph) = 0;

       private:
        std::type_index m_ID;

       private:
        friend class Frame;
    };

    template <typename TRenderModule>
    class RenderModule : public IRenderModule {
       public:
        RenderModule() : IRenderModule{typeid(TRenderModule)} {}

        ~RenderModule() override = default;
    };

}  // namespace Ignis
