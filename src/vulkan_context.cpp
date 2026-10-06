#include <coffee/vulkan_context.h>
#include <coffee/error.h>

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <type_traits>

namespace coffee {

namespace {

void require_vk(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw error(error_code::graphics_initialization,
                    std::string(operation) + " (VkResult " + std::to_string(static_cast<int>(result)) + ").");
    }
}

struct queue_families {
    std::optional<std::uint32_t> graphics;
    std::optional<std::uint32_t> present;
    [[nodiscard]] bool complete() const noexcept { return graphics.has_value() && present.has_value(); }
};

queue_families find_queues(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());
    queue_families result;
    for (std::uint32_t index = 0; index < count; ++index) {
        if ((properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) result.graphics = index;
        VkBool32 presentation = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentation);
        if (presentation == VK_TRUE) result.present = index;
        if (result.complete()) break;
    }
    return result;
}

bool supports_swapchain(VkPhysicalDevice device) {
    std::uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> properties(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, properties.data());
    return std::any_of(properties.begin(), properties.end(), [](const auto& property) {
        return std::string_view(property.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    });
}

template<typename Handle>
std::uintptr_t handle_value(Handle value) noexcept {
    if constexpr (std::is_pointer_v<Handle>) return reinterpret_cast<std::uintptr_t>(value);
    else return static_cast<std::uintptr_t>(value);
}

template<typename Handle>
Handle handle_from_uintptr(std::uintptr_t value) noexcept {
    if constexpr (std::is_pointer_v<Handle>) return reinterpret_cast<Handle>(value);
    else return static_cast<Handle>(value);
}

} // namespace

struct VulkanContext::impl {
    Window* owner = nullptr;
    vulkan_config config{};
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    std::uint32_t graphics_family = 0;
    std::uint32_t present_family = 0;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkSemaphore> image_available;
    std::vector<VkSemaphore> render_finished;
    std::vector<VkFence> in_flight;
    std::uint32_t current_frame = 0;
    std::uint32_t image_index = 0;
    bool frame_active = false;
    bool resize_requested = false;
    std::string gpu_name;
    std::uint32_t version = 0;

    ~impl() { release(); }

