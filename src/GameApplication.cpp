#include "GameApplication.hpp"

#include "Camera.hpp"
#include "Options.hpp"
#include "DrawList.hpp"
#include "PlayerInput.hpp"
#include "MouseHandler.hpp"
#include "ImGuiRenderer.hpp"

#include "imgui.h"
#include "GLFW/glfw3.h"
#include "ThreadPool.hpp"
#include "spdlog/spdlog.h"

#include <random>

GameApplication::GameApplication() {
    window = Arc<Window>::alloc(800, 600);
    window->setTitle("Game");
    window->delegate = this;

    context = Arc<vfx::Context>::alloc();
    device = Arc<vfx::Device>::alloc(context);
    swapchain = Arc<vfx::Layer>::alloc(device);
    swapchain->surface = window->makeSurface(context);
    swapchain->colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    swapchain->pixelFormat = vk::Format::eB8G8R8A8Unorm;
    swapchain->displaySyncEnabled = true;
    swapchain->updateDrawables();

    commandQueue = device->makeCommandQueue();

    options = Arc<Options>::alloc();
    playerInput = Arc<PlayerInput>::alloc(options);
    mouseHandler = Arc<MouseHandler>::alloc(window);
    imguiRenderer = Arc<ImGuiRenderer>::alloc(device, window);

    textureSampler = device->makeSampler(vk::SamplerCreateInfo{
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat
    });

    createDefaultPipelineObjects();
    createPresentPipelineObjects();
    createRaytracePipelineObjects();

    updateTextureAttachments();

    sceneConstantsBuffer = device->makeBuffer(
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        sizeof(SceneConstants),
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
    );
    defaultResourceGroup->setBuffer(sceneConstantsBuffer, 0, 0);
}

GameApplication::~GameApplication() {
    device->waitIdle();
}

void GameApplication::run() {
    cameraPosition = glm::vec3(0, 0, -8);
    cameraRotation = glm::vec3(0, 0, 0);

    f64 timeSinceStart = glfwGetTime();

    running = true;
    while (running) {
        f64 currentTime = glfwGetTime();
        f32 deltaTime = f32(currentTime - timeSinceStart);
        timeSinceStart = currentTime;

        pollEvents();
        update(deltaTime);
        render();
    }
}

void GameApplication::update(f32 dt) {
    playerInput->tick();

    if (mouseHandler->isMouseGrabbed()) {
        auto direction = glm::ivec3(
            playerInput->rightImpulse,
            0,
            playerInput->forwardImpulse
        );

        if (direction != glm::ivec3()) {
            auto orientation = glm::mat3x3(glm::quat(glm::radians(cameraRotation)));
            auto velocity = glm::normalize(orientation * glm::vec3(direction)) * 10.0f;

            cameraPosition += velocity * dt;
            accumulateFrame = 0;
        }

        f64 d4 = 2.0 * 0.5 * 0.6 + 0.2;
        f64 d5 = d4 * d4 * d4 * 8.0;

        auto delta = mouseHandler->getMouseDelta();
        if (delta != glm::dvec2()) {
            mouseHandler->resetMouseDelta();

            cameraRotation.x += f32(delta.y * d5) * dt;
            cameraRotation.y += f32(delta.x * d5) * dt;
            cameraRotation.x = glm::clamp(cameraRotation.x, -90.0f, 90.0f);
            accumulateFrame = 0;
        }

    }
}

