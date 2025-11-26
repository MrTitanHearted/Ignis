#pragma once

#include <Ignis/Engine.hpp>

#include <Ignis/Editor/Layer.hpp>

namespace Ignis {
    class Editor {
       public:
        Editor()  = default;
        ~Editor() = default;

        void initialize(int32_t argc, const char **argv);
        void release();

        void run();

       private:
        Logger m_Logger;
        Engine m_Engine;
    };
}  // namespace Ignis