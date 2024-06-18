#ifndef GLM_HASH_H
#define GLM_HASH_H

#include <glm/glm.hpp>
#include <functional>

namespace std {
    template <>
    struct hash<glm::ivec3> {
        size_t operator()(const glm::ivec3& v) const {
            std::hash<int> hasher;
            size_t h1 = hasher(v.x);
            size_t h2 = hasher(v.y);
            size_t h3 = hasher(v.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

#endif // GLM_HASH_H