    void initialize() {
        const auto extension_names = owner->vulkan_instance_extensions();
        std::vector<const char*> extensions;
        extensions.reserve(extension_names.size());
        for (const auto& name : extension_names) extensions.push_back(name.c_str());
        VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        application.pApplicationName = config.application_name.c_str();
        application.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        application.pEngineName = "Coffee";
        application.engineVersion = VK_MAKE_VERSION(COFFEE_VERSION_MAJOR, COFFEE_VERSION_MINOR, COFFEE_VERSION_PATCH);
        application.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        instance_info.pApplicationInfo = &application;
        instance_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        instance_info.ppEnabledExtensionNames = extensions.data();
        require_vk(vkCreateInstance(&instance_info, nullptr, &instance), "Could not create Vulkan instance");

        surface = handle_from_uintptr<VkSurfaceKHR>(
            owner->create_vulkan_surface(handle_value(instance)));

        std::uint32_t device_count = 0;
        require_vk(vkEnumeratePhysicalDevices(instance, &device_count, nullptr), "Could not enumerate Vulkan devices");
        if (device_count == 0) throw error(error_code::graphics_initialization, "No Vulkan-capable GPU was found.");
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
        for (const auto candidate : devices) {
            const auto queues = find_queues(candidate, surface);
            if (!queues.complete() || !supports_swapchain(candidate)) continue;
            std::uint32_t format_count = 0;
            std::uint32_t present_mode_count = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &format_count, nullptr);
            vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface, &present_mode_count, nullptr);
            if (format_count == 0 || present_mode_count == 0) continue;
            physical_device = candidate;
            graphics_family = *queues.graphics;
            present_family = *queues.present;
            break;
        }
        if (physical_device == VK_NULL_HANDLE) {
            throw error(error_code::graphics_initialization, "No Vulkan GPU supports graphics and presentation for this window.");
        }

        VkPhysicalDeviceProperties device_properties{};
        vkGetPhysicalDeviceProperties(physical_device, &device_properties);
        gpu_name = device_properties.deviceName;
        version = device_properties.apiVersion;

        const std::set<std::uint32_t> unique_families{graphics_family, present_family};
        constexpr float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        for (const auto family : unique_families) {
            VkDeviceQueueCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            info.queueFamilyIndex = family;
            info.queueCount = 1;
            info.pQueuePriorities = &priority;
            queue_infos.push_back(info);
        }
        const char* device_extensions[]{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_infos.size());
        device_info.pQueueCreateInfos = queue_infos.data();
        device_info.enabledExtensionCount = 1;
        device_info.ppEnabledExtensionNames = device_extensions;
        require_vk(vkCreateDevice(physical_device, &device_info, nullptr, &device), "Could not create Vulkan device");
        vkGetDeviceQueue(device, graphics_family, 0, &graphics_queue);
        vkGetDeviceQueue(device, present_family, 0, &present_queue);

        VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = graphics_family;
        require_vk(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool), "Could not create Vulkan command pool");

        config.frames_in_flight = clamp(config.frames_in_flight, 1u, 3u);
        command_buffers.resize(config.frames_in_flight);
        VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocation.commandPool = command_pool;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = config.frames_in_flight;
        require_vk(vkAllocateCommandBuffers(device, &allocation, command_buffers.data()), "Could not allocate Vulkan command buffers");
        create_synchronization();
        create_swapchain();
    }

    void create_synchronization() {
        image_available.resize(config.frames_in_flight);
        render_finished.resize(config.frames_in_flight);
        in_flight.resize(config.frames_in_flight);
        VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (std::uint32_t index = 0; index < config.frames_in_flight; ++index) {
            require_vk(vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available[index]), "Could not create Vulkan semaphore");
            require_vk(vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished[index]), "Could not create Vulkan semaphore");
            require_vk(vkCreateFence(device, &fence_info, nullptr, &in_flight[index]), "Could not create Vulkan fence");
        }
    }

    void create_swapchain() {
        const auto size = owner->framebuffer_size();
        if (size.width <= 0 || size.height <= 0) return;
        VkSurfaceCapabilitiesKHR capabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
        std::uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());
        VkSurfaceFormatKHR selected_format = formats.front();
        for (const auto& candidate : formats) {
            if (candidate.format == VK_FORMAT_B8G8R8A8_SRGB && candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                selected_format = candidate;
                break;
            }
        }
        std::uint32_t mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, nullptr);
        std::vector<VkPresentModeKHR> modes(mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, modes.data());
        VkPresentModeKHR selected_mode = VK_PRESENT_MODE_FIFO_KHR;
        if (!config.vsync && std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != modes.end()) {
            selected_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
        extent = capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()
            ? capabilities.currentExtent
            : VkExtent2D{
                clamp(static_cast<std::uint32_t>(size.width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                clamp(static_cast<std::uint32_t>(size.height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
        std::uint32_t image_count = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0) image_count = std::min(image_count, capabilities.maxImageCount);

        VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        info.surface = surface;
        info.minImageCount = image_count;
        info.imageFormat = selected_format.format;
        info.imageColorSpace = selected_format.colorSpace;
        info.imageExtent = extent;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const std::uint32_t queue_indices[]{graphics_family, present_family};
        if (graphics_family != present_family) {
            info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            info.queueFamilyIndexCount = 2;
            info.pQueueFamilyIndices = queue_indices;
        } else {
            info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        info.preTransform = capabilities.currentTransform;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = selected_mode;
        info.clipped = VK_TRUE;
        require_vk(vkCreateSwapchainKHR(device, &info, nullptr, &swapchain), "Could not create Vulkan swapchain");
        swapchain_format = selected_format.format;
        vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
        images.resize(image_count);
        vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data());

        image_views.resize(images.size());
        for (std::size_t index = 0; index < images.size(); ++index) {
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view.image = images[index];
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format = swapchain_format;
            view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view.subresourceRange.levelCount = 1;
            view.subresourceRange.layerCount = 1;
            require_vk(vkCreateImageView(device, &view, nullptr, &image_views[index]), "Could not create Vulkan image view");
        }

        VkAttachmentDescription attachment{};
        attachment.format = swapchain_format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &reference;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo render_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        render_info.attachmentCount = 1;
        render_info.pAttachments = &attachment;
        render_info.subpassCount = 1;
        render_info.pSubpasses = &subpass;
        render_info.dependencyCount = 1;
        render_info.pDependencies = &dependency;
        require_vk(vkCreateRenderPass(device, &render_info, nullptr, &render_pass), "Could not create Vulkan render pass");

        framebuffers.resize(image_views.size());
        for (std::size_t index = 0; index < image_views.size(); ++index) {
            VkFramebufferCreateInfo framebuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebuffer_info.renderPass = render_pass;
            framebuffer_info.attachmentCount = 1;
            framebuffer_info.pAttachments = &image_views[index];
            framebuffer_info.width = extent.width;
            framebuffer_info.height = extent.height;
            framebuffer_info.layers = 1;
            require_vk(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffers[index]), "Could not create Vulkan framebuffer");
        }
        resize_requested = false;
    }

    void destroy_swapchain() noexcept {
        for (const auto framebuffer : framebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffers.clear();
        if (render_pass != VK_NULL_HANDLE) vkDestroyRenderPass(device, render_pass, nullptr);
        render_pass = VK_NULL_HANDLE;
        for (const auto view : image_views) vkDestroyImageView(device, view, nullptr);
        image_views.clear();
        images.clear();
        if (swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }

    void recreate_swapchain() {
        vkDeviceWaitIdle(device);
        destroy_swapchain();
        create_swapchain();
    }

    void release() noexcept {
        if (device != VK_NULL_HANDLE) vkDeviceWaitIdle(device);
        destroy_swapchain();
        for (const auto value : in_flight) vkDestroyFence(device, value, nullptr);
        for (const auto value : render_finished) vkDestroySemaphore(device, value, nullptr);
        for (const auto value : image_available) vkDestroySemaphore(device, value, nullptr);
        if (command_pool != VK_NULL_HANDLE) vkDestroyCommandPool(device, command_pool, nullptr);
        if (device != VK_NULL_HANDLE) vkDestroyDevice(device, nullptr);
        if (surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance, surface, nullptr);
        if (instance != VK_NULL_HANDLE) vkDestroyInstance(instance, nullptr);
        device = VK_NULL_HANDLE;
        instance = VK_NULL_HANDLE;
    }
};

VulkanContext::VulkanContext(Window& window, vulkan_config config) : impl_(std::make_unique<impl>()) {
    if (window.backend() != graphics_backend::vulkan) {
        throw error(error_code::invalid_argument, "VulkanContext requires a Window configured with graphics_backend::vulkan.");
    }
    impl_->owner = &window;
    impl_->config = std::move(config);
    impl_->initialize();
}
VulkanContext::~VulkanContext() = default;
VulkanContext::VulkanContext(VulkanContext&&) noexcept = default;
VulkanContext& VulkanContext::operator=(VulkanContext&&) noexcept = default;

bool VulkanContext::begin_frame(color clear_color) {
    if (impl_->frame_active) throw error(error_code::invalid_operation, "A Vulkan frame is already active.");
    if (impl_->resize_requested || impl_->swapchain == VK_NULL_HANDLE) impl_->recreate_swapchain();
    if (impl_->swapchain == VK_NULL_HANDLE) return false;
    const auto frame = impl_->current_frame;
    require_vk(vkWaitForFences(impl_->device, 1, &impl_->in_flight[frame], VK_TRUE, UINT64_MAX), "Could not wait for Vulkan frame");
    const VkResult acquired = vkAcquireNextImageKHR(impl_->device, impl_->swapchain, UINT64_MAX,
                                                     impl_->image_available[frame], VK_NULL_HANDLE, &impl_->image_index);
    if (acquired == VK_ERROR_OUT_OF_DATE_KHR) { impl_->recreate_swapchain(); return false; }
    if (acquired != VK_SUCCESS && acquired != VK_SUBOPTIMAL_KHR) require_vk(acquired, "Could not acquire Vulkan swapchain image");
    require_vk(vkResetFences(impl_->device, 1, &impl_->in_flight[frame]), "Could not reset Vulkan fence");
    require_vk(vkResetCommandBuffer(impl_->command_buffers[frame], 0), "Could not reset Vulkan command buffer");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    require_vk(vkBeginCommandBuffer(impl_->command_buffers[frame], &begin), "Could not begin Vulkan command buffer");
    const VkClearValue clear{{{clear_color.x, clear_color.y, clear_color.z, clear_color.w}}};
    VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render.renderPass = impl_->render_pass;
    render.framebuffer = impl_->framebuffers[impl_->image_index];
    render.renderArea.extent = impl_->extent;
    render.clearValueCount = 1;
    render.pClearValues = &clear;
    vkCmdBeginRenderPass(impl_->command_buffers[frame], &render, VK_SUBPASS_CONTENTS_INLINE);
    impl_->frame_active = true;
    return true;
}

void VulkanContext::end_frame() {
    if (!impl_->frame_active) throw error(error_code::invalid_operation, "VulkanContext::end_frame requires an active frame.");
    const auto frame = impl_->current_frame;
    auto command = impl_->command_buffers[frame];
    vkCmdEndRenderPass(command);
    require_vk(vkEndCommandBuffer(command), "Could not finish Vulkan command buffer");
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &impl_->image_available[frame];
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &impl_->render_finished[frame];
    require_vk(vkQueueSubmit(impl_->graphics_queue, 1, &submit, impl_->in_flight[frame]), "Could not submit Vulkan frame");
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &impl_->render_finished[frame];
    present.swapchainCount = 1;
    present.pSwapchains = &impl_->swapchain;
    present.pImageIndices = &impl_->image_index;
    const VkResult result = vkQueuePresentKHR(impl_->present_queue, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || impl_->resize_requested) {
        impl_->recreate_swapchain();
    } else if (result != VK_SUCCESS) {
        require_vk(result, "Could not present Vulkan frame");
    }
    impl_->frame_active = false;
    impl_->current_frame = (frame + 1) % impl_->config.frames_in_flight;
}

void VulkanContext::request_resize() noexcept { impl_->resize_requested = true; }
void VulkanContext::wait_idle() { require_vk(vkDeviceWaitIdle(impl_->device), "Could not wait for Vulkan device"); }
std::string VulkanContext::device_name() const { return impl_->gpu_name; }
std::uint32_t VulkanContext::api_version() const noexcept { return impl_->version; }
std::uintptr_t VulkanContext::native_instance() const noexcept { return handle_value(impl_->instance); }
std::uintptr_t VulkanContext::native_physical_device() const noexcept { return handle_value(impl_->physical_device); }
std::uintptr_t VulkanContext::native_device() const noexcept { return handle_value(impl_->device); }

} // namespace coffee
