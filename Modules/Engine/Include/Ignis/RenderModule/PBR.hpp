#pragma once

#include <Ignis/Frame.hpp>

namespace Ignis {
    class PBR {
       public:
        struct TextureID {
            uint32_t ID;

            bool operator==(const TextureID &) const noexcept = default;
        };

        struct MaterialID {
            uint32_t ID;

            bool operator==(const MaterialID &) const noexcept = default;
        };

        struct PointLightID {
            uint32_t ID;

            bool operator==(const PointLightID &) const noexcept = default;
        };

        struct SpotLightID {
            uint32_t ID;

            bool operator==(const SpotLightID &) const noexcept = default;
        };

        struct StaticMeshID {
            uint32_t ID;

            bool operator==(const StaticMeshID &) const noexcept = default;
        };

        struct StaticInstanceID {
            uint32_t ID;

            bool operator==(const StaticInstanceID &) const noexcept = default;
        };

        struct StaticModelID {
            uint32_t ID;

            bool operator==(const StaticModelID &) const noexcept = default;
        };

        static constexpr TextureID  k_InvalidTextureID{~0u};
        static constexpr MaterialID k_InvalidMaterialID{~0u};

        static constexpr PointLightID k_InvalidPointLightID{~0u};
        static constexpr SpotLightID  k_InvalidSpotLightID{~0u};

        static constexpr StaticMeshID     k_InvalidStaticMeshID{~0u};
        static constexpr StaticInstanceID k_InvalidStaticInstanceID{~0u};
        static constexpr StaticModelID    k_InvalidStaticModelID{~0u};

        struct Material {
            glm::vec3 AlbedoFactor{1.0f};

            float MetallicFactor{1.0f};
            float RoughnessFactor{1.0f};

            TextureID AlbedoTexture{k_InvalidTextureID};
            TextureID NormalTexture{k_InvalidTextureID};
            TextureID MetallicRoughnessTexture{k_InvalidTextureID};
        };

        struct DirectionalLight {
            glm::vec3 Direction{0.0f, -1.0f, 0.0f};
            glm::vec3 Color{1.0f};
        };

        struct PointLight {
            glm::vec3 Position{0.0f};
            float Constant{0.0f};
            glm::vec3 Color{1.0f};
            float Linear{0.0f};

            float Quadratic{1.0f};
            float _ignis_padding[3]{};
        };

        struct SpotLight {
            glm::vec3 Position{0.0f};
            float Constant{0.0f};
            glm::vec3 Direction{0.0f, 0.0f, 1.0f};
            float Linear{0.0f};
            glm::vec3 Color{1.0f};
            float Quadratic{1.0f};

            float CutOff{glm::radians(12.5f)};
            float OuterCutOff{glm::radians(15.0f)};

            float _ignis_padding[2]{};
        };

        struct LightData {
            uint32_t PointLightCount{~0u};
            uint32_t SpotLightCount{~0u};
        };

        struct StaticMesh {
            glm::mat4x4 MeshTransform;
            glm::mat4x4 NormalTransform;

            MaterialID Material;

            uint32_t _ignis_padding[3];
        };

        struct StaticInstance {
            glm::mat4x4 ModelTransform;
            glm::mat4x4 NormalTransform;
        };

        struct StaticVertex {
            glm::vec3 Position{0.0f};
            glm::vec3 Normal{0.0f};
            glm::vec2 UV{0.0f};
            glm::vec4 Tangent{0.0f};
        };

        struct Camera {
            glm::mat4x4 ProjectionView;

            glm::vec3 Position;
        };

        struct StaticModel {
            std::string Path;

            uint32_t MeshCount;
            uint32_t InstanceCount;

            Vulkan::Buffer VertexBuffer;
            Vulkan::Buffer IndexBuffer;
            Vulkan::Buffer MeshBuffer;
            Vulkan::Buffer InstanceBuffer;
            Vulkan::Buffer IndirectBuffer;

            gtl::flat_hash_map<StaticMeshID, uint32_t> MeshToIndex;
            gtl::flat_hash_map<uint32_t, StaticMeshID> IndexToMesh;

            gtl::flat_hash_map<StaticInstanceID, uint32_t> InstanceToIndex;
            gtl::flat_hash_map<uint32_t, StaticInstanceID> IndexToInstance;
        };

        struct Settings {
            Ignis::FrameGraph &FrameGraph;

            FrameGraph::ImageID ViewportImage{FrameGraph::k_InvalidImageID};
            FrameGraph::ImageID DepthImage{FrameGraph::k_InvalidImageID};

            uint32_t MaxBindingCount{2 << 20};

            Settings(
                Ignis::FrameGraph &frame_graph,

                const FrameGraph::ImageID viewport_image,
                const FrameGraph::ImageID depth_image,

                const uint32_t max_binding_count = 2 << 20)
                : FrameGraph{frame_graph},
                  ViewportImage{viewport_image},
                  DepthImage{depth_image},
                  MaxBindingCount{max_binding_count} {}
        };

