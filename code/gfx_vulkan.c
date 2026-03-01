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

reverse Z
  use floating point buffer
  set range [1 .. 0] or flip depth with a projection matrix
  clear to 0 ; i.e. z-far, which means nothing changes here
  set depth test to "greater value writes"
*/

#define GFX_REVERSE_Z 0
#define GFX_FRAMES_IN_FLIGHT 2
#define GFX_PREFER_SRGB 0
#define GFX_PREFER_VSYNC_OFF 0
#define GFX_DEFINE_PROC(name) PFN_ ## name name = (PFN_ ## name)gfx_get_instance_proc(#name)
#define GFX_ENABLE_DEBUG (BUILD_DEBUG == BUILD_DEBUG_ENABLE)

#define GFX_FRAMEBUFFER_INDEX_COLOR 0
#define GFX_FRAMEBUFFER_INDEX_DEPTH 1
#define GFX_FRAMEBUFFER_INDEX_RSLVE 2

#define GFX_ALIGN32_SCALAR sizeof(f32)
#define GFX_ALIGN32_VEC2 (GFX_ALIGN32_SCALAR * 2)
#define GFX_ALIGN32_VEC3 (GFX_ALIGN32_SCALAR * 4)
#define GFX_ALIGN32_VEC4 (GFX_ALIGN32_SCALAR * 4)
#define GFX_ALIGN32_MAT4 GFX_ALIGN32_VEC4

// @note vulkan expects a 64 bit architecture at least
AssertStatic(sizeof(size_t) >= sizeof(uint64_t));
AssertStatic(GFX_FRAMES_IN_FLIGHT >= 1);

// ---- ---- ---- ----
// stringifiers
// ---- ---- ---- ----

#if GFX_ENABLE_DEBUG
AttrFileLocal()
str8 gfx_to_string_for_debug_utils_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT value) {
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   return str8_lit("ERROR");
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) return str8_lit("WARNING");
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    return str8_lit("INFO");
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) return str8_lit("VERBOSE");
	return str8_lit("unknown");
}

AttrFileLocal()
str8 gfx_to_string_for_debug_utils_message_type(VkDebugUtilsMessageTypeFlagsEXT value) {
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT) return str8_lit("Device address binding");
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)            return str8_lit("Performance");
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)             return str8_lit("Validation");
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)                return str8_lit("General");
	return str8_lit("unknown");
}

