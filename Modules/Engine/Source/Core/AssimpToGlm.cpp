#include <Ignis/Core/AssimpToGlm.hpp>

namespace Ignis {
    glm::vec4 AssimpToGlm(const aiColor4D &c) {
        glm::vec4 result;

        result.r = c.r;
        result.g = c.g;
        result.b = c.b;
        result.a = c.a;

        return result;
    }

    glm::vec3 AssimpToGlm(const aiColor3D &c) {
        glm::vec3 result;

        result.r = c.r;
        result.g = c.g;
        result.b = c.b;

        return result;
    }

    glm::mat4 AssimpToGlm(const aiMatrix4x4 &m) {
        glm::mat4 result;

        result[0][0] = m.a1;
        result[1][0] = m.a2;
        result[2][0] = m.a3;
        result[3][0] = m.a4;

        result[0][1] = m.b1;
        result[1][1] = m.b2;
        result[2][1] = m.b3;
        result[3][1] = m.b4;

        result[0][2] = m.c1;
        result[1][2] = m.c2;
        result[2][2] = m.c3;
        result[3][2] = m.c4;

        result[0][3] = m.d1;
        result[1][3] = m.d2;
        result[2][3] = m.d3;
        result[3][3] = m.d4;

        return result;
    }

    glm::mat3 AssimpToGlm(const aiMatrix3x3 &m) {
        glm::mat3 result;

        result[0][0] = m.a1;
        result[1][0] = m.a2;
        result[2][0] = m.a3;

        result[0][1] = m.b1;
        result[1][1] = m.b2;
        result[2][1] = m.b3;

        result[0][2] = m.c1;
        result[1][2] = m.c2;
        result[2][2] = m.c3;

        return result;
    }

    glm::vec3 AssimpToGlm(const aiVector3D &v) {
        glm::vec3 result;

        result.x = v.x;
        result.y = v.y;
        result.z = v.z;

        return result;
    }

}  // namespace Ignis