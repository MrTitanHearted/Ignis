#pragma once
#include <Ignis/Core/PCH.hpp>

namespace Ignis {
    class Timer {
       public:
        using Clock     = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<Clock>;
        using Duration  = std::chrono::duration<double>;

       public:
        static double GetTimeInSecondsSinceProgramStart();

       public:
        Timer()  = default;
        ~Timer() = default;

        void start();
        void stop();

        double getElapsedTime() const;

       private:
        explicit Timer(bool);

       private:
        std::optional<TimePoint> m_Start;
        std::optional<TimePoint> m_End;

       private:
        static Timer s_ProgramTimer;
    };
}  // namespace Ignis