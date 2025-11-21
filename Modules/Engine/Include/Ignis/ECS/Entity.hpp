#pragma once

#include <Ignis/Core.hpp>

namespace Ignis {
    typedef uint32_t Entity;

    static constexpr Entity INVALID_ENTITY = 0U;

    Entity CreateEntity();
}  // namespace Ignis