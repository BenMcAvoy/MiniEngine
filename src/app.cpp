#include <iterator>
#include <miniengine/app.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <fstream>
#include <map>
#include <set>

namespace MiniEngine {
App::App() {
  spdlog::trace("App::App()");
  this->startTime = std::chrono::high_resolution_clock::now();
}

App::~App() {
  spdlog::trace("App::~App()");
  App::Cleanup();
}

void App::Run() {
  spdlog::trace("App::Run()");

  InitWindow();
  InitVulkan();
  SetupImGui();
  MainLoop();
}

void App::InitWindow() {
  spdlog::trace("App::InitWindow()");

  if (glfwInit() == GLFW_FALSE) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "MiniEngine", nullptr, nullptr);
  if (window == nullptr) {
    throw std::runtime_error("Failed to create window");
  }

  glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);

  glfwSetWindowUserPointer(window, this);
}

void App::InitVulkan() {
  spdlog::trace("App::InitVulkan()");

  CreateInstance();
  SetupDebugMessenger();
  CreateSurface();
  PickPhysicalDevice();
  CreateLogicalDevice();
  CreateSwapchain();
  CreateImageViews();
  CreateRenderPass();
  CreateGraphicsPipeline();
  CreateFramebuffers();
  CreateCommandPool();
  CreateVertexBuffer();
  CreateCommandBuffers();
  CreateSyncObjects();
}

void App::CreateInstance() {
  spdlog::trace("App::CreateInstance()");

  if (enableValidationLayers && !CheckValidationLayerSupport()) {
    throw std::runtime_error("Validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "MiniEngine test";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  appInfo.pEngineName = "MiniEngine";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto requiredExtensions = GetRequiredExtensions();

#ifdef __APPLE__ // better macOS support
  requiredExtensions.emplace_back(
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  // Check all extensions are supported
  {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                           availableExtensions.data());

    spdlog::info("Available instance extensions:");
    for (const auto &ext : availableExtensions) {
      spdlog::info("  {}", ext.extensionName);
    }

    for (const auto &ext : requiredExtensions) {
      bool found = false;
      for (const auto &avail : availableExtensions) {
        if (strcmp(ext, avail.extensionName) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        throw std::runtime_error("Required extension not supported: " +
                                 std::string(ext));
      }
    }
  }

  createInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
  createInfo.ppEnabledExtensionNames = requiredExtensions.data();

  spdlog::info("Selected instance extensions:");
  for (auto &ext : requiredExtensions) {
    spdlog::info("  {}{}", ext, (ext == requiredExtensions.back()) ? "" : ",");
  }

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    spdlog::info("Validation layers enabled:");
    for (auto &layer : validationLayers) {
      spdlog::info("  {}", layer);
    }
  } else {
    createInfo.enabledLayerCount = 0;
  }

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    PopulateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create instance");
  }
}

void App::SetupDebugMessenger() {
  spdlog::trace("App::SetupDebugMessenger()");

  if (!enableValidationLayers) {
    return;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  PopulateDebugMessengerCreateInfo(createInfo);

  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("Failed to set up debug messenger");
  }
}

void App::CreateSurface() {
  spdlog::trace("App::CreateSurface()");

  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface");
  }
}

void App::PopulateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  spdlog::trace("App::PopulateDebugMessengerCreateInfo()");

  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = MiniEngine::DebugCallback;
  createInfo.pUserData = nullptr; // Optional
}

void App::PickPhysicalDevice() {
  spdlog::trace("App::PickPhysicalDevice()");

  physicalDevice = VK_NULL_HANDLE;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  // Use an ordered map to automatically sort candidates by increasing score
  // The last element will be the best candidate
  std::multimap<int, VkPhysicalDevice> candidates;

  for (const auto &device : devices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    int score = RateDeviceSuitability(device);
    candidates.insert(std::make_pair(score, device));

    spdlog::info("Device: {} (score: {})", deviceProperties.deviceName, score);
  }
  // Check if the best candidate is suitable at all
  if (candidates.rbegin()->first > 0) {
    // The begin is the best candidate (it's the last element)
    physicalDevice = candidates.rbegin()->second;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    spdlog::info("Selected device: {}", deviceProperties.deviceName);

    this->extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);
  } else {
    throw std::runtime_error("Failed to find a suitable GPU");
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("Failed to find a suitable GPU");
  }
}

