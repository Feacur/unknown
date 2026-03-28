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

#define RHI_REVERSE_Z 1
#define RHI_DEFINE_PROC(instance, name) PFN_ ## name name = (PFN_ ## name)rhi_get_instance_proc(instance, #name)
#define RHI_ENABLE_DEBUG (BUILD_DEBUG == BUILD_DEBUG_ENABLE)

#define RHI_FRAMEBUFFER_INDEX_COLOR 0
#define RHI_FRAMEBUFFER_INDEX_DEPTH 1
#define RHI_FRAMEBUFFER_INDEX_RSLVE 2

// @note vulkan expects a 64 bit architecture at least
AssertStatic(sizeof(size_t) >= sizeof(uint64_t));

// ---- ---- ---- ----
// stringifiers
// ---- ---- ---- ----

#if RHI_ENABLE_DEBUG
AttrFileLocal()
str8 rhi_to_string_for_debug_utils_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT value) {
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   return str8_lit("ERROR");
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) return str8_lit("WARNING");
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    return str8_lit("INFO");
	if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) return str8_lit("VERBOSE");
	return str8_lit("unknown");
}

AttrFileLocal()
str8 rhi_to_string_for_debug_utils_message_type(VkDebugUtilsMessageTypeFlagsEXT value) {
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT) return str8_lit("Device address binding");
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)            return str8_lit("Performance");
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)             return str8_lit("Validation");
	if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)                return str8_lit("General");
	return str8_lit("unknown");
}

