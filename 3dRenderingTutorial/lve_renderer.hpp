#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "lve_device.hpp"
#include "lve_swap_chain.hpp"
#include "lve_window.hpp"

namespace lve {
class LveRenderer {
   public:
    LveRenderer(LveWindow& window, LveDevice& device);
    ~LveRenderer();

    LveRenderer(const LveRenderer&) = delete;
    LveRenderer& operator=(const LveRenderer&) = delete;

    const VkRenderPass getSwapChainRenderPass() {
        return lveSwapchain->getRenderPass();
    };
    float getAspectRatio() { return lveSwapchain->extentAspectRatio(); }
    const bool isFrameInProgress() { return isFrameStarted; };

    const VkCommandBuffer getCurrentCommandBuffer() {
        assert(isFrameStarted &&
               "Cannot get command buffer when frame is not in progress");
        return commandBuffers[currentFrameIndex];
    };

    const int getFrameIndex() {
        assert(isFrameStarted &&
               "Cannot get frame index when frame is not in progress");
        return currentFrameIndex;
    };

    VkCommandBuffer beginFrame();
    void endFrame();

    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

   private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();

    LveWindow& lveWindow;
    LveDevice& lveDevice;
    std::unique_ptr<LveSwapChain> lveSwapchain;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentImageIndex;
    int currentFrameIndex{0};
    bool isFrameStarted{false};
};
}  // namespace lve