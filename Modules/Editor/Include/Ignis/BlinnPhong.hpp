#include <Ignis/Engine.hpp>

namespace Ignis {
    class BlinnPhong final : public RenderModule<BlinnPhong> {
       public:
        typedef uint32_t SamplerHandle;
        typedef uint32_t TextureHandle;
        typedef uint32_t MaterialHandle;

        typedef uint32_t StaticMeshHandle;
        typedef uint32_t StaticModelHandle;
        typedef uint32_t StaticInstanceHandle;

        static constexpr SamplerHandle  k_InvalidSamplerHandle  = ~0u;
        static constexpr TextureHandle  k_InvalidTextureHandle  = ~0u;
        static constexpr MaterialHandle k_InvalidMaterialHandle = ~0u;

        static constexpr StaticMeshHandle     k_InvalidStaticMeshHandle     = ~0u;
        static constexpr StaticModelHandle    k_InvalidStaticModelHandle    = ~0u;
        static constexpr StaticInstanceHandle k_InvalidStaticInstanceHandle = ~0u;

        struct SampledTexture {
            TextureHandle Texture{k_InvalidTextureHandle};
            SamplerHandle Sampler{k_InvalidSamplerHandle};
        };

        struct Material {
            SampledTexture Diffuse{k_InvalidTextureHandle};
            SampledTexture Specular{k_InvalidTextureHandle};

            float Shininess;
        };

        struct StaticInstance {
            glm::mat4x4 Transform{1.0f};
        };

        struct Camera {
            glm::mat4x4 Projection{1.0f};
            glm::mat4x4 View{1.0f};
        };

        struct StaticDrawData {
            MaterialHandle       Material;
            StaticInstanceHandle Instance;
        };

        struct StaticMesh {
            Vulkan::Buffer VertexBuffer;
            Vulkan::Buffer IndexBuffer;

            uint32_t VertexCount;
            uint32_t IndexCount;

            MaterialHandle Material{k_InvalidMaterialHandle};
        };

        struct StaticModel {
            std::string Path;

            std::vector<StaticMeshHandle> Meshes;

            uint32_t InstanceCount;
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
        explicit BlinnPhong(const Settings &settings);

        ~BlinnPhong() override = default;

        void onAttach(FrameGraph &frame_graph) override;
        void onDetach(FrameGraph &frame_graph) override;

        void onRenderPass(FrameGraph::RenderPass &render_pass);
        void onDraw(vk::CommandBuffer command_buffer);

        void updateCamera(const Camera &camera) const;

        StaticModel &getStaticModelRef(StaticModelHandle handle);

        const StaticModel &getStaticModelConstRef(StaticModelHandle handle) const;

        const gtl::flat_hash_map<std::string, StaticModelHandle> &getStaticModelMap() const;

        StaticModelHandle loadStaticModel(const std::filesystem::path &path, FrameGraph &frame_graph);

        void unloadStaticModel(StaticModelHandle handle, FrameGraph &frame_graph);

        StaticInstance getStaticInstance(StaticModelHandle model, StaticInstanceHandle handle);

        void setStaticInstance(StaticModelHandle model, StaticInstanceHandle handle, const StaticInstance &instance);

        StaticInstanceHandle addStaticInstance(StaticModelHandle model, const StaticInstance &instance, FrameGraph &frame_graph);

        void removeStaticInstance(StaticModelHandle model, StaticInstanceHandle handle);

       private:
        void processStaticNode(
            const std::filesystem::path &directory,
            const aiScene *ai_scene, const aiNode *ai_node,
            StaticModel &model, FrameGraph &frame_graph);

        StaticMeshHandle processStaticMesh(
            const std::filesystem::path &directory,
            const aiScene *ai_scene, const aiMesh *ai_mesh,
            FrameGraph &frame_graph);

        MaterialHandle processMaterial(
            const std::filesystem::path &directory,
            const aiScene *ai_scene, const aiMaterial *ai_material,
            FrameGraph &frame_graph);

        TextureHandle processTexture(
            const std::filesystem::path &directory,
            const aiScene *ai_scene, const aiMaterial *ai_material,
            aiTextureType ai_texture_type,
            FrameGraph   &frame_graph);

        void removeStaticMesh(StaticMeshHandle handle, FrameGraph &frame_graph);

        SamplerHandle  addSampler(vk::Sampler sampler);
        TextureHandle  addTexture(const Vulkan::Image &image, vk::ImageView view, FrameGraph &frame_graph);
        MaterialHandle addMaterial(const Material &material, FrameGraph &frame_graph);

