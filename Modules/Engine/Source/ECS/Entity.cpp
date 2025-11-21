#include <Ignis/ECS/Entity.hpp>

namespace Ignis {
    Entity CreateEntity() {
        static std::atomic<Entity> next{1U};
        return next.fetch_add(1U);
    }
}  // namespace Ignis