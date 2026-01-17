#include <Ignis/Editor.hpp>

namespace Ignis {
    void Editor::RenderModelPanel(ModelPanelState &state) {
        std::vector<Render::ModelID>                                         models_to_remove{};
        gtl::flat_hash_map<Render::ModelID, std::vector<Render::InstanceID>> instances_to_remove{};

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Load Model Section
            static char model_path[1024];
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##ModelPath", "Enter model path...", model_path, 1024);

            if (ImGui::Button("Load Model", ImVec2(-1, 0))) {
                if (const std::string_view path = model_path;
                    std::filesystem::exists(path)) {
                    if (const auto model = Render::AddModel(path);
                        Render::k_InvalidModelID != model &&
                        !state.ModelToUIState.contains(model)) {
                        state.ModelToUIState[model] = ModelUIState{};
                        state.Models.push_back(model);
                    } else {
                        DIGNIS_LOG_ENGINE_WARN("No model exists on path: {}", path);
                    }
                } else {
                    DIGNIS_LOG_ENGINE_WARN("No model exists on path: {}", path);
                }
            }

            // Loaded Models Section
            if (state.Models.empty()) {
                ImGui::TextDisabled("No models loaded");
            }

            for (uint32_t model_index = 0; model_index < state.Models.size(); ++model_index) {
                const auto &model = state.Models[model_index];

                auto &model_state = state.ModelToUIState[model];

                ImGui::PushID(static_cast<int32_t>(model_index));

                const std::string model_label = FormatString("Model {}", Render::GetModelPath(model));

                if (ImGui::TreeNodeEx(model_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Add Instance Section
                    if (ImGui::TreeNodeEx("Add Instance", ImGuiTreeNodeFlags_DefaultOpen)) {
                        static glm::vec3 add_position{0.0f};
                        static glm::vec3 add_rotation{0.0f};
                        static float     add_scale = 1.0f;

                        ImGui::DragFloat3("Position", glm::value_ptr(add_position), 0.1f);
                        ImGui::DragFloat3("Rotation", glm::value_ptr(add_rotation), 1.0f);
                        ImGui::DragFloat("Scale", &add_scale, 0.0005f, 0.001f, 10.0f, "%.4f");

                        if (ImGui::Button("Add", ImVec2(-1, 0))) {
                            const glm::mat4 scaled     = glm::scale(glm::mat4{1.0f}, glm::vec3{add_scale});
                            const glm::mat4 rotated    = glm::toMat4(glm::quat(glm::radians(add_rotation)));
                            const glm::mat4 translated = glm::translate(glm::mat4{1.0f}, add_position);
                            const glm::mat4 transform  = translated * rotated * scaled;

                            const auto instance = Render::AddInstance(model, transform);
                            model_state.Instances.push_back(instance);
                            state.InstanceToUIState[instance] = InstanceUIState{
                                add_position,
                                add_rotation,
                                add_scale,
                                true};
                        }

                        ImGui::TreePop();
                    }

                    // Instances List
                    if (model_state.Instances.empty()) {
                        ImGui::TextDisabled("No instances");
                    } else {
                        const std::string instances_label = "Instances (" + std::to_string(model_state.Instances.size()) + ")";
                        if (ImGui::TreeNodeEx(instances_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                            for (uint32_t instance_index = 0; instance_index < model_state.Instances.size(); ++instance_index) {
                                const auto &instance = model_state.Instances[instance_index];

                                auto &[position, rotation, scale, initialized] = state.InstanceToUIState[instance];

                                ImGui::PushID(static_cast<int32_t>(instance_index));

                                const std::string instance_label = "Instance " + std::to_string(instance_index);
                                if (ImGui::TreeNode(instance_label.c_str())) {
                                    bool changed = ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f);
                                    changed |= ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 1.0f);
                                    changed |= ImGui::DragFloat("Scale", &scale, 0.0005f, 0.001f, 10.0f, "%.4f");

                                    if (changed) {
                                        const glm::mat4 scaled     = glm::scale(glm::mat4{1.0f}, glm::vec3{scale});
                                        const glm::mat4 rotated    = glm::toMat4(glm::quat(glm::radians(rotation)));
                                        const glm::mat4 translated = glm::translate(glm::mat4{1.0f}, position);
                                        const glm::mat4 transform  = translated * rotated * scaled;

                                        Render::SetInstance(instance, transform);
                                    }

                                    if (ImGui::Button("Remove", ImVec2(-1, 0))) {
                                        instances_to_remove[model].push_back(instance);
                                    }

                                    ImGui::TreePop();
                                }

                                ImGui::PopID();
                            }
                            ImGui::TreePop();
                        }
                    }

                    ImGui::Spacing();
                    if (ImGui::Button("Unload Model", ImVec2(-1, 0))) {
                        models_to_remove.push_back(model);
                    }

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
        }

        // Remove instances
        for (auto &[model, instances] : instances_to_remove) {
            while (!instances.empty()) {
                const auto instance = instances.back();
                instances.pop_back();

                Render::RemoveInstance(instance);
                state.InstanceToUIState.erase(instance);

                if (const auto it = std::ranges::find(state.ModelToUIState[model].Instances, instance);
                    it != std::end(state.ModelToUIState[model].Instances))
                    state.ModelToUIState[model].Instances.erase(it);
            }
        }

        // Remove models
        while (!models_to_remove.empty()) {
            const auto model = models_to_remove.back();
            models_to_remove.pop_back();

            // Remove all instances of this model
            for (const auto instance : state.ModelToUIState[model].Instances) {
                state.InstanceToUIState.erase(instance);
            }

            Render::RemoveModel(model);
            state.ModelToUIState.erase(model);

            if (const auto it = std::ranges::find(state.Models, model);
                it != std::end(state.Models))
                state.Models.erase(it);
        }
    }

    void Editor::RenderLightPanel(LightPanelState &state) {
        std::vector<Render::PointLightID> point_lights_to_remove{};
        std::vector<Render::SpotLightID>  spot_lights_to_remove{};

        // Directional Light
        if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushID("DirectionalLight");

            bool changed = ImGui::DragFloat3("Direction", glm::value_ptr(state.DirectionalLight.Direction), 0.0001f, -1.0f, 1.0f, "%.5f");
            changed |= ImGui::DragFloat("Power", &state.DirectionalLight.Power, 0.001f, 0.0f, 1000.0f, "%.4f");
            changed |= ImGui::ColorEdit3("Color", glm::value_ptr(state.DirectionalLight.Color));

            if (changed) {
                Render::SetDirectionalLight(state.DirectionalLight);
            }

            ImGui::PopID();
        }

        // Point Lights
        if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Add Point Light
            if (ImGui::TreeNodeEx("Add Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushID("AddPointLight");

                static Render::PointLight add_light{};

                ImGui::DragFloat3("Position", glm::value_ptr(add_light.Position), 0.01f);
                ImGui::DragFloat("Power", &add_light.Power, 0.01f, 0.0f);
                ImGui::ColorEdit3("Color", glm::value_ptr(add_light.Color));

                if (ImGui::Button("Add", ImVec2(-1, 0))) {
                    const auto point_light = Render::AddPointLight(add_light);
                    state.PointLights.push_back(point_light);
                    state.PointLightValues[point_light] = add_light;
                }

                ImGui::PopID();
                ImGui::TreePop();
            }

            // Existing Point Lights
            if (state.PointLights.empty()) {
                ImGui::TextDisabled("No point lights");
            } else {
                const std::string lights_label = "Active Lights (" + std::to_string(state.PointLights.size()) + ")";
                if (ImGui::TreeNodeEx(lights_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    for (uint32_t light_index = 0; light_index < state.PointLights.size(); ++light_index) {
                        const auto &point_light = state.PointLights[light_index];
                        auto       &light       = state.PointLightValues[point_light];

                        ImGui::PushID(static_cast<int32_t>(light_index));

                        const std::string light_label = "Light " + std::to_string(light_index);
                        if (ImGui::TreeNode(light_label.c_str())) {
                            bool changed = ImGui::DragFloat3("Position", glm::value_ptr(light.Position), 0.01f);
                            changed |= ImGui::DragFloat("Power", &light.Power, 0.01f, 0.0f);
                            changed |= ImGui::ColorEdit3("Color", glm::value_ptr(light.Color));

                            if (changed) {
                                Render::SetPointLight(point_light, light);
                            }

                            if (ImGui::Button("Remove", ImVec2(-1, 0))) {
                                point_lights_to_remove.push_back(point_light);
                            }

                            ImGui::TreePop();
                        }

                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
            }
        }

        // Spot Lights
        if (ImGui::CollapsingHeader("Spot Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Add Spot Light
            if (ImGui::TreeNodeEx("Add Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushID("AddSpotLight");

                static Render::SpotLight add_light{};

                ImGui::DragFloat3("Position", glm::value_ptr(add_light.Position), 0.01f);
                ImGui::DragFloat3("Direction", glm::value_ptr(add_light.Direction), 0.01f);
                ImGui::DragFloat("CutOff", &add_light.CutOff, 0.01f, 0.0f);
                ImGui::DragFloat("OuterCutOff", &add_light.OuterCutOff, 0.01f, 0.0f);
                ImGui::DragFloat("Power", &add_light.Power, 0.01f, 0.0f);
                ImGui::ColorEdit3("Color", glm::value_ptr(add_light.Color));

                if (ImGui::Button("Add", ImVec2(-1, 0))) {
                    const auto spot_light = Render::AddSpotLight(add_light);
                    state.SpotLights.push_back(spot_light);
                    state.SpotLightValues[spot_light] = add_light;
                }

                ImGui::PopID();
                ImGui::TreePop();
            }

            // Existing Spot Lights
            if (state.SpotLights.empty()) {
                ImGui::TextDisabled("No spot lights");
            } else {
                const std::string lights_label = "Active Lights (" + std::to_string(state.SpotLights.size()) + ")";
                if (ImGui::TreeNodeEx(lights_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    for (uint32_t light_index = 0; light_index < state.SpotLights.size(); ++light_index) {
                        const auto &spot_light = state.SpotLights[light_index];
                        auto       &light      = state.SpotLightValues[spot_light];

                        ImGui::PushID(static_cast<int32_t>(light_index));

                        const std::string light_label = "Light " + std::to_string(light_index);
                        if (ImGui::TreeNode(light_label.c_str())) {
                            bool changed = ImGui::DragFloat3("Position", glm::value_ptr(light.Position), 0.01f);
                            changed |= ImGui::DragFloat3("Direction", glm::value_ptr(light.Direction), 0.01f);
                            changed |= ImGui::DragFloat("CutOff", &light.CutOff, 0.01f, 0.0f);
                            changed |= ImGui::DragFloat("OuterCutOff", &light.OuterCutOff, 0.01f, 0.0f);
                            changed |= ImGui::DragFloat("Power", &light.Power, 0.01f, 0.0f);
                            changed |= ImGui::ColorEdit3("Color", glm::value_ptr(light.Color));

                            if (changed) {
                                Render::SetSpotLight(spot_light, light);
                            }

                            if (ImGui::Button("Remove", ImVec2(-1, 0))) {
                                spot_lights_to_remove.push_back(spot_light);
                            }

                            ImGui::TreePop();
                        }

                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
            }
        }

        // Remove point lights
        while (!point_lights_to_remove.empty()) {
            const auto point_light = point_lights_to_remove.back();
            point_lights_to_remove.pop_back();

            Render::RemovePointLight(point_light);
            state.PointLightValues.erase(point_light);

            if (const auto it = std::ranges::find(state.PointLights, point_light);
                it != std::end(state.PointLights))
                state.PointLights.erase(it);
        }

        // Remove spot lights
        while (!spot_lights_to_remove.empty()) {
            const auto spot_light = spot_lights_to_remove.back();
            spot_lights_to_remove.pop_back();

            Render::RemoveSpotLight(spot_light);
            state.SpotLightValues.erase(spot_light);

            if (const auto it = std::ranges::find(state.SpotLights, spot_light);
                it != std::end(state.SpotLights))
                state.SpotLights.erase(it);
        }
    }
}  // namespace Ignis