void App::CreateLogicalDevice() {
  spdlog::trace("App::CreateLogicalDevice()");

  QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                            indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.queueCreateInfoCount = 1;

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();
  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create logical device");
  }

  this->graphicsQueue = VK_NULL_HANDLE;

  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void App::CreateSwapchain() {
  App::SwapchainSupportDetails swapChainSupport =
      QuerySwapchainSupport(physicalDevice);

  VkSurfaceFormatKHR surfaceFormat =
      ChooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      ChooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                   indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;     // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
  }

  // No transformation
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

  // Ignore alpha channel
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create swap chain");
  }

  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount,
                          swapchainImages.data());

  swapchainImageFormat = surfaceFormat.format;
  swapchainExtent = extent;
}

void App::CreateImageViews() {
  swapchainImageViews.resize(swapchainImages.size());

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapchainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapchainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &createInfo, nullptr,
                          &swapchainImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image views");
    }
  }
}

void App::CreateGraphicsPipeline() {
  spdlog::trace("App::CreateGraphicsPipeline()");

  auto vertShaderCode = this->ReadFile("demo/shaders/vert.spv");
  auto fragShaderCode = this->ReadFile("demo/shaders/frag.spv");

  VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

  // Creation of shaders creation info is almost identical between stages
  // (vertex, fragment, etc.). We can use a lambda to reduce code duplication.
  auto createShader =
      [](VkShaderStageFlagBits stage,
         VkShaderModule module) -> VkPipelineShaderStageCreateInfo {
    VkPipelineShaderStageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = stage;
    createInfo.module = module;
    createInfo.pName = "main";
    return createInfo;
  };

  VkPipelineShaderStageCreateInfo vertShaderStageInfo =
      createShader(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule);

  VkPipelineShaderStageCreateInfo fragShaderStageInfo =
      createShader(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule);

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  // Tell Vulkan we want to make use of dynamic states so we can modify
  // viewport and scissor dynamically.
  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  auto bindingDescription = Vertex::GetBindingDescription();
  auto attributeDescriptions = Vertex::GetAttributeDescriptions();

  // Vertex input is similar to a VAO in OpenGL. It describes the format of
  // the vertex data that will be passed to the vertex shader.
  // Only here it is in the form of a struct that is passed to the pipeline.
  // It is also done in an immutable way, so it cannot be changed after the
  // pipeline has been created.
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  // A binding description specifies the rate at which data is loaded from
  // memory throughout the vertices. The rate can be per vertex or per
  // instance. It is like stride and glVertexAttribPointer in OpenGL.
  // A vertex attribute description specifies the format of the vertex data
  // that will be passed to the vertex shader. It specifies the binding
  // location, format, offset and whether the data is per vertex or per
  // instance. It is like glVertexAttribPointer in OpenGL.

  // Input assembly describes what kind of geometry will be drawn from the
  // vertices and if primitive restart (allows for breaking up lines and
  // triangles in the strip topology by using a special index value) is enabled.
  // The topology is the type of primitive that will be drawn.
  // It can be point lists, line lists, line strips, triangle lists, triangle
  // strips or a list of patches.
  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE; // We don't need this for now

  // Viewport and scissor are part of the viewport state and describe the
  // region of the framebuffer that the output will be rendered to.
  // The scissor test will discard fragments that are outside the scissor
  // rectangle, this can be used for example to only render to a subsection of
  // the framebuffer which can be useful for things like mini-maps.
  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapchainExtent.width);
  viewport.height = static_cast<float>(swapchainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapchainExtent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1; // If we were doing something like a minimap
                                  // we could set this to 2 and have a second
                                  // scissor rectangle.
  viewportState.pScissors = &scissor;

  // Rasterizer takes the geometry that is shaped by the vertices from the
  // vertex shader and turns it into fragments to be colored by the fragment
  // shader. It also performs depth testing, face culling and the scissor test.
  // The depthClampEnable field allows fragments beyond the near and far planes
  // to be clamped rather than discarded.
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE; // Discard fragments beyond near/far
  rasterizer.rasterizerDiscardEnable = VK_FALSE; // Whether to discard data with
                                                 // no output to framebuffer
  rasterizer.polygonMode =
      VK_POLYGON_MODE_FILL;    // Fill mode (like the pen tool in GIMP/PS) any
                               // other mode will require enabling a GPU feature
  rasterizer.lineWidth = 1.0f; // Line width in fragments

  // Similar to `glEnable(GL_CULL_FACE)` and `glCullFace(GL_BACK)`
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;    // Cull back faces
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // Clockwise winding order

  // Depth bias is used for shadow mapping and other effects that require
  // offsetting depth values, we won't be using it.
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f;          // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

  // Multisampling is a way to smooth out the edges of rendered objects, it is
  // especially useful at lower resolutions. It works by combining the
  // fragment shader output of multiple polygons that rasterize to the same
  // pixel.
  // It's similar to `glEnable(GL_MULTISAMPLE)` in OpenGL and creating a
  // multisample framebuffer in OpenGL.
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;          // Optional
  multisampling.pSampleMask = nullptr;            // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE;      // Optional

  // Color blending is used to combine the color output of the fragment shader
  // with the color that is already in the framebuffer. It is used to implement
  // transparency.
  // It's similar to `glEnable(GL_BLEND)` in OpenGL.
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

  // The color blending is controlled per attached framebuffer and there can
  // be multiple. This struct is used to specify global color blending settings.
  // It's similar to `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` in
  // OpenGL.
  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  // Pipeline layout is used to specify uniform values in the shaders
  // Vulkan is strict about how shaders interface with the outside world
  // and requires that you specify in advance what types of resources the
  // shaders will use.
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;            // Optional
  pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
  pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout");
  }

  // Pipeline creation info is similar to the other creation info structs
  // in Vulkan. It is used to specify the shader stages, vertex input,
  // input assembly, viewport, rasterizer, multisampling, color blending
  // and pipeline layout.
  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2; // We have two shader stages, vertex and fragment
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState; // Optional
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle =
      VK_NULL_HANDLE; // Optional (this is deriving from a previous pipeline,
                      // e.g. for resizes)

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &pipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create graphics pipeline");
  }

  // Similar to OpenGL, we can delete the shader modules after the pipeline has
  // been created.
  vkDestroyShaderModule(device, vertShaderModule, nullptr);
  vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

