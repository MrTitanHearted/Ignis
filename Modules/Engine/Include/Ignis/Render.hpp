#pragma once

#include <Ignis/Frame.hpp>

namespace Ignis {
    class Render {
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

        struct MeshID {
            uint32_t ID;

            bool operator==(const MeshID &) const noexcept = default;
        };

        struct InstanceID {
            uint32_t ID;

            bool operator==(const InstanceID &) const noexcept = default;
        };

        struct ModelID {
            uint32_t ID;

            bool operator==(const ModelID &) const noexcept = default;
        };

        static constexpr TextureID  k_InvalidTextureID{~0u};
        static constexpr MaterialID k_InvalidMaterialID{~0u};

        static constexpr PointLightID k_InvalidPointLightID{~0u};
        static constexpr SpotLightID  k_InvalidSpotLightID{~0u};

        static constexpr MeshID     k_InvalidMeshID{~0u};
        static constexpr InstanceID k_InvalidInstanceID{~0u};
        static constexpr ModelID    k_InvalidModelID{~0u};

        struct Material {
            glm::vec3 AlbedoFactor{1.0f};
            glm::f32  MetallicFactor{1.0f};
            glm::vec3 EmissiveFactor{0.0f};
            glm::f32  RoughnessFactor{1.0f};

            TextureID AlbedoTexture{k_InvalidTextureID};
            TextureID NormalTexture{k_InvalidTextureID};
            TextureID EmissiveTexture{k_InvalidTextureID};
            TextureID AmbientOcclusionTexture{k_InvalidTextureID};

            TextureID MetallicRoughnessTexture{k_InvalidTextureID};
            TextureID MetallicTexture{k_InvalidTextureID};
            TextureID RoughnessTexture{k_InvalidTextureID};

            glm::f32 _ignis_padding{};
        };

        struct DirectionalLight {
            glm::vec3 Direction{0.0f, -1.0f, 0.0f};
            glm::f32  Power{1.0f};
            glm::vec3 Color{0.0f};

            glm::f32 _ignis_padding1{};
        };

        struct PointLight {
            glm::vec3 Position{0.0f};
            glm::f32  Power{1.0f};
            glm::vec3 Color{0.0f};

            glm::f32 _ignis_padding{};
        };

        struct SpotLight {
            glm::vec3 Position{0.0f};
            glm::f32  Power{1.0f};
            glm::vec3 Direction{0.0f, 0.0f, 1.0f};
            glm::f32  CutOff{glm::radians(12.5f)};
            glm::vec3 Color{0.0f};
            glm::f32  OuterCutOff{glm::radians(15.0f)};
        };

        struct LightData {
            glm::u32 PointLightCount{0u};
            glm::u32 SpotLightCount{0u};

            glm::f32 _ignis_padding[2]{};
        };

        struct Mesh {
            glm::mat4x4 VertexTransform;
            glm::mat4x4 NormalTransform;

            MaterialID Material;

            glm::f32 _ignis_padding[3];
        };

        struct Instance {
            glm::mat4x4 VertexTransform;
            glm::mat4x4 NormalTransform;
        };

        struct Model {
            std::string Path;

            uint32_t MeshCount;
            uint32_t InstanceCount;

            Vulkan::Buffer VertexBuffer;
            Vulkan::Buffer IndexBuffer;
            Vulkan::Buffer MeshBuffer;
            Vulkan::Buffer InstanceBuffer;
            Vulkan::Buffer IndirectBuffer;

            gtl::flat_hash_map<MeshID, uint32_t> MeshToIndex;
            gtl::flat_hash_map<uint32_t, MeshID> IndexToMesh;

            gtl::flat_hash_map<InstanceID, uint32_t> InstanceToIndex;
            gtl::flat_hash_map<uint32_t, InstanceID> IndexToInstance;
        };

        struct Camera {
            glm::mat4x4 Projection;
            glm::mat4x4 View;

            glm::vec3 Position;
        };

        struct CameraPC {
            glm::mat4x4 ProjectionView;

            glm::vec3 Position;
        };

        struct Vertex {
            glm::vec3 Position{0.0f};
            glm::vec3 Normal{0.0f};
            glm::vec2 UV{0.0f};
            glm::vec4 Tangent{0.0f};
        };