void GameApplication::render() {
    imguiRenderer->beginFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin("Debug info");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
    imguiRenderer->endFrame();

    auto cmd = commandQueue->makeCommandBuffer();
    cmd->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

//    // todo: move to a better place
//    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
//        .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
//        .srcAccessMask = vk::AccessFlags2{},
//        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
//        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
//        .oldLayout = vk::ImageLayout::eUndefined,
//        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
//        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .image = colorAttachmentTexture->image,
//        .subresourceRange = vk::ImageSubresourceRange{
//            .aspectMask = vk::ImageAspectFlagBits::eColor,
//            .levelCount = 1,
//            .layerCount = 1
//        }
//    });
//    // todo: move to a better place
//    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
//        .srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
//        .srcAccessMask = vk::AccessFlags2{},
//        .dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
//        .dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
//        .oldLayout = vk::ImageLayout::eUndefined,
//        .newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
//        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .image = depthAttachmentTexture->image,
//        .subresourceRange = vk::ImageSubresourceRange{
//            .aspectMask = vk::ImageAspectFlagBits::eDepth,
//            .levelCount = 1,
//            .layerCount = 1
//        }
//    });
//    cmd->flushBarriers();

    auto imageWidth = colorAttachmentTexture->size.width;
    auto imageHeight = colorAttachmentTexture->size.height;

    auto cameraAspect = f32(imageWidth) / f32(imageHeight);
    auto projectionMatrix = Camera::getInfinityProjectionMatrix(60.0f, cameraAspect, 0.01f);
    auto worldToCameraMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), cameraPosition) * glm::mat4x4(glm::quat(glm::radians(cameraRotation))));
    auto viewProjectionMatrix = projectionMatrix * worldToCameraMatrix;
    auto inverseViewProjectionMatrix = glm::inverse(viewProjectionMatrix);

    auto scene = SceneConstants{
        .ProjectionMatrix = projectionMatrix,
        .WorldToCameraMatrix = worldToCameraMatrix,
        .ViewProjectionMatrix = viewProjectionMatrix,
        .InverseViewProjectionMatrix = inverseViewProjectionMatrix,
        .CameraPosition = cameraPosition
    };
    auto resolution = glm::vec2(f32(imageWidth), f32(imageHeight));

    // todo: move to a better place
    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
        .srcAccessMask = vk::AccessFlagBits2{},
        .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = colorAttachmentTexture->image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .layerCount = 1
        }
    });
    // todo: move to a better place
    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
        .srcAccessMask = vk::AccessFlagBits2{},
        .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = accumulateAttachmentTexture->image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .layerCount = 1
        }
    });
    cmd->flushBarriers();
    accumulateFrame += 1;

    struct ComputeData {
        glm::vec3 cameraPosition;
        float time;
        int accumulateFrame;
    };
    auto computeData = ComputeData{
        .cameraPosition = cameraPosition,
        .time = float(glfwGetTime()),
        .accumulateFrame = accumulateFrame
    };

    cmd->setComputePipelineState(raytracePipelineState);
    cmd->bindResourceGroup(raytraceResourceGroup, 0);
    cmd->pushConstants(vk::ShaderStageFlagBits::eCompute, 0, sizeof(ComputeData), &computeData);

    cmd->dispatch(
        (colorAttachmentTexture->size.width / 10) + 1,
        (colorAttachmentTexture->size.height / 10) + 1,
        1
    );

    // todo: move to a better place
    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
        .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
        .oldLayout = vk::ImageLayout::eGeneral,
        .newLayout = vk::ImageLayout::eReadOnlyOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = colorAttachmentTexture->image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .layerCount = 1
        }
    });
    cmd->flushBarriers();

    auto rd_ptr = static_cast<glm::vec4*>(rayDirections->map());
    for (i32 y = 0; y < imageHeight; ++y) {
        for (i32 x = 0; x < imageWidth; ++x) {
            auto uv = 2.f * glm::vec2(x, y) / resolution - 1.f;
            auto rd = glm::vec3(inverseViewProjectionMatrix * glm::vec4(uv, 0.f, 1.f));
            rd_ptr[y * imageWidth + x] = glm::vec4(rd, 1.0f);
        }
    }
    rayDirections->unmap();

//    // todo: move to a better place
//    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
//        .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
//        .srcAccessMask = vk::AccessFlagBits2{},
//        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
//        .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
//        .oldLayout = vk::ImageLayout::eUndefined,
//        .newLayout = vk::ImageLayout::eTransferDstOptimal,
//        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .image = colorAttachmentTexture->image,
//        .subresourceRange = {
//            .aspectMask = vk::ImageAspectFlagBits::eColor,
//            .levelCount = 1,
//            .layerCount = 1
//        }
//    });
//    cmd->flushBarriers();
//
//    auto region = vk::BufferImageCopy{
//        .imageSubresource = {
//            .aspectMask = vk::ImageAspectFlagBits::eColor,
//            .layerCount = 1
//        },
//        .imageExtent = {
//            .width = colorAttachmentTexture->size.width,
//            .height = colorAttachmentTexture->size.height,
//            .depth = 1
//        }
//    };
//    cmd->handle->copyBufferToImage(
//        colorAttachmentPixels->handle,
//        colorAttachmentTexture->image,
//        vk::ImageLayout::eTransferDstOptimal,
//        1,
//        &region,
//        device->interface
//    );
//    // todo: move to a better place
//    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
//        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
//        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
//        .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
//        .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
//        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
//        .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
//        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .image = colorAttachmentTexture->image,
//        .subresourceRange = {
//            .aspectMask = vk::ImageAspectFlagBits::eColor,
//            .levelCount = 1,
//            .layerCount = 1
//        }
//    });
//    cmd->flushBarriers();
//    colorAttachmentTexture->update(pixels, traceAttachmentPixels.size() * sizeof(glm::vec4));

    auto rendering_area = vk::Rect2D{};
    rendering_area.setOffset(vk::Offset2D{0, 0});
    rendering_area.setExtent(swapchain->drawableSize);

    auto rendering_viewport = vk::Viewport{};
    rendering_viewport.setWidth(f32(rendering_area.extent.width));
    rendering_viewport.setHeight(f32(rendering_area.extent.height));
    rendering_viewport.setMinDepth(0.0f);
    rendering_viewport.setMaxDepth(1.0f);

