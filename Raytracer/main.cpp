#include <cstdio>
#include <vector>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


class GlfwVulcanRenderer
{
 private:
    struct PhysicalDevice
    {
        PhysicalDevice(VkPhysicalDevice _physDevice)
            :_device(_physDevice)
        {}
        
        VkPhysicalDevice _device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties _properties = {};
        VkPhysicalDeviceFeatures _features = {};
        std::vector<VkExtensionProperties> _extensions;
        std::vector<VkQueueFamilyProperties> _queueFamilies;
        int32_t _graphicsSupportIndex = -1;
        int32_t _presentationSupportIndex = -1;
        float _score = 0;
    };

 public:
    GlfwVulcanRenderer(const char* _pszAppName, uint32_t width, uint32_t height);
    ~GlfwVulcanRenderer();
    
    GLFWwindow* getWindow() {return m_window;}
    uint32_t getGpuCount() const {return (uint32_t)m_physicalDevices.size();}
    const char* getGpuName(uint32_t _i) const {return m_physicalDevices[_i]._properties.deviceName;}
 
 private:
    void createInstance(const char* _pszAppName);
    void destroyInstance();
    void createWindow(const char* _pszAppName, uint32_t _width, uint32_t _height);
    void destroyWindow();
    void enumeratePhysicalDevices();
    float scorePhysicalDevice(const PhysicalDevice& _device);
 
 private:
    VkInstance m_instance = VK_NULL_HANDLE;
    GLFWwindow* m_window = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    std::vector<PhysicalDevice> m_physicalDevices;
};


GlfwVulcanRenderer::GlfwVulcanRenderer(const char* _pszAppName, uint32_t _width, uint32_t _height)
{
    if (!glfwInit() || !glfwVulkanSupported())
    {
        throw std::runtime_error("Vulkan not available!");
    }
    
    createInstance(_pszAppName);
    createWindow(_pszAppName, _width, _height);
    enumeratePhysicalDevices();
}

GlfwVulcanRenderer::~GlfwVulcanRenderer()
{
    destroyWindow();
    destroyInstance();
    glfwTerminate();
}

void GlfwVulcanRenderer::createInstance(const char* _pszAppName)
{
    // setup required layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    // setup required extensions
    std::vector<const char*> enabledExtensions = {
        "VK_KHR_portability_enumeration"
    };

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = nullptr;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        enabledExtensions.push_back(glfwExtensions[i]);
    }
    
    // initialize the VkApplicationInfo structure
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = _pszAppName;
    app_info.applicationVersion = 1;
    app_info.pEngineName = _pszAppName;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_0;

    // initialize the VkInstanceCreateInfo structure
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = validationLayers.size();
    inst_info.ppEnabledLayerNames = validationLayers.data();
    inst_info.enabledExtensionCount = enabledExtensions.size();
    inst_info.ppEnabledExtensionNames = enabledExtensions.data();
    inst_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    VkResult res = vkCreateInstance(&inst_info, NULL, &m_instance);
    if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        throw std::runtime_error("Cannot find a compatible Vulkan ICD!");
    }
    else if (res)
    {
        throw std::runtime_error("Unknown error!");
    }
}

void GlfwVulcanRenderer::destroyInstance()
{
    vkDestroyInstance(m_instance, nullptr);
}

