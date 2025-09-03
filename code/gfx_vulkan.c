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

#define GFX_FRAMES_IN_FLIGHT 2
#define GFX_DEFINE_PROC(name) PFN_ ## name name = (PFN_ ## name)gfx_get_instance_proc(#name)
#define GFX_ENABLE_DEBUG (BUILD_DEBUG == BUILD_DEBUG_ENABLE)

// @note vulkan expects a 64 bit architecture at least
AssertStatic(sizeof(size_t) >= sizeof(uint64_t));
AssertStatic(GFX_FRAMES_IN_FLIGHT >= 1);

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

AttrFileLocal()
str8 gfx_to_string_queue_flags(struct Arena * arena, VkQueueFlags flags) {
	AttrFuncLocal() str8 const separator = str8_lit(", ");
	AttrFuncLocal() VkQueueFlagBits const bits[] = {
		VK_QUEUE_GRAPHICS_BIT,
		VK_QUEUE_COMPUTE_BIT,
		VK_QUEUE_TRANSFER_BIT,
		VK_QUEUE_SPARSE_BINDING_BIT,
		VK_QUEUE_PROTECTED_BIT,
		VK_QUEUE_VIDEO_DECODE_BIT_KHR,
		VK_QUEUE_VIDEO_ENCODE_BIT_KHR,
		VK_QUEUE_OPTICAL_FLOW_BIT_NV,
	};

	AttrFuncLocal() size_t const capacity = ArrayCount(bits) * 16;
	str8 ret = {.buffer = ArenaPushArray(arena, u8, capacity)};
	for (uint32_t i = 0; i < ArrayCount(bits); i++) {
		if (!(flags & (VkQueueFlags)bits[i])) continue;
		if (ret.count > 0) str8_append(&ret, separator);
		str8_append(&ret, gfx_to_string_queue(bits[i]));
	}
	return ret;
}

// ---- ---- ---- ----
// utilities
// ---- ---- ---- ----