//    cmd->setScissor(0, rendering_area);
//    cmd->setViewport(0, rendering_viewport);
//
//    // todo: draw world
//    f32 rendering_aspect = rendering_viewport.width / rendering_viewport.height;
//
//    SceneConstants scene_constants = {};
//    scene_constants.CameraPosition = cameraPosition;
//    scene_constants.ProjectionMatrix = Camera::getInfinityProjectionMatrix(60.0f, rendering_aspect, 0.01f);
//    scene_constants.WorldToCameraMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), cameraPosition) * glm::mat4x4(glm::quat(glm::radians(cameraRotation))));
//    scene_constants.ViewProjectionMatrix = scene_constants.ProjectionMatrix * scene_constants.WorldToCameraMatrix;
//    scene_constants.InverseViewProjectionMatrix = glm::inverse(scene_constants.ViewProjectionMatrix);
//
//    sceneConstantsBuffer->update(&scene_constants, sizeof(SceneConstants), 0);

//    auto scene_rendering_info = vfx::RenderingInfo{};
//    scene_rendering_info.renderArea = rendering_area;
//    scene_rendering_info.layerCount = 1;
//    scene_rendering_info.colorAttachments[0].texture = colorAttachmentTexture;
//    scene_rendering_info.colorAttachments[0].imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
//    scene_rendering_info.colorAttachments[0].loadOp = vk::AttachmentLoadOp::eClear;
//    scene_rendering_info.colorAttachments[0].storeOp = vk::AttachmentStoreOp::eStore;
//    scene_rendering_info.colorAttachments[0].clearColor = {};
//    vfx::ClearColor::init(173, 216, 230, 255);

//    scene_rendering_info.depthAttachment.texture = depthAttachmentTexture;
//    scene_rendering_info.depthAttachment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
//    scene_rendering_info.depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
//    scene_rendering_info.depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
//    scene_rendering_info.depthAttachment.clearDepth = 0.0f;

//    cmd->setPipelineState(defaultPipelineState);
//    cmd->setResourceGroup(defaultPipelineState, defaultResourceGroup, 0);
//    cmd->beginRendering(scene_rendering_info);

//    for (auto& chunkRenderer : chunkRendererArray->getChunks()) {
//        ObjectConstants constants{};
//        constants.LocalToWorldMatrix = glm::mat4(1.0f);
//
//        cmd->pushConstants(defaultPipelineState, vk::ShaderStageFlagBits::eVertex, 0, sizeof(ObjectConstants), &constants);
//        chunkRenderer.draw(cmd);
//    }
//    cmd->endRendering();

    auto drawable = swapchain->nextDrawable();