AttrFileLocal()
str8 rhi_to_string_for_allocation_type(VkInternalAllocationType value) {
	switch (value) {
		case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE: return str8_lit("Executable");
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 rhi_to_string_for_allocation_scope(VkSystemAllocationScope value) {
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
str8 rhi_to_string_for_physical_device_type(VkPhysicalDeviceType value) {
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
str8 rhi_to_string_for_format(VkFormat value) {
	switch (value) {
		case VK_FORMAT_UNDEFINED:                return str8_lit("UNDEFINED");
		case VK_FORMAT_R8G8B8A8_UNORM:           return str8_lit("R8 G8 B8 A8 UNORM");
		case VK_FORMAT_R8G8B8A8_SRGB:            return str8_lit("R8 G8 B8 A8 SRGB");
		case VK_FORMAT_B8G8R8A8_UNORM:           return str8_lit("B8 G8 R8 A8 UNORM");
		case VK_FORMAT_B8G8R8A8_SRGB:            return str8_lit("B8 G8 R8 A8 SRGB");
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return str8_lit("A2 B10 G10 R10 UNORM PACK32");
		default: return str8_lit("unknown"); // @ignore @not_implemented
	}
}

AttrFileLocal()
str8 rhi_to_string_for_color_space(VkColorSpaceKHR value) {
	switch (value) {
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return str8_lit("sRGB");
		default: return str8_lit("unknown"); // @ignore @not_implemented
	}
}

AttrFileLocal()
str8 rhi_to_string_for_present_mode(VkPresentModeKHR value) {
	switch (value) {
		case VK_PRESENT_MODE_IMMEDIATE_KHR:                 return str8_lit("Immediate");
		case VK_PRESENT_MODE_MAILBOX_KHR:                   return str8_lit("Mailbox");
		case VK_PRESENT_MODE_FIFO_KHR:                      return str8_lit("FIFO");
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR:              return str8_lit("FIFO relaxed");
		case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:     return str8_lit("Shared demand refresh");
		case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: return str8_lit("Shared continuous refresh");
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 rhi_to_string_for_queue(VkQueueFlagBits value) {
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
str8 rhi_to_string_for_queue_flags(struct Memory_Arena * arena, VkQueueFlags flags) {
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
	str8 ret = {.buffer = MemoryArenaPushArray(arena, u8, capacity)};
	for (uint32_t i = 0; i < ArrayCount(bits); i++) {
		if (flags & (VkQueueFlags)bits[i]) {
			if (ret.count > 0)
				str8_append(&ret, separator);
			str8_append(&ret, rhi_to_string_for_queue(bits[i]));
		}
	}
	return ret;
}

// ---- ---- ---- ----
// convertors
// ---- ---- ---- ----

AttrFileLocal()
VkFormat rhi_map_vector_format_to_primitive_format(enum VkFormat format) {
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
	AssertF(false, "[RHI] not implemented for %d", format);
	return VK_FORMAT_UNDEFINED;
}

AttrFileLocal()
VkFormat rhi_map_primitive_format_to_vector_format(enum VkFormat primitive, size_t count) {
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
	AssertF(false, "[RHI] not implemented for (%d, %zu)", primitive, count);
	return VK_FORMAT_UNDEFINED;
}

AttrFileLocal()
VkImageAspectFlags rhi_map_format_to_image_aspect(VkFormat format) {
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
	AssertF(false, "[RHI] not implemented for %d", format);
	return VK_IMAGE_ASPECT_NONE;
}

AttrFileLocal()
uint32_t rhi_get_image_count_for_present_mode(VkPresentModeKHR value) {
	// @todo check `mailbox` behaviour: current setup yields twice the display refresh rate
	// @todo expose as API
	switch (value) {
		case VK_PRESENT_MODE_IMMEDIATE_KHR:                 return 1;
		case VK_PRESENT_MODE_MAILBOX_KHR:                   return 2;
		case VK_PRESENT_MODE_FIFO_KHR:                      return 2;
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR:              return 2;
		case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:     return 1;
		case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: return 1;
		default: return 0; // @ignore @not_implemented
	}
}

// ---- ---- ---- ----
// common utilities
// ---- ---- ---- ----

AttrFileLocal()
bool rhi_match_extensions(
	uint32_t requested_count, char const * const * requested_set,
	uint32_t available_count, VkExtensionProperties const * avaliable_set) {
	bool available = true;
	for (uint32_t re_i = 0; re_i < requested_count; re_i++) {
		char const * requested = requested_set[re_i];
		for (uint32_t av_i = 0; av_i < available_count; av_i++) {
			char const * available = avaliable_set[av_i].extensionName;
			if (str_equals(requested, available))
				goto found;
		}
		available = false;
		DbgPrintF("[RHI] requested extension \"%s\" is not available\n", requested);
		found:;
	}
	return available;
}

AttrFileLocal()
bool rhi_match_layers(
	uint32_t requested_count, char const * const * requested_set,
	uint32_t available_count, VkLayerProperties const * avaliable_set) {
	bool available = true;
	for (uint32_t re_i = 0; re_i < requested_count; re_i++) {
		char const * requested = requested_set[re_i];
		for (uint32_t av_i = 0; av_i < available_count; av_i++) {
			char const * available = avaliable_set[av_i].layerName;
			if (str_equals(requested, available))
				goto found;
		}
		available = false;
		DbgPrintF("[RHI] requested layer \"%s\" is not available\n", requested);
		found:;
	}
	return available;
}

// ---- ---- ---- ----
// debugging
// ---- ---- ---- ----

#if RHI_ENABLE_DEBUG
AttrFileLocal() VKAPI_PTR
VkBool32 rhi_debug_utils_messenger_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*                                       pUserData) {
	str8 const severity_text = rhi_to_string_for_debug_utils_message_severity(messageSeverity);
	str8 const type_text = rhi_to_string_for_debug_utils_message_type(messageTypes); // @note argument is of flags type, but always has a single one
	fmt_print("[RHI] %.*s %.*s - %s\n",
		(int)severity_text.count, severity_text.buffer,
		(int)type_text.count, type_text.buffer,
		pCallbackData->pMessageIdName);
	fmt_print("      %s\n", pCallbackData->pMessage);
	fmt_print("\n");
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		Breakpoint();
	return VK_FALSE;
}

AttrFileLocal() VKAPI_PTR
void rhi_debug_on_internal_allocation(
	void*                    pUserData,
	size_t                   size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope  allocationScope) {
	str8 const type_text = rhi_to_string_for_allocation_type(allocationType);
	str8 const scope_text = rhi_to_string_for_allocation_scope(allocationScope);
	fmt_print("[RHI] internal allocation %zu %.*s %.*s\n", size,
		(int)type_text.count, type_text.buffer,
		(int)scope_text.count, scope_text.buffer);
	fmt_print("\n");
}

AttrFileLocal() VKAPI_PTR
void rhi_debug_on_internal_free(
	void*                    pUserData,
	size_t                   size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope  allocationScope) {
	str8 const type_text = rhi_to_string_for_allocation_type(allocationType);
	str8 const scope_text = rhi_to_string_for_allocation_scope(allocationScope);
	fmt_print("[RHI] internal free %zu %.*s %.*s\n", size,
		(int)type_text.count, type_text.buffer,
		(int)scope_text.count, scope_text.buffer);
	fmt_print("\n");
}
#endif

// ---- ---- ---- ----
// memory routines
// ---- ---- ---- ----

AttrFileLocal() VKAPI_PTR
void * rhi_memory_heap(void * ptr, size_t size, size_t align) {
	// @note debug info for local structs seems to be broken
	struct Header {u16 offset, align;};
	u16 const max_align = 0x8000; // UINT16_MAX / 2 + 1;
	AssertF(align <= max_align, "[RHI] alignment overflow %zu / %u\n", align, max_align);
	// restore original pointer
	if (ptr != NULL) {
		struct Header const header = *((struct Header *)ptr - 1);
		ptr = (u8 *)ptr - header.offset;
		if (header.align > 0)
			align = header.align;
	}
	// allocate enough space for header and offset
	if (size > 0)
		size += sizeof(struct Header) + (align - 1);
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
void * rhi_memory_allocate(
	void*                   pUserData,
	size_t                  size,
	size_t                  alignment,
	VkSystemAllocationScope allocationScope) {
	return rhi_memory_heap(NULL, size, alignment);
}

AttrFileLocal() VKAPI_PTR
void * rhi_memory_reallocate(
	void*                   pUserData,
	void*                   pOriginal,
	size_t                  size,
	size_t                  alignment,
	VkSystemAllocationScope allocationScope) {
	return rhi_memory_heap(pOriginal, size, alignment);
}

AttrFileLocal() VKAPI_PTR
void rhi_memory_free(
	void* pUserData,
	void* pMemory) {
	rhi_memory_heap(pMemory, 0, 0);
}

// ---- ---- ---- ----
// implementation
// ---- ---- ---- ----

#include "rhi.h"

struct RHI_QFamily { // @note values are `index + 1`
	uint32_t graphics;
	uint32_t transfer;
	uint32_t present;
};

struct RHI_Queues {
	VkQueue graphics;
	VkQueue transfer;
	VkQueue present;
};

struct RHI_QCmd {
	VkQueue queue;
	VkCommandBuffer commands; // @todo one per thread
};

struct RHI_Buffer {
	VkBuffer       handle;
	VkDeviceMemory memory;
	VkDeviceSize   size;
};

struct RHI_Texture {
	VkImageView    view;
	VkImage        handle;
	VkDeviceMemory memory;
};

struct RHI_Frame {
	// @note there's a discrepancy between `frame_current` and `presentable_index`
	// the first one iterates commands on the "client side", while the
	// second one points to an image that is expected to be presented;
	// it's still physically possible to ignore all that, but will lead
	// to missed presentation opportunities and possible performance issues
	VkFramebuffer   buffer;
	VkImage         image;
	VkImageView     view;
	// @note albeit these two blocks are grouped together for simplicity reasons,
	// they are not expected to be in sync whatsoever; additionlly, even with a lot
	// of images requested, it's possible to underutilize frames, leaving them idle
	VkCommandBuffer commands;
	VkSemaphore     semaphore_acquired;
	VkSemaphore     semaphore_rendered;
	VkFence         fence_executed;
};

AttrFileLocal()
struct RHI_Context { // persistent
	// instance
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_utils_messenger;
	// surface
	VkSurfaceKHR surface;
	// physical
	struct RHI_Device_Physical {
		VkPhysicalDevice handle;
		// cached
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceMemoryProperties memory_properties;
		// choices
		struct RHI_QFamily qfamily;
		VkSurfaceFormatKHR surface_format;
		VkPresentModeKHR present_mode;
		VkSampleCountFlagBits samples;
	} physical;
	// logical
	struct RHI_Device_Logical {
		VkDevice handle;
		struct RHI_Queues queue;
	} logical;
	// graphics command pool
	VkCommandPool   graphics_command_pool;
	VkCommandBuffer graphics_command_buffer;
	// transfer command pool
	VkCommandPool   transfer_command_pool;
	VkCommandBuffer transfer_command_buffer;
	// render pass
	VkRenderPass render_pass;
} fl_rhi_context;

AttrFileLocal()
struct RHI_Swapchain { // might be reinitialized
	VkExtent2D       extent;
	VkSwapchainKHR   handle;
	bool             needs_update;
	// frames
	struct RHI_Texture   color_texture;
	struct RHI_Texture   depth_texture;
	uint32_t             frame_current;
	uint32_t             frames_count;
	struct RHI_Frame   * frames;
} fl_rhi_swapchain;

AttrFileLocal()
struct RHI_User_Data { // user data
	// sampler
	VkSampler sampler;
	// shader (and more)
	struct RHI_Shader {
		VkPipeline       handle;
		VkPipelineLayout layout;
		struct RHI_Descriptor { // @note might be reused between shaders (?)
			VkDescriptorSetLayout layout;
			VkDescriptorPool      pool;
		} descriptor;
	} shader;
	// material
	struct RHI_Material {
		VkDescriptorSet * handles;
		u8 ** map;
		struct RHI_Buffer data;
	} material;
	// model
	struct RHI_Model {
		struct RHI_Buffer data;
		VkDeviceSize vertex_offset;
		VkDeviceSize index_offset;
		VkIndexType index_type;
		uint32_t    index_count;
	} model;
	// texture
	struct RHI_Texture texture;
} fl_rhi_ud;

AttrFileLocal()
VkAllocationCallbacks const fl_rhi_allocator = (VkAllocationCallbacks){
	.pUserData = &fl_rhi_context,
	.pfnAllocation   = rhi_memory_allocate,
	.pfnReallocation = rhi_memory_reallocate,
	.pfnFree         = rhi_memory_free,
	#if RHI_ENABLE_DEBUG
	.pfnInternalAllocation = rhi_debug_on_internal_allocation,
	.pfnInternalFree       = rhi_debug_on_internal_free,
	#endif
};

AttrFileLocal()
void * rhi_get_instance_proc(VkInstance instance, char const * name) {
	void * ret = (void *)vkGetInstanceProcAddr(instance, name);
	AssertF(ret, "[RHI] `vkGetInstanceProcAddr(0x%p, \"%s\")` failed\n", (void *)instance, name);
	return ret;
}

// ---- ---- ---- ----
// RHI utilities
// ---- ---- ---- ----

AttrFileLocal()
uint32_t rhi_find_memory_type_index(uint32_t bits, VkMemoryPropertyFlags flags) {
	for (uint32_t i = 0; i < fl_rhi_context.physical.memory_properties.memoryTypeCount; i++) {
		VkMemoryType const it = fl_rhi_context.physical.memory_properties.memoryTypes[i];
		uint32_t const mask = 1 << i;
		bool const memory_type_is_supported = mask & bits;
		bool const has_requested_properties = (it.propertyFlags & flags) == flags;
		if (memory_type_is_supported && has_requested_properties)
			return i;
	}
	AssertF(false, "[RHI] can't find memory property with bits %#b and flags %#b\n", bits, flags);
	return 0;
}

AttrFileLocal()
VkSampleCountFlagBits rhi_device_find_max_sample_count(VkPhysicalDeviceLimits const * all_limits) {
	AttrFuncLocal() VkSampleCountFlagBits const candidates[] = {
		VK_SAMPLE_COUNT_64_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_2_BIT,
	};
	VkFlags const limits = all_limits->framebufferColorSampleCounts
	/**/                 & all_limits->framebufferDepthSampleCounts;
	for (uint32_t i = 0; i < ArrayCount(candidates); i++)
		if (limits & (VkFlags)candidates[i])
			return candidates[i];
	return VK_SAMPLE_COUNT_1_BIT;
}

AttrFileLocal()
bool rhi_is_format_supported(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) {
	VkFormatProperties properties = {0};
	vkGetPhysicalDeviceFormatProperties(fl_rhi_context.physical.handle, format, &properties);
	switch (tiling) {
		default: return false; // @ignore @not_implemented
		case VK_IMAGE_TILING_LINEAR:  return (properties.linearTilingFeatures  & features) == features;
		case VK_IMAGE_TILING_OPTIMAL: return (properties.optimalTilingFeatures & features) == features;
	}
}

AttrFileLocal()
VkFormat rhi_find_depth_format(void) {
	AttrFuncLocal() VkFormat const candidates[] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
	};
	VkImageTiling        const tiling   = VK_IMAGE_TILING_OPTIMAL;
	VkFormatFeatureFlags const features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	for (uint32_t i = 0; i < ArrayCount(candidates); i++)
		if (rhi_is_format_supported(candidates[i], tiling, features))
			return candidates[i];
	Assert(false, "[RHI] can't find depth format\n");
	return VK_FORMAT_UNDEFINED;
}

AttrFileLocal()
struct RHI_QCmd rhi_get_graphics_qbuffer(void) {
	return (struct RHI_QCmd){
		.queue    = fl_rhi_context.logical.queue.graphics,
		.commands = fl_rhi_context.graphics_command_buffer,
	};
}

AttrFileLocal()
struct RHI_QCmd rhi_get_transfer_qbuffer(void) {
	return (struct RHI_QCmd){
		.queue    = fl_rhi_context.logical.queue.transfer,
		.commands = fl_rhi_context.transfer_command_buffer,
	};
}

AttrFileLocal()
void rhi_transfer_begin(struct RHI_QCmd context) {
	vkBeginCommandBuffer(context.commands, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	});
}

AttrFileLocal()
void rhi_transfer_end(struct RHI_QCmd context) {
	vkEndCommandBuffer(context.commands);
	vkQueueSubmit(context.queue, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1, .pCommandBuffers = &context.commands,
	}, VK_NULL_HANDLE);
	// @todo don't halt the thread
	vkQueueWaitIdle(context.queue);
}

// ---- ---- ---- ----
// instance
// ---- ---- ---- ----

AttrFileLocal()
VkInstance rhi_instance_init(void) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	VkInstance ret = VK_NULL_HANDLE;

	// ---- ---- ---- ----
	// debug
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

		fmt_print("[RHI] vulkan:\n");
		fmt_print("- [%u] v%u.%u.%u\n", v[0], v[1], v[2], v[3]);
		fmt_print("\n");
	}

	// ---- ---- ---- ----
	// collect available extensions
	// ---- ---- ---- ----

	uint32_t available_extensions_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, NULL);
	VkExtensionProperties * available_extension = MemoryArenaPushArray(scratch, VkExtensionProperties, available_extensions_count);
	vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, available_extension);

	// ---- ---- ---- ----
	// collect available layers
	// ---- ---- ---- ----

	uint32_t available_layers_count = 0;
	vkEnumerateInstanceLayerProperties(&available_layers_count, NULL);
	VkLayerProperties * available_layers = MemoryArenaPushArray(scratch, VkLayerProperties, available_layers_count);
	vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers);

	// ---- ---- ---- ----
	// debug
	// ---- ---- ---- ----

	fmt_print("[RHI] available instance extensions (%u):\n", available_extensions_count);
	for (uint32_t i = 0; i < available_extensions_count; i++) {
		VkExtensionProperties const it = available_extension[i];
		fmt_print("- %s\n", it.extensionName);
	}
	fmt_print("\n");

	fmt_print("[RHI] available instance layers (%u):\n", available_layers_count);
	for (uint32_t i = 0; i < available_layers_count; i++) {
		VkLayerProperties const it = available_layers[i];

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
	// request and verify extensions
	// ---- ---- ---- ----

	uint32_t required_extensions_count = 0;
	char const * required_extensions[4] = {0};
	required_extensions[required_extensions_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
	os_vulkan_push_extensions(&required_extensions_count, required_extensions);
	#if RHI_ENABLE_DEBUG
	required_extensions[required_extensions_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	#endif

	bool const extensions_match = rhi_match_extensions(
		required_extensions_count, required_extensions,
		available_extensions_count, available_extension
	);

	if (!extensions_match) {
		Assert(false, "[RHI] can't satisfy requested extensions set\n\n");
		goto cleanup;
	}

	// ---- ---- ---- ----
	// request and verify layers
	// ---- ---- ---- ----

	uint32_t required_layers_count = 0;
	char const * required_layers[1] = {0};
	#if RHI_ENABLE_DEBUG
	required_layers[required_layers_count++] = "VK_LAYER_KHRONOS_validation";
	#endif

	bool const layers_match = rhi_match_layers(
		required_layers_count, required_layers,
		available_layers_count, available_layers
	);

	if (!layers_match) {
		Assert(false, "[RHI] can't satisfy requested layers set\n\n");
		goto cleanup;
	}

	// ---- ---- ---- ----
	// form debug params
	// ---- ---- ---- ----

	#if RHI_ENABLE_DEBUG
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
		.pfnUserCallback = rhi_debug_utils_messenger_callback,
	};
	#endif

	// ---- ---- ---- ----
	// instance
	// ---- ---- ---- ----

	vkCreateInstance(
		&(VkInstanceCreateInfo){
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.enabledExtensionCount = required_extensions_count, .ppEnabledExtensionNames = required_extensions,
			.enabledLayerCount     = required_layers_count,     .ppEnabledLayerNames     = required_layers,
			// application
			.pApplicationInfo = &(VkApplicationInfo){
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				// application
				.pApplicationName = "rhi_unknown_vk_application",
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				// engine
				.pEngineName = "rhi_unknown_vk_engine",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				// API
				.apiVersion = VK_API_VERSION_1_4,
			},
			// enable lifetime debug
			#if RHI_ENABLE_DEBUG
			.pNext = debug_utils_messenger_create_info,
			#endif
		},
		&fl_rhi_allocator,
		&ret
	);

	// ---- ---- ---- ----
	// enable general debug
	// ---- ---- ---- ----

	#if RHI_ENABLE_DEBUG
	RHI_DEFINE_PROC(ret, vkCreateDebugUtilsMessengerEXT);
	vkCreateDebugUtilsMessengerEXT(
		ret,
		debug_utils_messenger_create_info,
		&fl_rhi_allocator,
		&fl_rhi_context.debug_utils_messenger
	);
	#endif

	// ---- ---- ---- ----
	// cleanup
	// ---- ---- ---- ----

	cleanup:;
	memory_arena_set_position(scratch, scratch_position);
	return ret;
}

AttrFileLocal()
void rhi_instance_free(VkInstance handle) {
	#if RHI_ENABLE_DEBUG
	RHI_DEFINE_PROC(handle, vkDestroyDebugUtilsMessengerEXT);
	vkDestroyDebugUtilsMessengerEXT(
		handle,
		fl_rhi_context.debug_utils_messenger,
		&fl_rhi_allocator
	);
	#endif

	vkDestroyInstance(handle, &fl_rhi_allocator);
}

// ---- ---- ---- ----
// device
// ---- ---- ---- ----

AttrFileLocal()
struct RHI_QFamily rhi_device_choose_qfamily(VkPhysicalDevice device, VkSurfaceKHR surface) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	uint32_t qfamilies_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qfamilies_count, NULL);
	VkQueueFamilyProperties * qfamilies = MemoryArenaPushArray(scratch, VkQueueFamilyProperties, qfamilies_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qfamilies_count, qfamilies);

	VkBool32 * surface_supports = MemoryArenaPushArray(scratch, VkBool32, qfamilies_count);
	for (uint32_t i = 0; i < qfamilies_count; i++)
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, surface_supports + i);

	struct RHI_QFamily ret = {0};

	// -- choose queue family: prefer a main queue that can present
	for (uint32_t i = 0; i < qfamilies_count; i++) {
		VkQueueFamilyProperties const it = qfamilies[i];
		if (!(it.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			continue;

		VkBool32 const surface_support = surface_supports[i];
		if (!surface_support)
			continue;

		ret.graphics = i + 1;
		ret.present  = i + 1;
		break;
	}

	// -- choose queue family: prefer a free-standing transfer queue
	for (uint32_t i = 0; i < qfamilies_count; i++) {
		VkQueueFamilyProperties const it = qfamilies[i];

		if (!(it.queueFlags & VK_QUEUE_TRANSFER_BIT))
			continue;

		if (it.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
			continue;

		ret.transfer = i + 1;
		break;
	}

	// -- choose queue family: fallback to first matching
	for (uint32_t i = 0; i < qfamilies_count && !ret.graphics; i++) {
		VkQueueFamilyProperties const it = qfamilies[i];
		if (it.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			ret.graphics = i + 1;
	}

	for (uint32_t i = 0; i < qfamilies_count && !ret.present; i++) {
		VkBool32 const surface_support = surface_supports[i];
		if (surface_support)
			ret.present = i + 1;
	}

	for (uint32_t i = 0; i < qfamilies_count && !ret.transfer; i++) {
		VkQueueFamilyProperties const it = qfamilies[i];
		if (it.queueFlags & VK_QUEUE_TRANSFER_BIT)
			ret.transfer = i + 1;
	}

	memory_arena_set_position(scratch, scratch_position);
	return ret;
}

AttrFileLocal()
VkFormat const fl_surface_format_preferences[] = {
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_B8G8R8A8_UNORM,
	// @bug RivaTuner Statistics Server doesn't work great with sRGB formats
	VK_FORMAT_R8G8B8A8_SRGB,
	VK_FORMAT_B8G8R8A8_SRGB,
};

AttrFileLocal()
VkSurfaceFormatKHR rhi_device_choose_surface_format(VkPhysicalDevice device, VkSurfaceKHR surface) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	uint32_t surface_formats_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_formats_count, NULL);
	VkSurfaceFormatKHR * surface_formats = MemoryArenaPushArray(scratch, VkSurfaceFormatKHR, surface_formats_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_formats_count, surface_formats);

	// -- choose surface format
	VkSurfaceFormatKHR ret = (VkSurfaceFormatKHR){
		.format     = VK_FORMAT_MAX_ENUM,
		.colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR,
	};

	for (uint32_t i = 0; i < ArrayCount(fl_surface_format_preferences) && ret.format == VK_FORMAT_MAX_ENUM; i++) {
		VkFormat const preference = fl_surface_format_preferences[i];
		for (uint32_t i = 0; i < surface_formats_count && ret.format == VK_FORMAT_MAX_ENUM; i++)
			if (surface_formats[i].format == preference)
				ret = surface_formats[i];
	}
	if (ret.format == VK_FORMAT_MAX_ENUM)
		ret = surface_formats[0];

	memory_arena_set_position(scratch, scratch_position);
	return ret;
}

AttrFileLocal()
VkPresentModeKHR const fl_present_mode_preferences[] = { // @note see swapchain's `.minImageCount`
	VK_PRESENT_MODE_MAILBOX_KHR,      // vblank wait, replace frames
	VK_PRESENT_MODE_IMMEDIATE_KHR,    // vblank skip, present as fast as possible
	VK_PRESENT_MODE_FIFO_RELAXED_KHR, // fifo if full, immediate if empty
	VK_PRESENT_MODE_FIFO_KHR,         // vblank wait, queue frames
};

AttrFileLocal()
VkPresentModeKHR rhi_device_choose_present_mode(VkPhysicalDevice device, VkSurfaceKHR surface) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	uint32_t present_modes_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, NULL);
	VkPresentModeKHR * present_modes = MemoryArenaPushArray(scratch, VkPresentModeKHR, present_modes_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, present_modes);

	// -- choose present mode
	VkPresentModeKHR ret = VK_PRESENT_MODE_MAX_ENUM_KHR; // @todo expose as API

	for (uint32_t i = 0; i < ArrayCount(fl_present_mode_preferences) && ret == VK_PRESENT_MODE_MAX_ENUM_KHR; i++) {
		VkPresentModeKHR const preference = fl_present_mode_preferences[i];
		for (uint32_t i = 0; i < present_modes_count && ret == VK_PRESENT_MODE_MAX_ENUM_KHR; i++)
			if (present_modes[i] == preference)
				ret = present_modes[i];
	}
	if (ret == VK_PRESENT_MODE_MAX_ENUM_KHR)
		ret = VK_PRESENT_MODE_FIFO_KHR; // @note This is the only value of presentMode that is required to be supported.

	memory_arena_set_position(scratch, scratch_position);
	return ret;
}