AttrFileLocal()
str8 gfx_to_string_for_allocation_type(VkInternalAllocationType value) {
	switch (value) {
		case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE: return str8_lit("Executable");
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 gfx_to_string_for_allocation_scope(VkSystemAllocationScope value) {
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
str8 gfx_to_string_for_physical_device_type(VkPhysicalDeviceType value) {
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
str8 gfx_to_string_for_format(VkFormat value) {
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
str8 gfx_to_string_for_color_space(VkColorSpaceKHR value) {
	switch (value) {
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return str8_lit("sRGB");
		// @note has a lot more cases
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 gfx_to_string_for_present_mode(VkPresentModeKHR value) {
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
str8 gfx_to_string_for_queue(VkQueueFlagBits value) {
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
str8 gfx_to_string_for_queue_flags(struct Arena * arena, VkQueueFlags flags) {
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
		str8_append(&ret, gfx_to_string_for_queue(bits[i]));
	}
	return ret;
}

// ---- ---- ---- ----
// convertors
// ---- ---- ---- ----

AttrFileLocal()
VkFormat gfx_map_vector_format_to_primitive_format(enum VkFormat format) {
	switch (format) {
		default: break; // @not_implemented
		// sRGB
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_R8G8B8A8_SRGB:
			return VK_FORMAT_R8_SRGB;
		// unsigned integers
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_B8G8R8A8_UINT:
			return VK_FORMAT_R8_UINT;
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16A16_UINT:
			return VK_FORMAT_R16_UINT;
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32A32_UINT:
			return VK_FORMAT_R32_UINT;
		case VK_FORMAT_R64_UINT:
		case VK_FORMAT_R64G64_UINT:
		case VK_FORMAT_R64G64B64_UINT:
		case VK_FORMAT_R64G64B64A64_UINT:
			return VK_FORMAT_R64_UINT;
		// unsigned integers, normalized
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8A8_UNORM:
			return VK_FORMAT_R8_UNORM;
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16A16_UNORM:
			return VK_FORMAT_R16_UNORM;
		// signed integers
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8A8_SINT:
			return VK_FORMAT_R8_SINT;
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16A16_SINT:
			return VK_FORMAT_R16_SINT;
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32A32_SINT:
			return VK_FORMAT_R32_SINT;
		case VK_FORMAT_R64_SINT:
		case VK_FORMAT_R64G64_SINT:
		case VK_FORMAT_R64G64B64_SINT:
		case VK_FORMAT_R64G64B64A64_SINT:
			return VK_FORMAT_R64_SINT;
		// signed integers, normalized
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
			return VK_FORMAT_R8_SNORM;
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
			return VK_FORMAT_R16_SNORM;
	}
	AssertF(false, "[gfx] not implemented for %d", format);
	return VK_FORMAT_UNDEFINED;
}

AttrFileLocal()
VkFormat gfx_map_primitive_format_to_vector_format(enum VkFormat primitive, size_t count) {
	switch (primitive) {
		default: break; // @not_implemented
		// sRGB
		case VK_FORMAT_R8_SRGB: switch (count) {
			case 1: return VK_FORMAT_R8_SRGB;
			case 2: return VK_FORMAT_R8G8_SRGB;
			case 3: return VK_FORMAT_R8G8B8_SRGB;
			case 4: return VK_FORMAT_R8G8B8A8_SRGB;
		} break;
		// unsigned integers
		case VK_FORMAT_R8_UINT: switch (count) {
			case 1: return VK_FORMAT_R8_UINT;
			case 2: return VK_FORMAT_R8G8_UINT;
			case 3: return VK_FORMAT_R8G8B8_UINT;
			case 4: return VK_FORMAT_R8G8B8A8_UINT;
		} break;
		case VK_FORMAT_R16_UINT: switch (count) {
			case 1: return VK_FORMAT_R16_UINT;
			case 2: return VK_FORMAT_R16G16_UINT;
			case 3: return VK_FORMAT_R16G16B16_UINT;
			case 4: return VK_FORMAT_R16G16B16A16_UINT;
		} break;
		case VK_FORMAT_R32_UINT: switch (count) {
			case 1: return VK_FORMAT_R32_UINT;
			case 2: return VK_FORMAT_R32G32_UINT;
			case 3: return VK_FORMAT_R32G32B32_UINT;
			case 4: return VK_FORMAT_R32G32B32A32_UINT;
		} break;
		case VK_FORMAT_R64_UINT: switch (count) {
			case 1: return VK_FORMAT_R64_UINT;
			case 2: return VK_FORMAT_R64G64_UINT;
			case 3: return VK_FORMAT_R64G64B64_UINT;
			case 4: return VK_FORMAT_R64G64B64A64_UINT;
		} break;
		// unsigned integers, normalized
		case VK_FORMAT_R8_UNORM: switch (count) {
			case 1: return VK_FORMAT_R8_UNORM;
			case 2: return VK_FORMAT_R8G8_UNORM;
			case 3: return VK_FORMAT_R8G8B8_UNORM;
			case 4: return VK_FORMAT_R8G8B8A8_UNORM;
		} break;
		case VK_FORMAT_R16_UNORM: switch (count) {
			case 1: return VK_FORMAT_R16_UNORM;
			case 2: return VK_FORMAT_R16G16_UNORM;
			case 3: return VK_FORMAT_R16G16B16_UNORM;
			case 4: return VK_FORMAT_R16G16B16A16_UNORM;
		} break;
		// signed integers
		case VK_FORMAT_R8_SINT: switch (count) {
			case 1: return VK_FORMAT_R8_SINT;
			case 2: return VK_FORMAT_R8G8_SINT;
			case 3: return VK_FORMAT_R8G8B8_SINT;
			case 4: return VK_FORMAT_R8G8B8A8_SINT;
		} break;
		case VK_FORMAT_R16_SINT: switch (count) {
			case 1: return VK_FORMAT_R16_SINT;
			case 2: return VK_FORMAT_R16G16_SINT;
			case 3: return VK_FORMAT_R16G16B16_SINT;
			case 4: return VK_FORMAT_R16G16B16A16_SINT;
		} break;
		case VK_FORMAT_R32_SINT: switch (count) {
			case 1: return VK_FORMAT_R32_SINT;
			case 2: return VK_FORMAT_R32G32_SINT;
			case 3: return VK_FORMAT_R32G32B32_SINT;
			case 4: return VK_FORMAT_R32G32B32A32_SINT;
		} break;
		case VK_FORMAT_R64_SINT: switch (count) {
			case 1: return VK_FORMAT_R64_SINT;
			case 2: return VK_FORMAT_R64G64_SINT;
			case 3: return VK_FORMAT_R64G64B64_SINT;
			case 4: return VK_FORMAT_R64G64B64A64_SINT;
		} break;
		// signed integers, normalized
		case VK_FORMAT_R8_SNORM: switch (count) {
			case 1: return VK_FORMAT_R8_SNORM;
			case 2: return VK_FORMAT_R8G8_SNORM;
			case 3: return VK_FORMAT_R8G8B8_SNORM;
			case 4: return VK_FORMAT_R8G8B8A8_SNORM;
		} break;
		case VK_FORMAT_R16_SNORM: switch (count) {
			case 1: return VK_FORMAT_R16_SNORM;
			case 2: return VK_FORMAT_R16G16_SNORM;
			case 3: return VK_FORMAT_R16G16B16_SNORM;
			case 4: return VK_FORMAT_R16G16B16A16_SNORM;
		} break;
		// floating points
		case VK_FORMAT_R32_SFLOAT: switch (count) {
			case 1: return VK_FORMAT_R32_SFLOAT;
			case 2: return VK_FORMAT_R32G32_SFLOAT;
			case 3: return VK_FORMAT_R32G32B32_SFLOAT;
			case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		} break;
		case VK_FORMAT_R64_SFLOAT: switch (count) {
			case 1: return VK_FORMAT_R64_SFLOAT;
			case 2: return VK_FORMAT_R64G64_SFLOAT;
			case 3: return VK_FORMAT_R64G64B64_SFLOAT;
			case 4: return VK_FORMAT_R64G64B64A64_SFLOAT;
		} break;
	}
	AssertF(false, "[gfx] not implemented for (%d, %zu)", primitive, count);
	return VK_FORMAT_UNDEFINED;
}

AttrFileLocal()
VkImageAspectFlags gfx_map_format_to_image_aspect(VkFormat format) {
	switch (format) {
		default: break; // @not_implemented

		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
			return VK_IMAGE_ASPECT_COLOR_BIT;

		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;

		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D16_UNORM_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	AssertF(false, "[gfx] not implemented for %d", format);
	return VK_IMAGE_ASPECT_NONE;
}

// ---- ---- ---- ----
// common utilities
// ---- ---- ---- ----

AttrFileLocal()
bool gfx_match_sets(
	uint32_t requested_count, char const * const * requested_set,
	uint32_t available_count, char const * const * avaliable_set) {
	bool available = true;
	for (uint32_t re_i = 0; re_i < requested_count; re_i++) {
		char const * requested = requested_set[re_i];
		for (uint32_t av_i = 0; av_i < available_count; av_i++) {
			char const * available = avaliable_set[av_i];
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
	str8 const severity_text = gfx_to_string_for_debug_utils_message_severity(messageSeverity);
	str8 const type_text = gfx_to_string_for_debug_utils_message_type(messageTypes); // @note argument is of flags type, but always has a single one
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
	str8 const type_text = gfx_to_string_for_allocation_type(allocationType);
	str8 const scope_text = gfx_to_string_for_allocation_scope(allocationScope);
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
	str8 const type_text = gfx_to_string_for_allocation_type(allocationType);
	str8 const scope_text = gfx_to_string_for_allocation_scope(allocationScope);
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
	AssertF(align <= max_align, "[gfx] alignment overflow %zu / %u\n", align, max_align);
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

struct GFX_QFamilies { // @note values are `index + 1`
	uint32_t main;
	uint32_t present;
	uint32_t transfer;
};

struct GFX_Queues {
	VkQueue main;
	VkQueue present;
	VkQueue transfer;
};

struct GFX_QCmd {
	VkQueue queue;
	VkCommandBuffer commands;
};

struct GFX_Buffer {
	VkBuffer       handle;
	VkDeviceMemory memory;
};

struct GFX_Texture {
	VkImageView    view;
	VkImage        handle;
	VkDeviceMemory memory;
};

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
			// cached
			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceMemoryProperties memory_properties;
			// choices
			struct GFX_QFamilies qfamily;
			VkSurfaceFormatKHR surface_format;
			VkPresentModeKHR present_mode;
			VkSampleCountFlagBits samples;
		} physical;
		VkDevice handle;
		struct GFX_Queues queue;
	} device;
	// samplers
	VkSampler sampler;
	// synchronization
	VkSemaphore image_available_semaphores[GFX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphores[GFX_FRAMES_IN_FLIGHT];
	VkFence     in_flight_fences[GFX_FRAMES_IN_FLIGHT];
	// main command pool
	VkCommandPool   main_command_pool;
	VkCommandBuffer main_command_buffers[GFX_FRAMES_IN_FLIGHT + 1]; // @note additional one is for transfers
	// transfer command pool
	VkCommandPool   transfer_command_pool;
	VkCommandBuffer transfer_command_buffer;
	// render pass and swapchain
	VkRenderPass render_pass;
	struct GFX_Swapchain {
		uint32_t         frame;
		VkExtent2D       extent;
		VkSwapchainKHR   handle;
		// color, created by the swapchain itself
		uint32_t         images_count;
		VkImage        * images;
		VkImageView    * image_views;
		// depth, created manually
		struct GFX_Texture color_texture;
		struct GFX_Texture depth_texture;
		// framebuffer
		VkFramebuffer  * framebuffers;
		// state
		bool             out_of_date_or_suboptimal;
	} swapchain;
	// dpool
	VkDescriptorPool descriptor_pool;
	// USER DATA / pipeline ("a shader program" and more)
	struct GFX_Pipeline {
		VkPipelineLayout layout;
		VkPipeline       handle;
		struct GFX_Descriptor_Set {
			VkDescriptorSetLayout layout;
			VkDescriptorSet       handles[GFX_FRAMES_IN_FLIGHT];
		} descriptor_set;
	} pipeline;
	// USER DATA / material data
	struct GFX_Material {
		struct GFX_Buffer data;
		u8 * map[GFX_FRAMES_IN_FLIGHT];
	} material;
	// USER DATA / model
	struct GFX_Model {
		struct GFX_Buffer data;
		VkDeviceSize offset_vertex;
		VkDeviceSize offset_index;
		VkIndexType index_type;
		uint32_t    index_count;
	} model;
	// USER DATA / texture
	struct GFX_Texture texture;
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
// GFX utilities
// ---- ---- ---- ----

AttrFileLocal()
bool gfx_is_format_supported(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) {
	VkFormatProperties properties = {0};
	vkGetPhysicalDeviceFormatProperties(fl_gfx.device.physical.handle, format, &properties);
	switch (tiling) {
		case VK_IMAGE_TILING_LINEAR:  return (properties.linearTilingFeatures  & features) == features;
		case VK_IMAGE_TILING_OPTIMAL: return (properties.optimalTilingFeatures & features) == features;
		default:                      return false;
	}
}

AttrFileLocal()
uint32_t gfx_find_memory_type_index(uint32_t bits, VkMemoryPropertyFlags flags) {
	for (uint32_t i = 0; i < fl_gfx.device.physical.memory_properties.memoryTypeCount; i++) {
		VkMemoryType const it = fl_gfx.device.physical.memory_properties.memoryTypes[i];
		if ((it.propertyFlags & flags) != flags)
			continue;

		uint32_t const mask = 1 << i;
		if (!(mask & bits))
			continue;

		return i;
	}

	AssertF(false, "[gfx] can't find memory property with bits %#b and flags %#b\n", bits, flags);
	return 0;
}

AttrFileLocal()
VkSampleCountFlagBits gfx_find_max_sample_count(void) {
	AttrFuncLocal() VkSampleCountFlagBits const bits[] = {
		VK_SAMPLE_COUNT_64_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_2_BIT,
	};
	VkFlags const limits = fl_gfx.device.physical.properties.limits.framebufferColorSampleCounts
	/**/                 & fl_gfx.device.physical.properties.limits.framebufferDepthSampleCounts;
	for (uint32_t i = 0; i < ArrayCount(bits); i++) {
		if (limits & (VkFlags)bits[i])
			return bits[i];
	}
	return VK_SAMPLE_COUNT_1_BIT;
}

AttrFileLocal()
VkFormat gfx_find_depth_format(void) {
	VkFormat const candidates[] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
	};
	for (uint32_t i = 0; i < ArrayCount(candidates); i++) {
		if (gfx_is_format_supported(candidates[i], VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
			return candidates[i];
	}
	Assert(false, "[gfx] can't find depth format\n");
	return VK_FORMAT_UNDEFINED;
}

AttrFileLocal()
struct GFX_QCmd gfx_get_main_qbuffer(uint32_t frame) {
	return (struct GFX_QCmd){
		.queue    = fl_gfx.device.queue.main,
		.commands = fl_gfx.main_command_buffers[frame],
	};
}

AttrFileLocal()
struct GFX_QCmd gfx_get_transfer_qbuffer(void) {
	return (struct GFX_QCmd){
		.queue    = fl_gfx.device.queue.transfer,
		.commands = fl_gfx.transfer_command_buffer,
	};
}

// ---- ---- ---- ----
// image view
// ---- ---- ---- ----

AttrFileLocal()
VkImageView gfx_texture_view_create(VkImage image, VkFormat format, u32 extra_mip_levels) {
	VkImageAspectFlags const aspect = gfx_map_format_to_image_aspect(format);
	VkImageView ret = VK_NULL_HANDLE;
	vkCreateImageView(
		fl_gfx.device.handle,
		&(VkImageViewCreateInfo){
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange = {
				.aspectMask = aspect,
				// mip map
				.levelCount = 1 + extra_mip_levels,
				// layers
				.layerCount = 1,
			},
		},
		&fl_gfx_allocator,
		&ret
	);
	return ret;
}

AttrFileLocal()
void gfx_texture_view_destroy(VkImageView image_view) {
	vkDestroyImageView(fl_gfx.device.handle, image_view, &fl_gfx_allocator);
}

// ---- ---- ---- ----
// sampler
// ---- ---- ---- ----

AttrFileLocal()
VkSampler gfx_sampler_create(void) {
	VkSampler ret = VK_NULL_HANDLE;
	vkCreateSampler(
		fl_gfx.device.handle,
		&(VkSamplerCreateInfo){
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.minFilter = VK_FILTER_LINEAR,
			.magFilter = VK_FILTER_LINEAR,
			.anisotropyEnable = fl_gfx.device.physical.properties.limits.maxSamplerAnisotropy > 1,
			.maxAnisotropy = fl_gfx.device.physical.properties.limits.maxSamplerAnisotropy,
		},
		&fl_gfx_allocator,
		&ret
	);
	return ret;
}

AttrFileLocal()
void gfx_sampler_destroy(VkSampler sampler) {
	vkDestroySampler(fl_gfx.device.handle, sampler, &fl_gfx_allocator);
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

	uint32_t api_version = 0;
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

	uint32_t available_extensions_count = 0;
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

	uint32_t available_layers_count = 0;
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
	Assert(extensions_match, "[gfx] can't satisfy requested extensions set\n\n");

	char const ** available_layers = ArenaPushArray(scratch, char const *, available_layers_count);
	for (uint32_t i = 0; i < available_layers_count; i++)
		available_layers[i] = available_layer_properties[i].layerName;
	bool const layers_match = gfx_match_sets(
		ArrayCount(required_instance_layers), required_instance_layers,
		available_layers_count, available_layers
	);
	Assert(layers_match, "[gfx] can't satisfy requested layers set\n\n");

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

	uint32_t all_physical_devices_count = 0;
	vkEnumeratePhysicalDevices(fl_gfx.instance, &all_physical_devices_count, NULL);
	Assert(all_physical_devices_count > 0, "[gfx] physical devices missing\n");

	VkPhysicalDevice * all_physical_devices = ArenaPushArray(scratch, VkPhysicalDevice, all_physical_devices_count);
	vkEnumeratePhysicalDevices(fl_gfx.instance, &all_physical_devices_count, all_physical_devices);

	// ---- ---- ---- ----
	// filter out devices
	// ---- ---- ---- ----

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
			DbgPrintF("[gfx] can't satisfy requested extensions set for 0x%p\n", (void *)handle);
	}
	Assert(physical_devices_count > 0, "[gfx] physical devices missing\n");

	// ---- ---- ---- ----
	// properties
	// ---- ---- ---- ----

	VkPhysicalDeviceProperties * physical_devices_properties = ArenaPushArray(scratch, VkPhysicalDeviceProperties, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) vkGetPhysicalDeviceProperties(physical_devices[i], physical_devices_properties + i);

	uint32_t * qfamilies_counts = ArenaPushArray(scratch, uint32_t, physical_devices_count);
	VkQueueFamilyProperties ** qfamily_properties_set = ArenaPushArray(scratch, VkQueueFamilyProperties *, physical_devices_count);
	VkBool32 ** qfamilies_surface_support_set = ArenaPushArray(scratch, VkBool32 *, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDevice const handle = physical_devices[i];

		uint32_t qfamilies_count = 0;
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

		VkPhysicalDeviceProperties const   properties           = physical_devices_properties[i];
		uint32_t                   const   qfamilies_count      = qfamilies_counts[i];
		VkQueueFamilyProperties    const * qfamilies_properties = qfamily_properties_set[i];

		str8 const type_text = gfx_to_string_for_physical_device_type(properties.deviceType);
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
			str8 const flags_text = gfx_to_string_for_queue_flags(scratch, it.queueFlags);
			fmt_print("    [%u] %.*s\n", i, (int)flags_text.count, flags_text.buffer);
			arena_set_position(scratch, scratch_position);
		}
	}
	fmt_print("\n");

	// ---- ---- ---- ----
	// queue families
	// ---- ---- ---- ----

	struct GFX_QFamilies * qfamily_choices = ArenaPushArray(scratch, struct GFX_QFamilies, physical_devices_count);
	for (uint32_t i = 0; i < physical_devices_count; i++) {
		VkPhysicalDevice const handle = physical_devices[i];

		uint32_t const count = qfamilies_counts[i];
		VkQueueFamilyProperties const * properties = qfamily_properties_set[i];
		VkBool32 const * surface_supports = qfamilies_surface_support_set[i];

		struct GFX_QFamilies * qfamily_choice = &qfamily_choices[i];

		*qfamily_choice = (struct GFX_QFamilies){0};

		// -- prefer a main queue that can present
		for (uint32_t i = 0; i < count; i++) {
			VkQueueFamilyProperties const it = properties[i];
			if (!(it.queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;

			VkBool32 const surface_support = surface_supports[i];
			if (!surface_support) continue;

			qfamily_choice->main = i + 1;
			qfamily_choice->present = i + 1;
			break;
		}

		// -- prefer a free-standing transfer queue
		for (uint32_t i = 0; i < count; i++) {
			VkQueueFamilyProperties const it = properties[i];
			if (!(it.queueFlags & VK_QUEUE_TRANSFER_BIT)) continue;
			if (it.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) continue;

			qfamily_choice->transfer = i + 1;
			break;
		}

		// -- fallback to first matching
		for (uint32_t i = 0; i < count && !qfamily_choice->main; i++) {
			VkQueueFamilyProperties const it = properties[i];
			if (it.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				qfamily_choice->main = i + 1;
		}

		for (uint32_t i = 0; i < count && !qfamily_choice->present; i++) {
			VkBool32 const surface_support = surface_supports[i];
			if (surface_support)
				qfamily_choice->present = i + 1;
		}

		for (uint32_t i = 0; i < count && !qfamily_choice->transfer; i++) {
			VkQueueFamilyProperties const it = properties[i];
			if (it.queueFlags & VK_QUEUE_TRANSFER_BIT)
				qfamily_choice->transfer = i + 1;
		}
	}

	// @note it's possible to forcibly prefer, say, a discrete GPU, but better to default
	// with a first suitable one, giving a chance to the user to sort priorities manually
	// namely with the "NVIDIA Control Panel" or the "AMD Software: Adrenalin Edition"
	uint32_t physical_device_choice = 0; // @note value is `index + 1`
	for (uint32_t i = 0; i < physical_devices_count && !physical_device_choice; i++)
		if (qfamily_choices[i].main && qfamily_choices[i].present && qfamily_choices[i].transfer)
			physical_device_choice = i + 1;

	Assert(physical_device_choice > 0, "[gfx] no suitable physical device found\n");
	uint32_t const physical_device_index = physical_device_choice
		? physical_device_choice - 1
		: 0;

	fl_gfx.device.physical.handle     = physical_devices[physical_device_index];
	fl_gfx.device.physical.properties = physical_devices_properties[physical_device_index];
	vkGetPhysicalDeviceMemoryProperties(fl_gfx.device.physical.handle, &fl_gfx.device.physical.memory_properties);

	fl_gfx.device.physical.qfamily = qfamily_choices[physical_device_index];
	fl_gfx.device.physical.samples = gfx_find_max_sample_count();


	// ---- ---- ---- ----
	// surface format
	// ---- ---- ---- ----

	fl_gfx.device.physical.surface_format = (VkSurfaceFormatKHR){
		.format     = VK_FORMAT_MAX_ENUM,
		.colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR,
	};

	VkFormat const surface_format_preferences[] = {
		// @bug RivaTuner Statistics Server doesn't work great with sRGB formats
		#if GFX_PREFER_SRGB
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_SRGB,
		#endif
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_B8G8R8A8_UNORM,
	};

	uint32_t surface_formats_count = 0;
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
		#if GFX_PREFER_VSYNC_OFF
		VK_PRESENT_MODE_IMMEDIATE_KHR,
		#endif
		VK_PRESENT_MODE_MAILBOX_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
	};

	uint32_t present_modes_count = 0;
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

	str8 const surface_format_text = gfx_to_string_for_format(fl_gfx.device.physical.surface_format.format);
	str8 const color_space_text = gfx_to_string_for_color_space(fl_gfx.device.physical.surface_format.colorSpace);
	str8 const present_mode_text = gfx_to_string_for_present_mode(fl_gfx.device.physical.present_mode);
	fmt_print("[gfx] chosen device:\n");
	fmt_print("- name:           \"%s\"\n", fl_gfx.device.physical.properties.deviceName);
	fmt_print("- handle:         0x%p\n", (void *)fl_gfx.device.physical.handle);
	fmt_print("- surface format: %.*s\n", (int)surface_format_text.count, surface_format_text.buffer);
	fmt_print("- color space:    %.*s\n", (int)color_space_text.count, color_space_text.buffer);
	fmt_print("- present mode:   %.*s\n", (int)present_mode_text.count, present_mode_text.buffer);
	fmt_print("\n");

	arr32 queue_families = {.capacity = 4, .buffer = (uint32_t[4]){0}};
	arr32_append_unique(&queue_families, fl_gfx.device.physical.qfamily.main - 1);
	arr32_append_unique(&queue_families, fl_gfx.device.physical.qfamily.present - 1);
	arr32_append_unique(&queue_families, fl_gfx.device.physical.qfamily.transfer - 1);

	VkDeviceQueueCreateInfo queue_infos[4];
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
			// features
			.pEnabledFeatures = &(VkPhysicalDeviceFeatures){
				.samplerAnisotropy = fl_gfx.device.physical.properties.limits.maxSamplerAnisotropy > 1,
				.sampleRateShading = fl_gfx.device.physical.samples > VK_SAMPLE_COUNT_1_BIT,
			},
		},
		&fl_gfx_allocator,
		&fl_gfx.device.handle
	);

	vkGetDeviceQueue(fl_gfx.device.handle, fl_gfx.device.physical.qfamily.main - 1,     0, &fl_gfx.device.queue.main);
	vkGetDeviceQueue(fl_gfx.device.handle, fl_gfx.device.physical.qfamily.present - 1,  0, &fl_gfx.device.queue.present);
	vkGetDeviceQueue(fl_gfx.device.handle, fl_gfx.device.physical.qfamily.transfer - 1, 0, &fl_gfx.device.queue.transfer);

	arena_set_position(scratch, scratch_position);
}

AttrFileLocal()
void gfx_device_free(void) {
	vkDestroyDevice(fl_gfx.device.handle, &fl_gfx_allocator);
}

// ---- ---- ---- ----
// texture
// ---- ---- ---- ----

AttrFileLocal()
struct GFX_Texture gfx_texture_create(
	uvec2 size, VkFormat format, u32 extra_mip_levels, VkSampleCountFlagBits samples, VkImageTiling tiling,
	VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags
) {
	arr32 queue_families = {.capacity = 2, .buffer = (uint32_t[2]){0}};
	arr32_append_unique(&queue_families, fl_gfx.device.physical.qfamily.main - 1);
	arr32_append_unique(&queue_families, fl_gfx.device.physical.qfamily.transfer - 1);

	struct GFX_Texture ret = {0};
	vkCreateImage(
		fl_gfx.device.handle,
		&(VkImageCreateInfo){
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			// image specific
			.imageType = VK_IMAGE_TYPE_2D,
			.extent = {size.x, size.y, 1},
			.mipLevels = 1 + extra_mip_levels,
			.arrayLayers = 1,
			.format = format,
			.tiling = tiling,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.samples = samples,
			// generic info
			.usage = usage_flags,
			.sharingMode = queue_families.count >= 2
				? VK_SHARING_MODE_CONCURRENT
				: VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = (uint32_t)queue_families.count,
			.pQueueFamilyIndices = queue_families.buffer,
		},
		&fl_gfx_allocator,
		&ret.handle
	);

	VkMemoryRequirements memory_requirements = {0};
	vkGetImageMemoryRequirements(fl_gfx.device.handle, ret.handle, &memory_requirements);

	VkPhysicalDeviceMemoryProperties physical_device_memory_properties = {0};
	vkGetPhysicalDeviceMemoryProperties(fl_gfx.device.physical.handle, &physical_device_memory_properties);

	vkAllocateMemory(
		fl_gfx.device.handle,
		&(VkMemoryAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memory_requirements.size,
			.memoryTypeIndex = gfx_find_memory_type_index(
				memory_requirements.memoryTypeBits,
				property_flags
			),
		},
		&fl_gfx_allocator,
		&ret.memory
	);

	VkDeviceSize const memory_offset = 0;
	vkBindImageMemory(fl_gfx.device.handle, ret.handle, ret.memory, memory_offset);

	ret.view = gfx_texture_view_create(ret.handle, format, 0);
	return ret;
}

AttrFileLocal()
void gfx_texture_destroy(struct GFX_Texture resource) {
	vkFreeMemory(fl_gfx.device.handle, resource.memory, &fl_gfx_allocator);
	vkDestroyImage(fl_gfx.device.handle, resource.handle, &fl_gfx_allocator);
	gfx_texture_view_destroy(resource.view);
}

AttrFileLocal()
void gfx_texture_transition(
	VkImage image, VkFormat format, u32 extra_mip_levels,
	VkImageLayout old_layout, VkImageLayout new_layout
) {
	// @note transfer queue is not applicable, graphics capabilities are required
	struct GFX_QCmd const context = gfx_get_main_qbuffer(GFX_FRAMES_IN_FLIGHT);

	// universal opening
	vkBeginCommandBuffer(context.commands, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	});

	// image commands
	VkImageAspectFlags dst_aspect_mask;
	switch (new_layout) {
		default:                                       dst_aspect_mask = 0; Breakpoint();           break;
		// color
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:     dst_aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT; break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: dst_aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT; break;
		// depth / stencil
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
			dst_aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
			dst_aspect_mask = VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			dst_aspect_mask = gfx_map_format_to_image_aspect(format);
			break;
	}

	VkPipelineStageFlags src_stage_mask;
	switch (old_layout) {
		default:                                   src_stage_mask = 0; Breakpoint();                   break;
		case VK_IMAGE_LAYOUT_UNDEFINED:            src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;    break;
	}

	VkPipelineStageFlags dst_stage_mask;
	switch (new_layout) {
		default:                                       dst_stage_mask = 0; Breakpoint();                       break;
		// color
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:     dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;        break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; break;
		// depth / stencil
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			dst_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;
	}

	VkAccessFlags src_access_mask;
	switch (old_layout) {
		default:                                   src_access_mask = VK_ACCESS_NONE; Breakpoint(); break;
		case VK_IMAGE_LAYOUT_UNDEFINED:            src_access_mask = VK_ACCESS_NONE;               break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT; break;
	}

	VkAccessFlags dst_access_mask;
	switch (new_layout) {
		default:                                       dst_access_mask = VK_ACCESS_NONE; Breakpoint(); break;
		// color
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:     dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: dst_access_mask = VK_ACCESS_SHADER_READ_BIT;    break;
		// depth / stencil
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
			/**/            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
	}

	vkCmdPipelineBarrier(
		context.commands,
		src_stage_mask, dst_stage_mask,
		0,       // dependency flags
		0, NULL, // memory barrier
		0, NULL, // buffer memory barrier
		1, &(VkImageMemoryBarrier){
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.image = image,
			.subresourceRange = {
				.aspectMask = dst_aspect_mask,
				.baseMipLevel = 0,
				.levelCount = 1 + extra_mip_levels,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.srcAccessMask = src_access_mask,
			.dstAccessMask = dst_access_mask,
		}
	);

	// universal ending
	vkEndCommandBuffer(context.commands);

	vkQueueSubmit(context.queue, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &context.commands,
	}, VK_NULL_HANDLE);

	vkQueueWaitIdle(context.queue);
}

AttrFileLocal()
void gfx_texture_upload(VkImage image, VkBuffer source, uvec2 size, VkFormat format) {
	struct GFX_QCmd const context = gfx_get_transfer_qbuffer();

	// universal opening
	vkBeginCommandBuffer(context.commands, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	});

	// image commands
	vkCmdCopyBufferToImage(context.commands, source, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &(VkBufferImageCopy){
			// buffer
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			// image
			.imageOffset = {0, 0, 0},
			.imageExtent = {size.x, size.y, 1},
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		}
	);

	// universal ending
	vkEndCommandBuffer(context.commands);

	vkQueueSubmit(context.queue, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &context.commands,
	}, VK_NULL_HANDLE);

	vkQueueWaitIdle(context.queue);
}

AttrFileLocal()
bool gfx_texture_generate_mipmaps(VkImage image, VkFormat format, u32 extra_mip_levels, uvec2 size) {
	if (!gfx_is_format_supported(format, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		return false;

	// @note transfer queue is not applicable, graphics capabilities are required
	struct GFX_QCmd const context = gfx_get_main_qbuffer(GFX_FRAMES_IN_FLIGHT);

	// universal opening
	vkBeginCommandBuffer(context.commands, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	});

	// image commands
	int32_t mip_src_width  = (int32_t)size.x;
	int32_t mip_src_height = (int32_t)size.y;
	for (u32 i = 0; i < extra_mip_levels; i++) {
		int32_t const mip_dst_width  = mip_src_width  > 1 ? mip_src_width  / 2 : 1;
		int32_t const mip_dst_height = mip_src_height > 1 ? mip_src_height / 2 : 1;
		vkCmdPipelineBarrier(context.commands,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,       // dependency flags
			0, NULL, // memory barrier
			0, NULL, // buffer memory barrier
			1, &(VkImageMemoryBarrier){
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = i,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
			}
		);
		vkCmdBlitImage(context.commands,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &(VkImageBlit){
				.srcOffsets[0] = (VkOffset3D){0, 0, 0},
				.srcOffsets[1] = (VkOffset3D){mip_src_width, mip_src_height, 1},
				.srcSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.dstOffsets[0] = (VkOffset3D){0, 0, 0},
				.dstOffsets[1] = (VkOffset3D){mip_dst_width, mip_dst_height, 1},
				.dstSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i + 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			},
			VK_FILTER_LINEAR
		);
		mip_src_width  = mip_dst_width;
		mip_src_height = mip_dst_height;
	}

	vkCmdPipelineBarrier(
		context.commands,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,       // dependency flags
		0, NULL, // memory barrier
		0, NULL, // buffer memory barrier
		2, (VkImageMemoryBarrier[]){
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = extra_mip_levels,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			},
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = extra_mip_levels,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			},
		}
	);

	// universal ending
	vkEndCommandBuffer(context.commands);

	vkQueueSubmit(context.queue, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &context.commands,
	}, VK_NULL_HANDLE);

	vkQueueWaitIdle(context.queue);
	return true;
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
			fl_gfx.image_available_semaphores + i
		);
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		vkCreateSemaphore(
			fl_gfx.device.handle,
			&(VkSemaphoreCreateInfo){
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			},
			&fl_gfx_allocator,
			fl_gfx.render_finished_semaphores + i
		);
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		vkCreateFence(
			fl_gfx.device.handle,
			&(VkFenceCreateInfo){
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT,
			},
			&fl_gfx_allocator,
			fl_gfx.in_flight_fences + i
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
	gfx_command_pool_create(fl_gfx.device.physical.qfamily.main - 1, GFX_FRAMES_IN_FLIGHT + 1, &fl_gfx.main_command_pool, fl_gfx.main_command_buffers);
	gfx_command_pool_create(fl_gfx.device.physical.qfamily.transfer - 1, 1, &fl_gfx.transfer_command_pool, &fl_gfx.transfer_command_buffer);
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
	VkAttachmentDescription const attachments[] = {
		[GFX_FRAMEBUFFER_INDEX_COLOR] = (VkAttachmentDescription){
			.format = fl_gfx.device.physical.surface_format.format,
			.samples = fl_gfx.device.physical.samples,
			// layout
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = fl_gfx.device.physical.samples > VK_SAMPLE_COUNT_1_BIT
				? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				: VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			// color and depth operations
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			// stencil operations
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		},
		[GFX_FRAMEBUFFER_INDEX_DEPTH] = (VkAttachmentDescription){
			.format = gfx_find_depth_format(),
			.samples = fl_gfx.device.physical.samples,
			// layout
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			// color and depth operations
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			// stencil operations
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		},
		[GFX_FRAMEBUFFER_INDEX_RSLVE] = (VkAttachmentDescription){
			.format = fl_gfx.device.physical.surface_format.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			// layout
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			// color and depth operations
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			// stencil operations
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		},
	};

	vkCreateRenderPass(
		fl_gfx.device.handle,
		&(VkRenderPassCreateInfo){
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			// attachments
			.attachmentCount = fl_gfx.device.physical.samples > VK_SAMPLE_COUNT_1_BIT ? 3 : 2,
			.pAttachments = attachments,
			// subpasses
			.subpassCount = 1,
			.pSubpasses = &(VkSubpassDescription){
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &(VkAttachmentReference){
					.attachment = GFX_FRAMEBUFFER_INDEX_COLOR,
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				},
				.pDepthStencilAttachment = &(VkAttachmentReference){
					.attachment = GFX_FRAMEBUFFER_INDEX_DEPTH,
					.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				},
				.pResolveAttachments = fl_gfx.device.physical.samples > VK_SAMPLE_COUNT_1_BIT
					? &(VkAttachmentReference){
						.attachment = GFX_FRAMEBUFFER_INDEX_RSLVE,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					}
					: NULL,
			},
			// dependencies
			.dependencyCount = 1,
			.pDependencies = &(VkSubpassDependency){
				// subpasses
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				// source stage masks
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				/**/          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				.srcAccessMask = 0,
				// destination stage masks
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				/**/          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
				/**/           | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
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

	VkSurfaceCapabilitiesKHR surface_capabilities = {0};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(fl_gfx.device.physical.handle, fl_gfx.surface, &surface_capabilities);

	fl_gfx.swapchain.extent = surface_capabilities.currentExtent;
	if (fl_gfx.swapchain.extent.width == UINT32_MAX && fl_gfx.swapchain.extent.height == UINT32_MAX)
		os_surface_get_size(&fl_gfx.swapchain.extent.width, &fl_gfx.swapchain.extent.height);

	if (fl_gfx.swapchain.extent.width == 0 || fl_gfx.swapchain.extent.height == 0)
		return;

	// ---- ---- ---- ----
	// swapchain
	// ---- ---- ---- ----

	arr32 queue_families = {.capacity = 2, .buffer = (uint32_t[2]){0}};
	arr32_append_unique(&queue_families, fl_gfx.device.physical.qfamily.main - 1);
	arr32_append_unique(&queue_families, fl_gfx.device.physical.qfamily.present - 1);

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
		fl_gfx.swapchain.image_views[i] = gfx_texture_view_create(fl_gfx.swapchain.images[i],
			fl_gfx.device.physical.surface_format.format, 0
		);

	// ---- ---- ---- ----
	// color
	// ---- ---- ---- ----

	VkFormat const color_format = fl_gfx.device.physical.surface_format.format;
	fl_gfx.swapchain.color_texture = gfx_texture_create(
		(uvec2){fl_gfx.swapchain.extent.width, fl_gfx.swapchain.extent.height},
		color_format, 0, fl_gfx.device.physical.samples, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// ---- ---- ---- ----
	// depth
	// ---- ---- ---- ----

	VkFormat const depth_format = gfx_find_depth_format();
	fl_gfx.swapchain.depth_texture = gfx_texture_create(
		(uvec2){fl_gfx.swapchain.extent.width, fl_gfx.swapchain.extent.height},
		depth_format, 0, fl_gfx.device.physical.samples, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// ---- ---- ---- ----
	// framebuffers
	// ---- ---- ---- ----

	fl_gfx.swapchain.framebuffers = os_memory_heap(NULL, sizeof(VkFramebuffer) * fl_gfx.swapchain.images_count);
	for (uint32_t i = 0; i < fl_gfx.swapchain.images_count; i++) {
		VkImageView const * attachments = fl_gfx.device.physical.samples > VK_SAMPLE_COUNT_1_BIT
			? (VkImageView[]){
				[GFX_FRAMEBUFFER_INDEX_COLOR] = fl_gfx.swapchain.color_texture.view,
				[GFX_FRAMEBUFFER_INDEX_DEPTH] = fl_gfx.swapchain.depth_texture.view,
				[GFX_FRAMEBUFFER_INDEX_RSLVE] = fl_gfx.swapchain.image_views[i],
			}
			: (VkImageView[]){
				[GFX_FRAMEBUFFER_INDEX_COLOR] = fl_gfx.swapchain.image_views[i],
				[GFX_FRAMEBUFFER_INDEX_DEPTH] = fl_gfx.swapchain.depth_texture.view,
			};
		vkCreateFramebuffer(
			fl_gfx.device.handle,
			&(VkFramebufferCreateInfo){
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = fl_gfx.render_pass,
				.width = fl_gfx.swapchain.extent.width,
				.height = fl_gfx.swapchain.extent.height,
				.layers = 1,
				// attachments
				.attachmentCount = fl_gfx.device.physical.samples > VK_SAMPLE_COUNT_1_BIT ? 3 : 2,
				.pAttachments = attachments,
			},
			&fl_gfx_allocator,
			fl_gfx.swapchain.framebuffers + i
		);
	}
}

AttrFileLocal()
void gfx_swapchain_free(struct GFX_Swapchain swapchain) {
	if (swapchain.handle == VK_NULL_HANDLE)
		return;

	for (uint32_t i = 0; i < swapchain.images_count; i++)
		vkDestroyFramebuffer(fl_gfx.device.handle, swapchain.framebuffers[i], &fl_gfx_allocator);

	gfx_texture_destroy(swapchain.depth_texture);

	gfx_texture_destroy(swapchain.color_texture);

	for (uint32_t i = 0; i < swapchain.images_count; i++)
		gfx_texture_view_destroy(swapchain.image_views[i]);

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
// buffer
// ---- ---- ---- ----

AttrFileLocal()
struct GFX_Buffer gfx_buffer_create(
	VkDeviceSize size,
	VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags
) {
	arr32 queue_families = {.capacity = 2, .buffer = (uint32_t[2]){0}};
	arr32_append_unique(&queue_families, fl_gfx.device.physical.qfamily.main - 1);
	arr32_append_unique(&queue_families, fl_gfx.device.physical.qfamily.transfer - 1);

	struct GFX_Buffer ret = {0};
	vkCreateBuffer(
		fl_gfx.device.handle,
		&(VkBufferCreateInfo){
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			// buffer specific
			.size = size,
			// generic info
			.usage = usage_flags,
			.sharingMode = queue_families.count >= 2
				? VK_SHARING_MODE_CONCURRENT
				: VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = (uint32_t)queue_families.count,
			.pQueueFamilyIndices = queue_families.buffer,
		},
		&fl_gfx_allocator,
		&ret.handle
	);

	VkMemoryRequirements memory_requirements = {0};
	vkGetBufferMemoryRequirements(fl_gfx.device.handle, ret.handle, &memory_requirements);

	vkAllocateMemory(
		fl_gfx.device.handle,
		&(VkMemoryAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memory_requirements.size,
			.memoryTypeIndex = gfx_find_memory_type_index(
				memory_requirements.memoryTypeBits,
				property_flags
			),
		},
		&fl_gfx_allocator,
		&ret.memory
	);

	VkDeviceSize const memory_offset = 0;
	vkBindBufferMemory(fl_gfx.device.handle, ret.handle, ret.memory, memory_offset);
	return ret;
}

AttrFileLocal()
void gfx_buffer_destroy(struct GFX_Buffer resource) {
	vkFreeMemory(fl_gfx.device.handle, resource.memory, &fl_gfx_allocator);
	vkDestroyBuffer(fl_gfx.device.handle, resource.handle, &fl_gfx_allocator);
}

AttrFileLocal()
void gfx_buffer_copy(VkBuffer source, VkBuffer target, VkDeviceSize size) {
	struct GFX_QCmd const context = gfx_get_transfer_qbuffer();

	// universal opening
	vkBeginCommandBuffer(context.commands, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	});

	// buffer commands
	vkCmdCopyBuffer(context.commands, source, target, 1, &(VkBufferCopy){
		.size = size,
	});

	// universal ending
	vkEndCommandBuffer(context.commands);

	vkQueueSubmit(context.queue, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &context.commands,
	}, VK_NULL_HANDLE);

	vkQueueWaitIdle(context.queue);
}

// ---- ---- ---- ----
// dpool
// ---- ---- ---- ----

AttrFileLocal()
void gfx_dpool_init(void) {
	vkCreateDescriptorPool(
		fl_gfx.device.handle,
		&(VkDescriptorPoolCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = GFX_FRAMES_IN_FLIGHT,
			.poolSizeCount = 2,
			.pPoolSizes = (VkDescriptorPoolSize[]){
				(VkDescriptorPoolSize){
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = GFX_FRAMES_IN_FLIGHT,
				},
				(VkDescriptorPoolSize){
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = GFX_FRAMES_IN_FLIGHT,
				},
			},
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		},
		&fl_gfx_allocator,
		&fl_gfx.descriptor_pool
	);
}

AttrFileLocal()
void gfx_dpool_free(void) {
	vkDestroyDescriptorPool(fl_gfx.device.handle, fl_gfx.descriptor_pool, &fl_gfx_allocator);
}

// ---- ---- ---- ----
// pipeline / USER DATA
// ---- ---- ---- ----

AttrFileLocal()
VkShaderModule gfx_shader_module_create(char const * name) {
	struct Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = arena_get_position(scratch);

	arr8 const source = base_file_read(scratch, name);
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
			.module = gfx_shader_module_create("data/shader_vert.spirv"),
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = gfx_shader_module_create("data/shader_frag.spirv"),
			.pName = "main",
		},
	};

	// -- create descriptor set layout
	vkCreateDescriptorSetLayout(
		fl_gfx.device.handle,
		&(VkDescriptorSetLayoutCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 2,
			.pBindings = (VkDescriptorSetLayoutBinding[]){
				(VkDescriptorSetLayoutBinding){
					.binding = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
				},
				(VkDescriptorSetLayoutBinding){
					.binding = 1,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				},
			},
		},
		&fl_gfx_allocator,
		&fl_gfx.pipeline.descriptor_set.layout
	);

	// -- create pipeline layout
	vkCreatePipelineLayout(
		fl_gfx.device.handle,
		&(VkPipelineLayoutCreateInfo){
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &fl_gfx.pipeline.descriptor_set.layout,
		},
		&fl_gfx_allocator,
		&fl_gfx.pipeline.layout
	);

	// -- create graphics pipeline
	VkDynamicState const dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		// VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
		// VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
		// VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
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
					.stride = sizeof(struct FVertex),
					.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
				},
				// attributes
				.vertexAttributeDescriptionCount = 3,
				.pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]){
					{
						.binding = 0,
						.location = 0,
						.format = gfx_map_primitive_format_to_vector_format(VK_FORMAT_R32_SFLOAT, FieldSize(struct FVertex, position) / sizeof(f32)),
						.offset = offsetof(struct FVertex, position),
					},
					{
						.binding = 0,
						.location = 1,
						.format = gfx_map_primitive_format_to_vector_format(VK_FORMAT_R32_SFLOAT, FieldSize(struct FVertex, texture) / sizeof(f32)),
						.offset = offsetof(struct FVertex, texture),
					},
					{
						.binding = 0,
						.location = 2,
						.format = gfx_map_primitive_format_to_vector_format(VK_FORMAT_R32_SFLOAT, FieldSize(struct FVertex, normal) / sizeof(f32)),
						.offset = offsetof(struct FVertex, normal),
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
				.rasterizationSamples = fl_gfx.device.physical.samples,
				.sampleShadingEnable = fl_gfx.device.physical.samples > VK_SAMPLE_COUNT_1_BIT,
				.minSampleShading = fl_gfx.device.physical.samples > VK_SAMPLE_COUNT_1_BIT ? 0.2f : 0,
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
			// depth / stencil
			.pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				// @todo use `VK_DYNAMIC_STATE_*` for these
				.depthTestEnable = VK_TRUE,
				.depthWriteEnable = VK_TRUE,
				.depthCompareOp = GFX_REVERSE_Z
					? VK_COMPARE_OP_GREATER
					: VK_COMPARE_OP_LESS,
			},
		},
		&fl_gfx_allocator,
		&fl_gfx.pipeline.handle
	);

	for (uint32_t i = 0, count = ArrayCount(shader_stages); i < count; i++)
		vkDestroyShaderModule(fl_gfx.device.handle, shader_stages[i].module, &fl_gfx_allocator);

	// -- create descriptor sets
	VkDescriptorSetLayout set_layouts[GFX_FRAMES_IN_FLIGHT];
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		set_layouts[i] = fl_gfx.pipeline.descriptor_set.layout;

	vkAllocateDescriptorSets(
		fl_gfx.device.handle,
		&(VkDescriptorSetAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = fl_gfx.descriptor_pool,
			.descriptorSetCount = GFX_FRAMES_IN_FLIGHT,
			.pSetLayouts = set_layouts,
		},
		fl_gfx.pipeline.descriptor_set.handles
	);
}

AttrFileLocal()
void gfx_graphics_pipeline_free(void) {
	vkDestroyPipeline(fl_gfx.device.handle, fl_gfx.pipeline.handle, &fl_gfx_allocator);
	vkDestroyPipelineLayout(fl_gfx.device.handle, fl_gfx.pipeline.layout, &fl_gfx_allocator);
	vkDestroyDescriptorSetLayout(fl_gfx.device.handle, fl_gfx.pipeline.descriptor_set.layout, &fl_gfx_allocator);
	vkFreeDescriptorSets(fl_gfx.device.handle, fl_gfx.descriptor_pool, GFX_FRAMES_IN_FLIGHT, fl_gfx.pipeline.descriptor_set.handles);
}

// ---- ---- ---- ----
// material / USER DATA
// ---- ---- ---- ----

struct UData {
	mat4 model;
	mat4 view;
	mat4 projection;
};
AssertAlign(struct UData, model,      GFX_ALIGN32_MAT4);
AssertAlign(struct UData, view,       GFX_ALIGN32_MAT4);
AssertAlign(struct UData, projection, GFX_ALIGN32_MAT4);

AttrFileLocal()
void gfx_material_init(void) {
	size_t const uniform_buffer_align = fl_gfx.device.physical.properties.limits.minUniformBufferOffsetAlignment;
	size_t const udata_entry_stride = align_size(sizeof(struct UData), uniform_buffer_align);
	size_t const udata_total_size = udata_entry_stride * GFX_FRAMES_IN_FLIGHT;
	fl_gfx.material.data = gfx_buffer_create(
		udata_total_size,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		// can be mapped                      and memory copy "immediately"
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void * target;
	vkMapMemory(fl_gfx.device.handle, fl_gfx.material.data.memory, 0, udata_total_size, 0, &target);
	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		fl_gfx.material.map[i] = (u8 *)target + udata_entry_stride * i;

	for (uint32_t i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
		vkUpdateDescriptorSets(
			fl_gfx.device.handle,
			2,
			(VkWriteDescriptorSet[]){
				(VkWriteDescriptorSet){
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = fl_gfx.pipeline.descriptor_set.handles[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = 1,
					.pBufferInfo = &(VkDescriptorBufferInfo){
						.buffer = fl_gfx.material.data.handle,
						.offset = udata_entry_stride * i,
						.range = sizeof(struct UData),
					},
				},
				(VkWriteDescriptorSet){
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = fl_gfx.pipeline.descriptor_set.handles[i],
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.pImageInfo = &(VkDescriptorImageInfo){
						.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						.imageView = fl_gfx.texture.view,
						.sampler = fl_gfx.sampler,
					},
				},
			},
			0,
			NULL
		);
}

AttrFileLocal()
void gfx_material_free(void) {
	vkUnmapMemory(fl_gfx.device.handle, fl_gfx.material.data.memory);
	gfx_buffer_destroy(fl_gfx.material.data);
}

// ---- ---- ---- ----
// model / USER DATA
// ---- ---- ---- ----

AttrFileLocal()
void gfx_model_init(void) {
	struct Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = arena_get_position(scratch);

	struct FVertex * vertices; u32 vertices_count;
	u16                 * indices;  u16 indices_count;

	struct File_Model * file_parsed = model_init("../data/viking_room.obj"); // @todo fix path
	model_dump_vertices(file_parsed, scratch, &vertices, &vertices_count, &indices, &indices_count);
	model_free(file_parsed);

	VkDeviceSize const total_size = sizeof(*vertices) * vertices_count + sizeof(*indices) * indices_count;
	fl_gfx.model.offset_vertex = 0;
	fl_gfx.model.offset_index  = sizeof(*vertices) * vertices_count;
	fl_gfx.model.index_count = indices_count;
	fl_gfx.model.index_type  = VK_INDEX_TYPE_UINT16;

	// @todo might be better to use a common allocator for this
	struct GFX_Buffer const staging_buffer = gfx_buffer_create(
		total_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		// can be mapped                      and memory copy "immediately"
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void * target;
	vkMapMemory(fl_gfx.device.handle, staging_buffer.memory, 0, total_size, 0, &target);
	mem_copy(vertices, (u8 *)target + fl_gfx.model.offset_vertex, sizeof(*vertices) * vertices_count);
	mem_copy(indices,  (u8 *)target + fl_gfx.model.offset_index,  sizeof(*indices) * indices_count);
	vkUnmapMemory(fl_gfx.device.handle, staging_buffer.memory);

	fl_gfx.model.data = gfx_buffer_create(
		total_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	gfx_buffer_copy(staging_buffer.handle, fl_gfx.model.data.handle, total_size);

	gfx_buffer_destroy(staging_buffer);

	arena_set_position(scratch, scratch_position);
}

AttrFileLocal()
void gfx_model_free(void) {
	gfx_buffer_destroy(fl_gfx.model.data);
}

// ---- ---- ---- ----
// texture / USER DATA
// ---- ---- ---- ----

AttrFileLocal()
void gfx_texture_init(void) {
	struct Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = arena_get_position(scratch);
	arr8 const file_bytes = base_file_read(scratch, "../data/viking_room.png"); // @todo fix path
	struct File_Image file_parsed = image_init(file_bytes);

	VkDeviceSize const total_size = file_parsed.scalar_size * file_parsed.size.x * file_parsed.size.y * file_parsed.channels;
	VkFormat const primitive = gfx_map_vector_format_to_primitive_format(fl_gfx.device.physical.surface_format.format);
	VkFormat const format = gfx_map_primitive_format_to_vector_format(primitive, file_parsed.channels);

	// @todo might be better to use a common allocator for this
	struct GFX_Buffer const staging_buffer = gfx_buffer_create(
		total_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		// can be mapped                      and memory copy "immediately"
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void * target;
	vkMapMemory(fl_gfx.device.handle, staging_buffer.memory, 0, total_size, 0, &target);
	mem_copy(file_parsed.buffer, target, total_size);
	vkUnmapMemory(fl_gfx.device.handle, staging_buffer.memory);

	u32 const extra_mip_levels = (u32)log2_32((f32)max_u32(file_parsed.size.x, file_parsed.size.y));
	fl_gfx.texture = gfx_texture_create(
		file_parsed.size, format, extra_mip_levels, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	gfx_texture_transition(fl_gfx.texture.handle, format, extra_mip_levels,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);
	gfx_texture_upload(fl_gfx.texture.handle, staging_buffer.handle, file_parsed.size, format);
	if (extra_mip_levels == 0 || !gfx_texture_generate_mipmaps(fl_gfx.texture.handle, format, extra_mip_levels, file_parsed.size))
		gfx_texture_transition(fl_gfx.texture.handle, format, extra_mip_levels,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

	gfx_buffer_destroy(staging_buffer);

	image_free(&file_parsed);
	arena_set_position(scratch, scratch_position);
}

AttrFileLocal()
void gfx_texture_free(void) {
	gfx_texture_destroy(fl_gfx.texture);
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
				gfx_dpool_init();
				gfx_render_pass_init();
					gfx_swapchain_init(VK_NULL_HANDLE);
					gfx_graphics_pipeline_init();
				gfx_model_init();
				fl_gfx.sampler = gfx_sampler_create();
				gfx_texture_init();
					gfx_material_init();
}

void gfx_free(void) {
	vkDeviceWaitIdle(fl_gfx.device.handle);

	// any order within same indentation
	gfx_synchronization_free();
	gfx_command_pool_free();
	gfx_graphics_pipeline_free();
	gfx_dpool_free();
	gfx_model_free();
	gfx_sampler_destroy(fl_gfx.sampler);
	gfx_texture_free();
	gfx_material_free();
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
	struct GFX_QCmd const context                   = gfx_get_main_qbuffer(frame);
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

	uint32_t image_index = 0;
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
	vkResetCommandBuffer(context.commands, 0);

	// ---- ---- ---- ----
	// upload uniforms
	// ---- ---- ---- ----

	{
		f32 const fov = PI32 / 3;
		vec2 const vp_scale = vec2_muls(
			(vec2){(f32)fl_gfx.swapchain.extent.height / (f32)fl_gfx.swapchain.extent.width, 1},
			1 / tan32(fov / 2)
		);
		u64 const rotation_period = AsNanos(10);
		f32 const rotation_offset = TAU32 * (f32)(os_timer_get_nanos() % rotation_period) / (f32)rotation_period;
		f32 const rotation = (PI32 / 10) * cos32(rotation_offset);
		struct UData const udata = {
			.model = mat4_transformation(vec3_0, quat_axis(vec3_y1, rotation), vec3_1),
			.view = mat4_transformation_inverse((vec3){1.2f, 1.8f, -1.2f}, quat_rotation((vec3){PI32/4, -PI32/4, 0}), vec3_1),
			.projection = gfx_mat4_projection(vp_scale, vec2_0, 0, 0.1f, INF32),
		};
		mem_copy(&udata, fl_gfx.material.map[frame], sizeof(udata));
	}

	// ---- ---- ---- ----
	// draw
	// ---- ---- ---- ----

	vkBeginCommandBuffer(context.commands, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	});

	VkClearValue const clear_values[] = {
		[GFX_FRAMEBUFFER_INDEX_COLOR] =(VkClearValue){
			.color.float32 = {0, 0, 0, 1},
		},
		[GFX_FRAMEBUFFER_INDEX_DEPTH] = (VkClearValue){
			.depthStencil = {
				.depth = GFX_REVERSE_Z ? 0 : 1,
			},
		},
	};

	vkCmdBeginRenderPass(context.commands, &(VkRenderPassBeginInfo){
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = fl_gfx.render_pass,
		.framebuffer = fl_gfx.swapchain.framebuffers[image_index],
		.renderArea = {
			.extent = fl_gfx.swapchain.extent,
		},
		// clear
		.clearValueCount = ArrayCount(clear_values),
		.pClearValues = clear_values,
	}, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(context.commands, VK_PIPELINE_BIND_POINT_GRAPHICS, fl_gfx.pipeline.handle);
	vkCmdBindDescriptorSets(
		context.commands,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		fl_gfx.pipeline.layout,
		0, 1,
		&fl_gfx.pipeline.descriptor_set.handles[frame],
		0, NULL
	);

	vkCmdSetViewport(context.commands, 0, 1, &(VkViewport){
		// offset, Y positive up
		.x = (float)0,
		.y = (float)fl_gfx.swapchain.extent.height,
		// scale, Y positive up
		.width = (float)fl_gfx.swapchain.extent.width,
		.height = -(float)fl_gfx.swapchain.extent.height,
		// depth
		.minDepth = GFX_REVERSE_Z ? 1 : 0,
		.maxDepth = GFX_REVERSE_Z ? 0 : 1,
	});
	vkCmdSetScissor(context.commands, 0, 1, &(VkRect2D){
		.extent = fl_gfx.swapchain.extent,
	});

	vkCmdBindVertexBuffers(context.commands, 0, 1, &fl_gfx.model.data.handle, &fl_gfx.model.offset_vertex);
	vkCmdBindIndexBuffer(context.commands, fl_gfx.model.data.handle, fl_gfx.model.offset_index, fl_gfx.model.index_type);
	vkCmdDrawIndexed(context.commands, fl_gfx.model.index_count, 1, 0, 0, 0);

	vkCmdEndRenderPass(context.commands);
	vkEndCommandBuffer(context.commands);

	// ---- ---- ---- ----
	// submit
	// ---- ---- ---- ----

	vkQueueSubmit(context.queue, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		// wait semaphores
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &image_available_semaphore,
		.pWaitDstStageMask = (VkPipelineStageFlags[]){
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		},
		// command buffers
		.commandBufferCount = 1,
		.pCommandBuffers = &context.commands,
		// signal semaphores
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &render_finished_semaphore,
	}, in_flight_fence);

	// ---- ---- ---- ----
	// present
	// ---- ---- ---- ----

	VkResult const queue_present_result =
	vkQueuePresentKHR(fl_gfx.device.queue.present, &(VkPresentInfoKHR){
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
		default: break; // @ignore
		case VK_SUBOPTIMAL_KHR:        // success
		case VK_ERROR_OUT_OF_DATE_KHR: // failure
			fl_gfx.swapchain.out_of_date_or_suboptimal = true;
			break;
	}

	fl_gfx.swapchain.frame = (fl_gfx.swapchain.frame + 1) % GFX_FRAMES_IN_FLIGHT;
}

void gfx_notify_surface_resized(void) {
	fl_gfx.swapchain.out_of_date_or_suboptimal = true;
}

mat4 gfx_mat4_projection(
	vec2 scale_xy, vec2 offset_xy, f32 ortho,
	f32 view_near, f32 view_far
) {
	f32 const ndc_near = 0;
	f32 const ndc_far  = 1;
	return mat4_projection(
		scale_xy, offset_xy, ortho,
		view_near, view_far,
		ndc_near, ndc_far
	);
}

#undef GFX_REVERSE_Z
#undef GFX_FRAMES_IN_FLIGHT
#undef GFX_PREFER_SRGB
#undef GFX_PREFER_VSYNC_OFF
#undef GFX_DEFINE_PROC
#undef GFX_ENABLE_DEBUG

#undef GFX_FRAMEBUFFER_INDEX_COLOR
#undef GFX_FRAMEBUFFER_INDEX_DEPTH

#undef GFX_ALIGN32_SCALAR
#undef GFX_ALIGN32_VEC2
#undef GFX_ALIGN32_VEC3
#undef GFX_ALIGN32_VEC4
#undef GFX_ALIGN32_MAT4
