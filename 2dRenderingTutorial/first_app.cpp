#include "first_app.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

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

    while (!lveWindow.shouldClose()) {
        glfwPollEvents();

        if (auto commandBuffer = lveRenderer.beginFrame()) {
            lveRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
            lveRenderer.endSwapChainRenderPass(commandBuffer);
            lveRenderer.endFrame();
        }
    }

    vkDeviceWaitIdle(lveDevice.device());
}

void FirstApp::loadGameObjects() {
    std::vector<LveModel::Vertex> vertices{};

    sierpinski(vertices,
               1,
               {{-0.5f, 0.5f}, {1.0f, 0, 0}},
               {{0.5f, 0.5f}, {0, 1.0f, 0}},
               {{0, -0.5f}, {0, 0, 1.0f}});

    auto lveModel = std::make_shared<LveModel>(lveDevice, vertices);

    // https://www.color-hex.com/color-palette/5361
    std::vector<glm::vec3> colors{
        {1.f, .7f, .73f},
        {1.f, .87f, .73f},
        {1.f, 1.f, .73f},
        {.73f, 1.f, .8f},
        {.73, .88f, 1.f}  //
    };
    for (auto& color : colors) {
        color = glm::pow(color, glm::vec3{2.2f});
    }

    for (int i = 0; i < 40; i++) {
        auto triangle = LveGameObject::createGameObject();
        triangle.model = lveModel;
        triangle.transform2d.scale = glm::vec2(0.5f) + i * 0.025f;
        triangle.transform2d.rotation = i * 0.025f * glm::pi<float>();
        triangle.color = colors[i % colors.size()];
        gameObjects.push_back(std::move(triangle));
    }
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