        struct Settings {
            std::array<std::filesystem::path, 6> SkyboxFacePaths{};

            uint32_t MaxBindingCount = 2 << 20;

            FrameGraph *pFrameGraph = nullptr;
        };

       public:
        static void SetViewport(FrameGraph::ImageID color_image, FrameGraph::ImageID depth_image);
        static void SetCamera(const Camera &camera);

        static glm::mat4x4 GetNormalTransform(const glm::mat4x4 &model_transform);

#pragma region Light
        static void SetDirectionalLight(const DirectionalLight &light);
        static void SetPointLight(PointLightID id, const PointLight &light);
        static void SetSpotLight(SpotLightID id, const SpotLight &light);

        static PointLightID AddPointLight(const PointLight &light);
        static SpotLightID  AddSpotLight(const SpotLight &light);

        static void RemovePointLight(PointLightID id);
        static void RemoveSpotLight(SpotLightID id);

        static DirectionalLight GetDirectionalLight();
        static PointLight       GetPointLight(PointLightID id);
        static SpotLight        GetSpotLight(SpotLightID id);
#pragma endregion
#pragma region Model
        static void SetInstance(InstanceID id, const glm::mat4x4 &transform);

        static ModelID    AddModel(const std::filesystem::path &path);
        static InstanceID AddInstance(ModelID model_id, const glm::mat4x4 &transform);

        static void RemoveModel(ModelID id);
        static void RemoveInstance(InstanceID id);

        static std::string_view GetModelPath(ModelID id);

        static glm::mat4x4 GetInstance(InstanceID id);
#pragma endregion
       public:
        void initialize(const Settings &settings);
        void shutdown();

        void onRender(FrameGraph &frame_graph);

       private:
        IGNIS_IF_DEBUG(class State {
           public:
            ~State();
        };);

       private:
#pragma region Skybox
        void initializeSkybox(const std::array<std::filesystem::path, 6> &skybox_face_paths);
        void releaseSkybox();

        void generateIrradianceMap();

        void setSkyboxViewport(FrameGraph::ImageID color_image, FrameGraph::ImageID depth_image);

        void readSkyboxImage(FrameGraph::RenderPass &render_pass) const;
        void readSkyboxBuffers(FrameGraph::RenderPass &render_pass) const;

        void onSkyboxDraw(vk::CommandBuffer command_buffer) const;
#pragma endregion
#pragma region Material
        void initializeMaterials(uint32_t max_binding_count);
        void releaseMaterials();

        TextureID  addTexture(const Vulkan::Image &image, vk::ImageView view);
        MaterialID addMaterial(const Material &material);

        void removeTextureRC(TextureID id);
        void removeMaterialRC(MaterialID id);

        [[nodiscard]] Material getMaterial(MaterialID id) const;

        void readMaterialImages(FrameGraph::RenderPass &render_pass);
        void readMaterialBuffers(FrameGraph::RenderPass &render_pass) const;
#pragma endregion
#pragma region Light
        void initializeLights();
        void releaseLights();

        void readLightBuffers(FrameGraph::RenderPass &render_pass);
#pragma endregion
#pragma region Model
        void initializeModels(uint32_t max_binding_count);
        void releaseModels();

        void setModelViewport(FrameGraph::ImageID color_image, FrameGraph::ImageID depth_image);

        void readModelBuffers(FrameGraph::RenderPass &render_pass);

        void onModelDraw(vk::CommandBuffer command_buffer);

        void setInstance(InstanceID id, const glm::mat4x4 &transform);

        ModelID    addModel(const std::filesystem::path &path);
        InstanceID addInstance(ModelID model_id, const glm::mat4x4 &transform);

        void removeModel(ModelID id);
        void removeInstance(InstanceID id);

        std::string_view getModelPath(ModelID id);

        glm::mat4x4 getInstance(InstanceID id);

        void processNode(
            const std::filesystem::path &path,

            const aiScene     *ai_scene,
            const aiNode      *ai_node,
            const glm::mat4x4 &transform,

            std::vector<Vertex>   &vertices,
            std::vector<uint32_t> &indices,
            std::vector<Mesh>     &meshes,

            std::vector<vk::DrawIndexedIndirectCommand> &indirect_commands);

