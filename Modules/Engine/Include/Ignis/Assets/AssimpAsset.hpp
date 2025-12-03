#pragma once

#include <Ignis/Core.hpp>

namespace Ignis {
    class AssimpAsset {
       public:
        static std::optional<std::unique_ptr<AssimpAsset>> LoadFromPath(const std::filesystem::path &path);

       public:
        AssimpAsset()  = default;
        ~AssimpAsset() = default;

        const aiScene *getScene() const;

       private:
        Assimp::Importer m_Importer;
    };
}  // namespace Ignis