void App::CreateRenderPass() {
  spdlog::trace("App::CreateRenderPass()");

  VkAttachmentDescription colorAttachment =
      {}; // An attachment is a description of the framebuffer
  colorAttachment.format =
      swapchainImageFormat; // The format of the color attachment is the same as
                            // the swap chain images
  colorAttachment.samples =
      VK_SAMPLE_COUNT_1_BIT; // Number of samples to write for multisampling

  // TODO: When we render more, we can use this to clear the framebuffer
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  // We don't care about the previous contents of the framebuffer as we're
  // rendering over it with the exact same data, so we can use
  // VK_ATTACHMENT_LOAD_OP_DONT_CARE to tell Vulkan that we don't care about
  // the previous contents.
  /*colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;*/
  /*colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;*/

  // Stencil buffer settings
  colorAttachment.initialLayout =
      VK_IMAGE_LAYOUT_UNDEFINED; // Similar to DONT_CARE
  colorAttachment.finalLayout =
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Transition to this layout when the
                                       // render pass finishes

  // A colour attachment reference is used to attach the color attachment to the
  // render pass The attachment reference is used to specify which attachment to
  // use for each subpass A subpass is a part of the render pass that can be
  // used for post-processing effects such as blurring, tone mapping, etc. We
  // only have one subpass for now, but we could have more if we wanted to do
  // more complex rendering For example, we could have a subpass for
  // post-processing effects We could also use subpasses for deferred rendering
  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment =
      0; // The index of the attachment in the attachment descriptions array,
         // it's our first and only attachment
  colorAttachmentRef.layout =
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // The layout the attachment
                                                // will have during the subpass

  // Subpasses are like stages in the rendering pipeline
  // They can read from and write to framebuffers
  // They can also read from the output of other subpasses
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint =
      VK_PIPELINE_BIND_POINT_GRAPHICS; // The type of pipeline this subpass is
                                       // for
  subpass.colorAttachmentCount = 1;    // The number of colour attachments
  subpass.pColorAttachments =
      &colorAttachmentRef; // The colour attachments to use, this is always
                           // required

  // Render passes can have dependencies on other render passes
  // This is useful for synchronising access to resources between render passes
  // For example, we could have a render pass that writes to a texture
  // And another render pass that reads from that texture
  // We would need to create a dependency between the two render passes
  // This is done using a subpass dependency
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // The index of the subpass that
                                               // writes to the resource
  dependency.dstSubpass = 0; // The index of the subpass that reads from the
                             // resource (our subpass)

  dependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // The stage of the source
                                                     // subpass
  dependency.srcAccessMask = 0; // The access mask of the source subpass, we
                                // don't need to wait on anything
  dependency.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // The stage of the
                                                     // destination subpass
  dependency.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // The access mask of the
                                            // destination subpass

  // A render pass is a collection of attachments, subpasses and dependencies
  // It describes how the attachments are used over the course of the subpasses
  // It also describes the dependencies between the subpasses
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;             // The number of attachments
  renderPassInfo.pAttachments = &colorAttachment; // The attachments
  renderPassInfo.subpassCount = 1;                // The number of subpasses
  renderPassInfo.pSubpasses = &subpass;           // The subpasses
  renderPassInfo.dependencyCount = 1;             // The number of dependencies
  renderPassInfo.pDependencies = &dependency;     // The dependencies

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass");
  }
}

