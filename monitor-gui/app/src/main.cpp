#if defined(__APPLE__)
#include <OpenGL/gl.h>
#endif

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "bridge_client.hpp"

#include <cstdio>
#include <cstdlib>
#include <map>

namespace {

constexpr uint32_t kSensorSpeedId = 0x100;
constexpr uint32_t kActuatorCmdId = 0x200;

double rate_hz(const monitor::FrameInfo& info) {
    if (info.count < 2) {
        return 0.0;
    }
    double span = info.last_seen - info.first_seen;
    if (span <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(info.count - 1) / span;
}

} // namespace

int main(int argc, char** argv) {
    uint16_t port = 35000;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }

    monitor::BridgeClient bridge("127.0.0.1", port);

    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(900, 600, "can-ecu-sim monitor", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        std::map<uint32_t, monitor::FrameInfo> frames = bridge.snapshot();

        ImGui::Begin("CAN Bus Monitor");

        if (bridge.connected()) {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "Bridge connected (127.0.0.1:%d)",
                                port);
        } else {
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f),
                                "Bridge not connected (127.0.0.1:%d) - retrying...", port);
        }

        ImGui::Separator();
        ImGui::Text("ECU State");
        {
            auto sensor = frames.find(kSensorSpeedId);
            if (sensor != frames.end() && sensor->second.dlc >= 2) {
                uint16_t speed = static_cast<uint16_t>(sensor->second.data[0]) |
                                 (static_cast<uint16_t>(sensor->second.data[1]) << 8);
                ImGui::Text("Sensor (0x%03X): %u", kSensorSpeedId, speed);
            } else {
                ImGui::TextDisabled("Sensor (0x%03X): no data yet", kSensorSpeedId);
            }

            auto actuator = frames.find(kActuatorCmdId);
            if (actuator != frames.end() && actuator->second.dlc >= 1) {
                bool on = actuator->second.data[0] != 0;
                ImGui::Text("Actuator (0x%03X): %s", kActuatorCmdId, on ? "ON" : "OFF");
            } else {
                ImGui::TextDisabled("Actuator (0x%03X): no data yet", kActuatorCmdId);
            }
        }

        ImGui::Separator();
        ImGui::Text("Live CAN Traffic (%zu IDs)", frames.size());

        if (ImGui::BeginTable("frames", 5,
                               ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                   ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("DLC");
            ImGui::TableSetupColumn("Data");
            ImGui::TableSetupColumn("Count");
            ImGui::TableSetupColumn("Rate (Hz)");
            ImGui::TableHeadersRow();

            for (const auto& [id, info] : frames) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%03X", id);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", info.dlc);

                ImGui::TableSetColumnIndex(2);
                char hex[3 * 8 + 1] = {0};
                for (uint8_t i = 0; i < info.dlc; ++i) {
                    std::snprintf(hex + i * 3, 4, "%02X ", info.data[i]);
                }
                ImGui::TextUnformatted(hex);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%llu", static_cast<unsigned long long>(info.count));

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%.1f", rate_hz(info));
            }

            ImGui::EndTable();
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