        void removeSampler(SamplerHandle handle);
        void removeTexture(TextureHandle handle, FrameGraph &frame_graph);
        void removeMaterial(MaterialHandle handle);

        Material getMaterial(MaterialHandle handle) const;

       private:
        vk::Format m_ColorAttachmentFormat;
        vk::Format m_DepthAttachmentFormat;

        uint32_t m_MaxBindingCount;

        vk::DescriptorSetLayout m_DescriptorLayout;
        vk::PipelineLayout      m_PipelineLayout;

        vk::DescriptorPool m_DescriptorPool;
        vk::DescriptorSet  m_DescriptorSet;

        vk::ShaderModule m_StaticShader;
        vk::Pipeline     m_StaticPipeline;

        SamplerHandle  m_NextSamplerHandle;
        TextureHandle  m_NextTextureHandle;
        MaterialHandle m_NextMaterialHandle;

        StaticMeshHandle  m_NextStaticMeshHandle;
        StaticModelHandle m_NextStaticModelHandle;

        gtl::flat_hash_map<StaticModelHandle, StaticInstanceHandle> m_NextStaticInstanceHandles;

        gtl::flat_hash_map<StaticModelHandle, uint32_t> m_NextStaticInstanceIndices;

        std::vector<SamplerHandle>  m_FreeSamplerHandles;
        std::vector<TextureHandle>  m_FreeTexturesHandles;
        std::vector<MaterialHandle> m_FreeMaterialHandles;

        std::vector<StaticMeshHandle>  m_FreeStaticMeshHandles;
        std::vector<StaticModelHandle> m_FreeStaticModelHandles;

        gtl::flat_hash_map<StaticModelHandle, std::vector<StaticInstanceHandle>> m_FreeStaticInstanceHandles;

        gtl::flat_hash_map<StaticModelHandle, std::vector<uint32_t>> m_FreeStaticInstanceIndices;

        gtl::flat_hash_map<SamplerHandle, vk::Sampler>   m_Samplers;
        gtl::flat_hash_map<TextureHandle, Vulkan::Image> m_Textures;
        gtl::flat_hash_map<TextureHandle, vk::ImageView> m_TextureViews;

        gtl::flat_hash_set<MaterialHandle> m_Materials;

        Vulkan::Buffer m_MaterialStagingBuffer;
        Vulkan::Buffer m_MaterialBuffer;
        Vulkan::Buffer m_CameraBuffer;

        SparseVector<StaticMeshHandle, StaticMesh>      m_StaticMeshes;
        SparseVector<StaticModelHandle, StaticModel>    m_StaticModels;
        SparseVector<StaticModelHandle, Vulkan::Buffer> m_StaticInstanceBuffers;

        gtl::flat_hash_map<StaticModelHandle, gtl::flat_hash_map<StaticInstanceHandle, uint32_t>> m_StaticInstanceToIndices;
        gtl::flat_hash_map<StaticModelHandle, gtl::flat_hash_map<uint32_t, StaticInstanceHandle>> m_IndicesToStaticInstance;

        SparseVector<TextureHandle, FrameGraph::ImageID> m_FrameGraphImages;

        FrameGraph::BufferInfo m_FrameGraphMaterialBuffer;
        FrameGraph::BufferInfo m_FrameGraphCameraBuffer;

        SparseVector<StaticMeshHandle, FrameGraph::BufferInfo>  m_FrameGraphStaticMeshVertexBuffers;
        SparseVector<StaticMeshHandle, FrameGraph::BufferInfo>  m_FrameGraphStaticMeshIndexBuffers;
        SparseVector<StaticModelHandle, FrameGraph::BufferInfo> m_FrameGraphStaticInstanceBuffers;

        gtl::flat_hash_map<std::string, TextureHandle>  m_LoadedTextures;
        gtl::flat_hash_map<std::string, MaterialHandle> m_LoadedMaterials;

        gtl::flat_hash_map<TextureHandle, std::string>  m_LoadedTextureNames;
        gtl::flat_hash_map<MaterialHandle, std::string> m_LoadedMaterialNames;

        gtl::flat_hash_map<SamplerHandle, uint32_t>  m_LoadedSamplerRCs;
        gtl::flat_hash_map<TextureHandle, uint32_t>  m_LoadedTextureRCs;
        gtl::flat_hash_map<MaterialHandle, uint32_t> m_LoadedMaterialRCs;

        gtl::flat_hash_map<std::string, StaticModelHandle> m_LoadedStaticModels;

        vk::Sampler m_NullSampler;

        SamplerHandle m_Sampler;
    };
}  // namespace Ignis