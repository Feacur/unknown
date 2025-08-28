#include <vulkan/vulkan.h>

#include "os.h"

/*
left-handed, positive rotation is counter-clockwise
  curled fingers show rotation direction between two vectors
  thumb shows resulting cross-product direction

axes
  X positive right
  Y positive up, even in NDC
  Z positive forward, as Z = X cross Y

matrices are colum-major style
  result = matrix x vector
*/

#define GFX_DEFINE_PROC(name) PFN_ ## name name = (PFN_ ## name)gfx_get_instance_proc(#name)
#define GFX_ENABLE_DEBUG (BUILD_DEBUG == BUILD_DEBUG_ENABLE)

// @note vulkan expects a 64 bit architecture at least
AssertStatic(sizeof(size_t) >= sizeof(uint64_t));

// ---- ---- ---- ----
// stringifiers
// ---- ---- ---- ----

#if GFX_ENABLE_DEBUG
AttrFileLocal()
str8 gfx_to_string_debug_utils_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT value) {
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   return str8_lit("ERROR");
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) return str8_lit("WARNING");
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    return str8_lit("INFO");
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) return str8_lit("VERBOSE");
	return str8_lit("unknown");
}

AttrFileLocal()
str8 gfx_to_string_debug_utils_message_type(VkDebugUtilsMessageTypeFlagsEXT value) {
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT) return str8_lit("Device address binding");
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)            return str8_lit("Performance");
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)             return str8_lit("Validation");
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)                return str8_lit("General");
	return str8_lit("unknown");
}

