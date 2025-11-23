#pragma once

#include <Ignis/Vulkan/Types.hpp>

namespace Ignis::Vulkan::Shader {
    vk::ShaderModule CreateModuleFromSPV(std::span<uint32_t> code, vk::Device device);
}