//    // todo: move to a better place
//    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
//        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
//        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
//        .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
//        .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
//        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
//        .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
//        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .image = colorAttachmentTexture->image,
//        .subresourceRange = vk::ImageSubresourceRange{
//            .aspectMask = vk::ImageAspectFlagBits::eColor,
//            .levelCount = 1,
//            .layerCount = 1
//        }
//    });
//    // todo: move to a better place
//    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
//        .srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
//        .srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
//        .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
//        .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
//        .oldLayout = vk::ImageLayout::eDepthAttachmentOptimal,
//        .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
//        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//        .image = depthAttachmentTexture->image,
//        .subresourceRange = vk::ImageSubresourceRange{
//            .aspectMask = vk::ImageAspectFlagBits::eDepth,
//            .levelCount = 1,
//            .layerCount = 1
//        }
//    });

    // todo: move to a better place
    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
        .srcAccessMask = vk::AccessFlags2{},
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = drawable->texture->image,
        .subresourceRange = vk::ImageSubresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .layerCount = 1
        }
    });
    cmd->flushBarriers();

    auto present_rendering_info = vfx::RenderingInfo{};
    present_rendering_info.renderArea = rendering_area;
    present_rendering_info.layerCount = 1;
    present_rendering_info.colorAttachments[0].texture = drawable->texture;
    present_rendering_info.colorAttachments[0].imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    present_rendering_info.colorAttachments[0].loadOp = vk::AttachmentLoadOp::eClear;
    present_rendering_info.colorAttachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    present_rendering_info.colorAttachments[0].clearColor = {};

    cmd->setRenderPipelineState(presentPipelineState);
    cmd->bindResourceGroup(presentResourceGroup, 0);
    cmd->beginRendering(present_rendering_info);

    auto present_area = vk::Rect2D{};
    present_area.setOffset(vk::Offset2D{0, 0});
    present_area.setExtent(drawable->texture->size);

    auto present_viewport = vk::Viewport{};
    present_viewport.setWidth(f32(present_area.extent.width));
    present_viewport.setHeight(f32(present_area.extent.height));

    cmd->setScissor(0, present_area);
    cmd->setViewport(0, present_viewport);

    HDR_Settings hdr_settings{};
    hdr_settings.exposure = options->exposure;
    hdr_settings.gamma    = options->gamma;

    cmd->pushConstants(vk::ShaderStageFlagBits::eFragment, 0, sizeof(HDR_Settings), &hdr_settings);
    cmd->draw(6, 1, 0, 0);
    cmd->endRendering();

    auto gui_rendering_info = vfx::RenderingInfo{};
    gui_rendering_info.renderArea = vk::Rect2D{.extent = drawable->texture->size};
    gui_rendering_info.layerCount = 1;
    gui_rendering_info.colorAttachments[0].texture = drawable->texture;
    gui_rendering_info.colorAttachments[0].imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    gui_rendering_info.colorAttachments[0].loadOp = vk::AttachmentLoadOp::eLoad;
    gui_rendering_info.colorAttachments[0].storeOp = vk::AttachmentStoreOp::eStore;

    // Blend imgui on swapchain to avoid gamma correction
    cmd->beginRendering(gui_rendering_info);
    imguiRenderer->draw(cmd);
    cmd->endRendering();

    // todo: move to a better place
    cmd->imageMemoryBarrier(vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
        .dstAccessMask = vk::AccessFlags2{},
        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = drawable->texture->image,
        .subresourceRange = vk::ImageSubresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .layerCount = 1
        }
    });
    cmd->flushBarriers();

    cmd->end();
    cmd->submit();
    cmd->present(drawable);
}

void GameApplication::updateTextureAttachments() {
    rayDirections = device->makeBuffer(
        vk::BufferUsageFlagBits::eStorageBuffer,
        swapchain->drawableSize.width * swapchain->drawableSize.height * sizeof(glm::vec4),
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
    );
    colorAttachmentTexture = device->makeTexture(vfx::TextureDescription{
        .format = vk::Format::eR32G32B32A32Sfloat,
        .width = swapchain->drawableSize.width,
        .height = swapchain->drawableSize.height,
        .usage = vk::ImageUsageFlagBits::eColorAttachment
               | vk::ImageUsageFlagBits::eInputAttachment
               | vk::ImageUsageFlagBits::eSampled
               | vk::ImageUsageFlagBits::eStorage
               | vk::ImageUsageFlagBits::eTransferDst
    });
    accumulateAttachmentTexture = device->makeTexture(vfx::TextureDescription{
        .format = vk::Format::eR32G32B32A32Sfloat,
        .width = swapchain->drawableSize.width,
        .height = swapchain->drawableSize.height,
        .usage = vk::ImageUsageFlagBits::eColorAttachment
               | vk::ImageUsageFlagBits::eInputAttachment
               | vk::ImageUsageFlagBits::eSampled
               | vk::ImageUsageFlagBits::eStorage
               | vk::ImageUsageFlagBits::eTransferDst
    });
    depthAttachmentTexture = device->makeTexture(vfx::TextureDescription{
        .format = vk::Format::eD32Sfloat,
        .width = swapchain->drawableSize.width,
        .height = swapchain->drawableSize.height,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled,
    });

    presentResourceGroup->setSampler(textureSampler, 0);
    presentResourceGroup->setTexture(colorAttachmentTexture, 1);
//    presentResourceGroup->setTexture(depthAttachmentTexture, 2);
    raytraceResourceGroup->setStorageImage(colorAttachmentTexture, 0);
    raytraceResourceGroup->setStorageImage(accumulateAttachmentTexture, 1);
    raytraceResourceGroup->setStorageBuffer(rayDirections, 0, 2);
}

