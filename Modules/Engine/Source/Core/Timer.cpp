#include <Ignis/Core/Timer.hpp>

#include <Ignis/Core/Logger.hpp>

namespace Ignis {
    void Timer::start() {
        m_Start = Clock::now();
    }

    void Timer::stop() {
        m_End = Clock::now();
    }

    double Timer::getElapsedTime() const {
        using Duration = std::chrono::duration<double>;

        DIGNIS_ASSERT(m_Start.has_value(), "Time start is not set");
        DIGNIS_ASSERT(m_End.has_value(), "Time end is not set");

        const TimePoint &start = m_Start.value();
        const TimePoint &end   = m_End.value();

        return Duration(end - start).count();
    }
}  // namespace Ignis