#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Assets.hpp>
#include <Ignis/Vulkan.hpp>
#include <Ignis/Frame.hpp>
#include <Ignis/Engine.hpp>

namespace Ignis {
    class BPR {
       public:
        typedef uint32_t TextureID;
        typedef uint32_t MaterialID;

        typedef uint32_t PointLightID;
        typedef uint32_t SpotLightID;

        typedef uint32_t StaticModelID;
        typedef uint32_t StaticInstanceID;

        static constexpr TextureID  k_InvalidTextureID  = ~0u;
        static constexpr MaterialID k_InvalidMaterialID = ~0u;

        static constexpr PointLightID k_InvalidPointLightID = ~0u;
        static constexpr SpotLightID  k_InvalidSpotLightID  = ~0u;

        static constexpr StaticModelID    k_InvalidStaticModelID    = ~0u;
        static constexpr StaticInstanceID k_InvalidStaticInstanceID = ~0u;

        struct Material {
            TextureID DiffuseTexture;
            TextureID SpecularTexture;

            float Shininess;
        };

        struct StaticInstance {
            glm::mat4x4 ModelTransform;
            glm::mat4x4 NormalTransform;
        };

        struct Camera {
            glm::mat4x4 Projection;
            glm::mat4x4 View;
        };

        struct DirectionalLight {
            glm::vec3 Direction{0.0f, -1.0f, 0.0f};

            glm::vec3 Ambient{0.0f};
            glm::vec3 Diffuse{0.0f};
            glm::vec3 Specular{0.0f};
        };

        struct PointLight {
            glm::vec3 Position;

            float Constant;
            float Linear;
            float Quadratic;

            glm::vec3 Ambient;
            glm::vec3 Diffuse;
            glm::vec3 Specular;
        };

        struct SpotLight {
            glm::vec3 Position;
            glm::vec3 Direction;

            float CutOff;
            float OuterCutOff;
            float _ignis_padding;

            float Constant;
            float Linear;
            float Quadratic;

            glm::vec3 Ambient;
            glm::vec3 Diffuse;
            glm::vec3 Specular;
        };

        struct LightData {
            uint32_t PointLightCount{0u};
            uint32_t SpotLightCount{0u};
            uint32_t _ignis_padding{~0u};

            glm::vec3 ViewPosition{0.0f};
        };

        struct StaticDrawData {
            MaterialID       Material;
            StaticInstanceID Instance;
        };

        struct StaticMesh {
            uint32_t IndexOffset{~0u};
            uint32_t VertexOffset{~0u};

            uint32_t IndexCount{~0u};
            uint32_t VertexCount{~0u};

            MaterialID Material{k_InvalidMaterialID};
        };

        struct StaticModel {
            std::string Path;

            uint32_t InstanceCount{~0u};

            std::vector<StaticMesh> Meshes;

            Vulkan::Buffer VertexBuffer;
            Vulkan::Buffer IndexBuffer;
            Vulkan::Buffer InstanceBuffer;

            uint32_t NextInstanceIndex{~0u};

            gtl::flat_hash_map<StaticInstanceID, uint32_t> InstanceToIndex;
            gtl::flat_hash_map<uint32_t, StaticInstanceID> IndexToInstance;
        };

        struct StaticVertex {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 UV;
        };

       public:
        struct Settings {
            vk::Format ColorAttachmentFormat{vk::Format::eUndefined};
            vk::Format DepthAttachmentFormat{vk::Format::eUndefined};

            uint32_t MaxBindingCount{2 << 16};
        };

       public:
        explicit BPR(const Settings &settings);
        ~BPR();

        void onRenderPass(FrameGraph::RenderPass &render_pass);
        void onDraw(vk::CommandBuffer command_buffer);

        void updateCamera(const Camera &camera) const;

        void setDirectionalLight(const DirectionalLight &light) const;

        PointLightID addPointLight(const PointLight &light);
        SpotLightID  addSpotLight(const SpotLight &light);

        void removePointLight(PointLightID id);
        void removeSpotLight(SpotLightID id);

        void setPointLight(PointLightID id, const PointLight &light);
        void setSpotLight(SpotLightID id, const SpotLight &light);

        StaticModelID    loadStaticModel(const std::filesystem::path &path);
        StaticInstanceID addStaticInstance(StaticModelID model_id, StaticInstance instance);

        void unloadStaticModel(StaticModelID id);
        void removeStaticInstance(StaticInstanceID id);

        [[nodiscard]] const StaticModel &getStaticModelConstRef(StaticModelID id) const;

        [[nodiscard]] StaticInstance getStaticInstance(StaticInstanceID id) const;

        void setStaticInstance(StaticInstanceID id, StaticInstance instance);

       private:
        void processStaticNode(
            const std::filesystem::path &directory,
            const aiScene *ai_scene, const aiNode *ai_node,
            std::vector<StaticVertex> &vertices,
            std::vector<uint32_t>     &indices,
            StaticModel               &model);

        StaticMesh processStaticMesh(
            const std::filesystem::path &directory,
            const aiScene *ai_scene, const aiMesh *ai_mesh,
            std::vector<StaticVertex> &vertices,
            std::vector<uint32_t>     &indices);