        void processMesh(
            const std::filesystem::path &path,

            const aiScene     *ai_scene,
            const aiMesh      *ai_mesh,
            const glm::mat4x4 &transform,

            std::vector<Vertex>   &vertices,
            std::vector<uint32_t> &indices,
            std::vector<Mesh>     &meshes,

            std::vector<vk::DrawIndexedIndirectCommand> &indirect_commands);

        MaterialID processMaterial(
            const std::filesystem::path &path,

            const aiScene    *ai_scene,
            const aiMaterial *ai_material,

            uint32_t ai_material_index);

        TextureID processTexture(
            const std::filesystem::path &directory,

            const aiScene    *ai_scene,
            const aiMaterial *ai_material,

            aiTextureType ai_texture_type,

            bool is_srgb = true);
#pragma endregion

       private:
        FrameGraph *m_pFrameGraph = nullptr;

        FrameGraph::ImageID m_ColorImage{FrameGraph::k_InvalidImageID};
        FrameGraph::ImageID m_DepthImage{FrameGraph::k_InvalidImageID};

        vk::DescriptorPool m_DescriptorPool = nullptr;

        vk::Sampler m_Sampler = nullptr;

        Camera m_Camera{};

#pragma region Skybox
        vk::DescriptorSetLayout m_SkyboxDescriptorLayout = nullptr;
        vk::DescriptorSet       m_SkyboxDescriptorSet    = nullptr;

        vk::PipelineLayout m_SkyboxPipelineLayout = nullptr;
        vk::Pipeline       m_SkyboxPipeline       = nullptr;

        Vulkan::Buffer m_SkyboxVertexBuffer{};
        Vulkan::Buffer m_SkyboxIndexBuffer{};

        Vulkan::Image m_SkyboxImage{};
        vk::ImageView m_SkyboxImageView{};

        Vulkan::Image m_IrradianceImage{};
        vk::ImageView m_IrradianceImageView{};

        FrameGraph::ImageID m_FrameGraphSkyboxImage{};
        FrameGraph::ImageID m_FrameGraphIrradianceImage{};

        FrameGraph::BufferInfo m_FrameGraphSkyboxVertexBuffer{};
        FrameGraph::BufferInfo m_FrameGraphSkyboxIndexBuffer{};
#pragma endregion
#pragma region Material
        vk::DescriptorSetLayout m_MaterialDescriptorLayout = nullptr;

        vk::DescriptorSet m_MaterialDescriptorSet = nullptr;

        TextureID  m_NextTextureID{k_InvalidTextureID};
        MaterialID m_NextMaterialID{k_InvalidMaterialID};

        std::vector<TextureID>  m_FreeTextureIDs{};
        std::vector<MaterialID> m_FreeMaterialIDs{};

        gtl::flat_hash_map<TextureID, Vulkan::Image> m_Textures{};
        gtl::flat_hash_map<TextureID, vk::ImageView> m_TextureViews{};

        gtl::flat_hash_set<MaterialID> m_Materials{};

        Vulkan::Buffer m_MaterialStagingBuffer{};
        Vulkan::Buffer m_MaterialBuffer{};

        SparseVector<TextureID, FrameGraph::ImageID> m_FrameGraphImages{};

        FrameGraph::BufferInfo m_FrameGraphMaterialBuffer{};

        gtl::flat_hash_map<std::string, MaterialID> m_LoadedMaterials{};
        gtl::flat_hash_map<std::string, TextureID>  m_LoadedTextures{};

        gtl::flat_hash_map<MaterialID, uint32_t> m_LoadedMaterialRCs{};
        gtl::flat_hash_map<TextureID, uint32_t>  m_LoadedTextureRCs{};

        gtl::flat_hash_map<MaterialID, std::string> m_LoadedMaterialPaths{};
        gtl::flat_hash_map<TextureID, std::string>  m_LoadedTexturePaths{};
#pragma endregion
#pragma region Light
        vk::DescriptorSetLayout m_LightDescriptorLayout = nullptr;

        vk::DescriptorSet m_LightDescriptorSet = nullptr;

        PointLightID m_NextPointLightID{k_InvalidPointLightID};
        SpotLightID  m_NextSpotLightID{k_InvalidSpotLightID};

