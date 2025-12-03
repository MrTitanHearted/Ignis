#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Assets.hpp>
#include <Ignis/Vulkan.hpp>
#include <Ignis/Render.hpp>

namespace Ignis {
    class BlinnPhongScene final : public IGPUScene<BlinnPhongScene> {
       public:
        typedef uint32_t TextureHandle;
        typedef uint32_t SamplerHandle;
        typedef uint32_t VertexBufferHandle;
        typedef uint32_t IndexBufferHandle;
        typedef uint32_t CameraHandle;
        typedef uint32_t MeshHandle;
        typedef uint32_t MaterialHandle;
        typedef uint32_t ModelHandle;

        static constexpr uint32_t k_InvalidTextureHandle      = UINT32_MAX;
        static constexpr uint32_t k_InvalidSamplerHandle      = UINT32_MAX;
        static constexpr uint32_t k_InvalidVertexBufferHandle = UINT32_MAX;
        static constexpr uint32_t k_InvalidIndexBufferHandle  = UINT32_MAX;
        static constexpr uint32_t k_InvalidCameraHandle       = UINT32_MAX;
        static constexpr uint32_t k_InvalidMeshHandle         = UINT32_MAX;
        static constexpr uint32_t k_InvalidMaterialHandle     = UINT32_MAX;
        static constexpr uint32_t k_InvalidModelHandle        = UINT32_MAX;

        struct Vertex {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 UV;
        };

        struct SampledTexture {
            TextureHandle Texture;
            SamplerHandle Sampler;
        };

        struct Material {
            SampledTexture DiffuseTexture;
            SampledTexture SpecularTexture;

            float SpecularPower;
        };

        struct Mesh {
            VertexBufferHandle VertexBuffer;
            IndexBufferHandle  IndexBuffer;

            MaterialHandle Material;
        };

        struct Camera {
            glm::mat4x4 Projection;
            glm::mat4x4 View;
        };

        struct DrawData {
            CameraHandle Camera;
            MeshHandle   Mesh;

            uint32_t __padding[2];

            glm::mat4x4 Model;
        };

        struct NodeMesh {
            MeshHandle Mesh;

            uint32_t VertexCount;
            uint32_t IndexCount;
        };

        struct Node {
            std::vector<Node> Children;

            std::vector<NodeMesh> Meshes;
        };

        struct Model {
            glm::mat4x4 Transform;

            Node RootNode;
        };

       public:
        BlinnPhongScene(vk::Format color_attachment_format, vk::Format depth_attachment_format);
        ~BlinnPhongScene() override = default;

        void initialize(FrameGraph &frame_graph) override;
        void release(FrameGraph &frame_graph) override;

        void onRender(FrameGraph::RenderPass &render_pass);

        void onDraw(vk::CommandBuffer command_buffer);

        void updateSceneCamera(const Camera &camera);

        ModelHandle loadModel(const std::filesystem::path &path, FrameGraph &frame_graph);

        void removeModel(ModelHandle handle, FrameGraph &frame_graph);

       private:
        Node processNode(const std::filesystem::path &directory, FrameGraph &frame_graph, const aiScene *ai_scene, const aiNode *ai_node);

        void removeNode(const Node &node, FrameGraph &frame_graph);

        void drawModel(const Model &model, vk::CommandBuffer command_buffer) const;
        void drawNode(DrawData &draw_data, const Node &node, vk::CommandBuffer command_buffer) const;

        TextureHandle      addTexture(const Vulkan::Image &image, vk::ImageView view, FrameGraph &frame_graph);
        SamplerHandle      addSampler(vk::Sampler sampler);
        VertexBufferHandle addVertexBuffer(const Vulkan::Buffer &buffer, FrameGraph &frame_graph);
        IndexBufferHandle  addIndexBuffer(const Vulkan::Buffer &buffer, FrameGraph &frame_graph);
        MaterialHandle     addMaterial(const Vulkan::Buffer &buffer, FrameGraph &frame_graph);
        MeshHandle         addMesh(const Vulkan::Buffer &buffer, FrameGraph &frame_graph);
        CameraHandle       addCamera(const Vulkan::Buffer &buffer, FrameGraph &frame_graph);