void App::CreateFramebuffers() {
  swapchainFramebuffers.resize(swapchainImageViews.size());

  for (size_t i = 0; i < swapchainImageViews.size(); i++) {
    VkImageView attachments[] = {swapchainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapchainExtent.width;
    framebufferInfo.height = swapchainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &swapchainFramebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void App::CreateCommandPool() {
  spdlog::trace("App::CreateCommandPool()");

  QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool");
  }
}

void App::CreateVertexBuffer() {
  spdlog::trace("App::CreateVertexBuffers()");
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = sizeof(vertices[0]) *
                    vertices.size(); // Size of a Vertex * Number of vertices
  bufferInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // Buffer is used as a vertex buffer
  bufferInfo.sharingMode =
      VK_SHARING_MODE_EXCLUSIVE; // Buffer is exclusive to a single queue family
                                 // (graphics queue family)

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create vertex buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = FindMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  // Memory the host can map to memory must be host visible we can map it. If
  // it's coherent it means it won't be cached, it will be written directly to
  // the memory.

  if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate vertex buffer memory");
  }

  // Connect the buffer to the memory
  vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

  void *data;
  vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);

  // We now have the memory mapped to the host, we can copy the vertex data
  // into it
  memcpy(data, vertices.data(), (size_t)bufferInfo.size);
  vkUnmapMemory(device, vertexBufferMemory);

  spdlog::info("Mapped data to vertex buffer");
}

void App::CreateCommandBuffers() {
  spdlog::trace("App::CreateCommandBuffer()");

  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers");
  }
}

void App::CreateSyncObjects() {
  spdlog::trace("App::CreateSyncObjects()");

  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.flags =
      VK_FENCE_CREATE_SIGNALED_BIT; // Start in the signaled state, so we can
                                    // wait on it immediately
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    // Create the semaphores and fences
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
            VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores!");
    }
  }
}

void App::CleanupSwapchain() {
  for (auto framebuffer : swapchainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  for (auto &imageView : swapchainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }

  vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void App::RecreateSwapchain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device);

  CleanupSwapchain();

  CreateSwapchain();
  CreateImageViews();
  CreateFramebuffers();
}