        MaterialID processMaterial(
            const std::filesystem::path &directory,
            const aiScene *ai_scene, const aiMaterial *ai_material);

        TextureID processTexture(
            const std::filesystem::path &directory,
            const aiScene *ai_scene, const aiMaterial *ai_material,
            aiTextureType ai_texture_type);

        void removeMaterial(MaterialID id);
        void removeTexture(TextureID id);

        [[nodiscard]] Material getMaterial(MaterialID id) const;

       private:
        FrameGraph &m_FrameGraph;

        vk::Format m_ColorAttachmentFormat;
        vk::Format m_DepthAttachmentFormat;

        uint32_t m_MaxBindingCount;

        vk::DescriptorPool m_DescriptorPool;

        vk::DescriptorSetLayout m_MaterialDescriptorLayout;
        vk::DescriptorSet       m_MaterialDescriptor;

        vk::DescriptorSetLayout m_LightDescriptorLayout;
        vk::DescriptorSet       m_LightDescriptor;

        vk::DescriptorSetLayout m_StaticDescriptorLayout;
        vk::PipelineLayout      m_StaticPipelineLayout;

        vk::DescriptorSet m_StaticDescriptor;
        vk::Pipeline      m_StaticPipeline;

        TextureID  m_NextTextureID;
        MaterialID m_NextMaterialID;

        PointLightID m_NextPointLightID;
        SpotLightID  m_NextSpotLightID;

        StaticModelID    m_NextStaticModelID;
        StaticInstanceID m_NextStaticInstanceID;

        std::vector<TextureID>  m_FreeTextureIDs;
        std::vector<MaterialID> m_FreeMaterialIDs;

        std::vector<PointLightID> m_FreePointLightIDs;
        std::vector<SpotLightID>  m_FreeSpotLightIDs;

        std::vector<StaticModelID>    m_FreeStaticModelIDs;
        std::vector<StaticInstanceID> m_FreeStaticInstanceIDs;

        vk::Sampler m_Sampler;

        gtl::flat_hash_map<TextureID, Vulkan::Image> m_Textures;
        gtl::flat_hash_map<TextureID, vk::ImageView> m_TextureViews;

        gtl::flat_hash_set<MaterialID> m_MaterialIDs;

        Vulkan::Buffer m_MaterialStagingBuffer;
        Vulkan::Buffer m_MaterialBuffer;
        Vulkan::Buffer m_CameraBuffer;

        gtl::flat_hash_map<PointLightID, uint32_t> m_PointLightToIndex;
        gtl::flat_hash_map<uint32_t, PointLightID> m_IndexToPointLight;
        gtl::flat_hash_map<SpotLightID, uint32_t>  m_SpotLightToIndex;
        gtl::flat_hash_map<uint32_t, SpotLightID>  m_IndexToSpotLight;

        Vulkan::Buffer m_PointLightStagingBuffer;
        Vulkan::Buffer m_SpotLightStagingBuffer;

        Vulkan::Buffer m_DirectionalLightBuffer;
        Vulkan::Buffer m_PointLightBuffer;
        Vulkan::Buffer m_SpotLightBuffer;
        Vulkan::Buffer m_LightDataBuffer;

        SparseVector<StaticModelID, StaticModel> m_StaticModels;

        gtl::flat_hash_map<StaticInstanceID, StaticModelID> m_StaticInstanceToStaticModel;

        SparseVector<TextureID, FrameGraph::ImageID> m_FrameGraphImages;

        FrameGraph::BufferInfo m_FrameGraphMaterialBuffer;
        FrameGraph::BufferInfo m_FrameGraphCameraBuffer;

        FrameGraph::BufferInfo m_FrameGraphDirectionalLightBuffer;
        FrameGraph::BufferInfo m_FrameGraphPointLightBuffer;
        FrameGraph::BufferInfo m_FrameGraphSpotLightBuffer;
        FrameGraph::BufferInfo m_FrameGraphLightDataBuffer;

        SparseVector<StaticModelID, FrameGraph::BufferInfo> m_FrameGraphStaticModelVertexBuffers;
        SparseVector<StaticModelID, FrameGraph::BufferInfo> m_FrameGraphStaticModelIndexBuffers;
        SparseVector<StaticModelID, FrameGraph::BufferInfo> m_FrameGraphStaticModelInstanceBuffers;

        gtl::flat_hash_map<std::string, TextureID>  m_LoadedTextures;
        gtl::flat_hash_map<std::string, MaterialID> m_LoadedMaterials;

        gtl::flat_hash_map<std::string, StaticModelID> m_LoadedStaticModels;

        gtl::flat_hash_map<TextureID, std::string>  m_LoadedTexturePaths;
        gtl::flat_hash_map<MaterialID, std::string> m_LoadedMaterialPaths;

        gtl::flat_hash_map<TextureID, uint32_t>  m_LoadedTextureRCs;
        gtl::flat_hash_map<MaterialID, uint32_t> m_LoadedMaterialRCs;
    };

}  // namespace Ignis