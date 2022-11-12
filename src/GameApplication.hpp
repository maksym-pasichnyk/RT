#pragma once

#include "Application.hpp"

#include <random>

struct Options;
struct PlayerInput;
struct MouseHandler;
struct ImGuiRenderer;

struct GameApplication final : Application, WindowDelegate {
public:
    GameApplication();
    ~GameApplication() override;

public:
    void run();

private:
    void update(f32 dt);
    void render();
    void updateTextureAttachments();
    void createPresentPipelineObjects();
    void createDefaultPipelineObjects();
    void createRaytracePipelineObjects();

private:
    void windowDidResize() override;
    void windowMouseEvent(i32 button, i32 action, i32 mods) override;
    void windowCursorEvent(f64 x, f64 y) override;
    void windowMouseEnter() override;
    void windowShouldClose() override;
    void windowKeyEvent(i32 keycode, i32 scancode, i32 action, i32 mods) override;

private:
    Arc<Window> window = {};
    Arc<MouseHandler> mouseHandler = {};

    Arc<vfx::Context> context = {};
    Arc<vfx::Device> device = {};
    Arc<vfx::Layer> swapchain = {};

    Arc<vfx::CommandQueue> commandQueue = {};

    Arc<Options> options = {};
    Arc<PlayerInput> playerInput = {};
    Arc<ImGuiRenderer> imguiRenderer = {};

    Arc<vfx::Sampler> defaultTextureSampler = {};
    Arc<vfx::Texture> colorAttachmentTexture = {};
    Arc<vfx::Texture> depthAttachmentTexture = {};

    u64 traceIndex = 0;
    std::vector<glm::vec3> rayDirections{};
    std::vector<glm::vec4> traceAttachmentPixels{};
    std::vector<glm::vec4> colorAttachmentPixels{};

    Arc<vfx::RenderPipelineState> presentPipelineState = {};
    Arc<vfx::ResourceGroup> presentResourceGroup = {};

    Arc<vfx::RenderPipelineState> defaultPipelineState = {};
    Arc<vfx::ResourceGroup> defaultResourceGroup = {};

    Arc<vfx::ComputePipelineState> raytracePipelineState = {};
    Arc<vfx::ResourceGroup> raytraceResourceGroup = {};

    Arc<vfx::Buffer> sceneConstantsBuffer = {};

    glm::vec3 cameraPosition = {};
    glm::vec3 cameraRotation = {};

    std::default_random_engine random{};

    volatile bool running = false;
};