        void removeTexture(TextureHandle handle, FrameGraph &frame_graph);
        void removeSampler(SamplerHandle handle);
        void removeVertexBuffer(VertexBufferHandle handle, FrameGraph &frame_graph);
        void removeIndexBuffer(IndexBufferHandle handle, FrameGraph &frame_graph);
        void removeMaterial(MaterialHandle handle, FrameGraph &frame_graph);
        void removeMesh(MeshHandle handle, FrameGraph &frame_graph);
        void removeCamera(CameraHandle handle, FrameGraph &frame_graph);

        Mesh     getMesh(MeshHandle handle);
        Material getMaterial(MaterialHandle handle);

       private:
        vk::Format m_ColorAttachmentFormat;
        vk::Format m_DepthAttachmentFormat;

        vk::DescriptorSetLayout m_DescriptorSetLayout;

        vk::ShaderModule   m_BlinnPhongShader;
        vk::PipelineLayout m_PipelineLayout;
        vk::Pipeline       m_Pipeline;

        vk::DescriptorPool m_DescriptorPool;
        vk::DescriptorSet  m_DescriptorSet;

        TextureHandle      m_NextTextureHandle;
        SamplerHandle      m_NextSamplerHandle;
        VertexBufferHandle m_NextVertexBufferHandle;
        IndexBufferHandle  m_NextIndexBufferHandle;
        MeshHandle         m_NextMeshHandle;
        MaterialHandle     m_NextMaterialHandle;
        CameraHandle       m_NextCameraHandle;
        ModelHandle        m_NextModelHandle;

        std::vector<TextureHandle>      m_FreeTextureHandles;
        std::vector<SamplerHandle>      m_FreeSamplerHandles;
        std::vector<VertexBufferHandle> m_FreeVertexBufferHandles;
        std::vector<IndexBufferHandle>  m_FreeIndexBufferHandles;
        std::vector<MeshHandle>         m_FreeMeshHandles;
        std::vector<MaterialHandle>     m_FreeMaterialHandles;
        std::vector<CameraHandle>       m_FreeCameraHandles;
        std::vector<ModelHandle>        m_FreeModelHandles;

        gtl::flat_hash_map<TextureHandle, Vulkan::Image>       m_Textures;
        gtl::flat_hash_map<TextureHandle, vk::ImageView>       m_TextureViews;
        gtl::flat_hash_map<SamplerHandle, vk::Sampler>         m_Samplers;
        gtl::flat_hash_map<VertexBufferHandle, Vulkan::Buffer> m_VertexBuffers;
        gtl::flat_hash_map<IndexBufferHandle, Vulkan::Buffer>  m_IndexBuffers;
        gtl::flat_hash_map<MeshHandle, Vulkan::Buffer>         m_Meshes;
        gtl::flat_hash_map<MaterialHandle, Vulkan::Buffer>     m_Materials;
        gtl::flat_hash_map<CameraHandle, Vulkan::Buffer>       m_Cameras;

        SparseVector<ModelHandle, Model> m_Models;

        SparseVector<TextureHandle, FrameGraph::ImageID>         m_FrameGraphImageIDs;
        SparseVector<VertexBufferHandle, FrameGraph::BufferInfo> m_FrameGraphVertexBufferIDs;
        SparseVector<IndexBufferHandle, FrameGraph::BufferInfo>  m_FrameGraphIndexBufferIDs;
        SparseVector<MaterialHandle, FrameGraph::BufferInfo>     m_FrameGraphMaterialIDs;
        SparseVector<MeshHandle, FrameGraph::BufferInfo>         m_FrameGraphMeshIDs;
        SparseVector<CameraHandle, FrameGraph::BufferInfo>       m_FrameGraphCameraIDs;

        gtl::flat_hash_map<std::string, TextureHandle> m_LoadedTextures;
        gtl::flat_hash_map<TextureHandle, uint32_t>    m_LoadedTextureRCs;

        SamplerHandle m_SceneSampler;
        CameraHandle  m_SceneCamera;
    };
}  // namespace Ignis