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

        int32_t desired_channels = 0;

        switch (type) {
            case Type::eRGBA32f:
            case Type::eRGB32f: {
                switch (type) {
                    case Type::eRGBA32f:
                        desired_channels = 4;
                        break;
                    case Type::eRGB32f:
                        desired_channels = 3;
                        break;
                    default:
                        break;
                }

                float *data = stbi_loadf(texture_path, &width, &height, nullptr, desired_channels);

                if (nullptr == data) {
                    DIGNIS_LOG_ENGINE_WARN("Failed to load texture from path: '{}'", spath);
                    return std::nullopt;
                }

                const size_t texture_size = width * height * desired_channels * sizeof(float);

                TextureAsset texture_asset{};
                texture_asset.m_Width  = width;
                texture_asset.m_Height = height;
                texture_asset.m_Type   = type;

                texture_asset.m_Data.clear();
                texture_asset.m_Data.resize(texture_size);

                memcpy(texture_asset.m_Data.data(), data, texture_size);

                stbi_image_free(data);

                return texture_asset;
            }
            case Type::eRGBA8u:
            case Type::eRGB8u: {
                switch (type) {
                    case Type::eRGBA8u:
                        desired_channels = 4;
                        break;
                    case Type::eRGB8u:
                        desired_channels = 3;
                        break;
                    default:
                        break;
                }

                uint8_t *data = stbi_load(texture_path, &width, &height, nullptr, desired_channels);

                if (nullptr == data) {
                    DIGNIS_LOG_ENGINE_WARN("Failed to load texture from path: '{}'", spath);
                    return std::nullopt;
                }

                const size_t texture_size = width * height * desired_channels * sizeof(uint8_t);

                TextureAsset texture_asset{};
                texture_asset.m_Width  = width;
                texture_asset.m_Height = height;
                texture_asset.m_Type   = type;

                texture_asset.m_Data.clear();
                texture_asset.m_Data.resize(texture_size);

                memcpy(texture_asset.m_Data.data(), data, texture_size);

                stbi_image_free(data);

                return texture_asset;
            }
        }

        return std::nullopt;
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