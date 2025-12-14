#include "first_app.hpp"

#include <stdexcept>
#include <array>
#include <cassert>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace lve
{

    struct SimplePushConstantData
    {
        glm::mat2 transform = {1.f};
        glm::vec2 offset;
        alignas(16) glm::vec3 color;
    };

    FirstApp::FirstApp()
    {
        loadGameObjects();
        createPipelineLayout();
        recreateSwapChain();
        createCommandBuffers();
    }

    FirstApp::~FirstApp()
    {
        vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
    }

    void FirstApp::run()
    {

        while (!lveWindow.shouldClose())
        {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(lveDevice.device());
    }

    void FirstApp::loadGameObjects()
    {
        std::vector<LveModel::Vertex> vertices{};

        sierpinski(vertices, 1, {{-0.5f, 0.5f}, {1.0f, 0, 0}}, {{0.5f, 0.5f}, {0, 1.0f, 0}}, {{0, -0.5f}, {0, 0, 1.0f}});

        auto lveModel = std::make_shared<LveModel>(lveDevice, vertices);

        // https://www.color-hex.com/color-palette/5361
        std::vector<glm::vec3> colors{
            {1.f, .7f, .73f},
            {1.f, .87f, .73f},
            {1.f, 1.f, .73f},
            {.73f, 1.f, .8f},
            {.73, .88f, 1.f} //
        };
        for (auto &color : colors)
        {
            color = glm::pow(color, glm::vec3{2.2f});
        }

        for (int i = 0; i<40; i++){
            auto triangle = LveGameObject::createGameObject();
            triangle.model = lveModel;
            triangle.transform2d.scale = glm::vec2(0.5f) + i*0.025f;
            triangle.transform2d.rotation = i * 0.025f * glm::pi<float>();
            triangle.color = colors[i % colors.size()];
            gameObjects.push_back(std::move(triangle));
        }
    }

    void FirstApp::createPipelineLayout()
    {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline");
        }
    }

    void FirstApp::createPipeline()
    {
        assert(lveSwapchain != nullptr && "Cannot create pipeline before swapchain");
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LvePipeline::defaultPipeLineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = lveSwapchain->getRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        lvePipeline = std::make_unique<LvePipeline>(lveDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
    }

    void FirstApp::createCommandBuffers()
    {
        commandBuffers.resize(lveSwapchain->imageCount());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = lveDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Could not allocate command buffers");
        }
    }

    void FirstApp::freeCommandBuffers()
    {
        vkFreeCommandBuffers(lveDevice.device(), lveDevice.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
    }

    void FirstApp::recordCommandBuffer(int imageIndex)
    {

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffers");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = lveSwapchain->getRenderPass();
        renderPassInfo.framebuffer = lveSwapchain->getFrameBuffer(imageIndex);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = lveSwapchain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(lveSwapchain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(lveSwapchain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, lveSwapchain->getSwapChainExtent()};
        vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
        vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

        renderGameObjects(commandBuffers[imageIndex]);

        vkCmdEndRenderPass(commandBuffers[imageIndex]);
        if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }
    }

    void FirstApp::renderGameObjects(VkCommandBuffer commandBuffer)
    {
        int i = 0;
        for (auto& obj : gameObjects){
            i+=1; 
            obj.transform2d.rotation = glm::mod<float>(obj.transform2d.rotation + 0.000001f * i, 2.f * glm::pi<float>());
        }


        lvePipeline->bind(commandBuffer);

        for (auto &obj : gameObjects)
        {
            SimplePushConstantData push{};
            push.offset = obj.transform2d.translation;
            push.color = obj.color;
            push.transform = obj.transform2d.mat2();

            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }
    }

    void FirstApp::recreateSwapChain()
    {
        auto extent = lveWindow.getExtent();
        while (extent.width == 0 || extent.height == 0)
        {
            extent = lveWindow.getExtent();
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(lveDevice.device());

        if (lveSwapchain == nullptr)
        {
            lveSwapchain = std::make_unique<LveSwapChain>(lveDevice, extent);
        }
        else
        {
            lveSwapchain = std::make_unique<LveSwapChain>(lveDevice, extent, std::move(lveSwapchain));
            if (lveSwapchain->imageCount() != commandBuffers.size())
            {
                freeCommandBuffers();
                createCommandBuffers();
            }
        }

        createPipeline();
    }

    void FirstApp::drawFrame()
    {
        uint32_t imageIndex;
        auto result = lveSwapchain->acquireNextImage(&imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire next swapchain image");
        }

        recordCommandBuffer(imageIndex);
        result = lveSwapchain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || lveWindow.wasWindowResized())
        {
            lveWindow.resetWindowResizedFlag();
            recreateSwapChain();
            return;
        }
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swapchain image");
        }
    }

    void FirstApp::sierpinski(
        std::vector<LveModel::Vertex> &vertices,
        int depth,
        LveModel::Vertex left,
        LveModel::Vertex right,
        LveModel::Vertex top)
    {
        if (depth <= 0)
        {
            vertices.push_back({top});
            vertices.push_back({right});
            vertices.push_back({left});
        }
        else
        {
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
}