AttrFileLocal()
char const * const fl_device_required_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	// VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
	// VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

AttrFileLocal()
struct RHI_Device_Physical rhi_device_collect_info(VkPhysicalDevice device, VkSurfaceKHR surface) {
	struct RHI_Device_Physical ret = {
		.handle = device,
	};

	vkGetPhysicalDeviceProperties(device, &ret.properties);
	vkGetPhysicalDeviceMemoryProperties(device, &ret.memory_properties);

	ret.qfamily        = rhi_device_choose_qfamily(device, surface);
	ret.surface_format = rhi_device_choose_surface_format(device, surface);
	ret.present_mode   = rhi_device_choose_present_mode(device, surface);
	ret.samples        = rhi_device_find_max_sample_count(&ret.properties.limits);

	return ret;
}

AttrFileLocal()
struct RHI_Device_Logical rhi_device_init(struct RHI_Device_Physical const * device) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	uint32_t available_extensions_count = 0;
	vkEnumerateDeviceExtensionProperties(device->handle, NULL, &available_extensions_count, NULL);
	VkExtensionProperties * available_extensions = MemoryArenaPushArray(scratch, VkExtensionProperties, available_extensions_count);
	vkEnumerateDeviceExtensionProperties(device->handle, NULL, &available_extensions_count, available_extensions);

	bool const valid = device->qfamily.graphics && device->qfamily.present && device->qfamily.transfer
		&& rhi_match_extensions(
			ArrayCount(fl_device_required_extensions), fl_device_required_extensions,
			available_extensions_count, available_extensions
		);

	struct RHI_Device_Logical ret = {0};
	if (valid) {
		arr32 queue_families = {.capacity = 4, .buffer = (uint32_t[4]){0}};
		arr32_append_unique(&queue_families, device->qfamily.graphics - 1);
		arr32_append_unique(&queue_families, device->qfamily.transfer - 1);
		arr32_append_unique(&queue_families, device->qfamily.present  - 1);

		float const queue_priorities = 1;
		VkDeviceQueueCreateInfo queue_infos[4];
		for (uint32_t i = 0; i < queue_families.count; i++)
			queue_infos[i] = (VkDeviceQueueCreateInfo){
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queue_families.buffer[i],
				.queueCount = 1, .pQueuePriorities = &queue_priorities,
			};

		vkCreateDevice(
			device->handle,
			&(VkDeviceCreateInfo){
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.queueCreateInfoCount  = (uint32_t)queue_families.count,            .pQueueCreateInfos       = queue_infos,
				.enabledExtensionCount = ArrayCount(fl_device_required_extensions), .ppEnabledExtensionNames = fl_device_required_extensions,
				// features
				.pEnabledFeatures = &(VkPhysicalDeviceFeatures){ // @todo expose as API
					.samplerAnisotropy = device->properties.limits.maxSamplerAnisotropy > 1,
					.sampleRateShading = device->samples > VK_SAMPLE_COUNT_1_BIT,
				},
			},
			&fl_rhi_allocator,
			&ret.handle
		);

		vkGetDeviceQueue(ret.handle, device->qfamily.graphics - 1, 0, &ret.queue.graphics);
		vkGetDeviceQueue(ret.handle, device->qfamily.transfer - 1, 0, &ret.queue.transfer);
		vkGetDeviceQueue(ret.handle, device->qfamily.present  - 1, 0, &ret.queue.present);
	}

	// -- cleanup
	memory_arena_set_position(scratch, scratch_position);
	return ret;
}

