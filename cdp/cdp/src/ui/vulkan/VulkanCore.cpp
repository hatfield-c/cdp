#include "VulkanCore.h"

VulkanCore::VulkanCore() {
    glfwSetErrorCallback(VulkanCore::glfw_error_callback);
    if (!glfwInit()) {
        printf("GLFW: Failed during init\n");
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    this->window = glfwCreateWindow(1280, 720, "[CDP] CudaPilot - Playground", nullptr, nullptr);
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan Not Supported\n");
        return;
    }

    ImVector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);
    SetupVulkan(extensions);

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(this->g_Instance, this->window, this->g_Allocator, &surface);
    this->check_vk_result(err);

    int w, h;
    glfwGetFramebufferSize(this->window, &w, &h);
    this->wd = &this->g_MainWindowData;
    this->SetupVulkanWindow(this->wd, surface, w, h);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO* io = &(ImGui::GetIO());
    this->io = io;
    this->io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    this->io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(this->window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = this->g_Instance;
    init_info.PhysicalDevice = this->g_PhysicalDevice;
    init_info.Device = this->g_Device;
    init_info.QueueFamily = this->g_QueueFamily;
    init_info.Queue = this->g_Queue;
    init_info.PipelineCache = this->g_PipelineCache;
    init_info.DescriptorPool = this->g_DescriptorPool;
    init_info.RenderPass = this->wd->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = this->g_MinImageCount;
    init_info.ImageCount = this->wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = this->g_Allocator;
    init_info.CheckVkResultFn = VulkanCore::check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

    this->io->Fonts->AddFontFromFileTTF("data/media/fonts/Roboto-Medium.ttf", 15.0f);
}

void VulkanCore::glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void VulkanCore::check_vk_result(VkResult err) {
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

bool VulkanCore::IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension) {
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

VkPhysicalDevice VulkanCore::SetupVulkan_SelectPhysicalDevice() {
    uint32_t gpu_count;
    VkResult err = vkEnumeratePhysicalDevices(this->g_Instance, &gpu_count, nullptr);
    this->check_vk_result(err);
    IM_ASSERT(gpu_count > 0);

    ImVector<VkPhysicalDevice> gpus;
    gpus.resize(gpu_count);
    err = vkEnumeratePhysicalDevices(this->g_Instance, &gpu_count, gpus.Data);
    this->check_vk_result(err);

    for (VkPhysicalDevice& device : gpus)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            return device;
    }

    if (gpu_count > 0) {
        return gpus[0];
    }

    return VK_NULL_HANDLE;
}

void VulkanCore::SetupVulkan(ImVector<const char*> instance_extensions)
{
    this->CreateInstance(instance_extensions);
    this->SetupDevice();
    this->CreateDescriptorPool();
}

void VulkanCore::CreateInstance(ImVector<const char*> instance_extensions) {
    if (!this->CheckValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    uint32_t properties_count;
    ImVector<VkExtensionProperties> properties;
    vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    this->err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
    this->check_vk_result(this->err);

    if (VulkanCore::IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
        instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    if (VulkanCore::IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    create_info.enabledLayerCount = static_cast<uint32_t>(this->validation_layers.size());
    create_info.ppEnabledLayerNames = this->validation_layers.data();

    create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
    create_info.ppEnabledExtensionNames = instance_extensions.Data;
    this->err = vkCreateInstance(&create_info, this->g_Allocator, &this->g_Instance);
    this->check_vk_result(this->err);
}

void VulkanCore::SetupDevice() {
    g_PhysicalDevice = SetupVulkan_SelectPhysicalDevice();

    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, nullptr);
    VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
    for (uint32_t i = 0; i < count; i++)
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            this->g_QueueFamily = i;
            break;
        }
    free(queues);
    IM_ASSERT(this->g_QueueFamily != (uint32_t)-1);

    // Create Logical Device (with 1 queue)
    ImVector<const char*> device_extensions;
    device_extensions.push_back("VK_KHR_swapchain");

    // Enumerate physical device extension
    uint32_t properties_count;
    ImVector<VkExtensionProperties> properties;
    vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, properties.Data);

    const float queue_priority[] = { 1.0f };
    VkDeviceQueueCreateInfo queue_info[1] = {};
    queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[0].queueFamilyIndex = this->g_QueueFamily;
    queue_info[0].queueCount = 1;
    queue_info[0].pQueuePriorities = queue_priority;
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
    create_info.pQueueCreateInfos = queue_info;
    create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
    create_info.ppEnabledExtensionNames = device_extensions.Data;
    this->err = vkCreateDevice(this->g_PhysicalDevice, &create_info, this->g_Allocator, &this->g_Device);
    this->check_vk_result(this->err);
    vkGetDeviceQueue(this->g_Device, this->g_QueueFamily, 0, &this->g_Queue);
}

void VulkanCore::CreateDescriptorPool() {
    // Create Descriptor Pool
    // The example only requires a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)
    // If you wish to load e.g. additional textures you may need to alter pools sizes.

    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 },
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 2;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    this->err = vkCreateDescriptorPool(this->g_Device, &pool_info, this->g_Allocator, &this->g_DescriptorPool);
    this->check_vk_result(this->err);
}

bool VulkanCore::CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : this->validation_layers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void VulkanCore::SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    wd->Surface = surface;

    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(this->g_PhysicalDevice, this->g_QueueFamily, wd->Surface, &res);
    if (res != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(this->g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(this->g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(this->g_Instance, this->g_PhysicalDevice, this->g_Device, wd, this->g_QueueFamily, this->g_Allocator, width, height, this->g_MinImageCount);
}