       public:
        explicit PBR(const Settings &settings);
        ~PBR();

        void setCamera(const Camera &camera);

        void onResize(FrameGraph::ImageID viewport_image, FrameGraph::ImageID depth_image);
        void onRender();

#pragma region Light
        void setDirectionalLight(const DirectionalLight &light) const;
        void setPointLight(PointLightID id, const PointLight &light) const;
        void setSpotLight(SpotLightID id, const SpotLight &light) const;

        PointLightID addPointLight(const PointLight &light);
        SpotLightID  addSpotLight(const SpotLight &light);

        void removePointLight(PointLightID id);
        void removeSpotLight(SpotLightID id);

        DirectionalLight getDirectionalLight() const;

        PointLight getPointLight(PointLightID id) const;
        SpotLight  getSpotLight(SpotLightID id) const;
#pragma endregion
#pragma region Static
        StaticModelID    addStaticModel(const std::filesystem::path &path);
        StaticInstanceID addStaticInstance(StaticModelID model_id, const glm::mat4x4 &transform);

        void removeStaticModel(StaticModelID id);
        void removeStaticInstance(StaticInstanceID id);

        void setStaticInstance(StaticInstanceID id, const glm::mat4x4 &transform);

        std::string_view getStaticModelPath(StaticModelID id);

        glm::mat4x4 getStaticInstance(StaticInstanceID id);
#pragma endregion

       private:
#pragma region Material
        void initMaterials(uint32_t max_binding_count);
        void releaseMaterials();

        TextureID  addTexture(const Vulkan::Image &image, vk::ImageView view);
        MaterialID addMaterial(const Material &material);

        void removeTextureRC(TextureID id);
        void removeMaterialRC(MaterialID id);

        Material getMaterial(MaterialID id) const;

        void readMaterialImages(FrameGraph::RenderPass &render_pass);
        void readMaterialBuffers(FrameGraph::RenderPass &render_pass) const;
#pragma endregion
#pragma region Light
        void initLights();
        void releaseLights();

        void readLightBuffers(FrameGraph::RenderPass &render_pass);
#pragma endregion
#pragma region Static
        void initStatic(uint32_t max_binding_count);
        void releaseStatic();

        void readStaticBuffers(FrameGraph::RenderPass &render_pass);

        void onStaticDraw(vk::CommandBuffer command_buffer);

        void processStaticNode(
            const std::filesystem::path &path,
            const aiScene *ai_scene, const aiNode *ai_node,
            const glm::mat4x4 &transform,

            std::vector<StaticVertex> &vertices,
            std::vector<uint32_t>     &indices,
            std::vector<StaticMesh>   &meshes,

            std::vector<vk::DrawIndexedIndirectCommand> &indirect_commands);

        void processStaticMesh(
            const std::filesystem::path &path,
            const aiScene *ai_scene, const aiMesh *ai_mesh,
            const glm::mat4x4 &transform,

            std::vector<StaticVertex> &vertices,
            std::vector<uint32_t>     &indices,
            std::vector<StaticMesh>   &meshes,

            std::vector<vk::DrawIndexedIndirectCommand> &indirect_commands);

        MaterialID processMaterial(
            const std::filesystem::path &path,
            const aiScene *ai_scene, const aiMaterial *ai_material,
            uint32_t ai_material_index);

        TextureID processTexture(
            const std::filesystem::path &directory,
            const aiScene *ai_scene, const aiMaterial *ai_material,
            aiTextureType ai_texture_type);

#pragma endregion

       private:
        FrameGraph &m_FrameGraph;

        FrameGraph::ImageID m_FrameGraphViewportImage{FrameGraph::k_InvalidImageID};
        FrameGraph::ImageID m_FrameGraphDepthImage{FrameGraph::k_InvalidImageID};

        vk::DescriptorPool m_DescriptorPool;

        vk::Sampler m_Sampler;

        Camera m_Camera;

#pragma region Material
        vk::DescriptorSetLayout m_MaterialDescriptorLayout;

        vk::DescriptorSet m_MaterialDescriptorSet;

        TextureID  m_NextTextureID{k_InvalidTextureID};
        MaterialID m_NextMaterialID{k_InvalidMaterialID};

        std::vector<TextureID>  m_FreeTextureIDs;
        std::vector<MaterialID> m_FreeMaterialIDs;

        gtl::flat_hash_map<TextureID, Vulkan::Image> m_Textures;
        gtl::flat_hash_map<TextureID, vk::ImageView> m_TextureViews;

        gtl::flat_hash_set<MaterialID> m_Materials;

        Vulkan::Buffer m_MaterialStagingBuffer;
        Vulkan::Buffer m_MaterialBuffer;

        SparseVector<TextureID, FrameGraph::ImageID> m_FrameGraphImages;

        FrameGraph::BufferInfo m_FrameGraphMaterialBuffer;

        gtl::flat_hash_map<std::string, MaterialID> m_LoadedMaterials;
        gtl::flat_hash_map<std::string, TextureID>  m_LoadedTextures;