        std::vector<PointLightID> m_FreePointLightIDs{};
        std::vector<SpotLightID>  m_FreeSpotLightIDs{};

        gtl::flat_hash_set<PointLightID> m_PointLights{};
        gtl::flat_hash_set<SpotLightID>  m_SpotLights{};

        gtl::flat_hash_map<PointLightID, uint32_t> m_PointLightToIndex{};
        gtl::flat_hash_map<SpotLightID, uint32_t>  m_SpotLightToIndex{};

        gtl::flat_hash_map<uint32_t, PointLightID> m_IndexToPointLight{};
        gtl::flat_hash_map<uint32_t, SpotLightID>  m_IndexToSpotLight{};

        Vulkan::Buffer m_DirectionalLightStagingBuffer{};
        Vulkan::Buffer m_PointLightStagingBuffer{};
        Vulkan::Buffer m_SpotLightStagingBuffer{};

        Vulkan::Buffer m_DirectionalLightBuffer{};
        Vulkan::Buffer m_PointLightBuffer{};
        Vulkan::Buffer m_SpotLightBuffer{};
        Vulkan::Buffer m_LightDataBuffer{};

        FrameGraph::BufferInfo m_FrameGraphDirectionalLightBuffer{};
        FrameGraph::BufferInfo m_FrameGraphPointLightBuffer{};
        FrameGraph::BufferInfo m_FrameGraphSpotLightBuffer{};
        FrameGraph::BufferInfo m_FrameGraphLightDataBuffer{};
#pragma endregion
#pragma region Model
        vk::DescriptorSetLayout m_ModelDescriptorLayout = nullptr;

        vk::DescriptorSet m_ModelDescriptorSet = nullptr;

        vk::PipelineLayout m_ModelPipelineLayout = nullptr;

        vk::Pipeline m_ModelPipeline = nullptr;

        MeshID     m_NextMeshID{k_InvalidMeshID};
        InstanceID m_NextInstanceID{k_InvalidInstanceID};
        ModelID    m_NextModelID{k_InvalidModelID};

        std::vector<MeshID>     m_FreeMeshIDs{};
        std::vector<InstanceID> m_FreeInstanceIDs{};
        std::vector<ModelID>    m_FreeModelIDs{};

        gtl::flat_hash_set<MeshID>     m_Meshes{};
        gtl::flat_hash_set<InstanceID> m_Instances{};

        gtl::flat_hash_map<ModelID, Model> m_Models{};

        gtl::flat_hash_map<MeshID, ModelID>     m_MeshToModel{};
        gtl::flat_hash_map<InstanceID, ModelID> m_InstanceToModel{};

        SparseVector<ModelID, FrameGraph::BufferInfo> m_FrameGraphModelVertexBuffers{};
        SparseVector<ModelID, FrameGraph::BufferInfo> m_FrameGraphModelIndexBuffers{};
        SparseVector<ModelID, FrameGraph::BufferInfo> m_FrameGraphModelMeshBuffers{};
        SparseVector<ModelID, FrameGraph::BufferInfo> m_FrameGraphModelInstanceBuffers{};
        SparseVector<ModelID, FrameGraph::BufferInfo> m_FrameGraphModelIndirectBuffers{};

        gtl::flat_hash_map<std::string, ModelID> m_LoadedModels{};
#pragma endregion

       private:
        static Render *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State;)
    };
}  // namespace Ignis

template <>
struct std::hash<Ignis::Render::TextureID> {
    size_t operator()(const Ignis::Render::TextureID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::Render::MaterialID> {
    size_t operator()(const Ignis::Render::MaterialID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::Render::PointLightID> {
    size_t operator()(const Ignis::Render::PointLightID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::Render::SpotLightID> {
    size_t operator()(const Ignis::Render::SpotLightID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::Render::MeshID> {
    size_t operator()(const Ignis::Render::MeshID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::Render::InstanceID> {
    size_t operator()(const Ignis::Render::InstanceID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};

template <>
struct std::hash<Ignis::Render::ModelID> {
    size_t operator()(const Ignis::Render::ModelID &id) const noexcept {
        return std::hash<uint32_t>{}(id.ID);
    }
};