void GlfwVulcanRenderer::createWindow(const char* _pszAppName, uint32_t width, uint32_t height)
{
    // create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    m_window = glfwCreateWindow( 1280, 720, _pszAppName, NULL, NULL );
    if (!m_window)
    {
        throw std::runtime_error("Failed to create window!");
    }

    // create surface
    if (VkResult res = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface); res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void GlfwVulcanRenderer::destroyWindow()
{
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    glfwDestroyWindow(m_window);
}

void GlfwVulcanRenderer::enumeratePhysicalDevices()
{
    // find number of GPUs
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    
    if (deviceCount == 0)
    {
        throw("Failed to find GPUs with Vulkan support!");
    }
    
    // build list of GPUs with properties, features, extensions, queueFamilies, etc.
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    
    for (VkPhysicalDevice deviceHandle : devices)
    {
        // get all GPU properties and features
        m_physicalDevices.emplace_back(deviceHandle);
        PhysicalDevice &device = m_physicalDevices.back();
        
        vkGetPhysicalDeviceProperties(deviceHandle, &device._properties);
        vkGetPhysicalDeviceFeatures(deviceHandle, &device._features);
        
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(deviceHandle, nullptr, &extensionCount, nullptr);
        device._extensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(deviceHandle, nullptr, &extensionCount, device._extensions.data());
        
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &queueFamilyCount, nullptr);
        device._queueFamilies.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &queueFamilyCount, device._queueFamilies.data());
        
        // find valid queue family indices
        for (uint32_t i = 0; i < device._queueFamilies.size(); i++)
        {
            const auto& queueFamily = device._queueFamilies[i];
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                device._graphicsSupportIndex = i;
            }
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(deviceHandle, i, m_surface, &presentSupport);
            if (presentSupport)
            {
                device._presentationSupportIndex = i;
            }
            
            if (device._graphicsSupportIndex >= 0 && device._presentationSupportIndex >= 0)
            {
                break;
            }
        }

        // score GPU
        device._score = scorePhysicalDevice(device);
    }
    
    // sort (best GPU first)
    std::sort(m_physicalDevices.begin(), m_physicalDevices.end(), [](const PhysicalDevice& a, const PhysicalDevice& b){return a._score > b._score;});
}

float GlfwVulcanRenderer::scorePhysicalDevice(const PhysicalDevice& _device)
{
    // prefer discrete GPU
    float score = 0;
    if (_device._properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score = 2000;
    }
    else
    {
        score = 1000;
    }

    // set score to 0 if no swap chain support available
    bool supportSwapChain = false;
    for (const auto& extension : _device._extensions)
    {
        if (!strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        {
            supportSwapChain = true;
            break;
        }
    }

    score *= (float)supportSwapChain;
    
    return score;
}







int main(int argc, char** argv)
{
    GlfwVulcanRenderer renderer("Test", 1024, 768);


    
    
    /*
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0;       // TODO: use correct queue family index here
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkPhysicalDeviceFeatures deviceFeatures{};
    
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    // TODO: create two queues if graphics family and presentation family has different indices
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
 
    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(devices[0], &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        printf("failed to create logical device!");
        return -1;
    }
    
    
 
    // TODO: use correct presentation family index
    VkQueue presentQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, 0, 0, &presentQueue);
 
 

    // TODO: query swap chain support and check that it is adequate
    // TODO: pick best swap chain surface format
    // TODO: pick best presentation mode
    // TODO: pick a swap extent
    // TODO: pick swap chain image count
    
    
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    
    VkSwapchainCreateInfoKHR createInfoSwapChain{};
    createInfoSwapChain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfoSwapChain.surface = surface;
    createInfoSwapChain.minImageCount = 2;
    createInfoSwapChain.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    createInfoSwapChain.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfoSwapChain.imageExtent = {(uint32_t)width, (uint32_t)height};
    createInfoSwapChain.imageArrayLayers = 1;
    createInfoSwapChain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    // TODO: support case where graphics and presentation family indices are not the same
    createInfoSwapChain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    // TODO:
    // createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    
    createInfoSwapChain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    
    createInfoSwapChain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfoSwapChain.clipped = VK_TRUE;
    
    createInfoSwapChain.oldSwapchain = VK_NULL_HANDLE;
    
    
    
    
    VkSwapchainKHR swapChain;
    if (vkCreateSwapchainKHR(device, &createInfoSwapChain, nullptr, &swapChain) != VK_SUCCESS)
    {
        printf("failed to create swap chain!");
    }
    
    
    
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    std::vector<VkImage> swapChainImages(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    */
    
    
    
    
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(renderer.getWindow()))
    {
        /* Poll for and process events */
        glfwPollEvents();
    }
    
    /*

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(inst, surface, nullptr);
    */

    return 0;
}