        gtl::flat_hash_map<MaterialID, uint32_t> m_LoadedMaterialRCs;
        gtl::flat_hash_map<TextureID, uint32_t>  m_LoadedTextureRCs;

        gtl::flat_hash_map<MaterialID, std::string> m_LoadedMaterialPaths;
        gtl::flat_hash_map<TextureID, std::string>  m_LoadedTexturePaths;

#pragma endregion
#pragma region Light
        vk::DescriptorSetLayout m_LightDescriptorLayout;

        vk::DescriptorSet m_LightDescriptorSet;

        PointLightID m_NextPointLightID{k_InvalidPointLightID};
        SpotLightID  m_NextSpotLightID{k_InvalidSpotLightID};

        std::vector<PointLightID> m_FreePointLightIDs;
        std::vector<SpotLightID>  m_FreeSpotLightIDs;

        gtl::flat_hash_set<PointLightID> m_PointLights;
        gtl::flat_hash_set<SpotLightID>  m_SpotLights;

        gtl::flat_hash_map<PointLightID, uint32_t> m_PointLightToIndex;
        gtl::flat_hash_map<SpotLightID, uint32_t>  m_SpotLightToIndex;

        gtl::flat_hash_map<uint32_t, PointLightID> m_IndexToPointLight;
        gtl::flat_hash_map<uint32_t, SpotLightID>  m_IndexToSpotLight;

        Vulkan::Buffer m_DirectionalLightStagingBuffer;
        Vulkan::Buffer m_PointLightStagingBuffer;
        Vulkan::Buffer m_SpotLightStagingBuffer;

        Vulkan::Buffer m_DirectionalLightBuffer;
        Vulkan::Buffer m_PointLightBuffer;
        Vulkan::Buffer m_SpotLightBuffer;
        Vulkan::Buffer m_LightDataBuffer;

        FrameGraph::BufferInfo m_FrameGraphDirectionalLightBuffer;
        FrameGraph::BufferInfo m_FrameGraphPointLightBuffer;
        FrameGraph::BufferInfo m_FrameGraphSpotLightBuffer;
        FrameGraph::BufferInfo m_FrameGraphLightDataBuffer;
#pragma endregion
#pragma region Static
        vk::DescriptorSetLayout m_StaticDescriptorLayout;

        vk::DescriptorSet m_StaticDescriptorSet;

        vk::PipelineLayout m_StaticPipelineLayout;

        vk::Pipeline m_StaticPipeline;

        StaticMeshID     m_NextStaticMeshID{k_InvalidStaticMeshID};
        StaticInstanceID m_NextStaticInstanceID{k_InvalidStaticInstanceID};
        StaticModelID    m_NextStaticModelID{k_InvalidStaticModelID};

        std::vector<StaticMeshID>     m_FreeStaticMeshIDs;
        std::vector<StaticInstanceID> m_FreeStaticInstanceIDs;
        std::vector<StaticModelID>    m_FreeStaticModelIDs;

        gtl::flat_hash_set<StaticMeshID>     m_StaticMeshes;
        gtl::flat_hash_set<StaticInstanceID> m_StaticInstances;

        gtl::flat_hash_map<StaticModelID, StaticModel> m_StaticModels;

        gtl::flat_hash_map<StaticMeshID, StaticModelID>     m_StaticMeshToStaticModel;
        gtl::flat_hash_map<StaticInstanceID, StaticModelID> m_StaticInstanceToStaticModel;

        SparseVector<StaticModelID, FrameGraph::BufferInfo> m_FrameGraphStaticModelVertexBuffers;
        SparseVector<StaticModelID, FrameGraph::BufferInfo> m_FrameGraphStaticModelIndexBuffers;
        SparseVector<StaticModelID, FrameGraph::BufferInfo> m_FrameGraphStaticModelMeshBuffers;
        SparseVector<StaticModelID, FrameGraph::BufferInfo> m_FrameGraphStaticModelInstanceBuffers;
        SparseVector<StaticModelID, FrameGraph::BufferInfo> m_FrameGraphStaticModelIndirectBuffers;

        gtl::flat_hash_map<std::string, StaticModelID> m_LoadedStaticModels;
#pragma endregion
    };
}  // namespace Ignis

template <>
struct std::hash<Ignis::PBR::TextureID> {
    size_t operator()(const Ignis::PBR::TextureID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::PBR::MaterialID> {
    size_t operator()(const Ignis::PBR::MaterialID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::PBR::PointLightID> {
    size_t operator()(const Ignis::PBR::PointLightID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::PBR::SpotLightID> {
    size_t operator()(const Ignis::PBR::SpotLightID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::PBR::StaticMeshID> {
    size_t operator()(const Ignis::PBR::StaticMeshID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::PBR::StaticInstanceID> {
    size_t operator()(const Ignis::PBR::StaticInstanceID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::PBR::StaticModelID> {
    size_t operator()(const Ignis::PBR::StaticModelID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};