AttrFileLocal()
str8 gfx_to_string_allocation_type(VkInternalAllocationType value) {
	switch (value) {
		case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE: return str8_lit("Executable");
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 gfx_to_string_allocation_scope(VkSystemAllocationScope value) {
	switch (value) {
		case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:  return str8_lit("Command");
		case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:   return str8_lit("Object");
		case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:    return str8_lit("Cache");
		case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:   return str8_lit("Device");
		case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE: return str8_lit("Instance");
		default: return str8_lit("unknown");
	}
}
#endif

AttrFileLocal()
str8 gfx_to_string_physical_device_type(VkPhysicalDeviceType value) {
	switch (value) {
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:          return str8_lit("Other");
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return str8_lit("Integrated GPU");
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return str8_lit("Discrete GPU");
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return str8_lit("Virtual GPU");
		case VK_PHYSICAL_DEVICE_TYPE_CPU:            return str8_lit("CPU");
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 gfx_to_string_format(VkFormat value) {
	switch (value) {
		case VK_FORMAT_UNDEFINED:                return str8_lit("UNDEFINED");
		case VK_FORMAT_R8G8B8A8_UNORM:           return str8_lit("R8 G8 B8 A8 UNORM");
		case VK_FORMAT_R8G8B8A8_SRGB:            return str8_lit("R8 G8 B8 A8 SRGB");
		case VK_FORMAT_B8G8R8A8_UNORM:           return str8_lit("B8 G8 R8 A8 UNORM");
		case VK_FORMAT_B8G8R8A8_SRGB:            return str8_lit("B8 G8 R8 A8 SRGB");
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return str8_lit("A2 B10 G10 R10 UNORM PACK32");
		// @note has a lot more cases
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 gfx_to_string_color_space(VkColorSpaceKHR value) {
	switch (value) {
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return str8_lit("sRGB");
		// @note has a lot more cases
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 gfx_to_string_present_mode(VkPresentModeKHR value) {
	switch (value) {
		case VK_PRESENT_MODE_IMMEDIATE_KHR:                 return str8_lit("Immediate");
		case VK_PRESENT_MODE_MAILBOX_KHR:                   return str8_lit("Mailbox");
		case VK_PRESENT_MODE_FIFO_KHR:                      return str8_lit("FIFO");
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR:              return str8_lit("FIFO relaxed");
		case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:     return str8_lit("Shared demand refresh");
		case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: return str8_lit("Shared continuous refresh");
		case VK_PRESENT_MODE_FIFO_LATEST_READY_EXT:         return str8_lit("FIFO latest ready");
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 gfx_to_string_queue(VkQueueFlagBits value) {
	switch (value) {
		case VK_QUEUE_GRAPHICS_BIT:         return str8_lit("Graphics");
		case VK_QUEUE_COMPUTE_BIT:          return str8_lit("Compute");
		case VK_QUEUE_TRANSFER_BIT:         return str8_lit("Transfer");
		case VK_QUEUE_SPARSE_BINDING_BIT:   return str8_lit("Sparse binding");
		case VK_QUEUE_PROTECTED_BIT:        return str8_lit("Protected");
		case VK_QUEUE_VIDEO_DECODE_BIT_KHR: return str8_lit("Video decode");
		case VK_QUEUE_VIDEO_ENCODE_BIT_KHR: return str8_lit("Video encode");
		case VK_QUEUE_OPTICAL_FLOW_BIT_NV:  return str8_lit("Optical flow");
		default: return str8_lit("unknown");
	}
}

// ---- ---- ---- ----
// debugging
// ---- ---- ---- ----

#if GFX_ENABLE_DEBUG
AttrFileLocal() VKAPI_PTR
VkBool32 gfx_debug_utils_messenger_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*                                       pUserData) {
	str8 const severity_text = gfx_to_string_debug_utils_message_severity(messageSeverity);
	str8 const type_text = gfx_to_string_debug_utils_message_type(messageTypes); // @note argument is of flags type, but always has a single one
	fmt_print("[gfx] %.*s %.*s - %s\n",
		(int)severity_text.count, severity_text.buffer,
		(int)type_text.count, type_text.buffer,
		pCallbackData->pMessageIdName);
	fmt_print("      %s\n", pCallbackData->pMessage);
	fmt_print("\n");
	Assert(!(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT), "");
	return VK_FALSE;
}

AttrFileLocal() VKAPI_PTR
void gfx_debug_on_internal_allocation(
	void*                    pUserData,
	size_t                   size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope  allocationScope) {
	str8 const type_text = gfx_to_string_allocation_type(allocationType);
	str8 const scope_text = gfx_to_string_allocation_scope(allocationScope);
	fmt_print("[gfx] internal allocation %zu %.*s %.*s\n", size,
		(int)type_text.count, type_text.buffer,
		(int)scope_text.count, scope_text.buffer);
	fmt_print("\n");
}

AttrFileLocal() VKAPI_PTR
void gfx_debug_on_internal_free(
	void*                    pUserData,
	size_t                   size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope  allocationScope) {
	str8 const type_text = gfx_to_string_allocation_type(allocationType);
	str8 const scope_text = gfx_to_string_allocation_scope(allocationScope);
	fmt_print("[gfx] internal free %zu %.*s %.*s\n", size,
		(int)type_text.count, type_text.buffer,
		(int)scope_text.count, scope_text.buffer);
	fmt_print("\n");
}
#endif

// ---- ---- ---- ----
// memory routines
// ---- ---- ---- ----

AttrFileLocal() VKAPI_PTR
void * gfx_memory_heap(void * ptr, size_t size, size_t align) {
	// @note debug info for local structs seems to be broken
	struct Header {u16 offset, align;};
	u16 const max_align = 0x8000; // UINT16_MAX / 2 + 1;
	AssertF(align <= max_align, "alignment overflow %zu / %u\n", align, max_align);
	// restore original pointer
	if (ptr != NULL) {
		struct Header const header = *((struct Header *)ptr - 1);
		ptr = (u8 *)ptr - header.offset;
		if (align == 0) align = header.align;
	}
	// allocate enough space for header and offset
	if (size > 0) size += sizeof(struct Header) + (align - 1);
	void * ret = os_memory_heap(ptr, size);
	// align the new pointer and store info
	if (ret != NULL) {
		void * aligned = (void *)align_size((size_t)ret + sizeof(struct Header), align);
		struct Header * header = ((struct Header *)aligned - 1);
		header->offset = (u16)((size_t)aligned - (size_t)ret);
		header->align = (u16)align;
		ret = aligned;
	}
	return ret;
}

AttrFileLocal() VKAPI_PTR
void * gfx_memory_allocate(
	void*                   pUserData,
	size_t                  size,
	size_t                  alignment,
	VkSystemAllocationScope allocationScope) {
	return gfx_memory_heap(NULL, size, alignment);
}

AttrFileLocal() VKAPI_PTR
void * gfx_memory_reallocate(
	void*                   pUserData,
	void*                   pOriginal,
	size_t                  size,
	size_t                  alignment,
	VkSystemAllocationScope allocationScope) {
	return gfx_memory_heap(pOriginal, size, alignment);
}

AttrFileLocal() VKAPI_PTR
void gfx_memory_free(
	void* pUserData,
	void* pMemory) {
	gfx_memory_heap(pMemory, 0, 0);
}

// ---- ---- ---- ----
// implementation
// ---- ---- ---- ----

#include "gfx.h"

AttrFileLocal()
struct GFX {
	VkAllocationCallbacks allocator;
	// instance
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_utils_messenger;
	// surface
	VkSurfaceKHR surface;
	// device
	VkPhysicalDevice physical_device;
	VkFormat surface_format;
	VkDevice device;
	VkQueue device_graphics_queue;
	VkQueue device_surface_queue;
	// syncronization
	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
	VkFence     in_flight_fence;
	// command pool
	VkCommandPool   command_pool;
	VkCommandBuffer command_buffer;
	// render pass and swapchain
	VkRenderPass render_pass;
	VkExtent2D       swapchain_extent;
	VkSwapchainKHR   swapchain;
	uint32_t         swapchain_images_count;
	VkImage        * swapchain_images;
	VkImageView    * swapchain_image_views;
	VkFramebuffer  * framebuffers;
	// pipeline / USER DATA
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;
} fl_gfx;

AttrFileLocal()
void * gfx_get_instance_proc(char const * name) {
	void * ret = (void *)vkGetInstanceProcAddr(fl_gfx.instance, name);
	AssertF(ret, "[gfx] `vkGetInstanceProcAddr(0x%p, \"%s\")` failed\n", (void *)fl_gfx.instance, name);
	return ret;
}

AttrFileLocal()
VkShaderModule gfx_create_shader_module(char const * name) {
	struct Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = arena_get_position(scratch);

	struct Array_U8 const source = base_file_read(scratch, name);
	VkShaderModule ret;
	vkCreateShaderModule(
		fl_gfx.device,
		&(VkShaderModuleCreateInfo){
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = source.count,
			.pCode = (void *)source.buffer,
		},
		&fl_gfx.allocator,
		&ret
	);

	arena_set_position(scratch, scratch_position);
	return ret;
}

void gfx_init(void) {
	struct Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = arena_get_position(scratch);

	fl_gfx.allocator = (VkAllocationCallbacks){
		// .pUserData = fl_gfx.arena,
		.pfnAllocation   = gfx_memory_allocate,
		.pfnReallocation = gfx_memory_reallocate,
		.pfnFree         = gfx_memory_free,
		#if GFX_ENABLE_DEBUG
		.pfnInternalAllocation = gfx_debug_on_internal_allocation,
		.pfnInternalFree       = gfx_debug_on_internal_free,
		#endif
	};

	// -- collect instance version
	uint32_t api_version;
	vkEnumerateInstanceVersion(&api_version);

	// -- log instance version
	{
		uint32_t const v[4] = {
			VK_API_VERSION_VARIANT(api_version),
			VK_API_VERSION_MAJOR(api_version),
			VK_API_VERSION_MINOR(api_version),
			VK_API_VERSION_PATCH(api_version),
		};

		fmt_print("[gfx] vulkan:\n");
		fmt_print("- [%u] v%u.%u.%u\n", v[0], v[1], v[2], v[3]);
		fmt_print("\n");
	}

	// -- collect available instance extensions
	uint32_t available_instance_extension_properties_count;
	vkEnumerateInstanceExtensionProperties(NULL, &available_instance_extension_properties_count, NULL);
	VkExtensionProperties * available_instance_extension_properties = ArenaPushArray(scratch, VkExtensionProperties, available_instance_extension_properties_count);
	vkEnumerateInstanceExtensionProperties(NULL, &available_instance_extension_properties_count, available_instance_extension_properties);

	// -- log available instance extensions
	fmt_print("[gfx] available instance extensions (%u):\n", available_instance_extension_properties_count);
	for (uint32_t i = 0; i < available_instance_extension_properties_count; i++) {
		VkExtensionProperties const * it = available_instance_extension_properties + i;
		fmt_print("- %s\n", it->extensionName);
	}
	fmt_print("\n");

	// -- collect available instance layers
	uint32_t available_instance_layer_properties_count;
	vkEnumerateInstanceLayerProperties(&available_instance_layer_properties_count, NULL);
	VkLayerProperties * available_instance_layer_properties = ArenaPushArray(scratch, VkLayerProperties, available_instance_layer_properties_count);
	vkEnumerateInstanceLayerProperties(&available_instance_layer_properties_count, available_instance_layer_properties);

	// -- log available instance layers
	fmt_print("[gfx] available instance layers (%u):\n", available_instance_layer_properties_count);
	for (uint32_t i = 0; i < available_instance_layer_properties_count; i++) {
		VkLayerProperties const * it = available_instance_layer_properties + i;

		uint32_t const v[4] = {
			VK_API_VERSION_VARIANT(it->specVersion),
			VK_API_VERSION_MAJOR(it->specVersion),
			VK_API_VERSION_MINOR(it->specVersion),
			VK_API_VERSION_PATCH(it->specVersion),
		};

		fmt_print("- %-36s - [%u] v%u.%u.%-3u - %s\n", it->layerName, v[0], v[1], v[2], v[3], it->description);
	}
	fmt_print("\n");

	// --prepare instance extensions
	uint32_t requested_instance_extensions_count = 0;
	char const ** const requested_instance_extensions = ArenaPushArray(scratch, char const *, available_instance_extension_properties_count);
	os_vulkan_push_extensions(&requested_instance_extensions_count, requested_instance_extensions);
	requested_instance_extensions[requested_instance_extensions_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
	#if GFX_ENABLE_DEBUG
	requested_instance_extensions[requested_instance_extensions_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	#endif

	for (uint32_t i = 0; i < requested_instance_extensions_count; i++) {
		char const * requested = requested_instance_extensions[i];
		for (uint32_t i = 0; i < available_instance_extension_properties_count; i++) {
			char const * available = available_instance_extension_properties[i].extensionName;
			if (str_equals(requested, available))
				goto instance_extension_found;
		}
		AssertF(false, "extension \"%s\" is not available\n", requested);
		instance_extension_found:;
	}

	// --prepare instance layers
	char const * const requested_instance_layers[] = {
		#if GFX_ENABLE_DEBUG
		"VK_LAYER_KHRONOS_validation",
		#endif
	};

	for (uint32_t i = 0; i < ArrayCount(requested_instance_layers); i++) {
		char const * requested = requested_instance_layers[i];
		for (uint32_t i = 0; i < available_instance_layer_properties_count; i++) {
			char const * available = available_instance_layer_properties[i].layerName;
			if (str_equals(requested, available))
				goto instance_layer_found;
		}
		AssertF(false, "layer \"%s\" is not available\n", requested);
		instance_layer_found:;
	}

	// -- prepare debug
	#if GFX_ENABLE_DEBUG
	VkDebugUtilsMessengerCreateInfoEXT const * debug_utils_messenger_create_info = &(VkDebugUtilsMessengerCreateInfoEXT){
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		/**/             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		/**/             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
		/**/             ,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		/**/         | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		/**/         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
		/**/         ,
		.pfnUserCallback = gfx_debug_utils_messenger_callback,
	};
	#endif

	// -- create instance
	vkCreateInstance(
		&(VkInstanceCreateInfo){
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			// extensions
			.enabledExtensionCount = requested_instance_extensions_count,
			.ppEnabledExtensionNames = requested_instance_extensions,
			// layers
			.enabledLayerCount = ArrayCount(requested_instance_layers),
			.ppEnabledLayerNames = requested_instance_layers,
			// application
			.pApplicationInfo = &(VkApplicationInfo){
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				// application
				.pApplicationName = "gfx_unknown_vk_application",
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				// engine
				.pEngineName = "gfx_unknown_vk_engine",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				// API
				.apiVersion = VK_API_VERSION_1_4,
			},
			// lifetime debug
			#if GFX_ENABLE_DEBUG
			.pNext = debug_utils_messenger_create_info,
			#endif
		},
		&fl_gfx.allocator,
		&fl_gfx.instance
	);

	// -- init debug messenger
	#if GFX_ENABLE_DEBUG
	GFX_DEFINE_PROC(vkCreateDebugUtilsMessengerEXT);
	vkCreateDebugUtilsMessengerEXT(
		fl_gfx.instance,
		debug_utils_messenger_create_info,
		&fl_gfx.allocator,
		&fl_gfx.debug_utils_messenger
	);
	#endif

	// -- init an OS-specific presentation surface
	fl_gfx.surface = (VkSurfaceKHR)os_vulkan_create_surface(fl_gfx.instance, &fl_gfx.allocator);

	// -- collect physical device handles
	uint32_t physical_devices_count;
	vkEnumeratePhysicalDevices(fl_gfx.instance, &physical_devices_count, NULL);
	VkPhysicalDevice * physical_devices = ArenaPushArray(scratch, VkPhysicalDevice, physical_devices_count);
	vkEnumeratePhysicalDevices(fl_gfx.instance, &physical_devices_count, physical_devices);

	// -- collect physical device properties
	VkPhysicalDeviceProperties * physical_device_properties = ArenaPushArray(scratch, VkPhysicalDeviceProperties, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDeviceProperties * properties = physical_device_properties + i;
		vkGetPhysicalDeviceProperties(physical_devices[i], properties);
	}

	// -- collect physical device surface capabilities
	VkSurfaceCapabilitiesKHR * surface_capabilities = ArenaPushArray(scratch, VkSurfaceCapabilitiesKHR, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkSurfaceCapabilitiesKHR * capabilities = surface_capabilities + i;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_devices[i], fl_gfx.surface, capabilities);
	}

	// -- collect physical device surface formats
	uint32_t * surface_formats_counts = ArenaPushArray(scratch, uint32_t, physical_devices_count);
	VkSurfaceFormatKHR ** surface_formats_set = ArenaPushArray(scratch, VkSurfaceFormatKHR *, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDevice const handle = physical_devices[i];
		uint32_t * surface_formats_count = surface_formats_counts + i;
		vkGetPhysicalDeviceSurfaceFormatsKHR(handle, fl_gfx.surface, surface_formats_count, NULL);
		surface_formats_set[i] = ArenaPushArray(scratch, VkSurfaceFormatKHR, *surface_formats_count);
		VkSurfaceFormatKHR * surface_formats = surface_formats_set[i];
		vkGetPhysicalDeviceSurfaceFormatsKHR(handle, fl_gfx.surface, surface_formats_count, surface_formats);
	}

	// -- collect physical device surface formats
	uint32_t * present_modes_counts = ArenaPushArray(scratch, uint32_t, physical_devices_count);
	VkPresentModeKHR ** present_modes_set = ArenaPushArray(scratch, VkPresentModeKHR *, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDevice const handle = physical_devices[i];
		uint32_t * present_modes_count = present_modes_counts + i;
		vkGetPhysicalDeviceSurfacePresentModesKHR(handle, fl_gfx.surface, present_modes_count, NULL);
		present_modes_set[i] = ArenaPushArray(scratch, VkPresentModeKHR, *present_modes_count);
		VkPresentModeKHR * present_modes = present_modes_set[i];
		vkGetPhysicalDeviceSurfacePresentModesKHR(handle, fl_gfx.surface, present_modes_count, present_modes);
	}

	// -- collect physical devices extensions
	uint32_t * physical_devices_extensions_counts = ArenaPushArray(scratch, uint32_t, physical_devices_count);
	VkExtensionProperties ** physical_devices_extensions = ArenaPushArray(scratch, VkExtensionProperties *, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDevice const handle = physical_devices[i];
		uint32_t * extensions_count = physical_devices_extensions_counts + i;
		vkEnumerateDeviceExtensionProperties(handle, NULL, extensions_count, NULL);
		physical_devices_extensions[i] = ArenaPushArray(scratch, VkExtensionProperties, *extensions_count);
		VkExtensionProperties * extensions = physical_devices_extensions[i];
		vkEnumerateDeviceExtensionProperties(handle, NULL, extensions_count, extensions);
	}

	// -- log physical devices
	fmt_print("[gfx] physical devices (%u / %u):\n", physical_devices_count, physical_devices_count);
	for (uint32_t device_i = 0; device_i < physical_devices_count; device_i++) {
		VkPhysicalDevice           const   handle       = physical_devices[device_i];
		VkPhysicalDeviceProperties const * properties   = physical_device_properties + device_i;
		VkSurfaceCapabilitiesKHR   const * capabilities = surface_capabilities + device_i;

		uint32_t           const   surface_formats_count = surface_formats_counts[device_i];
		VkSurfaceFormatKHR const * surface_formats       = surface_formats_set[device_i];

		uint32_t         const   present_modes_count = present_modes_counts[device_i];
		VkPresentModeKHR const * present_modes       = present_modes_set[device_i];

		uint32_t              const   extensions_count = physical_devices_extensions_counts[device_i];
		VkExtensionProperties const * extensions       = physical_devices_extensions[device_i];

		str8 const type_text = gfx_to_string_physical_device_type(properties->deviceType);
		uint32_t const v[4] = {
			VK_API_VERSION_VARIANT(properties->apiVersion),
			VK_API_VERSION_MAJOR(properties->apiVersion),
			VK_API_VERSION_MINOR(properties->apiVersion),
			VK_API_VERSION_PATCH(properties->apiVersion),
		};

		fmt_print("- %s\n", properties->deviceName);
		fmt_print("  handle 0x%p\n", (void *)handle);
		fmt_print("  device type: %.*s\n", (int)type_text.count, type_text.buffer);
		fmt_print("  vulkan:      [%u] v%u.%u.%u\n", v[0], v[1], v[2], v[3]);

		fmt_print("  - capabilities:\n");
		fmt_print("    array layers: %u\n",         capabilities->maxImageArrayLayers);
		fmt_print("    image count:  [%u .. %u]\n", capabilities->minImageCount, capabilities->maxImageCount);
		fmt_print("    - extent:\n");
		fmt_print("      current: %ux%u\n", capabilities->currentExtent.width,  capabilities->currentExtent.height);
		fmt_print("      minimum: %ux%u\n", capabilities->minImageExtent.width, capabilities->minImageExtent.height);
		fmt_print("      maximum: %ux%u\n", capabilities->maxImageExtent.width, capabilities->maxImageExtent.height);
		fmt_print("    - supported:\n");
		fmt_print("      transforms:      0b%08b\n", capabilities->supportedTransforms);
		fmt_print("      composite alpha: 0b%08b\n", capabilities->supportedCompositeAlpha);
		fmt_print("      usage flags:     0b%08b\n", capabilities->supportedUsageFlags);

		fmt_print("  surface formats (%u):\n", surface_formats_count);
		for (uint32_t i = 0; i < surface_formats_count; i++) {
			VkSurfaceFormatKHR const * it = surface_formats + i;
			str8 const surface_format_text = gfx_to_string_format(it->format);
			str8 const color_space_text = gfx_to_string_color_space(it->colorSpace);
			fmt_print("  - %-30.*s - %.*s\n",
				(int)surface_format_text.count, surface_format_text.buffer,
				(int)color_space_text.count, color_space_text.buffer
			);
		}

		fmt_print("  present modes (%u):\n", present_modes_count);
		for (uint32_t i = 0; i < present_modes_count; i++) {
			VkPresentModeKHR const it = present_modes[i];
			str8 const it_text = gfx_to_string_present_mode(it);
			fmt_print("  - %.*s\n", (int)it_text.count, it_text.buffer);
		}

		fmt_print("  extensions (%u):\n", extensions_count);
		for (uint32_t i = 0; i < extensions_count; i++) {
			VkExtensionProperties const * it = extensions + i; (void)it;
			// fmt_print("  - %s\n", it->extensionName);
		}
	}
	fmt_print("\n");

	// -- choose two queue families per single physical device
	//    first and foremost we are interested in graphics operations
	//    @todo probably later compute ones will come handy too
	//    secondly, we need another compatible with the surface
	//    and it's likely to be the same one
	uint32_t * queue_family_graphics_choices = ArenaPushArray(scratch, uint32_t, physical_devices_count); // @note that is `index + 1`
	uint32_t * queue_family_surface_choices = ArenaPushArray(scratch, uint32_t, physical_devices_count); // @note that is `index + 1`
	for (uint32_t device_i = 0; device_i < physical_devices_count; device_i++) {
		VkPhysicalDevice const physical_device = physical_devices[device_i];

		// -- collect various device properties
		uint32_t queue_family_properties_count;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, NULL);

		enum { QUEUE_FAMILY_PROPERTIES_LIMIT = 8 }; // @todo replace with an arena
		uint32_t queue_family_properties_enum = queue_family_properties_count < QUEUE_FAMILY_PROPERTIES_LIMIT
			? queue_family_properties_count
			: QUEUE_FAMILY_PROPERTIES_LIMIT;

		VkQueueFamilyProperties queue_family_properties[QUEUE_FAMILY_PROPERTIES_LIMIT];
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_enum, queue_family_properties);

		VkBool32 queue_family_support_surface[QUEUE_FAMILY_PROPERTIES_LIMIT];
		for (uint32_t queue_family_i = 0; queue_family_i < queue_family_properties_enum; queue_family_i++) {
			VkBool32 * it = queue_family_support_surface + queue_family_i;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_i, fl_gfx.surface, it);
		}

		// -- prefer a single universal queue but still try to come up with something otherwise
		for (uint32_t queue_family_i = 0; queue_family_i < queue_family_properties_enum; queue_family_i++) {
			VkQueueFamilyProperties const * it = queue_family_properties + queue_family_i;
			if ((it->queueFlags & VK_QUEUE_GRAPHICS_BIT) && queue_family_support_surface[queue_family_i]) {
				queue_family_graphics_choices[device_i] = queue_family_i + 1;
				queue_family_surface_choices[device_i] = queue_family_i + 1;
				break;
			}
		}
		for (uint32_t queue_family_i = 0; queue_family_i < queue_family_properties_enum && !queue_family_graphics_choices[device_i]; queue_family_i++) {
			VkQueueFamilyProperties const * it = queue_family_properties + queue_family_i;
			if (it->queueFlags & VK_QUEUE_GRAPHICS_BIT)
				queue_family_graphics_choices[device_i] = queue_family_i + 1;
		}
		for (uint32_t queue_family_i = 0; queue_family_i < queue_family_properties_enum && !queue_family_surface_choices[device_i]; queue_family_i++) {
			if (queue_family_support_surface[device_i])
				queue_family_surface_choices[device_i] = queue_family_i + 1;
		}
	}

	// -- choose a device to go with
	//    we are interested in an actual GPU
	uint32_t physical_device_choice = 0; // @note that is `index + 1`
	for (uint32_t i = 0; i < physical_devices_count && !physical_device_choice; i++) {
		VkPhysicalDeviceProperties const * it = physical_device_properties + i;
		if (it->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&& queue_family_graphics_choices[i] == queue_family_surface_choices[i]
			&& queue_family_graphics_choices[i]
			&& queue_family_surface_choices[i])
				physical_device_choice = i + 1;
	}
	for (uint32_t i = 0; i < physical_devices_count && !physical_device_choice; i++) {
		VkPhysicalDeviceProperties const * it = physical_device_properties + i;
		if (it->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&& queue_family_graphics_choices[i]
			&& queue_family_surface_choices[i])
				physical_device_choice = i + 1;
	}
	for (uint32_t i = 0; i < physical_devices_count && !physical_device_choice; i++) {
		VkPhysicalDeviceProperties const * it = physical_device_properties + i;
		if (it->deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
			&& queue_family_graphics_choices[i] == queue_family_surface_choices[i]
			&& queue_family_graphics_choices[i]
			&& queue_family_surface_choices[i])
				physical_device_choice = i + 1;
	}
	for (uint32_t i = 0; i < physical_devices_count && !physical_device_choice; i++) {
		VkPhysicalDeviceProperties const * it = physical_device_properties + i;
		if (it->deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
			&& queue_family_graphics_choices[i]
			&& queue_family_surface_choices[i])
				physical_device_choice = i + 1;
	}

	uint32_t const physical_device_index = physical_device_choice
		? physical_device_choice - 1
		: 0;

	fl_gfx.physical_device = physical_devices[physical_device_index];

	// -- prepare queue families
	uint32_t const queue_family_graphics_index = queue_family_graphics_choices[physical_device_index] - 1;
	uint32_t const queue_family_surface_index = queue_family_surface_choices[physical_device_index] - 1;

	uint32_t queues_count = 0;
	enum { QUEUES_LIMIT = 2 }; // @todo replace with an arena
	uint32_t queue_indices[QUEUES_LIMIT];
	queue_indices[queues_count++] = queue_family_graphics_index;
	if (queue_family_graphics_index != queue_family_surface_index)
		queue_indices[queues_count++] = queue_family_surface_index;

	VkDeviceQueueCreateInfo queue_infos[QUEUES_LIMIT];
	for (uint32_t i = 0; i < queues_count; i++)
		queue_infos[i] = (VkDeviceQueueCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queue_indices[i],
			// priorities
			.queueCount = 1,
			.pQueuePriorities = (float[]){ 1 },
		};

	// --prepare logcal device layers
	char const * const requested_device_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	uint32_t              const   chosen_device_extensions_count = physical_devices_extensions_counts[physical_device_index];
	VkExtensionProperties const * chosen_device_extensions       = physical_devices_extensions[physical_device_index];
	for (uint32_t i = 0; i < ArrayCount(requested_device_extensions); i++) {
		char const * requested = requested_device_extensions[i];
		for (uint32_t i = 0; i < chosen_device_extensions_count; i++) {
			char const * available = chosen_device_extensions[i].extensionName;
			if (str_equals(requested, available))
				goto device_extension_found;
		}
		AssertF(false, "device extension \"%s\" is not available\n", requested);
		device_extension_found:;
	}

	// -- create a logical device
	vkCreateDevice(
		fl_gfx.physical_device,
		&(VkDeviceCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			// queue families
			.queueCreateInfoCount = queues_count,
			.pQueueCreateInfos = queue_infos,
			// extensions
			.enabledExtensionCount = ArrayCount(requested_device_extensions),
			.ppEnabledExtensionNames = requested_device_extensions,
		},
		&fl_gfx.allocator,
		&fl_gfx.device
	);

	// -- retrieve the device queues
	vkGetDeviceQueue(fl_gfx.device, queue_family_graphics_index, 0, &fl_gfx.device_graphics_queue);
	vkGetDeviceQueue(fl_gfx.device, queue_family_surface_index,  0, &fl_gfx.device_surface_queue);

	// -- prepare swapchain surface capabilities
	VkSurfaceCapabilitiesKHR capabilities = surface_capabilities[physical_device_index];
	fl_gfx.swapchain_extent = capabilities.currentExtent;
	if (fl_gfx.swapchain_extent.width == UINT32_MAX && fl_gfx.swapchain_extent.height == UINT32_MAX)
		os_surface_get_size(&fl_gfx.swapchain_extent.width, &fl_gfx.swapchain_extent.height);

	// -- prepare swapchain surface format
	uint32_t           const   surface_formats_count = surface_formats_counts[physical_device_index];
	VkSurfaceFormatKHR const * surface_formats       = surface_formats_set[physical_device_index];
	VkSurfaceFormatKHR         surface_format        = {
		.format     = VK_FORMAT_MAX_ENUM,
		.colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR,
	};
	for (uint32_t i = 0; i < surface_formats_count && surface_format.format == VK_FORMAT_MAX_ENUM; i++)
		if (surface_formats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
			surface_format = surface_formats[i];
	for (uint32_t i = 0; i < surface_formats_count && surface_format.format == VK_FORMAT_MAX_ENUM; i++)
		if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
			surface_format = surface_formats[i];
	if (surface_format.format == VK_FORMAT_MAX_ENUM)
		surface_format = surface_formats[0];
	fl_gfx.surface_format = surface_format.format;

	// -- prepare swapchain present mode
	uint32_t         const   present_modes_count = present_modes_counts[physical_device_index];
	VkPresentModeKHR const * present_modes       = present_modes_set[physical_device_index];
	VkPresentModeKHR         present_mode        = VK_PRESENT_MODE_MAX_ENUM_KHR;
	for (uint32_t i = 0; i < present_modes_count && present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR; i++)
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			present_mode = present_modes[i];
	if (present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR)
		present_mode = VK_PRESENT_MODE_FIFO_KHR;

	// -- create a swap chain
	vkCreateSwapchainKHR(
		fl_gfx.device,
		&(VkSwapchainCreateInfoKHR){
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = fl_gfx.surface,
			// present mode
			.presentMode = present_mode,
			// surface capabilities
			.preTransform = capabilities.currentTransform,
			.minImageCount = min_u32(capabilities.minImageCount + 1, capabilities.maxImageCount),
			.imageExtent = fl_gfx.swapchain_extent,
			// surface format
			.imageFormat = surface_format.format,
			.imageColorSpace = surface_format.colorSpace,
			// queue families
			.imageSharingMode = queues_count >= 2
				? VK_SHARING_MODE_CONCURRENT
				: VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = queues_count,
			.pQueueFamilyIndices = queue_indices,
			// image params
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			// composition params
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.clipped = VK_TRUE,
		},
		&fl_gfx.allocator,
		&fl_gfx.swapchain
	);

	// -- retrieve the swapchain images
	vkGetSwapchainImagesKHR(fl_gfx.device, fl_gfx.swapchain, &fl_gfx.swapchain_images_count, NULL);
	fl_gfx.swapchain_images = os_memory_heap(NULL, sizeof(VkImage) * fl_gfx.swapchain_images_count);
	vkGetSwapchainImagesKHR(fl_gfx.device, fl_gfx.swapchain, &fl_gfx.swapchain_images_count, fl_gfx.swapchain_images);

	// -- create corresponding image views
	fl_gfx.swapchain_image_views = os_memory_heap(NULL, sizeof(VkImageView) * fl_gfx.swapchain_images_count);
	for (uint32_t i = 0; i < fl_gfx.swapchain_images_count; i++)
		vkCreateImageView(
			fl_gfx.device, &(VkImageViewCreateInfo){
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = fl_gfx.swapchain_images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = surface_format.format,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					// mip map
					.levelCount = 1,
					// layers
					.layerCount = 1,
				},
			},
			&fl_gfx.allocator,
			fl_gfx.swapchain_image_views + i
		);

	// -- create pipeline layout
	vkCreatePipelineLayout(
		fl_gfx.device,
		&(VkPipelineLayoutCreateInfo){
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		},
		&fl_gfx.allocator,
		&fl_gfx.pipeline_layout
	);

	// -- create render pass
	vkCreateRenderPass(
		fl_gfx.device,
		&(VkRenderPassCreateInfo){
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			// attachments
			.attachmentCount = 1,
			.pAttachments = &(VkAttachmentDescription){
				.format = surface_format.format,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				// layout
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				// color and depth operations
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				// stencil operations
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			},
			// subpasses
			.subpassCount = 1,
			.pSubpasses = &(VkSubpassDescription){
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &(VkAttachmentReference){
					.attachment = 0,
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				},
			},
			// dependencies
			.dependencyCount = 1,
			.pDependencies = &(VkSubpassDependency){
				// subpasses
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				// source stage masks
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				// destination stage masks
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			},
		},
		&fl_gfx.allocator,
		&fl_gfx.render_pass
	);

	// -- create graphics pipeline
	VkPipelineShaderStageCreateInfo const shader_stages[] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = gfx_create_shader_module("shader.vert.spirv"),
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = gfx_create_shader_module("shader.frag.spirv"),
			.pName = "main",
		},
	};

	AttrFuncLocal() VkDynamicState const dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	vkCreateGraphicsPipelines(
		fl_gfx.device,
		VK_NULL_HANDLE,
		1, &(VkGraphicsPipelineCreateInfo){
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.layout = fl_gfx.pipeline_layout,
			.renderPass = fl_gfx.render_pass,
			.subpass = 0,
			// stages
			.stageCount = ArrayCount(shader_stages),
			.pStages = shader_stages,
			// vertices
			.pVertexInputState = &(VkPipelineVertexInputStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			},
			// assembly
			.pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			},
			// viewport
			.pViewportState = &(VkPipelineViewportStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
				.viewportCount = 1,
				.scissorCount = 1,
			},
			// rasterizer
			.pRasterizationState = &(VkPipelineRasterizationStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_BACK_BIT,
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, // @note left-handed
				.lineWidth = 1,
			},
			// multisample
			.pMultisampleState = &(VkPipelineMultisampleStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			},
			// blend
			.pColorBlendState = &(VkPipelineColorBlendStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				.attachmentCount = 1,
				.pAttachments = &(VkPipelineColorBlendAttachmentState){
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
					/**/            | VK_COLOR_COMPONENT_G_BIT
					/**/            | VK_COLOR_COMPONENT_B_BIT
					/**/            | VK_COLOR_COMPONENT_A_BIT
					,
					// color
					.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
					.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
					.colorBlendOp = VK_BLEND_OP_ADD,
					// alpha
					.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
					.alphaBlendOp = VK_BLEND_OP_ADD,
				},
			},
			// dynamic
			.pDynamicState = &(VkPipelineDynamicStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = ArrayCount(dynamic_states),
				.pDynamicStates = dynamic_states,
			},
		},
		&fl_gfx.allocator,
		&fl_gfx.graphics_pipeline
	);

	for (uint32_t i = 0, count = ArrayCount(shader_stages); i < count; i++)
		vkDestroyShaderModule(fl_gfx.device, shader_stages[i].module, &fl_gfx.allocator);

	// --create framebuffers
	fl_gfx.framebuffers = os_memory_heap(NULL, sizeof(VkFramebuffer) * fl_gfx.swapchain_images_count);
	for (uint32_t i = 0; i < fl_gfx.swapchain_images_count; i++) {
		vkCreateFramebuffer(
			fl_gfx.device,
			&(VkFramebufferCreateInfo){
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = fl_gfx.render_pass,
				.width = fl_gfx.swapchain_extent.width,
				.height = fl_gfx.swapchain_extent.height,
				.layers = 1,
				// attachments
				.attachmentCount = 1,
				.pAttachments = fl_gfx.swapchain_image_views + i,
			},
			&fl_gfx.allocator,
			fl_gfx.framebuffers + i
		);
	}

	// -- create command pool with buffers
	vkCreateCommandPool(
		fl_gfx.device,
		&(VkCommandPoolCreateInfo){
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queue_family_graphics_index,
		},
		&fl_gfx.allocator,
		&fl_gfx.command_pool
	);

	vkAllocateCommandBuffers(
		fl_gfx.device,
		&(VkCommandBufferAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = fl_gfx.command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		},
		&fl_gfx.command_buffer
	);

	// -- create synchronization
	vkCreateSemaphore(
		fl_gfx.device,
		&(VkSemaphoreCreateInfo){
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		},
		&fl_gfx.allocator,
		&fl_gfx.image_available_semaphore
	);
	vkCreateSemaphore(
		fl_gfx.device,
		&(VkSemaphoreCreateInfo){
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		},
		&fl_gfx.allocator,
		&fl_gfx.render_finished_semaphore
	);
	vkCreateFence(
		fl_gfx.device,
		&(VkFenceCreateInfo){
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		},
		&fl_gfx.allocator,
		&fl_gfx.in_flight_fence
	);

	arena_set_position(scratch, scratch_position);
}

void gfx_free(void) {
	// -- wait for device to idle
	vkDeviceWaitIdle(fl_gfx.device);

	// -- destroy syncronzation
	vkDestroySemaphore(fl_gfx.device, fl_gfx.image_available_semaphore, &fl_gfx.allocator);
	vkDestroySemaphore(fl_gfx.device, fl_gfx.render_finished_semaphore, &fl_gfx.allocator);
	vkDestroyFence(fl_gfx.device, fl_gfx.in_flight_fence, &fl_gfx.allocator);

	// -- destroy command pool
	vkDestroyCommandPool(fl_gfx.device, fl_gfx.command_pool, &fl_gfx.allocator);

	// -- destroy framebuffers
	for (uint32_t i = 0; i < fl_gfx.swapchain_images_count; i++)
		vkDestroyFramebuffer(fl_gfx.device, fl_gfx.framebuffers[i], &fl_gfx.allocator);
	os_memory_heap(fl_gfx.framebuffers, 0);

	// -- destroy pipeline
	vkDestroyPipeline(fl_gfx.device, fl_gfx.graphics_pipeline, &fl_gfx.allocator);

	// -- destroy render pass
	vkDestroyRenderPass(fl_gfx.device, fl_gfx.render_pass, &fl_gfx.allocator);

	// -- destroy pipeline layout
	vkDestroyPipelineLayout(fl_gfx.device, fl_gfx.pipeline_layout, &fl_gfx.allocator);

	// -- destroy the swapchain images array
	for (uint32_t i = 0; i < fl_gfx.swapchain_images_count; i++)
		vkDestroyImageView(fl_gfx.device, fl_gfx.swapchain_image_views[i], &fl_gfx.allocator);
	os_memory_heap(fl_gfx.swapchain_image_views, 0);

	// -- destroy the swapchain images array
	os_memory_heap(fl_gfx.swapchain_images, 0);

	// -- destroy the swapchain
	vkDestroySwapchainKHR(fl_gfx.device, fl_gfx.swapchain, &fl_gfx.allocator);

	// -- destroy the logical device
	vkDestroyDevice(fl_gfx.device, &fl_gfx.allocator);

	// -- deinit the debug messenger
	#if GFX_ENABLE_DEBUG
	GFX_DEFINE_PROC(vkDestroyDebugUtilsMessengerEXT);
	vkDestroyDebugUtilsMessengerEXT(
		fl_gfx.instance,
		fl_gfx.debug_utils_messenger,
		&fl_gfx.allocator
	);
	#endif

	// -- destroy the surface
	vkDestroySurfaceKHR(fl_gfx.instance, fl_gfx.surface, &fl_gfx.allocator);

	// destroy the instance
	vkDestroyInstance(fl_gfx.instance, &fl_gfx.allocator);

	// -- zero the memory
	mem_zero(&fl_gfx, sizeof(fl_gfx));
}

void gfx_tick(void) {
	// -- wait for the previous frame
	vkWaitForFences(fl_gfx.device, 1, &fl_gfx.in_flight_fence, VK_TRUE, UINT64_MAX);

	// -- prepare for the next frame
	uint32_t image_index;
	VkResult const aquire_next_image_result =
	vkAcquireNextImageKHR(
		fl_gfx.device, fl_gfx.swapchain,
		UINT64_MAX, fl_gfx.image_available_semaphore, VK_NULL_HANDLE, &image_index
	);
	switch (aquire_next_image_result) {
		// success, continue rendering
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			break;

		// failure, cancel rendering
		case VK_ERROR_OUT_OF_DATE_KHR:
		AttrFallthrough();
		default: return;
	}

	vkResetFences(fl_gfx.device, 1, &fl_gfx.in_flight_fence);
	vkResetCommandBuffer(fl_gfx.command_buffer, 0);

	// -- draw
	vkBeginCommandBuffer(fl_gfx.command_buffer, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	});

	vkCmdBeginRenderPass(fl_gfx.command_buffer, &(VkRenderPassBeginInfo){
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = fl_gfx.render_pass,
		.framebuffer = fl_gfx.framebuffers[image_index],
		.renderArea = {
			.extent = fl_gfx.swapchain_extent,
		},
		// clear
		.clearValueCount = 1,
		.pClearValues = &(VkClearValue){
			.color.float32 = {0, 0, 0, 1},
		},
	}, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(fl_gfx.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fl_gfx.graphics_pipeline);
	vkCmdSetViewport(fl_gfx.command_buffer, 0, 1, &(VkViewport){
		// offset, Y positive up
		.x = (float)0,
		.y = (float)fl_gfx.swapchain_extent.height,
		// scale, Y positive up
		.width = (float)fl_gfx.swapchain_extent.width,
		.height = -(float)fl_gfx.swapchain_extent.height,
		// depth
		.minDepth = 0,
		.maxDepth = 1,
	});
	vkCmdSetScissor(fl_gfx.command_buffer, 0, 1, &(VkRect2D){
		.extent = fl_gfx.swapchain_extent,
	});
	vkCmdDraw(fl_gfx.command_buffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(fl_gfx.command_buffer);
	vkEndCommandBuffer(fl_gfx.command_buffer);

	// -- submit
	VkSemaphore const submit_wait_semaphores[] = {
		fl_gfx.image_available_semaphore,
	};
	VkSemaphore const submit_signal_semaphores[] = {
		fl_gfx.render_finished_semaphore,
	};
	vkQueueSubmit(fl_gfx.device_graphics_queue, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		// wait semaphores
		.waitSemaphoreCount = ArrayCount(submit_wait_semaphores),
		.pWaitSemaphores = submit_wait_semaphores,
		.pWaitDstStageMask = (VkPipelineStageFlags[]){
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		},
		// command buffers
		.commandBufferCount = 1,
		.pCommandBuffers = &fl_gfx.command_buffer,
		// signal semaphores
		.signalSemaphoreCount = ArrayCount(submit_signal_semaphores),
		.pSignalSemaphores = submit_signal_semaphores,
	}, fl_gfx.in_flight_fence);

	// -- present
	VkResult const queue_present_result =
	vkQueuePresentKHR(fl_gfx.device_surface_queue, &(VkPresentInfoKHR){
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		// wait semaphores
		.waitSemaphoreCount = ArrayCount(submit_signal_semaphores),
		.pWaitSemaphores = submit_signal_semaphores,
		// swap chains
		.swapchainCount = 1,
		.pSwapchains = &fl_gfx.swapchain,
		.pImageIndices = &image_index,
	});
	switch (queue_present_result) {
		case VK_SUBOPTIMAL_KHR:        // success
		case VK_ERROR_OUT_OF_DATE_KHR: // failure
		AttrFallthrough();
		default: break;
	}
}

#undef GFX_DEFINE_PROC
#undef GFX_ENABLE_DEBUG
