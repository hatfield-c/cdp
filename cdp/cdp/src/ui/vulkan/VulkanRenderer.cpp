#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer(VulkanCore* vulkan_core) {
    this->vulkan_core = vulkan_core;
}
bool VulkanRenderer::Update() {
    glfwPollEvents();

    int fb_width, fb_height;
    glfwGetFramebufferSize(this->vulkan_core->window, &fb_width, &fb_height);
    if (fb_width > 0 && fb_height > 0 && (this->vulkan_core->g_SwapChainRebuild || this->vulkan_core->g_MainWindowData.Width != fb_width || this->vulkan_core->g_MainWindowData.Height != fb_height))
    {
        ImGui_ImplVulkan_SetMinImageCount(this->vulkan_core->g_MinImageCount);
        ImGui_ImplVulkanH_CreateOrResizeWindow(
            this->vulkan_core->g_Instance,
            this->vulkan_core->g_PhysicalDevice,
            this->vulkan_core->g_Device,
            &this->vulkan_core->g_MainWindowData,
            this->vulkan_core->g_QueueFamily,
            this->vulkan_core->g_Allocator,
            fb_width,
            fb_height,
            this->vulkan_core->g_MinImageCount
        );
        this->vulkan_core->g_MainWindowData.FrameIndex = 0;
        this->vulkan_core->g_SwapChainRebuild = false;
    }

    if (glfwGetWindowAttrib(this->vulkan_core->window, GLFW_ICONIFIED) != 0)
    {
        ImGui_ImplGlfw_Sleep(10);
        return false;
    }
    
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    return true;
}

bool VulkanRenderer::IsWindowClosed() {
    return glfwWindowShouldClose(this->vulkan_core->window);
}

void VulkanRenderer::Render() {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized)
    {
        this->vulkan_core->wd->ClearValue.color.float32[0] = this->default_color.x * this->default_color.w;
        this->vulkan_core->wd->ClearValue.color.float32[1] = this->default_color.y * this->default_color.w;
        this->vulkan_core->wd->ClearValue.color.float32[2] = this->default_color.z * this->default_color.w;
        this->vulkan_core->wd->ClearValue.color.float32[3] = this->default_color.w;
        this->FrameRender(this->vulkan_core->wd, draw_data);
        this->FramePresent(this->vulkan_core->wd);
    }
}

void VulkanRenderer::FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkResult err;

    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(this->vulkan_core->g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        this->vulkan_core->g_SwapChainRebuild = true;
        return;
    }
    this->vulkan_core->check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    // wait indefinitely instead of periodically checking
    err = vkWaitForFences(this->vulkan_core->g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
    this->vulkan_core->check_vk_result(err);

    err = vkResetFences(this->vulkan_core->g_Device, 1, &fd->Fence);
    this->vulkan_core->check_vk_result(err);

    err = vkResetCommandPool(this->vulkan_core->g_Device, fd->CommandPool, 0);
    this->vulkan_core->check_vk_result(err);
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
    this->vulkan_core->check_vk_result(err);

    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        this->vulkan_core->check_vk_result(err);
        err = vkQueueSubmit(this->vulkan_core->g_Queue, 1, &info, fd->Fence);
        this->vulkan_core->check_vk_result(err);
    }
}

void VulkanRenderer::FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if (this->vulkan_core->g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(this->vulkan_core->g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        this->vulkan_core->g_SwapChainRebuild = true;
        return;
    }
    this->vulkan_core->check_vk_result(err);
    // Now we can use the next set of semaphores
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
}




