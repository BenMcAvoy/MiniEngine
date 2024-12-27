#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace MiniEngine {
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

// NOLINTNEXTLINE
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

class App {
public:
  App();
  ~App();

  void Run();

private:
  // Member functions
  void InitWindow();
  void InitVulkan();
  void CreateInstance();
  void SetupDebugMessenger();
  void PopulateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT &createInfo);
  void CreateSurface();
  void PickPhysicalDevice();
  void CreateLogicalDevice();
  void CreateSwapchain();
  void CreateImageViews();
  void CreateRenderPass();
  void CreateGraphicsPipeline();
  void CreateFramebuffers();
  void CreateCommandPool();
  void CreateCommandBuffer();
  void CreateSyncObjects();
  void DrawFrame();
  void MainLoop();
  void Cleanup();

  // Helper functions
  bool CheckValidationLayerSupport();
  bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
  std::vector<const char *> GetRequiredExtensions();

  // NOTE: This is a Vulkan extension function, but it is not defined in the
  // Vulkan headers. This is a proxy function that will load the function if it
  // is available.
  VkResult CreateDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkDebugUtilsMessengerEXT *pDebugMessenger);

  // NOTE: This is a Vulkan extension function, but it is not defined in the
  // Vulkan headers. This is a proxy function that will load the function if it
  // is available.
  void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks *pAllocator);

  int RateDeviceSuitability(VkPhysicalDevice device);

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily = std::nullopt;
    std::optional<uint32_t> presentFamily = std::nullopt;

    bool IsComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

  struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);

  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);

  VkPresentModeKHR ChooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);

  VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  std::vector<char> ReadFile(const std::string &filename);

  VkShaderModule CreateShaderModule(const std::vector<char> &code);

  void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  // Data members
  std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

  GLFWwindow *window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;
  VkFormat swapchainImageFormat;
  VkExtent2D swapchainExtent;
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;

  std::vector<VkImage> swapchainImages;
  std::vector<VkImageView> swapchainImageViews;

  std::vector<VkFramebuffer> swapchainFramebuffers;

  // This semaphore will signal when the image is available to render to.
  VkSemaphore imageAvailableSemaphore;

  // This semaphore will signal when rendering has finished and the image can be
  // presented.
  VkSemaphore renderFinishedSemaphore;

  // This fence will signal when the command buffer has finished executing.
  VkFence inFlightFence;

  bool extensionsSupported;
};
} // namespace MiniEngine
