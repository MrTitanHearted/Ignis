#pragma once

#include <Ignis/Core/Defines.hpp>

#include <span>
#include <queue>
#include <deque>
#include <tuple>
#include <mutex>
#include <array>
#include <vector>
#include <string>
#include <ranges>
#include <random>
#include <format>
#include <chrono>
#include <atomic>
#include <memory>
#include <fstream>
#include <variant>
#include <utility>
#include <optional>
#include <algorithm>
#include <execution>
#include <typeinfo>
#include <typeindex>
#include <filesystem>
#include <type_traits>

#include <function2/function2.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/sink.h>

#include <gtl/phmap.hpp>
#include <gtl/vector.hpp>

#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.hpp>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
