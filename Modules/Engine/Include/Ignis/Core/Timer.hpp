#pragma once
#include <Ignis/Core/PCH.hpp>

namespace Ignis {
    class Timer {
       public:
        using Clock     = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<Clock>;

       public:
        Timer()  = default;
        ~Timer() = default;

        void start();
        void stop();

        double getElapsedTime() const;

       private:
        std::optional<TimePoint> m_Start;
        std::optional<TimePoint> m_End;
    };
}  // namespace Ignis