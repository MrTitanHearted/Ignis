#pragma once

#include <Ignis/Core/PCH.hpp>

namespace Ignis {
    glm::vec4 AssimpToGlm(const aiColor4D &c);
    glm::vec3 AssimpToGlm(const aiColor3D &c);

    glm::mat4 AssimpToGlm(const aiMatrix4x4 &m);
    glm::mat3 AssimpToGlm(const aiMatrix3x3 &m);

    glm::vec3 AssimpToGlm(const aiVector3D &v);
}  // namespace Ignis