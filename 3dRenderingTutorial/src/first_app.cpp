#include "first_app.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <stdexcept>

#include "keyboard_movement_controller.hpp"
#include "lve_camera.hpp"
#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace lve {

FirstApp::FirstApp() { loadGameObjects(); }

FirstApp::~FirstApp() {}

void FirstApp::run() {
    SimpleRenderSystem simpleRenderSystem{lveDevice,
                                          lveRenderer.getSwapChainRenderPass()};
    LveCamera camera{};
    camera.setViewTarget(glm::vec3{-1.f, -2.f, 2.f}, glm::vec3{0.f, 0.f, 2.5f});

    auto viewerObject = LveGameObject::createGameObject();
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!lveWindow.shouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();

        float frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(
                newTime - currentTime)
                .count();

        frameTime = glm::min(frameTime, MAX_FRAME_TIME);

        currentTime = newTime;

        cameraController.moveInPlaneXZ(
            lveWindow.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation,
                          viewerObject.transform.rotation);

        float aspectRatio = lveRenderer.getAspectRatio();
        // camera.setOrthographicProjection(
        //     -aspectRatio, aspectRatio, -1, 1, -1, 1);

        camera.setPerspectiveProjection(
            glm::radians(50.f), aspectRatio, 0.1f, 10.f);

        if (auto commandBuffer = lveRenderer.beginFrame()) {
            lveRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(
                commandBuffer, gameObjects, camera);
            lveRenderer.endSwapChainRenderPass(commandBuffer);
            lveRenderer.endFrame();
        }
    }

    vkDeviceWaitIdle(lveDevice.device());
}

void FirstApp::loadGameObjects() {
    std::shared_ptr<LveModel> lveModel =
        LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");

    auto gameObj = LveGameObject::createGameObject();
    gameObj.model = lveModel;
    gameObj.transform.translation = {0.f, 0.f, 2.5f};
    gameObj.transform.scale = glm::vec3{3.f};
    gameObjects.push_back(std::move(gameObj));
}

void FirstApp::sierpinski(std::vector<LveModel::Vertex>& vertices,
                          int depth,
                          LveModel::Vertex left,
                          LveModel::Vertex right,
                          LveModel::Vertex top) {
    if (depth <= 0) {
        vertices.push_back({top});
        vertices.push_back({right});
        vertices.push_back({left});
    } else {
        auto leftTopPos = 0.5f * (left.position + top.position);
        auto rightTopPos = 0.5f * (right.position + top.position);
        auto leftRightPos = 0.5f * (left.position + right.position);

        auto leftTopColor = 0.5f * (left.color + top.color);
        auto rightTopColor = 0.5f * (right.color + top.color);
        auto leftRightColor = 0.5f * (left.color + right.color);

        LveModel::Vertex leftTop = {leftTopPos, leftTopColor};
        LveModel::Vertex rightTop = {rightTopPos, rightTopColor};
        LveModel::Vertex leftRight = {leftRightPos, leftRightColor};

        sierpinski(vertices, depth - 1, left, leftRight, leftTop);
        sierpinski(vertices, depth - 1, leftRight, right, rightTop);
        sierpinski(vertices, depth - 1, leftTop, rightTop, top);
    }
}

}  // namespace lve
