#include <Ignis/Assets/FileAsset.hpp>

namespace Ignis {
    std::optional<FileAsset> FileAsset::LoadTextFromPath(const std::filesystem::path &path) {
        std::string spath = path.string();

        if (!std::filesystem::exists(path)) {
            DIGNIS_LOG_ENGINE_WARN("Failed to find a file from path: '{}'", spath);
            return std::nullopt;
        }

        std::ifstream ifile{path, std::ios::ate};
        DIGNIS_ASSERT(ifile.is_open(), "Failed to open file from path: '{}'", spath);

        const size_t file_size = ifile.tellg();
        std::string  content{};
        content.resize(file_size);

        ifile.seekg(0);
        ifile.read(content.data(), file_size);
        ifile.close();

        DIGNIS_LOG_ENGINE_INFO("Loaded an Ignis::FileAsset from path: '{}'", spath);

        return LoadFromMemory(path, content);
    }

    std::optional<FileAsset> FileAsset::LoadBinaryFromPath(const std::filesystem::path &path) {
        std::string spath = path.string();

        if (!std::filesystem::exists(path)) {
            DIGNIS_LOG_ENGINE_WARN("Failed to find a file from path: '{}'", spath);
            return std::nullopt;
        }

        std::ifstream ifile{path, std::ios::ate | std::ios::binary};
        DIGNIS_ASSERT(ifile.is_open(), "Failed to open file from path: '{}'", spath);

        const size_t file_size = ifile.tellg();
        std::string  content{};
        content.resize(file_size);

        ifile.seekg(0);
        ifile.read(content.data(), file_size);
        ifile.close();

        DIGNIS_LOG_ENGINE_INFO("Loaded an Ignis::FileAsset from path: '{}'", spath);

        return LoadFromMemory(path, content);
    }

    FileAsset FileAsset::LoadFromMemory(const std::filesystem::path &path, const std::string_view content) {
        FileAsset file_asset{};
        file_asset.m_Path    = path;
        file_asset.m_Content = content;
        return file_asset;
    }

    std::filesystem::path FileAsset::getPath() const {
        return m_Path;
    }

    std::string_view FileAsset::getContent() const {
        return m_Content;
    }

    size_t FileAsset::getSize() const {
        return m_Content.size();
    }
}  // namespace Ignis