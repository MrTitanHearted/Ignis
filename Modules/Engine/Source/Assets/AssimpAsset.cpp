#include <Ignis/Assets/AssimpAsset.hpp>

namespace Ignis {
    std::optional<std::unique_ptr<AssimpAsset>> AssimpAsset::LoadFromPath(const std::filesystem::path &path) {
        const std::string spath = path.string();

        if (!std::filesystem::exists(path)) {
            DIGNIS_LOG_ENGINE_WARN("Failed to find an assimp asset from path: '{}'", spath);
            return std::nullopt;
        }

        auto asset = std::make_unique<AssimpAsset>();

        if (nullptr == asset->m_Importer.ReadFile(
                           spath,
                           aiProcess_Triangulate |
                               aiProcess_GenNormals |
                               aiProcess_GenUVCoords |
                               aiProcess_FlipUVs)) {
            DIGNIS_LOG_ENGINE_WARN("Failed to load asset from path: '{}'. Assimp error: '{}'", spath, asset->m_Importer.GetErrorString());
            return std::nullopt;
        }

        DIGNIS_LOG_ENGINE_INFO("Loaded an Ignis::AssimpAsset from path: '{}'", spath);

        return asset;
    }

    const aiScene *AssimpAsset::getScene() const {
        return m_Importer.GetScene();
    }
}  // namespace Ignis