void GameApplication::createDefaultPipelineObjects() {
    vfx::RenderPipelineStateDescription description{};

    vfx::RenderPipelineVertexDescription vertexDescription{};
    vertexDescription.layouts = {{
        {0, sizeof(DrawVertex), vk::VertexInputRate::eVertex}
    }};
    vertexDescription.attributes = {{
        {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(DrawVertex, position) },
        {1, 0, vk::Format::eR8G8B8A8Unorm,   offsetof(DrawVertex, color) }
    }};
    description.vertexDescription = vertexDescription;

    description.colorAttachmentFormats[0] = vk::Format::eR32G32B32A32Sfloat;
    description.depthAttachmentFormat = vk::Format::eD32Sfloat;

    description.attachments[0].blendEnable = false;
    description.attachments[0].colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;

    description.depthStencilState.depthTestEnable = VK_TRUE;
    description.depthStencilState.depthWriteEnable = VK_TRUE;
    description.depthStencilState.depthCompareOp = vk::CompareOp::eGreater;

    description.inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
    description.rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
    description.rasterizationState.lineWidth = 1.0f;

    auto vertexLibrary = device->makeLibrary(Assets::readFile("shaders/default.vert.spv"));
    auto fragmentLibrary = device->makeLibrary(Assets::readFile("shaders/default.frag.spv"));

    description.vertexFunction = vertexLibrary->makeFunction("main");
    description.fragmentFunction = fragmentLibrary->makeFunction("main");

    defaultPipelineState = device->makeRenderPipelineState(description);
    defaultResourceGroup = device->makeResourceGroup(defaultPipelineState->descriptorSetLayouts[0], {
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2}
    });
}

void GameApplication::createRaytracePipelineObjects() {
    auto library = device->makeLibrary(Assets::readFile("shaders/raytrace.comp.spv"));
    auto function = library->makeFunction("main");

    raytracePipelineState = device->makeComputePipelineState(function);
    raytraceResourceGroup = device->makeResourceGroup(raytracePipelineState->descriptorSetLayouts[0], {
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 2},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 2}
    });
}

void GameApplication::createPresentPipelineObjects() {
    auto description = vfx::RenderPipelineStateDescription{};

    description.colorAttachmentFormats[0] = swapchain->pixelFormat;

    description.attachments[0].blendEnable = false;
    description.attachments[0].colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;

    description.inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
    description.rasterizationState.lineWidth = 1.0f;

    auto vertexLibrary = device->makeLibrary(Assets::readFile("shaders/blit.vert.spv"));
    auto fragmentLibrary = device->makeLibrary(Assets::readFile("shaders/blit.frag.spv"));

    description.vertexFunction = vertexLibrary->makeFunction("main");
    description.fragmentFunction = fragmentLibrary->makeFunction("main");

    presentPipelineState = device->makeRenderPipelineState(description);
    presentResourceGroup = device->makeResourceGroup(presentPipelineState->descriptorSetLayouts[0], {
        vk::DescriptorPoolSize{vk::DescriptorType::eSampler, 1},
        vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, 2},
    });
}

void GameApplication::windowDidResize() {
    device->waitIdle();
    swapchain->updateDrawables();
    mouseHandler->setIgnoreFirstMove();

    accumulateFrame = 0;
    updateTextureAttachments();

    render();
}

void GameApplication::windowMouseEvent(i32 button, i32 action, i32 mods) {
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    mouseHandler->onPress(button, action, mods);
}

void GameApplication::windowCursorEvent(f64 x, f64 y) {
    mouseHandler->onMove(x, y);
}

void GameApplication::windowMouseEnter() {
    mouseHandler->setIgnoreFirstMove();
}

void GameApplication::windowShouldClose() {
    running = false;
}

void GameApplication::windowKeyEvent(i32 keycode, i32 scancode, i32 action, i32 mods) {
    if (keycode == GLFW_KEY_W) {
        switch (action) {
            case GLFW_PRESS:
                options->keyUp->down = true;
                break;
            case GLFW_RELEASE:
                options->keyUp->down = false;
                break;
            default:
                break;
        }
    }
    if (keycode == GLFW_KEY_S) {
        switch (action) {
            case GLFW_PRESS:
                options->keyDown->down = true;
                break;
            case GLFW_RELEASE:
                options->keyDown->down = false;
                break;
            default:
                break;
        }
    }
    if (keycode == GLFW_KEY_A) {
        switch (action) {
            case GLFW_PRESS:
                options->keyLeft->down = true;
                break;
            case GLFW_RELEASE:
                options->keyLeft->down = false;
                break;
            default:
                break;
        }
    }
    if (keycode == GLFW_KEY_D) {
        switch (action) {
            case GLFW_PRESS:
                options->keyRight->down = true;
                break;
            case GLFW_RELEASE:
                options->keyRight->down = false;
                break;
            default:
                break;
        }
    }

    if (keycode == GLFW_KEY_ESCAPE) {
        if (action == GLFW_PRESS) {
            mouseHandler->releaseMouse();
        }
    }
}