AttrFileLocal()
void rhi_device_free(VkDevice handle) {
	vkDestroyDevice(handle, &fl_rhi_allocator);
}

AttrFileLocal()
void rhi_device_find_and_create(VkInstance instance, VkSurfaceKHR surface) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	struct RHI_Device_Logical ret = {0};

	// -- collect all devices
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(instance, &count, NULL);
	VkPhysicalDevice * handles = MemoryArenaPushArray(scratch, VkPhysicalDevice, count);
	vkEnumeratePhysicalDevices(instance, &count, handles);

	if (count == 0) {
		Assert(false, "[RHI] physical devices missing\n");
		goto cleanup;
	}

	// -- choose first suitable
	for (uint32_t i = 0; i < count; i++) {
		// @note it's possible to forcibly prefer, say, a discrete GPU, but better to default
		// with a first suitable one, giving a chance to the user to sort priorities manually
		// namely with the "NVIDIA Control Panel" or the "AMD Software: Adrenalin Edition"
		fl_rhi_context.physical = rhi_device_collect_info(handles[i], surface);
		fl_rhi_context.logical = rhi_device_init(&fl_rhi_context.physical);
		if (fl_rhi_context.logical.handle != VK_NULL_HANDLE)
			goto cleanup;
	}

	// -- cleanup
	cleanup:;
	memory_arena_set_position(scratch, scratch_position);
}

// ---- ---- ---- ----
// command pools
// ---- ---- ---- ----