void App::SetupImGui() {
  spdlog::trace("App::SetupImGui()");
  // TODO: Is this oversized?
  VkDescriptorPoolSize poolSizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.poolSizeCount = std::size(poolSizes);
  poolInfo.pPoolSizes = poolSizes;
  poolInfo.maxSets = 1000;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr,
                             &imguiDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool");
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForVulkan(window, true);

  // Log out the ImGui version
  spdlog::info("ImGui version: {}", IMGUI_VERSION);

  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = instance;
  initInfo.PhysicalDevice = physicalDevice;
  initInfo.Device = device;
  initInfo.Queue = graphicsQueue;
  initInfo.DescriptorPool = imguiDescriptorPool;
  initInfo.MinImageCount = MAX_FRAMES_IN_FLIGHT;
  initInfo.ImageCount = static_cast<uint32_t>(swapchainImages.size());
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  initInfo.RenderPass = renderPass;
  initInfo.Allocator = nullptr;

  ImGui_ImplVulkan_Init(&initInfo);
}

void App::DrawFrame() {
  static uint32_t currentFrame = 0;

  auto &inFlightFence = inFlightFences[currentFrame];
  auto &imageAvailableSemaphore = imageAvailableSemaphores[currentFrame];
  auto &renderFinishedSemaphore = renderFinishedSemaphores[currentFrame];
  auto &commandBuffer = commandBuffers[currentFrame];

  // Wait for the fence to signal that the frame is finished
  // This is important because we don't want to start drawing a new frame
  // while the previous one is still in flight
  // As long as our update logic is before this function, it will not be blocked
  // by this function meaning if our drawing takes 1ms, we can still update
  // using the remaining 15ms of the frame
  vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

  // Acquire an image from the swap chainm, using a semaphore not a fence!
  uint32_t imageIndex;
  auto res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                                   imageAvailableSemaphore, VK_NULL_HANDLE,
                                   &imageIndex);

  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    RecreateSwapchain();
    return;
  } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swap chain image");
  }

  // Now that we know the surface is up to date, we can reset the fence
  vkResetFences(device, 1, &inFlightFence); // Reset the fence to unsignaled

  // Reset the command buffer to the initial state
  vkResetCommandBuffer(commandBuffer, 0);

  // Begin the command buffer recording
  RecordCommandBuffer(commandBuffer, imageIndex);

  // Submit the command buffer to the graphics queue
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  // Tell the submition info to wait for the
  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount =
      sizeof(waitSemaphores) / sizeof(waitSemaphores[0]);
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  // Inform the submit info of the command buffer
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // Inform the submit info of the semaphore to signal when the command buffer
  // has finished
  VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount =
      sizeof(signalSemaphores) / sizeof(signalSemaphores[0]);
  submitInfo.pSignalSemaphores = signalSemaphores;

  // Submit the command buffer to the graphics queue
  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to submit draw command buffer");
  }

  // Present the image to the swap chain
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  // Inform the present info of the semaphore to wait on
  presentInfo.waitSemaphoreCount =
      sizeof(signalSemaphores) / sizeof(signalSemaphores[0]);
  presentInfo.pWaitSemaphores = signalSemaphores;

  // Inform the present info of the swap chain to present to
  VkSwapchainKHR swapChains[] = {swapchain};
  presentInfo.swapchainCount = sizeof(swapChains) / sizeof(swapChains[0]);
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr; // Optional

  res = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    framebufferResized = false;
    RecreateSwapchain();
  } else if (res != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void App::MainLoop() {
  spdlog::trace("App::MainLoop() after {}ms",
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - startTime)
                    .count());

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    DrawFrame();
  }

  // Wait for the device to finish before cleaning up
  vkDeviceWaitIdle(device);
}

void App::Cleanup() {
  spdlog::trace("App::Cleanup()");

  CleanupSwapchain();

  // Cleanup ImGui
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  // TODO: Is this the correct way to clean up the descriptor pool?
  vkWaitForFences(device, MAX_FRAMES_IN_FLIGHT, inFlightFences.data(), VK_TRUE,
                  UINT64_MAX); // Wait for the fences to signal that the frame
                               // is finished, this is important because we
                               // don't want to wipe out the descriptor pool
                               // while ImGui is still using it
  vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);

  // Clean up the vertex buffer and memory
  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);

  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);

  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);
  glfwTerminate();
}

