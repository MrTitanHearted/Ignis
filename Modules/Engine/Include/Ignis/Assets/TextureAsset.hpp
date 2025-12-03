#pragma once

#include <Ignis/Core.hpp>

namespace Ignis {
    class TextureAsset {
       public:
        enum class Type {
            eRGBA32f,
            eRGB32f,
            eRGBA16u,
            eRGB16u,
            eRGBA8u,
            eRGB8u,
        };

       public:
        static std::optional<TextureAsset> LoadFromPath(const std::filesystem::path &path, Type type = Type::eRGBA8u);
        static std::optional<TextureAsset> LoadFromMemory(const void *data, size_t size, Type type = Type::eRGBA8u);

        static uint8_t  GetChannelCount(Type type);
        static uint32_t GetChannelSize(Type type);

        static void SetFlipVertically(bool flip);

       public:
        TextureAsset()  = default;
        ~TextureAsset() = default;

        uint32_t getWidth() const;
        uint32_t getHeight() const;

        Type getType() const;

        std::span<const uint8_t> getData() const;

       private:
        uint32_t m_Width;
        uint32_t m_Height;

        Type m_Type;

        std::vector<uint8_t> m_Data;
    };
}  // namespace Ignis