AttrFileLocal()
void rhi_command_pool_create(
	uint32_t qfamily_index, uint32_t buffers_count,
	VkCommandPool * out_pool, VkCommandBuffer * out_buffers
) {
	vkCreateCommandPool(
		fl_rhi_context.logical.handle,
		&(VkCommandPoolCreateInfo){
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = qfamily_index,
		},
		&fl_rhi_allocator,
		out_pool
	);

	// @idea should they be allocated on demand for some reason ?
	vkAllocateCommandBuffers(
		fl_rhi_context.logical.handle,
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
void rhi_command_pool_destroy(VkCommandPool command_pool) {
	vkDestroyCommandPool(fl_rhi_context.logical.handle, command_pool, &fl_rhi_allocator);
}

AttrFileLocal()
void rhi_command_pool_init(void) {
	rhi_command_pool_create(fl_rhi_context.physical.qfamily.graphics - 1, 1, &fl_rhi_context.graphics_command_pool, &fl_rhi_context.graphics_command_buffer);
	rhi_command_pool_create(fl_rhi_context.physical.qfamily.transfer - 1, 1, &fl_rhi_context.transfer_command_pool, &fl_rhi_context.transfer_command_buffer);
}

AttrFileLocal()
void rhi_command_pool_free(void) {
	rhi_command_pool_destroy(fl_rhi_context.graphics_command_pool);
	rhi_command_pool_destroy(fl_rhi_context.transfer_command_pool);
}

// ---- ---- ---- ----
// render pass
// ---- ---- ---- ----

AttrFileLocal()
void rhi_render_pass_init(void) {
	uint32_t const attachments_count = fl_rhi_context.physical.samples > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
	VkAttachmentDescription const attachments[] = {
		[RHI_FRAMEBUFFER_INDEX_COLOR] = (VkAttachmentDescription){
			.format = fl_rhi_context.physical.surface_format.format,
			.samples = fl_rhi_context.physical.samples,
			// layout
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = fl_rhi_context.physical.samples > VK_SAMPLE_COUNT_1_BIT
				? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				: VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			// color and depth operations
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			// stencil operations
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		},
		[RHI_FRAMEBUFFER_INDEX_DEPTH] = (VkAttachmentDescription){
			.format = rhi_find_depth_format(),
			.samples = fl_rhi_context.physical.samples,
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
		[RHI_FRAMEBUFFER_INDEX_RSLVE] = (VkAttachmentDescription){
			.format = fl_rhi_context.physical.surface_format.format,
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
		fl_rhi_context.logical.handle,
		&(VkRenderPassCreateInfo){
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			// attachments
			.attachmentCount = attachments_count, .pAttachments = attachments,
			// subpasses
			.subpassCount = 1, .pSubpasses = &(VkSubpassDescription){
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1, .pColorAttachments = &(VkAttachmentReference){
					.attachment = RHI_FRAMEBUFFER_INDEX_COLOR,
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				},
				.pDepthStencilAttachment = &(VkAttachmentReference){
					.attachment = RHI_FRAMEBUFFER_INDEX_DEPTH,
					.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				},
				.pResolveAttachments = fl_rhi_context.physical.samples > VK_SAMPLE_COUNT_1_BIT
					? &(VkAttachmentReference){
						.attachment = RHI_FRAMEBUFFER_INDEX_RSLVE,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					}
					: NULL,
			},
			// dependencies
			.dependencyCount = 1, .pDependencies = &(VkSubpassDependency){
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
		&fl_rhi_allocator,
		&fl_rhi_context.render_pass
	);
}

AttrFileLocal()
void rhi_render_pass_free(void) {
	vkDestroyRenderPass(fl_rhi_context.logical.handle, fl_rhi_context.render_pass, &fl_rhi_allocator);
}

// ---- ---- ---- ----
// image view
// ---- ---- ---- ----

AttrFileLocal()
VkImageView rhi_texture_view_create(VkImage image, VkFormat format, u32 extra_mip_levels) {
	VkImageViewType    const view_type = VK_IMAGE_VIEW_TYPE_2D; // @todo expose as API
	VkImageAspectFlags const aspect    = rhi_map_format_to_image_aspect(format);
	VkImageView ret = VK_NULL_HANDLE;
	vkCreateImageView(
		fl_rhi_context.logical.handle,
		&(VkImageViewCreateInfo){
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = view_type,
			.format = format,
			.subresourceRange = {
				.aspectMask = aspect,
				// mip map
				.levelCount = 1 + extra_mip_levels,
				// layers
				.layerCount = 1,
			},
		},
		&fl_rhi_allocator,
		&ret
	);
	return ret;
}

AttrFileLocal()
void rhi_texture_view_destroy(VkImageView image_view) {
	vkDestroyImageView(fl_rhi_context.logical.handle, image_view, &fl_rhi_allocator);
}

// ---- ---- ---- ----
// texture
// ---- ---- ---- ----

AttrFileLocal()
struct RHI_Texture rhi_texture_create(
	uvec2 size, VkFormat format, u32 extra_mip_levels, VkSampleCountFlagBits samples, VkImageTiling tiling,
	VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags
) {
	arr32 queue_families = {.capacity = 2, .buffer = (uint32_t[2]){0}};
	arr32_append_unique(&queue_families, fl_rhi_context.physical.qfamily.graphics - 1);
	arr32_append_unique(&queue_families, fl_rhi_context.physical.qfamily.transfer - 1);

	struct RHI_Texture ret = {0};
	vkCreateImage(
		fl_rhi_context.logical.handle,
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
			.queueFamilyIndexCount = (uint32_t)queue_families.count, .pQueueFamilyIndices = queue_families.buffer,
		},
		&fl_rhi_allocator,
		&ret.handle
	);

	VkMemoryRequirements memory_requirements = {0};
	vkGetImageMemoryRequirements(fl_rhi_context.logical.handle, ret.handle, &memory_requirements);

	VkPhysicalDeviceMemoryProperties physical_device_memory_properties = {0};
	vkGetPhysicalDeviceMemoryProperties(fl_rhi_context.physical.handle, &physical_device_memory_properties);

	vkAllocateMemory(
		fl_rhi_context.logical.handle,
		&(VkMemoryAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memory_requirements.size,
			.memoryTypeIndex = rhi_find_memory_type_index(
				memory_requirements.memoryTypeBits,
				property_flags
			),
		},
		&fl_rhi_allocator,
		&ret.memory
	);

	VkDeviceSize const memory_offset = 0;
	vkBindImageMemory(fl_rhi_context.logical.handle, ret.handle, ret.memory, memory_offset);

	ret.view = rhi_texture_view_create(ret.handle, format, 0);
	return ret;
}

AttrFileLocal()
void rhi_texture_destroy(struct RHI_Texture resource) {
	vkFreeMemory(fl_rhi_context.logical.handle, resource.memory, &fl_rhi_allocator);
	vkDestroyImage(fl_rhi_context.logical.handle, resource.handle, &fl_rhi_allocator);
	rhi_texture_view_destroy(resource.view);
}

AttrFileLocal()
void rhi_texture_transition(
	VkImage image, VkFormat format, u32 extra_mip_levels,
	VkImageLayout old_layout, VkImageLayout new_layout
) {
	struct RHI_QCmd const context = rhi_get_transfer_qbuffer();
	rhi_transfer_begin(context);

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
			dst_aspect_mask = rhi_map_format_to_image_aspect(format);
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

	rhi_transfer_end(context);
}

AttrFileLocal()
void rhi_texture_upload(VkImage image, VkBuffer source, uvec2 size, VkFormat format) {
	struct RHI_QCmd const context = rhi_get_transfer_qbuffer();
	rhi_transfer_begin(context);

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

	rhi_transfer_end(context);
}

AttrFileLocal()
bool rhi_texture_generate_mipmaps(VkImage image, VkFormat format, u32 extra_mip_levels, uvec2 size) {
	VkImageTiling        const tiling   = VK_IMAGE_TILING_OPTIMAL;
	VkFormatFeatureFlags const features = VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
	if (!rhi_is_format_supported(format, tiling, features))
		return false;

	// @note transfer queue is not applicable, graphics capabilities are required
	struct RHI_QCmd const context = rhi_get_graphics_qbuffer();
	rhi_transfer_begin(context);

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

	rhi_transfer_end(context);
	return true;
}

// ---- ---- ---- ----
// swapchain
// ---- ---- ---- ----

AttrFileLocal()
struct RHI_Frame rhi_frame_init(struct RHI_Swapchain const * swapchain, VkImage image) {
	struct RHI_Frame ret = {0};

	ret.view = rhi_texture_view_create(image, fl_rhi_context.physical.surface_format.format, 0);
	{
		uint32_t const attachments_count = fl_rhi_context.physical.samples > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
		VkImageView const * attachments = fl_rhi_context.physical.samples > VK_SAMPLE_COUNT_1_BIT
			? (VkImageView[]){
				[RHI_FRAMEBUFFER_INDEX_COLOR] = swapchain->color_texture.view,
				[RHI_FRAMEBUFFER_INDEX_DEPTH] = swapchain->depth_texture.view,
				[RHI_FRAMEBUFFER_INDEX_RSLVE] = ret.view,
			}
			: (VkImageView[]){
				[RHI_FRAMEBUFFER_INDEX_COLOR] = ret.view,
				[RHI_FRAMEBUFFER_INDEX_DEPTH] = swapchain->depth_texture.view,
			};
		vkCreateFramebuffer(
			fl_rhi_context.logical.handle,
			&(VkFramebufferCreateInfo){
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = fl_rhi_context.render_pass,
				.width = swapchain->extent.width,
				.height = swapchain->extent.height,
				.layers = 1,
				// attachments
				.attachmentCount = attachments_count, .pAttachments = attachments,
			},
			&fl_rhi_allocator,
			&ret.buffer
		);
	}

	vkAllocateCommandBuffers(
		fl_rhi_context.logical.handle,
		&(VkCommandBufferAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = fl_rhi_context.graphics_command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		},
		&ret.commands
	);

	vkCreateSemaphore(
		fl_rhi_context.logical.handle,
		&(VkSemaphoreCreateInfo){
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		},
		&fl_rhi_allocator,
		&ret.semaphore_acquired
	);
	vkCreateSemaphore(
		fl_rhi_context.logical.handle,
		&(VkSemaphoreCreateInfo){
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		},
		&fl_rhi_allocator,
		&ret.semaphore_rendered
	);
	vkCreateFence(
		fl_rhi_context.logical.handle,
		&(VkFenceCreateInfo){
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		},
		&fl_rhi_allocator,
		&ret.fence_executed
	);

	return ret;
}

AttrFileLocal()
void rhi_frame_free(struct RHI_Frame frame) {
	vkDestroyFramebuffer(fl_rhi_context.logical.handle, frame.buffer, &fl_rhi_allocator);
	rhi_texture_view_destroy(frame.view);

	vkDestroySemaphore(fl_rhi_context.logical.handle, frame.semaphore_acquired, &fl_rhi_allocator);
	vkDestroySemaphore(fl_rhi_context.logical.handle, frame.semaphore_rendered, &fl_rhi_allocator);
	vkDestroyFence(fl_rhi_context.logical.handle, frame.fence_executed, &fl_rhi_allocator);
}

AttrFileLocal()
struct RHI_Swapchain rhi_swapchain_init(VkSurfaceKHR surface, VkSwapchainKHR old_swapchain) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	// ---- ---- ---- ----
	// surface capabilities
	// ---- ---- ---- ----

	VkSurfaceCapabilitiesKHR surface_capabilities = {0};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(fl_rhi_context.physical.handle, surface, &surface_capabilities);

	struct RHI_Swapchain ret = {.extent = surface_capabilities.currentExtent};
	if (ret.extent.width == UINT32_MAX && ret.extent.height == UINT32_MAX)
		os_surface_get_size(&ret.extent.width, &ret.extent.height);

	if (ret.extent.width == 0 || ret.extent.height == 0)
		goto cleanup;

	// ---- ---- ---- ----
	// color
	// ---- ---- ---- ----

	if (fl_rhi_context.physical.samples > VK_SAMPLE_COUNT_1_BIT) {
		VkFormat const color_format = fl_rhi_context.physical.surface_format.format;
		ret.color_texture = rhi_texture_create(
			(uvec2){ret.extent.width, ret.extent.height},
			color_format, 0, fl_rhi_context.physical.samples, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
	}

	// ---- ---- ---- ----
	// depth
	// ---- ---- ---- ----

	VkFormat const depth_format = rhi_find_depth_format();
	ret.depth_texture = rhi_texture_create(
		(uvec2){ret.extent.width, ret.extent.height},
		depth_format, 0, fl_rhi_context.physical.samples, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// ---- ---- ---- ----
	// swapchain
	// ---- ---- ---- ----

	arr32 queue_families = {.capacity = 2, .buffer = (uint32_t[2]){0}};
	arr32_append_unique(&queue_families, fl_rhi_context.physical.qfamily.graphics - 1);
	arr32_append_unique(&queue_families, fl_rhi_context.physical.qfamily.present  - 1);

	uint32_t min_image_count = rhi_get_image_count_for_present_mode(fl_rhi_context.physical.present_mode);
	min_image_count = max_u32(min_image_count, surface_capabilities.minImageCount);
	if (surface_capabilities.maxImageCount)
		min_image_count = min_u32(min_image_count, surface_capabilities.maxImageCount);

	vkCreateSwapchainKHR(
		fl_rhi_context.logical.handle,
		&(VkSwapchainCreateInfoKHR){
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = surface,
			.oldSwapchain = old_swapchain,
			// present mode
			.presentMode = fl_rhi_context.physical.present_mode,
			.minImageCount = min_image_count,
			// surface capabilities
			.preTransform = surface_capabilities.currentTransform,
			.imageExtent = ret.extent,
			// surface format
			.imageFormat = fl_rhi_context.physical.surface_format.format,
			.imageColorSpace = fl_rhi_context.physical.surface_format.colorSpace,
			// queue families
			.imageSharingMode = queue_families.count >= 2
				? VK_SHARING_MODE_CONCURRENT
				: VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = (uint32_t)queue_families.count, .pQueueFamilyIndices = queue_families.buffer,
			// image params
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			// composition params
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.clipped = VK_TRUE,
		},
		&fl_rhi_allocator,
		&ret.handle
	);

	uint32_t images_count = 0;
	vkGetSwapchainImagesKHR(fl_rhi_context.logical.handle, ret.handle, &images_count, NULL);
	VkImage * images = MemoryArenaPushArray(scratch, VkImage, images_count);
	vkGetSwapchainImagesKHR(fl_rhi_context.logical.handle, ret.handle, &images_count, images);

	ret.frames_count = images_count;
	ret.frames = os_memory_heap(NULL, sizeof(*ret.frames) * images_count);
	for (uint32_t i = 0; i < images_count; i++)
		ret.frames[i] = rhi_frame_init(&ret, images[i]);

	// ---- ---- ---- ----
	// framebuffers
	// ---- ---- ---- ----

	cleanup:;
	memory_arena_set_position(scratch, scratch_position);
	return ret;
}

AttrFileLocal()
void rhi_swapchain_free(struct RHI_Swapchain swapchain) {
	if (swapchain.handle == VK_NULL_HANDLE)
		return;

	if (swapchain.color_texture.handle != VK_NULL_HANDLE)
		rhi_texture_destroy(swapchain.color_texture);
	rhi_texture_destroy(swapchain.depth_texture);

	for (uint32_t i = 0; i < swapchain.frames_count; i++)
		rhi_frame_free(swapchain.frames[i]);
	os_memory_heap(swapchain.frames, 0);

	if (swapchain.handle != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(fl_rhi_context.logical.handle, swapchain.handle, &fl_rhi_allocator);
}

AttrFileLocal()
void rhi_swapchain_recreate(void) {
	struct RHI_Swapchain const prev = fl_rhi_swapchain;
	struct RHI_Swapchain const next = rhi_swapchain_init(fl_rhi_context.surface, prev.handle);
	// @todo don't halt the thread
	vkDeviceWaitIdle(fl_rhi_context.logical.handle);
	fl_rhi_swapchain = next;
	rhi_swapchain_free(prev);
}

// ---- ---- ---- ----
// buffer
// ---- ---- ---- ----

AttrFileLocal()
struct RHI_Buffer rhi_buffer_create(
	VkDeviceSize size,
	VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags
) {
	arr32 queue_families = {.capacity = 2, .buffer = (uint32_t[2]){0}};
	arr32_append_unique(&queue_families, fl_rhi_context.physical.qfamily.graphics - 1);
	arr32_append_unique(&queue_families, fl_rhi_context.physical.qfamily.transfer - 1);

	struct RHI_Buffer ret = {.size = size};
	vkCreateBuffer(
		fl_rhi_context.logical.handle,
		&(VkBufferCreateInfo){
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			// buffer specific
			.size = size,
			// generic info
			.usage = usage_flags,
			.sharingMode = queue_families.count >= 2
				? VK_SHARING_MODE_CONCURRENT
				: VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = (uint32_t)queue_families.count, .pQueueFamilyIndices = queue_families.buffer,
		},
		&fl_rhi_allocator,
		&ret.handle
	);

	VkMemoryRequirements memory_requirements = {0};
	vkGetBufferMemoryRequirements(fl_rhi_context.logical.handle, ret.handle, &memory_requirements);

	vkAllocateMemory(
		fl_rhi_context.logical.handle,
		&(VkMemoryAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memory_requirements.size,
			.memoryTypeIndex = rhi_find_memory_type_index(
				memory_requirements.memoryTypeBits,
				property_flags
			),
		},
		&fl_rhi_allocator,
		&ret.memory
	);

	VkDeviceSize const memory_offset = 0;
	vkBindBufferMemory(fl_rhi_context.logical.handle, ret.handle, ret.memory, memory_offset);
	return ret;
}

AttrFileLocal()
void rhi_buffer_destroy(struct RHI_Buffer resource) {
	vkFreeMemory(fl_rhi_context.logical.handle, resource.memory, &fl_rhi_allocator);
	vkDestroyBuffer(fl_rhi_context.logical.handle, resource.handle, &fl_rhi_allocator);
}

AttrFileLocal()
void rhi_buffer_copy(VkBuffer source, VkBuffer target, VkDeviceSize size) {
	struct RHI_QCmd const context = rhi_get_transfer_qbuffer();
	rhi_transfer_begin(context);

	// buffer commands
	vkCmdCopyBuffer(context.commands, source, target, 1, &(VkBufferCopy){
		.size = size,
	});

	rhi_transfer_end(context);
}

// ---- ---- ---- ----
// sampler
// ---- ---- ---- ----

AttrFileLocal()
VkSampler rhi_sampler_create(void) {
	VkFilter const min_filter = VK_FILTER_LINEAR; // @todo expose as API
	VkFilter const max_filter = VK_FILTER_LINEAR; // @todo expose as API
	VkSampler ret = VK_NULL_HANDLE;
	vkCreateSampler(
		fl_rhi_context.logical.handle,
		&(VkSamplerCreateInfo){
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.minFilter = min_filter,
			.magFilter = max_filter,
			.anisotropyEnable = fl_rhi_context.physical.properties.limits.maxSamplerAnisotropy > 1,
			.maxAnisotropy = fl_rhi_context.physical.properties.limits.maxSamplerAnisotropy,
		},
		&fl_rhi_allocator,
		&ret
	);
	return ret;
}

AttrFileLocal()
void rhi_sampler_destroy(VkSampler sampler) {
	vkDestroySampler(fl_rhi_context.logical.handle, sampler, &fl_rhi_allocator);
}

// ---- ---- ---- ----
// shader / USER DATA
// ---- ---- ---- ----

AttrFileLocal()
VkShaderModule rhi_shader_module_create(char const * name) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	arr8 const source = base_file_read(scratch, name);
	VkShaderModule ret;
	vkCreateShaderModule(
		fl_rhi_context.logical.handle,
		&(VkShaderModuleCreateInfo){
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = source.count,
			.pCode = (void *)source.buffer,
		},
		&fl_rhi_allocator,
		&ret
	);

	memory_arena_set_position(scratch, scratch_position);
	return ret;
}

AttrFileLocal()
void rhi_shader_init(void) {
	// @todo it seems the brunt of it can be automated via reflection of sorts
	// -- create descriptors
	vkCreateDescriptorPool(
		fl_rhi_context.logical.handle,
		&(VkDescriptorPoolCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = fl_rhi_swapchain.frames_count,
			.poolSizeCount = 2, .pPoolSizes = (VkDescriptorPoolSize[]){
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = fl_rhi_swapchain.frames_count,
				},
				{
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = fl_rhi_swapchain.frames_count,
				},
			},
			// @optimize use `vkResetDescriptorPool` instead without this flag;
			// allow `vkFreeDescriptorSets` on individual allocations
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		},
		&fl_rhi_allocator,
		&fl_rhi_ud.shader.descriptor.pool
	);

	vkCreateDescriptorSetLayout(
		fl_rhi_context.logical.handle,
		&(VkDescriptorSetLayoutCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 2, .pBindings = (VkDescriptorSetLayoutBinding[]){
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
		&fl_rhi_allocator,
		&fl_rhi_ud.shader.descriptor.layout
	);

	// -- create pipeline
	vkCreatePipelineLayout(
		fl_rhi_context.logical.handle,
		&(VkPipelineLayoutCreateInfo){
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1, .pSetLayouts = &fl_rhi_ud.shader.descriptor.layout,
		},
		&fl_rhi_allocator,
		&fl_rhi_ud.shader.layout
	);

	VkPipelineShaderStageCreateInfo const shader_stages[] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = rhi_shader_module_create("data/shader.vert.spv"),
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = rhi_shader_module_create("data/shader.frag.spv"),
			.pName = "main",
		},
	};

	VkDynamicState const dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		// VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
		// VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
		// VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
	};

	vkCreateGraphicsPipelines(
		fl_rhi_context.logical.handle,
		VK_NULL_HANDLE,
		1, &(VkGraphicsPipelineCreateInfo){
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.layout = fl_rhi_ud.shader.layout,
			.renderPass = fl_rhi_context.render_pass,
			.subpass = 0,
			// stages
			.stageCount = ArrayCount(shader_stages), .pStages = shader_stages,
			// vertices
			.pVertexInputState = &(VkPipelineVertexInputStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				// binding
				.vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &(VkVertexInputBindingDescription){
					.binding = 0,
					.stride = sizeof(struct RMVertex),
					.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
				},
				// attributes
				.vertexAttributeDescriptionCount = 3, .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]){
					{
						.binding = 0,
						.location = 0,
						.format = rhi_map_primitive_format_to_vector_format(VK_FORMAT_R32_SFLOAT, FieldSize(struct RMVertex, position) / sizeof(f32)),
						.offset = offsetof(struct RMVertex, position),
					},
					{
						.binding = 0,
						.location = 1,
						.format = rhi_map_primitive_format_to_vector_format(VK_FORMAT_R32_SFLOAT, FieldSize(struct RMVertex, texture) / sizeof(f32)),
						.offset = offsetof(struct RMVertex, texture),
					},
					{
						.binding = 0,
						.location = 2,
						.format = rhi_map_primitive_format_to_vector_format(VK_FORMAT_R32_SFLOAT, FieldSize(struct RMVertex, normal) / sizeof(f32)),
						.offset = offsetof(struct RMVertex, normal),
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
				.rasterizationSamples = fl_rhi_context.physical.samples,
				.sampleShadingEnable = fl_rhi_context.physical.samples > VK_SAMPLE_COUNT_1_BIT,
				.minSampleShading = fl_rhi_context.physical.samples > VK_SAMPLE_COUNT_1_BIT ? 0.2f : 0,
			},
			// blend
			.pColorBlendState = &(VkPipelineColorBlendStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				.attachmentCount = 1, .pAttachments = &(VkPipelineColorBlendAttachmentState){
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
				.dynamicStateCount = ArrayCount(dynamic_states), .pDynamicStates = dynamic_states,
			},
			// depth / stencil
			.pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				// @todo use `VK_DYNAMIC_STATE_*` for these
				.depthTestEnable = VK_TRUE,
				.depthWriteEnable = VK_TRUE,
				.depthCompareOp = RHI_REVERSE_Z
					? VK_COMPARE_OP_GREATER
					: VK_COMPARE_OP_LESS,
			},
		},
		&fl_rhi_allocator,
		&fl_rhi_ud.shader.handle
	);

	for (uint32_t i = 0, count = ArrayCount(shader_stages); i < count; i++)
		vkDestroyShaderModule(fl_rhi_context.logical.handle, shader_stages[i].module, &fl_rhi_allocator);
}

AttrFileLocal()
void rhi_shader_free(void) {
	vkDestroyPipeline(fl_rhi_context.logical.handle, fl_rhi_ud.shader.handle, &fl_rhi_allocator);
	vkDestroyPipelineLayout(fl_rhi_context.logical.handle, fl_rhi_ud.shader.layout, &fl_rhi_allocator);

	// vkResetDescriptorPool(fl_rhi_context.device.handle, fl_rhi_ud.shader.descriptor.pool, 0);
	vkDestroyDescriptorPool(fl_rhi_context.logical.handle, fl_rhi_ud.shader.descriptor.pool, &fl_rhi_allocator);
	vkDestroyDescriptorSetLayout(fl_rhi_context.logical.handle, fl_rhi_ud.shader.descriptor.layout, &fl_rhi_allocator);
}

// ---- ---- ---- ----
// material / USER DATA
// ---- ---- ---- ----

struct UData {
	mat4 model;
	mat4 view;
	mat4 projection;
};
AssertStatic(sizeof(struct UData) % ALIGN_MAT4 == 0);
AssertAlign(struct UData, model,      ALIGN_MAT4);
AssertAlign(struct UData, view,       ALIGN_MAT4);
AssertAlign(struct UData, projection, ALIGN_MAT4);

AttrFileLocal()
void rhi_material_init(void) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	VkDescriptorSetLayout * set_layouts = MemoryArenaPushArray(scratch, VkDescriptorSetLayout, fl_rhi_swapchain.frames_count);
	for (uint32_t i = 0; i < fl_rhi_swapchain.frames_count; i++)
		set_layouts[i] = fl_rhi_ud.shader.descriptor.layout;

	fl_rhi_ud.material.handles = os_memory_heap(NULL, sizeof(*fl_rhi_ud.material.handles) * fl_rhi_swapchain.frames_count);
	vkAllocateDescriptorSets(
		fl_rhi_context.logical.handle,
		&(VkDescriptorSetAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = fl_rhi_ud.shader.descriptor.pool,
			.descriptorSetCount = fl_rhi_swapchain.frames_count, .pSetLayouts = set_layouts,
		},
		fl_rhi_ud.material.handles
	);

	size_t const uniform_buffer_align = fl_rhi_context.physical.properties.limits.minUniformBufferOffsetAlignment;
	size_t const udata_entry_stride = align_size(sizeof(struct UData), uniform_buffer_align);
	size_t const udata_total_size = udata_entry_stride * fl_rhi_swapchain.frames_count;
	fl_rhi_ud.material.data = rhi_buffer_create(
		udata_total_size,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		// can be mapped                      and memory copied "immediately"
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void * target;
	vkMapMemory(fl_rhi_context.logical.handle, fl_rhi_ud.material.data.memory, 0, udata_total_size, 0, &target);
	fl_rhi_ud.material.map = os_memory_heap(NULL, sizeof(*fl_rhi_ud.material.map) * fl_rhi_swapchain.frames_count);
	for (uint32_t i = 0; i < fl_rhi_swapchain.frames_count; i++)
		fl_rhi_ud.material.map[i] = (u8 *)target + udata_entry_stride * i;

	// @note see the corresponding shader for uniforms and stuff
	// although CPU side needs to provide sizes or references
	for (uint32_t i = 0; i < fl_rhi_swapchain.frames_count; i++)
		vkUpdateDescriptorSets(
			fl_rhi_context.logical.handle,
			2, (VkWriteDescriptorSet[]){
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = fl_rhi_ud.material.handles[i],
					.dstBinding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = 1, .pBufferInfo = &(VkDescriptorBufferInfo){
						.buffer = fl_rhi_ud.material.data.handle,
						.offset = udata_entry_stride * i,
						.range = sizeof(struct UData),
					},
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = fl_rhi_ud.material.handles[i],
					.dstBinding = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1, .pImageInfo = &(VkDescriptorImageInfo){
						.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						.imageView = fl_rhi_ud.texture.view,
						.sampler = fl_rhi_ud.sampler,
					},
				},
			},
			0,
			NULL
		);

	memory_arena_set_position(scratch, scratch_position);
}

AttrFileLocal()
void rhi_material_free(void) {
	vkUnmapMemory(fl_rhi_context.logical.handle, fl_rhi_ud.material.data.memory);
	os_memory_heap(fl_rhi_ud.material.map, 0);
	rhi_buffer_destroy(fl_rhi_ud.material.data);
	// @note: make sure the pool was created with `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`
	vkFreeDescriptorSets(fl_rhi_context.logical.handle, fl_rhi_ud.shader.descriptor.pool, fl_rhi_swapchain.frames_count, fl_rhi_ud.material.handles);
	os_memory_heap(fl_rhi_ud.material.handles, 0);
}

// ---- ---- ---- ----
// model / USER DATA
// ---- ---- ---- ----

AttrFileLocal()
void rhi_model_init(void) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	struct RMVertex * vertices; u32 vertices_count;
	u16            * indices;  u16 indices_count;

	struct Resource_Model * file_parsed = resource_model_init("../data/viking_room.obj"); // @todo fix path
	resource_model_dump_vertices(file_parsed, scratch, &vertices, &vertices_count, &indices, &indices_count);
	resource_model_free(file_parsed);

	VkDeviceSize const total_size = sizeof(*vertices) * vertices_count + sizeof(*indices) * indices_count;
	fl_rhi_ud.model.vertex_offset = 0;
	fl_rhi_ud.model.index_offset  = sizeof(*vertices) * vertices_count;
	fl_rhi_ud.model.index_count = indices_count;
	fl_rhi_ud.model.index_type  = VK_INDEX_TYPE_UINT16;

	// @todo might be better to use a common allocator for this
	struct RHI_Buffer const staging_buffer = rhi_buffer_create(
		total_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		// can be mapped                      and memory copied "immediately"
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void * target;
	vkMapMemory(fl_rhi_context.logical.handle, staging_buffer.memory, 0, total_size, 0, &target);
	mem_copy(vertices, target, sizeof(*vertices) * vertices_count); target = (u8*)target + sizeof(*vertices) * vertices_count;
	mem_copy(indices,  target, sizeof(*indices)  * indices_count);  target = (u8*)target + sizeof(*indices)  * indices_count;
	vkUnmapMemory(fl_rhi_context.logical.handle, staging_buffer.memory);

	fl_rhi_ud.model.data = rhi_buffer_create(
		total_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	rhi_buffer_copy(staging_buffer.handle, fl_rhi_ud.model.data.handle, total_size);

	rhi_buffer_destroy(staging_buffer);

	memory_arena_set_position(scratch, scratch_position);
}

AttrFileLocal()
void rhi_model_free(void) {
	rhi_buffer_destroy(fl_rhi_ud.model.data);
}

// ---- ---- ---- ----
// texture / USER DATA
// ---- ---- ---- ----

AttrFileLocal()
void rhi_texture_init(void) {
	struct Memory_Arena * scratch = thread_ctx_get_scratch();
	u64 const scratch_position = memory_arena_get_position(scratch);

	arr8 const file_bytes = base_file_read(scratch, "../data/viking_room.png"); // @todo fix path
	struct Resource_Image file_parsed = resource_image_init(file_bytes);

	VkDeviceSize const total_size = file_parsed.scalar_size * file_parsed.size.x * file_parsed.size.y * file_parsed.channels;
	VkFormat const primitive = rhi_map_vector_format_to_primitive_format(fl_rhi_context.physical.surface_format.format);
	VkFormat const format = rhi_map_primitive_format_to_vector_format(primitive, file_parsed.channels);

	// @todo might be better to use a common allocator for this
	struct RHI_Buffer const staging_buffer = rhi_buffer_create(
		total_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		// can be mapped                      and memory copied "immediately"
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void * target;
	vkMapMemory(fl_rhi_context.logical.handle, staging_buffer.memory, 0, total_size, 0, &target);
	mem_copy(file_parsed.buffer, target, total_size);
	vkUnmapMemory(fl_rhi_context.logical.handle, staging_buffer.memory);

	u32 const extra_mip_levels = (u32)log2_32((f32)max_u32(file_parsed.size.x, file_parsed.size.y));
	fl_rhi_ud.texture = rhi_texture_create(
		file_parsed.size, format, extra_mip_levels, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	rhi_texture_transition(fl_rhi_ud.texture.handle, format, extra_mip_levels,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);
	rhi_texture_upload(fl_rhi_ud.texture.handle, staging_buffer.handle, file_parsed.size, format);
	if (extra_mip_levels == 0 || !rhi_texture_generate_mipmaps(fl_rhi_ud.texture.handle, format, extra_mip_levels, file_parsed.size))
		rhi_texture_transition(fl_rhi_ud.texture.handle, format, extra_mip_levels,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

	rhi_buffer_destroy(staging_buffer);

	resource_image_free(&file_parsed);
	memory_arena_set_position(scratch, scratch_position);
}

AttrFileLocal()
void rhi_texture_free(void) {
	rhi_texture_destroy(fl_rhi_ud.texture);
}

// ---- ---- ---- ----
// API
// ---- ---- ---- ----

void rhi_init(void) {
	// -- system
	VkInstance   instance = fl_rhi_context.instance = rhi_instance_init();
	VkSurfaceKHR surface  = fl_rhi_context.surface  = (VkSurfaceKHR)os_vulkan_create_surface(instance, &fl_rhi_allocator);
	rhi_device_find_and_create(instance, surface);

	// -- universal
	rhi_command_pool_init();

	// -- fixed
	rhi_render_pass_init();
	fl_rhi_swapchain = rhi_swapchain_init(surface, VK_NULL_HANDLE);

	// -- user data
	rhi_model_init();
	fl_rhi_ud.sampler = rhi_sampler_create();
	rhi_texture_init();

	// -- user data: shader and material
	rhi_shader_init();
	rhi_material_init();
}

void rhi_free(void) {
	vkDeviceWaitIdle(fl_rhi_context.logical.handle);

	// -- user data: shader and material
	rhi_material_free();
	rhi_shader_free();

	// -- user data
	rhi_texture_free();
	rhi_sampler_destroy(fl_rhi_ud.sampler);
	rhi_model_free();

	// -- fixed
	rhi_swapchain_free(fl_rhi_swapchain);
	rhi_render_pass_free();

	// -- universal
	rhi_command_pool_free();

	// -- system
	rhi_device_free(fl_rhi_context.logical.handle);
	vkDestroySurfaceKHR(fl_rhi_context.instance, fl_rhi_context.surface, &fl_rhi_allocator);
	rhi_instance_free(fl_rhi_context.instance);

	mem_zero(&fl_rhi_context, sizeof(fl_rhi_context));
	mem_zero(&fl_rhi_swapchain, sizeof(fl_rhi_swapchain));
	mem_zero(&fl_rhi_ud, sizeof(fl_rhi_ud));
}

void rhi_tick(void) {
	if (fl_rhi_swapchain.needs_update)
		rhi_swapchain_recreate();

	if (fl_rhi_swapchain.handle == VK_NULL_HANDLE)
		return;

	uint32_t         const current_index = fl_rhi_swapchain.frame_current;
	struct RHI_Frame const current_frame = fl_rhi_swapchain.frames[current_index];

	// -- prepare
	vkWaitForFences(fl_rhi_context.logical.handle, 1, &current_frame.fence_executed, VK_TRUE, UINT64_MAX);

	uint32_t presentable_index = 0;
	VkResult const acquire_next_image_result =
	vkAcquireNextImageKHR(
		fl_rhi_context.logical.handle, fl_rhi_swapchain.handle,
		UINT64_MAX, current_frame.semaphore_acquired, VK_NULL_HANDLE, &presentable_index
	);
	switch (acquire_next_image_result) {
		// success, continue rendering
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			// @note lock the fence only if it is expected to be unlocked eventually
			vkResetFences(fl_rhi_context.logical.handle, 1, &current_frame.fence_executed);
			break;

		// failure, cancel rendering
		case VK_ERROR_OUT_OF_DATE_KHR:
			fmt_print("[RHI] swapchain needs update\n");
			fmt_print("\n");
			fl_rhi_swapchain.needs_update = true;
		AttrFallthrough();
		default: return;
	}

	// @note render into a buffer which is the closest to presentation
	struct RHI_Frame const presentable_frame = fl_rhi_swapchain.frames[presentable_index];

	// -- upload uniforms
	{
		f32 const fov = PI32 / 3;
		vec2 const vp_scale = vec2_muls(
			(vec2){(f32)fl_rhi_swapchain.extent.height / (f32)fl_rhi_swapchain.extent.width, 1},
			1 / tan32(fov / 2)
		);
		u64 const rotation_period = SecondsToNanos(10);
		f32 const rotation_offset = TAU32 * (f32)(os_timer_get_nanos() % rotation_period) / (f32)rotation_period;
		f32 const rotation = (PI32 / 10) * cos32(rotation_offset);
		struct UData const udata = {
			.model = mat4_transformation(vec3_0, quat_axis(vec3_y1, rotation), vec3_1),
			.view = mat4_transformation_inverse((vec3){1.2f, 1.8f, -1.2f}, quat_rotation((vec3){PI32/4, -PI32/4, 0}), vec3_1),
			.projection = rhi_mat4_projection(vp_scale, vec2_0, 0, 0.1f, INF32),
		};
		mem_copy(&udata, fl_rhi_ud.material.map[current_index], sizeof(udata));
	}

	// -- draw: begin
	vkBeginCommandBuffer(current_frame.commands, &(VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	});

	vkCmdBeginRenderPass(current_frame.commands, &(VkRenderPassBeginInfo){
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = fl_rhi_context.render_pass,
		.framebuffer = presentable_frame.buffer,
		.renderArea = {
			.extent = fl_rhi_swapchain.extent,
		},
		.clearValueCount = 2, .pClearValues = (VkClearValue[]){
			[RHI_FRAMEBUFFER_INDEX_COLOR] = {
				.color.float32 = {0, 0, 0, 1},
			},
			[RHI_FRAMEBUFFER_INDEX_DEPTH] = {
				.depthStencil = {
					.depth = RHI_REVERSE_Z ? 0 : 1,
				},
			},
		},
	}, VK_SUBPASS_CONTENTS_INLINE);

	// -- scope drawing area
	vkCmdSetViewport(current_frame.commands, 0, 1, &(VkViewport){
		// offset, Y positive up
		.x = (float)0,
		.y = (float)fl_rhi_swapchain.extent.height,
		// scale, Y positive up
		.width = (float)fl_rhi_swapchain.extent.width,
		.height = -(float)fl_rhi_swapchain.extent.height,
		// depth
		.minDepth = RHI_REVERSE_Z ? 1 : 0,
		.maxDepth = RHI_REVERSE_Z ? 0 : 1,
	});
	vkCmdSetScissor(current_frame.commands, 0, 1, &(VkRect2D){
		.extent = fl_rhi_swapchain.extent,
	});

	// -- choose shader and material
	vkCmdBindPipeline(current_frame.commands, VK_PIPELINE_BIND_POINT_GRAPHICS, fl_rhi_ud.shader.handle);
	vkCmdBindDescriptorSets(current_frame.commands, VK_PIPELINE_BIND_POINT_GRAPHICS, fl_rhi_ud.shader.layout,
		0, 1, &fl_rhi_ud.material.handles[current_index],
		0, NULL
	);

	// -- choose and draw mesh
	vkCmdBindVertexBuffers(current_frame.commands, 0, 1, &fl_rhi_ud.model.data.handle, &fl_rhi_ud.model.vertex_offset);
	vkCmdBindIndexBuffer(current_frame.commands, fl_rhi_ud.model.data.handle, fl_rhi_ud.model.index_offset, fl_rhi_ud.model.index_type);
	vkCmdDrawIndexed(current_frame.commands, fl_rhi_ud.model.index_count, 1, 0, 0, 0);

	// -- draw: end
	vkCmdEndRenderPass(current_frame.commands);
	vkEndCommandBuffer(current_frame.commands);
	VkPipelineStageFlags const queue_wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// -- submit
	vkQueueSubmit(fl_rhi_context.logical.queue.graphics, 1, &(VkSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount   = 1, .pWaitSemaphores   = &current_frame.semaphore_acquired, .pWaitDstStageMask = &queue_wait_stages,
		.commandBufferCount   = 1, .pCommandBuffers   = &current_frame.commands,
		.signalSemaphoreCount = 1, .pSignalSemaphores = &current_frame.semaphore_rendered,
	}, current_frame.fence_executed);

	// -- present
	VkResult const queue_present_result =
	vkQueuePresentKHR(fl_rhi_context.logical.queue.present, &(VkPresentInfoKHR){
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1, .pWaitSemaphores = &current_frame.semaphore_rendered,
		.swapchainCount     = 1, .pSwapchains     = &fl_rhi_swapchain.handle,
		.pImageIndices = &presentable_index,
	});
	switch (queue_present_result) {
		default: break; // @ignore
		case VK_SUBOPTIMAL_KHR:        // success
		case VK_ERROR_OUT_OF_DATE_KHR: // failure
			fmt_print("[RHI] swapchain needs update\n");
			fmt_print("\n");
			fl_rhi_swapchain.needs_update = true;
			break;
	}

	fl_rhi_swapchain.frame_current = (fl_rhi_swapchain.frame_current + 1) % fl_rhi_swapchain.frames_count;
}

void rhi_notify_surface_resized(void) {
	// @note it's better to know this in advance than at the `tick` time
	fl_rhi_swapchain.needs_update = true;
}

mat4 rhi_mat4_projection(
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

#undef RHI_REVERSE_Z
#undef RHI_DEFINE_PROC
#undef RHI_ENABLE_DEBUG

#undef RHI_FRAMEBUFFER_INDEX_COLOR
#undef RHI_FRAMEBUFFER_INDEX_DEPTH