bool App::CheckValidationLayerSupport() {
  spdlog::trace("App::CheckValidationLayerSupport()");
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
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

std::vector<const char *> App::GetRequiredExtensions() {
  spdlog::trace("App::GetRequiredExtensions()");
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  (void)pUserData;

  static auto logger = spdlog::stdout_color_mt("validation");

  auto messageTypeStr = "Unknown";
  switch (messageType) {
  case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
    messageTypeStr = "General";
    break;
  case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
    messageTypeStr = "Validation";
    break;
  case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
    messageTypeStr = "Performance";
    break;
  case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT:
    break;
  }

  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    logger->trace("[{}] {}", messageTypeStr, pCallbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    logger->info("[{}] {}", messageTypeStr, pCallbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    logger->warn("[{}] {}", messageTypeStr, pCallbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    logger->error("[{}] {}", messageTypeStr, pCallbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    break;
  }

  return VK_FALSE;
}

void FramebufferResizeCallback(GLFWwindow *window, int width, int height) {
  (void)width;
  (void)height;
  auto app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

VkResult App::CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  static auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void App::DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator) {
  static auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

int App::RateDeviceSuitability(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  int score = 0;

  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    score += 1000;
  }

  score += deviceProperties.limits.maxImageDimension2D;

  if (!deviceFeatures.geometryShader) {
    return 0; // No geometry shader support
  }

  if (!FindQueueFamilies(device).IsComplete()) {
    return 0; // No graphics queue family
  }

  if (!CheckDeviceExtensionSupport(device)) {
    return 0; // Missing required extensions
  }

  bool swapChainAdequate = false;
  SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device);
  swapChainAdequate = !swapchainSupport.formats.empty() &&
                      !swapchainSupport.presentModes.empty();

  if (!swapChainAdequate) {
    return 0; // Inadequate swap chain support
  }

  return score;
}

bool App::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

App::QueueFamilyIndices App::FindQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  VkBool32 presentSupport = false;
  vkGetPhysicalDeviceSurfaceSupportKHR(device, 0, surface, &presentSupport);

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.IsComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

App::SwapchainSupportDetails
App::QuerySwapchainSupport(VkPhysicalDevice device) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

VkSurfaceFormatKHR App::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR App::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D App::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

std::vector<char> App::ReadFile(const std::string &filename) {
  spdlog::trace("App::ReadFile({})", filename);

  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

// Shader modules in Vulkan do not seem to care what stage of the pipeline
// that they are.
VkShaderModule App::CreateShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    // It doesn't matter if the error is generic as the error should've come out
    // at compile time of the shader (either external to the engine entirely
    // or somewhere else in the engine).
    throw std::runtime_error("Failed to create shader module");
  }

  return shaderModule;
}

// NOTE: This function also starts a new render pass
void App::RecordCommandBuffer(VkCommandBuffer commandBuffer,
                              uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags =
      0; // Optional, can be VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  beginInfo.pInheritanceInfo =
      nullptr; // Optional this is for secondary command buffers

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("Failed to begin recording command buffer");
  }

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer =
      swapchainFramebuffers[imageIndex]; // This will usually be 0 in our case,
                                         // as we only have one framebuffer
  renderPassInfo.renderArea.offset = {0, 0}; // Start at the top left corner
  renderPassInfo.renderArea.extent =
      swapchainExtent; // The size of the framebuffer (window size which is the
                       // swapchain extent)S

  renderPassInfo.clearValueCount = 1;        // We only have one clear value
  renderPassInfo.pClearValues = &clearColor; // The clear value

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Bind the graphics pipeline
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  // New viewport and scissor
  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapchainExtent.width);
  viewport.height = static_cast<float>(swapchainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapchainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  // Bind the vertex buffer
  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  // Draw the vertices
  vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);
  ImGui::Begin("Controls");

  ImGui::ColorEdit3("Clear Color", &clearColor.color.float32[0]);

  ImGui::End();

  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
  ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to record command buffer");
  }
}

uint32_t App::FindMemoryType(uint32_t typeFilter,
                             VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      // The memory type is suitable for the buffer (it's in the type filter and
      // has the required properties)
      // We aren't concerned with the index of the memory type, just that it
      // exists, we could check for whether it's the best memory type for the
      // buffer, but we're not doing that here
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory type");
}
} // namespace MiniEngine
