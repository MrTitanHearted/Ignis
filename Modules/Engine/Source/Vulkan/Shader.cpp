#include <Ignis/Vulkan/Shader.hpp>

namespace Ignis::Vulkan::Shader {
    vk::ShaderModule CreateModuleFromSPV(const std::span<uint32_t> code, const vk::Device device) {
        vk::ShaderModuleCreateInfo vk_shader_module_create_info{};
        vk_shader_module_create_info.setCode(code);

        auto [result, vk_shader_module] = device.createShaderModule(vk_shader_module_create_info);
        DIGNIS_VK_CHECK(result);
        return vk_shader_module;
    }
}  // namespace Ignis::Vulkan::Shader
