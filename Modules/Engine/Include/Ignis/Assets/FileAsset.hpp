#pragma once

#include <Ignis/Core.hpp>

namespace Ignis {
    class FileAsset {
       public:
        static std::optional<FileAsset> LoadTextFromPath(const std::filesystem::path &path);
        static std::optional<FileAsset> LoadBinaryFromPath(const std::filesystem::path &path);

        static FileAsset LoadFromMemory(const std::filesystem::path &path, std::string_view content);

       public:
        FileAsset()  = default;
        ~FileAsset() = default;

        std::filesystem::path getPath() const;
        std::string_view      getContent() const;

        size_t getSize() const;

       private:
        std::filesystem::path m_Path;
        std::string           m_Content;
    };
}  // namespace Ignis