AttrFileLocal()
bool gfx_match_sets(
	uint32_t requested_count, char const * const * requested_set,
	uint32_t available_count, char const * const * avaliable_set) {
	bool available = true;
	for (uint32_t i = 0; i < requested_count; i++) {
		char const * requested = requested_set[i];
		for (uint32_t i = 0; i < available_count; i++) {
			char const * available = avaliable_set[i];
			if (str_equals(requested, available))
				goto found;
		}
		available = false;
		DbgPrintF("requested value \"%s\" is not available\n", requested);
		found:;
	}
	return available;
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
	// instance
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_utils_messenger;
	// surface
	VkSurfaceKHR surface;
	// device
	struct GFX_Device_Logical {
		struct GFX_Device_Physical {
			VkPhysicalDevice handle;
			uint32_t main_qfamily_index;
			uint32_t present_qfamily_index;
			uint32_t transfer_qfamily_index;
			VkSurfaceFormatKHR surface_format;
			VkPresentModeKHR present_mode;
		} physical;
		VkDevice handle;
		VkQueue main_queue;
		VkQueue present_queue;
		VkQueue transfer_queue;
	} device;
	// synchronization
	VkSemaphore image_available_semaphores[GFX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphores[GFX_FRAMES_IN_FLIGHT];
	VkFence     in_flight_fences[GFX_FRAMES_IN_FLIGHT];
	// main command pool
	VkCommandPool   main_command_pool;
	VkCommandBuffer main_command_buffers[GFX_FRAMES_IN_FLIGHT];
	// transfer command pool
	VkCommandPool   transfer_command_pool;
	VkCommandBuffer transfer_command_buffer;
	// render pass and swapchain
	VkRenderPass render_pass;
	struct GFX_Swapchain {
		uint32_t         frame;
		VkExtent2D       extent;
		VkSwapchainKHR   handle;
		uint32_t         images_count;
		VkImage        * images;
		VkImageView    * image_views;
		VkFramebuffer  * framebuffers;
		bool             out_of_date_or_suboptimal;
	} swapchain;
	// pipeline / USER DATA
	struct GFX_Pipeline {
		VkPipelineLayout layout;
		VkPipeline       handle;
	} pipeline;
	// vertices / USER DATA
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
} fl_gfx;

AttrFileLocal()
VkAllocationCallbacks const fl_gfx_allocator = (VkAllocationCallbacks){
	.pUserData = &fl_gfx,
	.pfnAllocation   = gfx_memory_allocate,
	.pfnReallocation = gfx_memory_reallocate,
	.pfnFree         = gfx_memory_free,
	#if GFX_ENABLE_DEBUG
	.pfnInternalAllocation = gfx_debug_on_internal_allocation,
	.pfnInternalFree       = gfx_debug_on_internal_free,
	#endif
};

AttrFileLocal()
void * gfx_get_instance_proc(char const * name) {
	void * ret = (void *)vkGetInstanceProcAddr(fl_gfx.instance, name);
	AssertF(ret, "[gfx] `vkGetInstanceProcAddr(0x%p, \"%s\")` failed\n", (void *)fl_gfx.instance, name);
	return ret;
}

// ---- ---- ---- ----
// instance and surface
// ---- ---- ---- ----

AttrFileLocal()
void gfx_instance_init(void) {
	struct Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = arena_get_position(scratch);

	// ---- ---- ---- ----
	// version
	// ---- ---- ---- ----

	uint32_t api_version;
	vkEnumerateInstanceVersion(&api_version);

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

	// ---- ---- ---- ----
	// extensions
	// ---- ---- ---- ----

	uint32_t available_extensions_count;
	vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, NULL);
	VkExtensionProperties * available_extension_properties = ArenaPushArray(scratch, VkExtensionProperties, available_extensions_count);
	vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, available_extension_properties);

	fmt_print("[gfx] available instance extensions (%u):\n", available_extensions_count);
	for (uint32_t i = 0; i < available_extensions_count; i++) {
		VkExtensionProperties const it = available_extension_properties[i];
		fmt_print("- %s\n", it.extensionName);
	}
	fmt_print("\n");

	// ---- ---- ---- ----
	// layers
	// ---- ---- ---- ----

	uint32_t available_layers_count;
	vkEnumerateInstanceLayerProperties(&available_layers_count, NULL);
	VkLayerProperties * available_layer_properties = ArenaPushArray(scratch, VkLayerProperties, available_layers_count);
	vkEnumerateInstanceLayerProperties(&available_layers_count, available_layer_properties);

	fmt_print("[gfx] available instance layers (%u):\n", available_layers_count);
	for (uint32_t i = 0; i < available_layers_count; i++) {
		VkLayerProperties const it = available_layer_properties[i];

		uint32_t const v[4] = {
			VK_API_VERSION_VARIANT(it.specVersion),
			VK_API_VERSION_MAJOR(it.specVersion),
			VK_API_VERSION_MINOR(it.specVersion),
			VK_API_VERSION_PATCH(it.specVersion),
		};

		fmt_print("- %-36s - [%u] v%u.%u.%-3u - %s\n", it.layerName, v[0], v[1], v[2], v[3], it.description);
	}
	fmt_print("\n");

	// ---- ---- ---- ----
	// form requirements
	// ---- ---- ---- ----

	uint32_t required_instance_extensions_count = 0;
	char const ** const required_instance_extensions = ArenaPushArray(scratch, char const *, available_extensions_count);
	os_vulkan_push_extensions(&required_instance_extensions_count, required_instance_extensions);
	required_instance_extensions[required_instance_extensions_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
	#if GFX_ENABLE_DEBUG
	required_instance_extensions[required_instance_extensions_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	#endif

	char const * const required_instance_layers[] = {
		#if GFX_ENABLE_DEBUG
		"VK_LAYER_KHRONOS_validation",
		#endif
	};

	// ---- ---- ---- ----
	// verify requirements
	// ---- ---- ---- ----

	char const ** available_extensions = ArenaPushArray(scratch, char const *, available_extensions_count);
	for (uint32_t i = 0; i < available_extensions_count; i++)
		available_extensions[i] = available_extension_properties[i].extensionName;
	bool const extensions_match = gfx_match_sets(
		required_instance_extensions_count, required_instance_extensions,
		available_extensions_count, available_extensions
	);
	Assert(extensions_match, "can't satisfy requested extensions set\n\n");

	char const ** available_layers = ArenaPushArray(scratch, char const *, available_layers_count);
	for (uint32_t i = 0; i < available_layers_count; i++)
		available_layers[i] = available_layer_properties[i].layerName;
	bool const layers_match = gfx_match_sets(
		ArrayCount(required_instance_layers), required_instance_layers,
		available_layers_count, available_layers
	);
	Assert(layers_match, "can't satisfy requested layers set\n\n");

	// ---- ---- ---- ----
	// common debug info
	// ---- ---- ---- ----

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

	// ---- ---- ---- ----
	// instance
	// ---- ---- ---- ----

	vkCreateInstance(
		&(VkInstanceCreateInfo){
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			// extensions
			.enabledExtensionCount = required_instance_extensions_count,
			.ppEnabledExtensionNames = required_instance_extensions,
			// layers
			.enabledLayerCount = ArrayCount(required_instance_layers),
			.ppEnabledLayerNames = required_instance_layers,
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
		&fl_gfx_allocator,
		&fl_gfx.instance
	);

	// ---- ---- ---- ----
	// debug
	// ---- ---- ---- ----

	#if GFX_ENABLE_DEBUG
	GFX_DEFINE_PROC(vkCreateDebugUtilsMessengerEXT);
	vkCreateDebugUtilsMessengerEXT(
		fl_gfx.instance,
		debug_utils_messenger_create_info,
		&fl_gfx_allocator,
		&fl_gfx.debug_utils_messenger
	);
	#endif

	arena_set_position(scratch, scratch_position);
}

AttrFileLocal()
void gfx_instance_free(void) {
	#if GFX_ENABLE_DEBUG
	GFX_DEFINE_PROC(vkDestroyDebugUtilsMessengerEXT);
	vkDestroyDebugUtilsMessengerEXT(
		fl_gfx.instance,
		fl_gfx.debug_utils_messenger,
		&fl_gfx_allocator
	);
	#endif

	vkDestroyInstance(fl_gfx.instance, &fl_gfx_allocator);
}

// ---- ---- ---- ----
// device
// ---- ---- ---- ----

AttrFileLocal()
void gfx_device_init(void) {
	struct Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = arena_get_position(scratch);

	// ---- ---- ---- ----
	// collect all devices
	// ---- ---- ---- ----

	uint32_t all_physical_devices_count;
	vkEnumeratePhysicalDevices(fl_gfx.instance, &all_physical_devices_count, NULL);
	Assert(all_physical_devices_count > 0, "[gfx] physical devices missing\n");

	VkPhysicalDevice * all_physical_devices = ArenaPushArray(scratch, VkPhysicalDevice, all_physical_devices_count);
	vkEnumeratePhysicalDevices(fl_gfx.instance, &all_physical_devices_count, all_physical_devices);

	// ---- ---- ---- ----
	// filter out devices
	// ---- ---- ---- ----

	AttrFuncLocal()
	char const * const required_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	uint32_t physical_devices_count = 0;
	VkPhysicalDevice * physical_devices = ArenaPushArray(scratch, VkPhysicalDevice, physical_devices_count);
	for (uint32_t i = 0; i < all_physical_devices_count; i++) {
		VkPhysicalDevice const handle = all_physical_devices[i];

		uint32_t extensions_count;
		vkEnumerateDeviceExtensionProperties(handle, NULL, &extensions_count, NULL);
		VkExtensionProperties * extensions = ArenaPushArray(scratch, VkExtensionProperties, extensions_count);
		vkEnumerateDeviceExtensionProperties(handle, NULL, &extensions_count, extensions);

		char const ** extension_strings = ArenaPushArray(scratch, char const *, extensions_count);
		for (uint32_t i = 0; i < extensions_count; i++)
			extension_strings[i] = extensions[i].extensionName;

		bool const match = gfx_match_sets(
			ArrayCount(required_extensions), required_extensions,
			extensions_count, extension_strings
		);

		if (match)
			physical_devices[physical_devices_count++] = handle;
		else
			DbgPrintF("can't satisfy requested extensions set for 0x%p\n", (void *)handle);
	}
	Assert(physical_devices_count > 0, "[gfx] physical devices missing\n");

	// ---- ---- ---- ----
	// properties
	// ---- ---- ---- ----

	VkPhysicalDeviceProperties * physical_device_properties = ArenaPushArray(scratch, VkPhysicalDeviceProperties, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDeviceProperties * properties = &physical_device_properties[i];
		vkGetPhysicalDeviceProperties(physical_devices[i], properties);
	}

	uint32_t * qfamilies_counts = ArenaPushArray(scratch, uint32_t, physical_devices_count);
	VkQueueFamilyProperties ** qfamily_properties_set = ArenaPushArray(scratch, VkQueueFamilyProperties *, physical_devices_count);
	VkBool32 ** qfamilies_surface_support_set = ArenaPushArray(scratch, VkBool32 *, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDevice const handle = physical_devices[i];

		uint32_t qfamilies_count;
		vkGetPhysicalDeviceQueueFamilyProperties(handle, &qfamilies_count, NULL);
		qfamilies_counts[i] = qfamilies_count;
		qfamily_properties_set[i] = ArenaPushArray(scratch, VkQueueFamilyProperties, qfamilies_count);
		vkGetPhysicalDeviceQueueFamilyProperties(handle, &qfamilies_count, qfamily_properties_set[i]);

		qfamilies_surface_support_set[i] = ArenaPushArray(scratch, VkBool32, qfamilies_count);
		VkBool32 * qfamilies_surface_support = qfamilies_surface_support_set[i];
		for (uint32_t i = 0; i < qfamilies_count; i++) {
			VkBool32 * qfamily_surface_support = &qfamilies_surface_support[i];
			vkGetPhysicalDeviceSurfaceSupportKHR(handle, i, fl_gfx.surface, qfamily_surface_support);
		}
	}

	fmt_print("[gfx] physical devices (%u):\n", physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDevice const handle = physical_devices[i];

		VkPhysicalDeviceProperties const properties = physical_device_properties[i];
		uint32_t qfamilies_count = qfamilies_counts[i];
		VkQueueFamilyProperties * qfamilies_properties = qfamily_properties_set[i];

		str8 const type_text = gfx_to_string_physical_device_type(properties.deviceType);
		uint32_t const v[4] = {
			VK_API_VERSION_VARIANT(properties.apiVersion),
			VK_API_VERSION_MAJOR(properties.apiVersion),
			VK_API_VERSION_MINOR(properties.apiVersion),
			VK_API_VERSION_PATCH(properties.apiVersion),
		};

		fmt_print("- name:        \"%s\"\n", properties.deviceName);
		fmt_print("  handle:      0x%p\n", (void *)handle);
		fmt_print("  device type: %.*s\n", (int)type_text.count, type_text.buffer);
		fmt_print("  vulkan:      [%u] v%u.%u.%u\n", v[0], v[1], v[2], v[3]);
		fmt_print("  - qfamilies (%u):\n", qfamilies_count);
		for (uint32_t i = 0; i < qfamilies_count; i++) {
			struct Arena * scratch = thread_ctx_get_scratch();
			u64 const scratch_position = arena_get_position(scratch);
			VkQueueFamilyProperties const it = qfamilies_properties[i];
			str8 const flags_text = gfx_to_string_queue_flags(scratch, it.queueFlags);
			fmt_print("    [%u] %.*s\n", i, (int)flags_text.count, flags_text.buffer);
			arena_set_position(scratch, scratch_position);
		}
	}
	fmt_print("\n");

	// -- choose two queue families per single physical device
	//    first and foremost we are interested in graphics operations
	//    @todo probably later compute ones will come handy too
	//    secondly, we need another compatible with the surface
	//    and it's likely to be the same one
	uint32_t * main_qfamily_choices = ArenaPushArray(scratch, uint32_t, physical_devices_count); // @note that is `index + 1`
	uint32_t * present_qfamily_choices = ArenaPushArray(scratch, uint32_t, physical_devices_count); // @note that is `index + 1`
	uint32_t * transfer_qfamily_choices = ArenaPushArray(scratch, uint32_t, physical_devices_count); // @note that is `index + 1`
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDevice const handle = physical_devices[i];

		uint32_t const count = qfamilies_counts[i];
		VkQueueFamilyProperties const * properties = qfamily_properties_set[i];
		VkBool32 const * surface_supports = qfamilies_surface_support_set[i];

		uint32_t * main_qfamily_choice     = &main_qfamily_choices[i];
		uint32_t * present_qfamily_choice  = &present_qfamily_choices[i];
		uint32_t * transfer_qfamily_choice = &transfer_qfamily_choices[i];

		*main_qfamily_choice = 0;
		*present_qfamily_choice = 0;
		*transfer_qfamily_choice = 0;

		// -- prefer a main queue that can present
		for (uint32_t i = 0; i < count; i++) {
			VkQueueFamilyProperties const it = properties[i];
			if (!(it.queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;

			VkBool32 const surface_support = surface_supports[i];
			if (!surface_support) continue;

			*main_qfamily_choice = i + 1;
			*present_qfamily_choice = i + 1;
			break;
		}

		// -- prefer a free-standing transfer queue
		for (uint32_t i = 0; i < count; i++) {
			VkQueueFamilyProperties const it = properties[i];
			if (!(it.queueFlags & VK_QUEUE_TRANSFER_BIT)) continue;
			if (it.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) continue;

			*transfer_qfamily_choice = i + 1;
			break;
		}

		// -- fallback to first matching
		for (uint32_t i = 0; i < count && !*main_qfamily_choice; i++) {
			VkQueueFamilyProperties const it = properties[i];
			if (it.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				*main_qfamily_choice = i + 1;
		}

		for (uint32_t i = 0; i < count && !*present_qfamily_choice; i++) {
			VkBool32 const surface_support = surface_supports[i];
			if (surface_support)
				*present_qfamily_choice = i + 1;
		}

		for (uint32_t i = 0; i < count && !*transfer_qfamily_choice; i++) {
			VkQueueFamilyProperties const it = properties[i];
			if (it.queueFlags & VK_QUEUE_TRANSFER_BIT)
				*transfer_qfamily_choice = i + 1;
		}
	}

	// @note it's possible to forcibly prefer, say, a discrete GPU, but better to default
	// with a first suitable one, giving a chance to the user to sort priorities manually
	// namely with the "NVIDIA Control Panel" or the "AMD Software: Adrenalin Edition"
	uint32_t physical_device_choice = 0; // @note that is `index + 1`
	for (uint32_t i = 0; i < physical_devices_count && !physical_device_choice; i++) {
		VkPhysicalDeviceProperties const it = physical_device_properties[i];
		if (transfer_qfamily_choices[i] && main_qfamily_choices[i] && present_qfamily_choices[i])
			physical_device_choice = i + 1;
	}

	Assert(physical_device_choice > 0, "[gfx] no suitable physical device found\n");
	uint32_t const physical_device_index = physical_device_choice
		? physical_device_choice - 1
		: 0;

	fl_gfx.device.physical.handle = physical_devices[physical_device_index];
	fl_gfx.device.physical.main_qfamily_index     = main_qfamily_choices[physical_device_index] - 1;
	fl_gfx.device.physical.present_qfamily_index  = present_qfamily_choices[physical_device_index] - 1;
	fl_gfx.device.physical.transfer_qfamily_index = transfer_qfamily_choices[physical_device_index] - 1;

	// ---- ---- ---- ----
	// surface format
	// ---- ---- ---- ----

	fl_gfx.device.physical.surface_format = (VkSurfaceFormatKHR){
		.format     = VK_FORMAT_MAX_ENUM,
		.colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR,
	};

	VkFormat const surface_format_preferences[] = {
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_B8G8R8A8_UNORM,
	};

	uint32_t surface_formats_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(fl_gfx.device.physical.handle, fl_gfx.surface, &surface_formats_count, NULL);
	VkSurfaceFormatKHR * surface_formats = ArenaPushArray(scratch, VkSurfaceFormatKHR, surface_formats_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(fl_gfx.device.physical.handle, fl_gfx.surface, &surface_formats_count, surface_formats);
	for (uint32_t i = 0; i < ArrayCount(surface_format_preferences) && fl_gfx.device.physical.surface_format.format == VK_FORMAT_MAX_ENUM; i++) {
		VkFormat const preference = surface_format_preferences[i];
		for (uint32_t i = 0; i < surface_formats_count && fl_gfx.device.physical.surface_format.format == VK_FORMAT_MAX_ENUM; i++)
			if (surface_formats[i].format == preference)
				fl_gfx.device.physical.surface_format = surface_formats[i];
	}
	if (fl_gfx.device.physical.surface_format.format == VK_FORMAT_MAX_ENUM)
		fl_gfx.device.physical.surface_format = surface_formats[0];

	// ---- ---- ---- ----
	// present mode
	// ---- ---- ---- ----

	fl_gfx.device.physical.present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;

	VkPresentModeKHR const present_mode_preferences[] = {
		VK_PRESENT_MODE_MAILBOX_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
	};

	uint32_t present_modes_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(fl_gfx.device.physical.handle, fl_gfx.surface, &present_modes_count, NULL);
	VkPresentModeKHR * present_modes = ArenaPushArray(scratch, VkPresentModeKHR, present_modes_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(fl_gfx.device.physical.handle, fl_gfx.surface, &present_modes_count, present_modes);
	for (uint32_t i = 0; i < ArrayCount(present_mode_preferences) && fl_gfx.device.physical.present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR; i++) {
		VkPresentModeKHR const preference = present_mode_preferences[i];
		for (uint32_t i = 0; i < present_modes_count && fl_gfx.device.physical.present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR; i++)
			if (present_modes[i] == preference)
				fl_gfx.device.physical.present_mode = present_modes[i];
	}
	if (fl_gfx.device.physical.present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
		// @note This is the only value of presentMode that is required to be supported.
		fl_gfx.device.physical.present_mode = VK_PRESENT_MODE_FIFO_KHR;
	}

	// ---- ---- ---- ----
	// create logical device
	// ---- ---- ---- ----

	VkPhysicalDeviceProperties const properties = physical_device_properties[physical_device_index];
	str8 const surface_format_text = gfx_to_string_format(fl_gfx.device.physical.surface_format.format);
	str8 const color_space_text = gfx_to_string_color_space(fl_gfx.device.physical.surface_format.colorSpace);
	str8 const present_mode_text = gfx_to_string_present_mode(fl_gfx.device.physical.present_mode);
	fmt_print("[gfx] chosen device:\n");
	fmt_print("- name:           \"%s\"\n", properties.deviceName);
	fmt_print("- handle:         0x%p\n", (void *)fl_gfx.device.physical.handle);
	fmt_print("- surface format: %.*s\n", (int)surface_format_text.count, surface_format_text.buffer);
	fmt_print("- color space:    %.*s\n", (int)color_space_text.count, color_space_text.buffer);
	fmt_print("- present mode:   %.*s\n", (int)present_mode_text.count, present_mode_text.buffer);
	fmt_print("\n");

	uint32_t queue_families_buffer[4];
	arr32 queue_families = {
		.capacity = ArrayCount(queue_families_buffer),
		.buffer = queue_families_buffer,
	};
	arr32_append_unique(&queue_families, fl_gfx.device.physical.main_qfamily_index);
	arr32_append_unique(&queue_families, fl_gfx.device.physical.present_qfamily_index);
	arr32_append_unique(&queue_families, fl_gfx.device.physical.transfer_qfamily_index);

	VkDeviceQueueCreateInfo queue_infos[ArrayCount(queue_families_buffer)];
	for (uint32_t i = 0; i < queue_families.count; i++)
		queue_infos[i] = (VkDeviceQueueCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queue_families.buffer[i],
			// priorities
			.queueCount = 1,
			.pQueuePriorities = (float[]){ 1 },
		};

	vkCreateDevice(
		fl_gfx.device.physical.handle,
		&(VkDeviceCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			// queue families
			.queueCreateInfoCount = (uint32_t)queue_families.count,
			.pQueueCreateInfos = queue_infos,
			// extensions
			.enabledExtensionCount = ArrayCount(required_extensions),
			.ppEnabledExtensionNames = required_extensions,
		},
		&fl_gfx_allocator,
		&fl_gfx.device.handle
	);

	vkGetDeviceQueue(fl_gfx.device.handle, fl_gfx.device.physical.main_qfamily_index,     0, &fl_gfx.device.main_queue);
	vkGetDeviceQueue(fl_gfx.device.handle, fl_gfx.device.physical.present_qfamily_index,  0, &fl_gfx.device.present_queue);
	vkGetDeviceQueue(fl_gfx.device.handle, fl_gfx.device.physical.transfer_qfamily_index, 0, &fl_gfx.device.transfer_queue);

	arena_set_position(scratch, scratch_position);
}

AttrFileLocal()
void gfx_device_free(void) {
	vkDestroyDevice(fl_gfx.device.handle, &fl_gfx_allocator);
}

// ---- ---- ---- ----
// synchronization
// ---- ---- ---- ----

AttrFileLocal()
void gfx_synchronization_init(void) {
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		vkCreateSemaphore(
			fl_gfx.device.handle,
			&(VkSemaphoreCreateInfo){
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			},
			&fl_gfx_allocator,
			&fl_gfx.image_available_semaphores[i]
		);
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		vkCreateSemaphore(
			fl_gfx.device.handle,
			&(VkSemaphoreCreateInfo){
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			},
			&fl_gfx_allocator,
			&fl_gfx.render_finished_semaphores[i]
		);
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		vkCreateFence(
			fl_gfx.device.handle,
			&(VkFenceCreateInfo){
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT,
			},
			&fl_gfx_allocator,
			&fl_gfx.in_flight_fences[i]
		);
}

AttrFileLocal()
void gfx_synchronization_free(void) {
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		vkDestroySemaphore(fl_gfx.device.handle, fl_gfx.image_available_semaphores[i], &fl_gfx_allocator);
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		vkDestroySemaphore(fl_gfx.device.handle, fl_gfx.render_finished_semaphores[i], &fl_gfx_allocator);
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		vkDestroyFence(fl_gfx.device.handle, fl_gfx.in_flight_fences[i], &fl_gfx_allocator);
}

// ---- ---- ---- ----
// command pools
// ---- ---- ---- ----

AttrFileLocal()
void gfx_command_pool_create(
	uint32_t qfamily_index, uint32_t buffers_count,
	VkCommandPool * out_pool, VkCommandBuffer * out_buffers
) {
	vkCreateCommandPool(
		fl_gfx.device.handle,
		&(VkCommandPoolCreateInfo){
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = qfamily_index,
		},
		&fl_gfx_allocator,
		out_pool
	);

	vkAllocateCommandBuffers(
		fl_gfx.device.handle,
		&(VkCommandBufferAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = *out_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = buffers_count,
		},
		out_buffers
	);
}

AttrFileLocal()
void gfx_command_pool_destroy(VkCommandPool command_pool) {
	vkDestroyCommandPool(fl_gfx.device.handle, command_pool, &fl_gfx_allocator);
}

AttrFileLocal()
void gfx_command_pool_init(void) {
	gfx_command_pool_create(fl_gfx.device.physical.main_qfamily_index, GFX_FRAMES_IN_FLIGHT, &fl_gfx.main_command_pool, fl_gfx.main_command_buffers);
	gfx_command_pool_create(fl_gfx.device.physical.transfer_qfamily_index, 1, &fl_gfx.transfer_command_pool, &fl_gfx.transfer_command_buffer);
}

AttrFileLocal()
void gfx_command_pool_free(void) {
	gfx_command_pool_destroy(fl_gfx.main_command_pool);
	gfx_command_pool_destroy(fl_gfx.transfer_command_pool);
}

// ---- ---- ---- ----
// render pass
// ---- ---- ---- ----

AttrFileLocal()
void gfx_render_pass_init(void) {
	vkCreateRenderPass(
		fl_gfx.device.handle,
		&(VkRenderPassCreateInfo){
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			// attachments
			.attachmentCount = 1,
			.pAttachments = &(VkAttachmentDescription){
				.format = fl_gfx.device.physical.surface_format.format,
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
		&fl_gfx_allocator,
		&fl_gfx.render_pass
	);
}

AttrFileLocal()
void gfx_render_pass_free(void) {
	vkDestroyRenderPass(fl_gfx.device.handle, fl_gfx.render_pass, &fl_gfx_allocator);
}

// ---- ---- ---- ----
// swapchain
// ---- ---- ---- ----

AttrFileLocal()
void gfx_swapchain_init(VkSwapchainKHR old_swapchain) {
	// ---- ---- ---- ----
	// surface capabilities
	// ---- ---- ---- ----

	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(fl_gfx.device.physical.handle, fl_gfx.surface, &surface_capabilities);

	fl_gfx.swapchain.extent = surface_capabilities.currentExtent;
	if (fl_gfx.swapchain.extent.width == UINT32_MAX && fl_gfx.swapchain.extent.height == UINT32_MAX)
		os_surface_get_size(&fl_gfx.swapchain.extent.width, &fl_gfx.swapchain.extent.height);

	if (fl_gfx.swapchain.extent.width == 0 || fl_gfx.swapchain.extent.height == 0)
		return;

	// ---- ---- ---- ----
	// swapchain
	// ---- ---- ---- ----

	uint32_t queue_families_buffer[2];
	arr32 queue_families = {
		.capacity = ArrayCount(queue_families_buffer),
		.buffer = queue_families_buffer,
	};
	arr32_append_unique(&queue_families, fl_gfx.device.physical.main_qfamily_index);
	arr32_append_unique(&queue_families, fl_gfx.device.physical.present_qfamily_index);

	vkCreateSwapchainKHR(
		fl_gfx.device.handle,
		&(VkSwapchainCreateInfoKHR){
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = fl_gfx.surface,
			.oldSwapchain = old_swapchain,
			// present mode
			.presentMode = fl_gfx.device.physical.present_mode,
			// surface capabilities
			.preTransform = surface_capabilities.currentTransform,
			.minImageCount = min_u32(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount),
			.imageExtent = fl_gfx.swapchain.extent,
			// surface format
			.imageFormat = fl_gfx.device.physical.surface_format.format,
			.imageColorSpace = fl_gfx.device.physical.surface_format.colorSpace,
			// queue families
			.imageSharingMode = queue_families.count >= 2
				? VK_SHARING_MODE_CONCURRENT
				: VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = (uint32_t)queue_families.count,
			.pQueueFamilyIndices = queue_families.buffer,
			// image params
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			// composition params
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.clipped = VK_TRUE,
		},
		&fl_gfx_allocator,
		&fl_gfx.swapchain.handle
	);

	vkGetSwapchainImagesKHR(fl_gfx.device.handle, fl_gfx.swapchain.handle, &fl_gfx.swapchain.images_count, NULL);
	fl_gfx.swapchain.images = os_memory_heap(NULL, sizeof(VkImage) * fl_gfx.swapchain.images_count);
	vkGetSwapchainImagesKHR(fl_gfx.device.handle, fl_gfx.swapchain.handle, &fl_gfx.swapchain.images_count, fl_gfx.swapchain.images);

	// ---- ---- ---- ----
	// image views
	// ---- ---- ---- ----

	fl_gfx.swapchain.image_views = os_memory_heap(NULL, sizeof(VkImageView) * fl_gfx.swapchain.images_count);
	for (uint32_t i = 0; i < fl_gfx.swapchain.images_count; i++)
		vkCreateImageView(
			fl_gfx.device.handle,
			&(VkImageViewCreateInfo){
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = fl_gfx.swapchain.images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = fl_gfx.device.physical.surface_format.format,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					// mip map
					.levelCount = 1,
					// layers
					.layerCount = 1,
				},
			},
			&fl_gfx_allocator,
			&fl_gfx.swapchain.image_views[i]
		);

	// ---- ---- ---- ----
	// framebuffers
	// ---- ---- ---- ----

	fl_gfx.swapchain.framebuffers = os_memory_heap(NULL, sizeof(VkFramebuffer) * fl_gfx.swapchain.images_count);
	for (uint32_t i = 0; i < fl_gfx.swapchain.images_count; i++) {
		vkCreateFramebuffer(
			fl_gfx.device.handle,
			&(VkFramebufferCreateInfo){
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = fl_gfx.render_pass,
				.width = fl_gfx.swapchain.extent.width,
				.height = fl_gfx.swapchain.extent.height,
				.layers = 1,
				// attachments
				.attachmentCount = 1,
				.pAttachments = &fl_gfx.swapchain.image_views[i],
			},
			&fl_gfx_allocator,
			&fl_gfx.swapchain.framebuffers[i]
		);
	}
}

AttrFileLocal()
void gfx_swapchain_free(struct GFX_Swapchain swapchain) {
	if (swapchain.handle == VK_NULL_HANDLE)
		return;

	for (uint32_t i = 0; i < swapchain.images_count; i++)
		vkDestroyFramebuffer(fl_gfx.device.handle, swapchain.framebuffers[i], &fl_gfx_allocator);

	for (uint32_t i = 0; i < swapchain.images_count; i++)
		vkDestroyImageView(fl_gfx.device.handle, swapchain.image_views[i], &fl_gfx_allocator);

	if (swapchain.handle != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(fl_gfx.device.handle, swapchain.handle, &fl_gfx_allocator);
		swapchain.handle = VK_NULL_HANDLE;
	}

	os_memory_heap(swapchain.framebuffers, 0);
	os_memory_heap(swapchain.image_views, 0);
	os_memory_heap(swapchain.images, 0);
}

AttrFileLocal()
void gfx_swapchain_recreate(void) {
	struct GFX_Swapchain const previous = fl_gfx.swapchain;
	fl_gfx.swapchain = (struct GFX_Swapchain){0};
	gfx_swapchain_init(previous.handle);
	// @todo don't halt the main thread
	vkDeviceWaitIdle(fl_gfx.device.handle);
	gfx_swapchain_free(previous);
}

// ---- ---- ---- ----
// pipeline / USER DATA
// ---- ---- ---- ----

struct Vertex {
	float position[2];
	float color[3];
};

AttrFileLocal()
VkShaderModule gfx_shader_module_create(char const * name) {
	struct Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = arena_get_position(scratch);

	struct Array_U8 const source = base_file_read(scratch, name);
	VkShaderModule ret;
	vkCreateShaderModule(
		fl_gfx.device.handle,
		&(VkShaderModuleCreateInfo){
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = source.count,
			.pCode = (void *)source.buffer,
		},
		&fl_gfx_allocator,
		&ret
	);

	arena_set_position(scratch, scratch_position);
	return ret;
}

AttrFileLocal()
void gfx_graphics_pipeline_init(void) {
	// -- prepare shaders
	VkPipelineShaderStageCreateInfo const shader_stages[] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = gfx_shader_module_create("shader.vert.spirv"),
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = gfx_shader_module_create("shader.frag.spirv"),
			.pName = "main",
		},
	};

	// -- create pipeline layout
	vkCreatePipelineLayout(
		fl_gfx.device.handle,
		&(VkPipelineLayoutCreateInfo){
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		},
		&fl_gfx_allocator,
		&fl_gfx.pipeline.layout
	);

	// -- create graphics pipeline
	AttrFuncLocal() VkDynamicState const dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	vkCreateGraphicsPipelines(
		fl_gfx.device.handle,
		VK_NULL_HANDLE,
		1, &(VkGraphicsPipelineCreateInfo){
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.layout = fl_gfx.pipeline.layout,
			.renderPass = fl_gfx.render_pass,
			.subpass = 0,
			// stages
			.stageCount = ArrayCount(shader_stages),
			.pStages = shader_stages,
			// vertices
			.pVertexInputState = &(VkPipelineVertexInputStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				// binding
				.vertexBindingDescriptionCount = 1,
				.pVertexBindingDescriptions = &(VkVertexInputBindingDescription){
					.binding = 0,
					.stride = sizeof(struct Vertex),
					.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
				},
				// attributes
				.vertexAttributeDescriptionCount = 2,
				.pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]){
					[0] = {
						.binding = 0,
						.location = 0,
						.format = VK_FORMAT_R32G32_SFLOAT,
						.offset = offsetof(struct Vertex, position),
					},
					[1] = {
						.binding = 0,
						.location = 1,
						.format = VK_FORMAT_R32G32B32_SFLOAT,
						.offset = offsetof(struct Vertex, color),
					},
				},
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
		&fl_gfx_allocator,
		&fl_gfx.pipeline.handle
	);

	for (uint32_t i = 0, count = ArrayCount(shader_stages); i < count; i++)
		vkDestroyShaderModule(fl_gfx.device.handle, shader_stages[i].module, &fl_gfx_allocator);
}

AttrFileLocal()
void gfx_graphics_pipeline_free(void) {
	vkDestroyPipeline(fl_gfx.device.handle, fl_gfx.pipeline.handle, &fl_gfx_allocator);
	vkDestroyPipelineLayout(fl_gfx.device.handle, fl_gfx.pipeline.layout, &fl_gfx_allocator);
}

// ---- ---- ---- ----
// vertices / USER DATA
// ---- ---- ---- ----

AttrFileLocal()
struct Vertex fl_gfx_vertices[3] = {
	{.position = { 0.0f,  0.5f}, .color = {1.0f, 0.0f, 0.0f}},
	{.position = {-0.5f, -0.5f}, .color = {0.0f, 1.0f, 0.0f}},
	{.position = { 0.5f, -0.5f}, .color = {0.0f, 0.0f, 1.0f}},
};

AttrFileLocal()
uint32_t gfx_physical_memory_find_type(uint32_t bits, VkMemoryPropertyFlags flags) {
	VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
	vkGetPhysicalDeviceMemoryProperties(fl_gfx.device.physical.handle, &physical_device_memory_properties);

	for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; i++) {
		VkMemoryType const it = physical_device_memory_properties.memoryTypes[i];
		if ((it.propertyFlags & flags) != flags)
			continue;

		uint32_t const mask = 1 << i;
		if (!(mask & bits))
			continue;

		return i;
	}

	AssertF(false, "can't find memory property with bits %#b and flags %#b\n", bits, flags);
	return 0;
}

AttrFileLocal()
void gfx_buffer_create(
	VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags,
	VkBuffer * out_buffer, VkDeviceMemory * out_buffer_memory
) {
	uint32_t queue_families_buffer[2];
	arr32 queue_families = {
		.capacity = ArrayCount(queue_families_buffer),
		.buffer = queue_families_buffer,
	};
	arr32_append_unique(&queue_families, fl_gfx.device.physical.main_qfamily_index);
	arr32_append_unique(&queue_families, fl_gfx.device.physical.transfer_qfamily_index);

	vkCreateBuffer(
		fl_gfx.device.handle,
		&(VkBufferCreateInfo){
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage_flags,
			.sharingMode = queue_families.count >= 2
				? VK_SHARING_MODE_CONCURRENT
				: VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = (uint32_t)queue_families.count,
			.pQueueFamilyIndices = queue_families.buffer,
		},
		&fl_gfx_allocator,
		out_buffer
	);

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(fl_gfx.device.handle, *out_buffer, &memory_requirements);

	VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
	vkGetPhysicalDeviceMemoryProperties(fl_gfx.device.physical.handle, &physical_device_memory_properties);

	vkAllocateMemory(
		fl_gfx.device.handle,
		&(VkMemoryAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memory_requirements.size,
			.memoryTypeIndex = gfx_physical_memory_find_type(
				memory_requirements.memoryTypeBits,
				property_flags
			),
		},
		&fl_gfx_allocator,
		out_buffer_memory
	);

	VkDeviceSize const memory_offset = 0;
	vkBindBufferMemory(fl_gfx.device.handle, *out_buffer, *out_buffer_memory, 0);
}

AttrFileLocal()
void gfx_buffer_destroy(VkBuffer buffer, VkDeviceMemory buffer_memory) {
	vkFreeMemory(fl_gfx.device.handle, buffer_memory, &fl_gfx_allocator);
	vkDestroyBuffer(fl_gfx.device.handle, buffer, &fl_gfx_allocator);
}

AttrFileLocal()
void gfx_buffer_copy(VkBuffer source, VkBuffer target, VkDeviceSize size) {
	vkBeginCommandBuffer(fl_gfx.transfer_command_buffer, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	});

	vkCmdCopyBuffer(fl_gfx.transfer_command_buffer, source, target, 1, &(VkBufferCopy){
		.size = size,
	});

	vkEndCommandBuffer(fl_gfx.transfer_command_buffer);

	vkQueueSubmit(fl_gfx.device.transfer_queue, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &fl_gfx.transfer_command_buffer,
	}, VK_NULL_HANDLE);

	vkQueueWaitIdle(fl_gfx.device.transfer_queue);
}

AttrFileLocal()
void gfx_vertex_buffer_init(void) {
	size_t const input_data_size = sizeof(fl_gfx_vertices);

	// @todo might be better to use a common allocator for this
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	gfx_buffer_create(
		input_data_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&staging_buffer, &staging_buffer_memory
	);

	void * staging_memory_pointer;
	VkDeviceSize const memory_offset = 0;
	vkMapMemory(fl_gfx.device.handle, staging_buffer_memory, memory_offset, input_data_size, 0, &staging_memory_pointer);
	mem_copy(fl_gfx_vertices, staging_memory_pointer, input_data_size);
	vkUnmapMemory(fl_gfx.device.handle, staging_buffer_memory);

	gfx_buffer_create(
		input_data_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		| VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&fl_gfx.vertex_buffer, &fl_gfx.vertex_buffer_memory
	);

	gfx_buffer_copy(staging_buffer, fl_gfx.vertex_buffer, input_data_size);
	gfx_buffer_destroy(staging_buffer, staging_buffer_memory);
}

AttrFileLocal()
void gfx_vertex_buffer_free(void) {
	gfx_buffer_destroy(fl_gfx.vertex_buffer, fl_gfx.vertex_buffer_memory);
}

// ---- ---- ---- ----
// API
// ---- ---- ---- ----

void gfx_init(void) {
	// -- indentation symbolizes order dependence
	gfx_instance_init();
		fl_gfx.surface = (VkSurfaceKHR)os_vulkan_create_surface(fl_gfx.instance, &fl_gfx_allocator);
			gfx_device_init();
				gfx_synchronization_init();
				gfx_command_pool_init();
				gfx_render_pass_init();
					gfx_swapchain_init(VK_NULL_HANDLE);
					gfx_graphics_pipeline_init();
				gfx_vertex_buffer_init();
}

void gfx_free(void) {
	vkDeviceWaitIdle(fl_gfx.device.handle);

	// any order within same indentation
	gfx_synchronization_free();
	gfx_command_pool_free();
	gfx_graphics_pipeline_free();
	gfx_vertex_buffer_free();
	gfx_render_pass_free();

	gfx_swapchain_free(fl_gfx.swapchain);
		vkDestroySurfaceKHR(fl_gfx.instance, fl_gfx.surface, &fl_gfx_allocator);

	// -- always last
	gfx_device_free();
		gfx_instance_free();

	mem_zero(&fl_gfx, sizeof(fl_gfx));
}

void gfx_tick(void) {
	if (fl_gfx.swapchain.out_of_date_or_suboptimal)
		gfx_swapchain_recreate();

	if (fl_gfx.swapchain.handle == VK_NULL_HANDLE)
		return;

	uint32_t        const frame                     = fl_gfx.swapchain.frame;
	VkCommandBuffer const command_buffer            = fl_gfx.main_command_buffers[frame];
	VkSemaphore     const image_available_semaphore = fl_gfx.image_available_semaphores[frame];
	VkSemaphore     const render_finished_semaphore = fl_gfx.render_finished_semaphores[frame];
	VkFence         const in_flight_fence           = fl_gfx.in_flight_fences[frame];

	// ---- ---- ---- ----
	// wait previous frame
	// ---- ---- ---- ----

	vkWaitForFences(fl_gfx.device.handle, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);

	// ---- ---- ---- ----
	// prepare next frame
	// ---- ---- ---- ----

	uint32_t image_index;
	VkResult const aquire_next_image_result =
	vkAcquireNextImageKHR(
		fl_gfx.device.handle, fl_gfx.swapchain.handle,
		UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index
	);
	switch (aquire_next_image_result) {
		// success, continue rendering
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			break;

		// failure, cancel rendering
		case VK_ERROR_OUT_OF_DATE_KHR:
			fl_gfx.swapchain.out_of_date_or_suboptimal = true;
		AttrFallthrough();
		default: return;
	}

	vkResetFences(fl_gfx.device.handle, 1, &in_flight_fence);
	vkResetCommandBuffer(command_buffer, 0);

	// ---- ---- ---- ----
	// draw
	// ---- ---- ---- ----

	vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	});

	vkCmdBeginRenderPass(command_buffer, &(VkRenderPassBeginInfo){
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = fl_gfx.render_pass,
		.framebuffer = fl_gfx.swapchain.framebuffers[image_index],
		.renderArea = {
			.extent = fl_gfx.swapchain.extent,
		},
		// clear
		.clearValueCount = 1,
		.pClearValues = &(VkClearValue){
			.color.float32 = {0, 0, 0, 1},
		},
	}, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fl_gfx.pipeline.handle);
	vkCmdSetViewport(command_buffer, 0, 1, &(VkViewport){
		// offset, Y positive up
		.x = (float)0,
		.y = (float)fl_gfx.swapchain.extent.height,
		// scale, Y positive up
		.width = (float)fl_gfx.swapchain.extent.width,
		.height = -(float)fl_gfx.swapchain.extent.height,
		// depth
		.minDepth = 0,
		.maxDepth = 1,
	});
	vkCmdSetScissor(command_buffer, 0, 1, &(VkRect2D){
		.extent = fl_gfx.swapchain.extent,
	});

	vkCmdBindVertexBuffers(command_buffer, 0, 1, &fl_gfx.vertex_buffer, (VkDeviceSize[]){0});

	vkCmdDraw(command_buffer, ArrayCount(fl_gfx_vertices), 1, 0, 0);

	vkCmdEndRenderPass(command_buffer);
	vkEndCommandBuffer(command_buffer);

	// ---- ---- ---- ----
	// submit
	// ---- ---- ---- ----

	vkQueueSubmit(fl_gfx.device.main_queue, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		// wait semaphores
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &image_available_semaphore,
		.pWaitDstStageMask = (VkPipelineStageFlags[]){
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		},
		// command buffers
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer,
		// signal semaphores
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &render_finished_semaphore,
	}, in_flight_fence);

	// ---- ---- ---- ----
	// present
	// ---- ---- ---- ----

	VkResult const queue_present_result =
	vkQueuePresentKHR(fl_gfx.device.present_queue, &(VkPresentInfoKHR){
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		// wait semaphores
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &render_finished_semaphore,
		// swap chains
		.swapchainCount = 1,
		.pSwapchains = &fl_gfx.swapchain.handle,
		.pImageIndices = &image_index,
	});
	switch (queue_present_result) {
		case VK_SUBOPTIMAL_KHR:        // success
		case VK_ERROR_OUT_OF_DATE_KHR: // failure
			fl_gfx.swapchain.out_of_date_or_suboptimal = true;
		AttrFallthrough();
		default: break;
	}

	fl_gfx.swapchain.frame = (fl_gfx.swapchain.frame + 1) % GFX_FRAMES_IN_FLIGHT;
}

void gfx_notify_surface_resized(void) {
	fl_gfx.swapchain.out_of_date_or_suboptimal = true;
}

#undef GFX_DEFINE_PROC
#undef GFX_ENABLE_DEBUG
