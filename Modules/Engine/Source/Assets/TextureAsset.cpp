#include <Ignis/Assets/TextureAsset.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Ignis {
    std::optional<TextureAsset> TextureAsset::LoadFromPath(
        const std::filesystem::path &path,
        const Type                   type) {
        const std::string spath = path.string();

        if (!std::filesystem::exists(path)) {
            DIGNIS_LOG_ENGINE_WARN("Failed to find a texture from path: '{}'", spath);
            return std::nullopt;
        }

        const char *texture_path = spath.c_str();

        int32_t width  = 0;
        int32_t height = 0;

        const int32_t desired_channels = GetChannelCount(type);
        const int32_t texel_size       = GetChannelSize(type);

        uint8_t *texture_data = nullptr;

        switch (type) {
            case Type::eRGBA32f:
            case Type::eRGB32f: {
                texture_data =
                    reinterpret_cast<uint8_t *>(stbi_loadf(texture_path, &width, &height, nullptr, desired_channels));
            } break;
            case Type::eRGBA16u:
            case Type::eRGB16u: {
                texture_data =
                    reinterpret_cast<uint8_t *>(stbi_load_16(texture_path, &width, &height, nullptr, desired_channels));
            } break;
            case Type::eRGBA8u:
            case Type::eRGB8u: {
                texture_data = stbi_load(texture_path, &width, &height, nullptr, desired_channels);

            } break;
        }

        const size_t texture_size = width * height * texel_size;

        TextureAsset texture_asset{};
        texture_asset.m_Width  = width;
        texture_asset.m_Height = height;
        texture_asset.m_Type   = type;

        texture_asset.m_Data.clear();
        texture_asset.m_Data.resize(texture_size);

        memcpy(texture_asset.m_Data.data(), texture_data, texture_size);

        stbi_image_free(texture_data);

        DIGNIS_LOG_ENGINE_INFO("Loaded an Ignis::TextureAsset from path: '{}'", spath);

        return texture_asset;
    }

    std::optional<TextureAsset> TextureAsset::LoadFromMemory(const void *data, const size_t size, const Type type) {
        int32_t width  = 0;
        int32_t height = 0;

        const int32_t desired_channels = GetChannelCount(type);
        const int32_t texel_size       = GetChannelSize(type);

        uint8_t *texture_data = nullptr;

        switch (type) {
            case Type::eRGBA32f:
            case Type::eRGB32f: {
                texture_data =
                    reinterpret_cast<uint8_t *>(stbi_loadf_from_memory(
                        static_cast<stbi_uc const *>(data), size, &width, &height, nullptr, desired_channels));
            } break;
            case Type::eRGBA16u:
            case Type::eRGB16u: {
                texture_data =
                    reinterpret_cast<uint8_t *>(stbi_load_16_from_memory(
                        static_cast<stbi_uc const *>(data), size, &width, &height, nullptr, desired_channels));
            } break;
            case Type::eRGBA8u:
            case Type::eRGB8u: {
                texture_data = stbi_load_from_memory(
                    static_cast<stbi_uc const *>(data), size, &width, &height, nullptr, desired_channels);
            } break;
        }

        const size_t texture_size = width * height * texel_size;

        TextureAsset texture_asset{};
        texture_asset.m_Width  = width;
        texture_asset.m_Height = height;
        texture_asset.m_Type   = type;

        texture_asset.m_Data.clear();
        texture_asset.m_Data.resize(texture_size);

        memcpy(texture_asset.m_Data.data(), texture_data, texture_size);

        stbi_image_free(texture_data);

        return texture_asset;
    }

    uint8_t TextureAsset::GetChannelCount(const Type type) {
        switch (type) {
            case Type::eRGBA32f:
            case Type::eRGBA16u:
            case Type::eRGBA8u:
                return 4;
            case Type::eRGB32f:
            case Type::eRGB16u:
            case Type::eRGB8u:
                return 3;
        }
        return 0;
    }

    uint32_t TextureAsset::GetChannelSize(const Type type) {
        switch (type) {
            case Type::eRGBA32f:
                return 4 * sizeof(float);
            case Type::eRGB32f:
                return 3 * sizeof(float);
            case Type::eRGBA16u:
                return 4 * sizeof(uint16_t);
            case Type::eRGB16u:
                return 3 * sizeof(uint16_t);
            case Type::eRGBA8u:
                return 4 * sizeof(uint8_t);
            case Type::eRGB8u:
                return 3 * sizeof(uint8_t);
        }
        return 0;
    }

    void TextureAsset::SetFlipVertically(const bool flip) {
        stbi_set_flip_vertically_on_load(flip);
    }

    uint32_t TextureAsset::getWidth() const {
        return m_Width;
    }

    uint32_t TextureAsset::getHeight() const {
        return m_Height;
    }

    TextureAsset::Type TextureAsset::getType() const {
        return m_Type;
    }

    std::span<const uint8_t> TextureAsset::getData() const {
        return m_Data;
    }
}  // namespace Ignis