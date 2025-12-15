#include "lve_renderer.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

namespace lve {
LveRenderer::LveRenderer(LveWindow& window, LveDevice& device)
    : lveWindow{window},
      lveDevice{device} {
    recreateSwapChain();
    createCommandBuffers();
}

LveRenderer::~LveRenderer() { freeCommandBuffers(); }

void LveRenderer::createCommandBuffers() {
    commandBuffers.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = lveDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Could not allocate command buffers");
    }
}

void LveRenderer::freeCommandBuffers() {
    vkFreeCommandBuffers(lveDevice.device(),
                         lveDevice.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
    commandBuffers.clear();
}

void LveRenderer::recreateSwapChain() {
    auto extent = lveWindow.getExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = lveWindow.getExtent();
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(lveDevice.device());

    if (lveSwapchain == nullptr) {
        lveSwapchain = std::make_unique<LveSwapChain>(lveDevice, extent);
    } else {
        std::shared_ptr<LveSwapChain> oldSwapChain = std::move(lveSwapchain);
        lveSwapchain = std::make_unique<LveSwapChain>(lveDevice, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormats(*lveSwapchain.get())) {
            throw std::runtime_error("Swap chain image or depth format has changed");
        }
    }

    // createPipeline();
    // TODO
}

VkCommandBuffer LveRenderer::beginFrame() {
    assert(!isFrameStarted && "Cant call beginFrame while frame is aleady in progress");

    auto result = lveSwapchain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire next swapchain image");
    }

    isFrameStarted = true;

    auto commandBuffer = getCurrentCommandBuffer();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffers");
    }

    return commandBuffer;
};

void LveRenderer::endFrame() {
    assert(isFrameStarted && "Cannot call endFrame when frame is not started");
    auto commandBuffer = getCurrentCommandBuffer();

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }

    auto result = lveSwapchain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        lveWindow.wasWindowResized()) {
        lveWindow.resetWindowResizedFlag();
        recreateSwapChain();
    }
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }

    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % LveSwapChain::MAX_FRAMES_IN_FLIGHT;
};

void LveRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted &&
           "Cannot call beginSwapChainRenderPass while frame is not started");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Can't begin render pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = lveSwapchain->getRenderPass();
    renderPassInfo.framebuffer = lveSwapchain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = lveSwapchain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(lveSwapchain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(lveSwapchain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, lveSwapchain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
};

void LveRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted &&
           "Cannot call endSwapChainRenderPass while frame is not started");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Can't end render pass on command buffer from a different frame");

    vkCmdEndRenderPass(commandBuffer);
};
}  // namespace lve
