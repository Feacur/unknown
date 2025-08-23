#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// #include <Windowsx.h> // @note GET_X_LPARAM GET_Y_LPARAM
#include <signal.h>

#include "base.h"
#include "windows_resources.h"

AttrFileLocal()
char const * const fl_os_default_window_class_name = "os_default_window";

AttrFileLocal()
UINT_PTR const fl_os_sizemove_fiber_timer_id = 0;

#define OS_MESSAGING_NONE  0
#define OS_MESSAGING_TICK  1
#define OS_MESSAGING_FIBER 2
#define OS_MESSAGING OS_MESSAGING_FIBER

// ---- ---- ---- ----
// stringifiers
// ---- ---- ---- ----

AttrFileLocal()
str8 os_to_string_signal(int value) {
	switch (value) {
		case SIGABRT:  return str8_lit("abort()");
		case SIGBREAK: return str8_lit("Ctrl+Break");
		case SIGFPE:   return str8_lit("Floating-point error");
		case SIGILL:   return str8_lit("Illegal instruction");
		case SIGINT:   return str8_lit("Ctrl+C");
		case SIGSEGV:  return str8_lit("Illegal storage access");
		case SIGTERM:  return str8_lit("Termination request");
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 os_to_string_processor_architecture(DWORD value) {
	switch (value) {
		case PROCESSOR_ARCHITECTURE_INTEL:          return str8_lit("Intel");
		case PROCESSOR_ARCHITECTURE_MIPS:           return str8_lit("MIPS");
		case PROCESSOR_ARCHITECTURE_ALPHA:          return str8_lit("Alpha");
		case PROCESSOR_ARCHITECTURE_PPC:            return str8_lit("PPC");
		case PROCESSOR_ARCHITECTURE_SHX:            return str8_lit("SHx");
		case PROCESSOR_ARCHITECTURE_ARM:            return str8_lit("ARM");
		case PROCESSOR_ARCHITECTURE_IA64:           return str8_lit("IA-64");
		case PROCESSOR_ARCHITECTURE_ALPHA64:        return str8_lit("Alpha 64");
		case PROCESSOR_ARCHITECTURE_MSIL:           return str8_lit("MSIL");
		case PROCESSOR_ARCHITECTURE_AMD64:          return str8_lit("AMD64");
		case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:  return str8_lit("IA32 on Win64");
		case PROCESSOR_ARCHITECTURE_NEUTRAL:        return str8_lit("NEUTRAL");
		case PROCESSOR_ARCHITECTURE_ARM64:          return str8_lit("ARM64");
		case PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64: return str8_lit("ARM32 on Win64");
		case PROCESSOR_ARCHITECTURE_IA32_ON_ARM64:  return str8_lit("IA32 on ARM64");
		case PROCESSOR_ARCHITECTURE_UNKNOWN:        return str8_lit("UNKNOWN");
		default: return str8_lit("unknown");
	}
}

AttrFileLocal()
str8 os_to_string_processor_type(DWORD value) {
	switch (value) {
		case PROCESSOR_INTEL_386:     return str8_lit("Intel 386");
		case PROCESSOR_INTEL_486:     return str8_lit("Intel 486");
		case PROCESSOR_INTEL_PENTIUM: return str8_lit("Intel Pentium");
		case PROCESSOR_INTEL_IA64:    return str8_lit("Intel IA-64");
		case PROCESSOR_AMD_X8664:     return str8_lit("AMD X8664");
		case PROCESSOR_MIPS_R4000:    return str8_lit("MIPS R4000");
		case PROCESSOR_ALPHA_21064:   return str8_lit("Alpha 21064");
		case PROCESSOR_PPC_601:       return str8_lit("PPC 601");
		case PROCESSOR_PPC_603:       return str8_lit("PPC 603");
		case PROCESSOR_PPC_604:       return str8_lit("PPC 604");
		case PROCESSOR_PPC_620:       return str8_lit("PPC 620");
		case PROCESSOR_HITACHI_SH3:   return str8_lit("Hitachi SH3");
		case PROCESSOR_HITACHI_SH3E:  return str8_lit("Hitachi SH3E");
		case PROCESSOR_HITACHI_SH4:   return str8_lit("Hitachi SH4");
		case PROCESSOR_MOTOROLA_821:  return str8_lit("Motorola 821");
		case PROCESSOR_SHx_SH3:       return str8_lit("SHx SH3");
		case PROCESSOR_SHx_SH4:       return str8_lit("SHx SH4");
		case PROCESSOR_STRONGARM:     return str8_lit("Strongarm");
		case PROCESSOR_ARM720:        return str8_lit("ARM 720");
		case PROCESSOR_ARM820:        return str8_lit("ARM 820");
		case PROCESSOR_ARM920:        return str8_lit("ARM 920");
		case PROCESSOR_ARM_7TDMI:     return str8_lit("ARM 7TDMI");
		case PROCESSOR_OPTIL:         return str8_lit("OPTIL");
		default: return str8_lit("unknown");
	}
}

// ---- ---- ---- ----
// error handling
// ---- ---- ---- ----

AttrFileLocal() NTAPI
LONG os_vectored_exception_handler(EXCEPTION_POINTERS * ExceptionInfo) {
	DWORD const code  = ExceptionInfo->ExceptionRecord->ExceptionCode;
	DWORD const flags = ExceptionInfo->ExceptionRecord->ExceptionFlags;
	switch (code) {
		case 0x406d1388: // SetThreadName
		case 0xe0434352: // CLR
		case 0xe06d7363: // C++
			break;

		default:
			AssertF(!(flags & EXCEPTION_NONCONTINUABLE), "[os] exception 0x%lx\n", code);
			break;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

AttrFileLocal() __CRTDECL
void os_signal_handler(int signal) {
	str8 signal_text = os_to_string_signal(signal);
	AssertF(false, "[os] signal %.*s\n", (int)signal_text.count, signal_text.buffer);
	// https://learn.microsoft.com/cpp/c-runtime-library/reference/signal
	// https://learn.microsoft.com/windows/console/ctrl-c-and-ctrl-break-signals
	// https://cplusplus.com/reference/csignal/signal
	// https://en.cppreference.com/w/c/header/signal
}

// ---- ---- ---- ----
// implementation
// ---- ---- ---- ----

#include "os.h"

AttrFileLocal()
struct OS {
	struct OS_IInfo info;
	HANDLE module;
	HANDLE vector;
	HANDLE heap;
	HWND   window;
	// fibers
	#if OS_MESSAGING == OS_MESSAGING_FIBER
	LPVOID main_fiber;
	LPVOID message_fiber;
	#endif
	// timer
	LARGE_INTEGER timer_frequency;
	LARGE_INTEGER timer_initial;
	//
	bool quit;
	// move window
	WORD width;
	WORD height;
	#if OS_MESSAGING == OS_MESSAGING_TICK
	enum OS_Move_Window {
		OS_MOVE_WINDOW_NONE,
		OS_MOVE_WINDOW_LEFT,
		OS_MOVE_WINDOW_RIGHT,
		OS_MOVE_WINDOW_TOP,
		OS_MOVE_WINDOW_TOPLEFT,
		OS_MOVE_WINDOW_TOPRIGHT,
		OS_MOVE_WINDOW_BOTTOM,
		OS_MOVE_WINDOW_BOTTOMLEFT,
		OS_MOVE_WINDOW_BOTTOMRIGHT,
		OS_MOVE_WINDOW_FULL,
	} move_window;
	POINT move_window_cursor;
	RECT move_window_rect;
	#endif
	// borderless fullscreen
	WINDOWPLACEMENT pre_bfs_placement;
	LONG_PTR pre_bfs_style;
} fl_os;

struct OS_Info g_os_info;

#if OS_MESSAGING == OS_MESSAGING_TICK
AttrFileLocal()
void os_syscommand_move(BYTE type) {
	if (fl_os.move_window != OS_MOVE_WINDOW_NONE) return;
	switch (type) {
		case HTCAPTION: fl_os.move_window = OS_MOVE_WINDOW_FULL; break;
		default: return;
	}
	GetCursorPos(&fl_os.move_window_cursor);
	GetWindowRect(fl_os.window, &fl_os.move_window_rect);
}

AttrFileLocal()
void os_syscommand_size(BYTE type) {
	if (fl_os.move_window != OS_MOVE_WINDOW_NONE) return;
	switch (type) {
		case WMSZ_LEFT:        fl_os.move_window = OS_MOVE_WINDOW_LEFT;        break;
		case WMSZ_RIGHT:       fl_os.move_window = OS_MOVE_WINDOW_RIGHT;       break;
		case WMSZ_TOP:         fl_os.move_window = OS_MOVE_WINDOW_TOP;         break;
		case WMSZ_TOPLEFT:     fl_os.move_window = OS_MOVE_WINDOW_TOPLEFT;     break;
		case WMSZ_TOPRIGHT:    fl_os.move_window = OS_MOVE_WINDOW_TOPRIGHT;    break;
		case WMSZ_BOTTOM:      fl_os.move_window = OS_MOVE_WINDOW_BOTTOM;      break;
		case WMSZ_BOTTOMLEFT:  fl_os.move_window = OS_MOVE_WINDOW_BOTTOMLEFT;  break;
		case WMSZ_BOTTOMRIGHT: fl_os.move_window = OS_MOVE_WINDOW_BOTTOMRIGHT; break;
		default: return;
	}
	GetCursorPos(&fl_os.move_window_cursor);
	GetWindowRect(fl_os.window, &fl_os.move_window_rect);
}

AttrFileLocal()
void os_syscommand_tick(void) {
	// @note downside is that tick is application-dependent instead of OS-based
	enum OS_Move_Window const command = fl_os.move_window;
	if (command == OS_MOVE_WINDOW_NONE) return;

	if (!(GetKeyState(VK_LBUTTON) & 0x80))
		fl_os.move_window = OS_MOVE_WINDOW_NONE;

	RECT rect = fl_os.move_window_rect;
	POINT cursor; GetCursorPos(&cursor);
	POINT const offset = {
		cursor.x - fl_os.move_window_cursor.x,
		cursor.y - fl_os.move_window_cursor.y,
	};

	switch (command) {
		case OS_MOVE_WINDOW_FULL:
			rect.left   += offset.x;
			rect.right  += offset.x;
			rect.bottom += offset.y;
			rect.top    += offset.y;
			break;

		case OS_MOVE_WINDOW_LEFT:        rect.left  += offset.x;                          break;
		case OS_MOVE_WINDOW_RIGHT:       rect.right += offset.x;                          break;
		case OS_MOVE_WINDOW_TOP:                                 rect.top    += offset.y; break;
		case OS_MOVE_WINDOW_TOPLEFT:     rect.left  += offset.x; rect.top    += offset.y; break;
		case OS_MOVE_WINDOW_TOPRIGHT:    rect.right += offset.x; rect.top    += offset.y; break;
		case OS_MOVE_WINDOW_BOTTOM:                              rect.bottom += offset.y; break;
		case OS_MOVE_WINDOW_BOTTOMLEFT:  rect.left  += offset.x; rect.bottom += offset.y; break;
		case OS_MOVE_WINDOW_BOTTOMRIGHT: rect.right += offset.x; rect.bottom += offset.y; break;

		default: break;
	}

	MoveWindow(
		fl_os.window,
		rect.left, rect.top,
		rect.right  - rect.left,
		rect.bottom - rect.top,
		FALSE
	);
}
#endif

AttrFileLocal()
void os_toggle_borderless_fullscreen(void) {
	ShowWindow(fl_os.window, SW_HIDE);
	LONG_PTR const window_style = GetWindowLongPtr(fl_os.window, GWL_STYLE);

	if (window_style & WS_CAPTION) { // set borderless fullscreen mode
		MONITORINFO monitor_info = {.cbSize = sizeof(MONITORINFO)};
		if (!GetMonitorInfo(MonitorFromWindow(fl_os.window, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
			goto finalize;

		WINDOWPLACEMENT placement = {.length = sizeof(placement)};
		if (!GetWindowPlacement(fl_os.window, &placement))
			goto finalize;

		fl_os.pre_bfs_placement = placement;
		fl_os.pre_bfs_style = window_style;

		SetWindowLongPtr(fl_os.window, GWL_STYLE, WS_CLIPSIBLINGS);
		SetWindowPos(
			fl_os.window, HWND_TOP,
			monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
			monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
			monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
			SWP_FRAMECHANGED
		);
	}
	else { // restore windowed mode
		SetWindowLongPtr(fl_os.window, GWL_STYLE, fl_os.pre_bfs_style);
		SetWindowPos(
			fl_os.window, HWND_TOP,
			0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE
		);
		SetWindowPlacement(fl_os.window, &fl_os.pre_bfs_placement);
	}

	finalize:;
	ShowWindow(fl_os.window, SW_SHOW);
	// @note without an active framebuffer window might seem invisible
	// but the `WS_VISIBLE` is there
}

AttrFileLocal()
WORD os_fix_virtual_key(WORD virtual_key_code, WORD scan_code) {
	switch (virtual_key_code) {
		case VK_SHIFT:
		case VK_CONTROL:
		case VK_MENU:
			UINT const mapped = MapVirtualKeyA(scan_code, MAPVK_VSC_TO_VK_EX);
			return LOWORD(mapped);
	}
	return virtual_key_code;
	// https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-mapvirtualkeya
}

AttrFileLocal()
void os_window_on_key(WPARAM wparam, LPARAM lparam) {
	WORD const flags   = HIWORD(lparam);
	bool const is_extended = (flags & KF_EXTENDED) == KF_EXTENDED; // (lparam >> 24) & 1;
	bool const was_down    = (flags & KF_REPEAT)   == KF_REPEAT;   // (lparam >> 30) & 1;
	bool const is_up       = (flags & KF_UP)       == KF_UP;       // (lparam >> 31) & 1;

	WORD const scan_code = (is_extended ? 0xe000 : 0x0000) | LOBYTE(flags);
	WORD const virtual_key_code = os_fix_virtual_key(LOWORD(wparam), scan_code);

	if (!is_up && !was_down) {
		if (virtual_key_code == VK_F11) {
			os_toggle_borderless_fullscreen();
		}
	}
	// https://learn.microsoft.com/windows/win32/inputdev/keyboard-input
}

AttrFileLocal() CALLBACK
LRESULT os_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
		case WM_CREATE:
			// CREATESTRUCT const * create = (void *)lparam;
			// void const * create_params = create->lpCreateParams;
			fmt_print("[os] WM_CREATE 0x%p\n\n", (void *)window);
			return 0;

		case WM_CLOSE:
			fmt_print("[os] WM_CLOSE 0x%p\n\n", (void *)window);
			DestroyWindow(window);
			return 0;

		case WM_DESTROY:
			fmt_print("[os] WM_DESTROY 0x%p\n\n", (void *)window);
			if (fl_os.window == window) {
				fl_os.window = NULL;
				PostQuitMessage(0);
			}
			return 0;

		#if OS_MESSAGING == OS_MESSAGING_FIBER
		case WM_ENTERSIZEMOVE:
			SetTimer(window, fl_os_sizemove_fiber_timer_id, USER_TIMER_MINIMUM, NULL);
			break;

		case WM_EXITSIZEMOVE:
			KillTimer(window, fl_os_sizemove_fiber_timer_id);
			break;

		case WM_TIMER:
			if (wparam == fl_os_sizemove_fiber_timer_id)
				SwitchToFiber(fl_os.main_fiber);
			break;
		#endif

		case WM_SYSCOMMAND:
			if (fl_os.window == window) {
				// int cursor_x = GET_X_LPARAM(lparam); // LOWORD
				// int cursor_y = GET_Y_LPARAM(lparam); // HIWORD
				switch (wparam & 0xfff0) {
					#if OS_MESSAGING == OS_MESSAGING_TICK
					case SC_MOVE: os_syscommand_move(wparam & 0x000f); return 0;
					case SC_SIZE: os_syscommand_size(wparam & 0x000f); return 0;
					#endif
					// case SC_CLOSE: return 0; // @note alt+f4
					case SC_KEYMENU: // @note hotkeys
						if (lparam == VK_RETURN)
							os_toggle_borderless_fullscreen();
						return 0;
				}
			}
			break;

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_KEYDOWN:
			if (fl_os.window == window) {
				os_window_on_key(wparam, lparam);
			}
			break;

		case WM_MOVE:
			// int const width  = (int)(short)LOWORD(lparam);
			// int const height = (int)(short)HIWORD(lparam);
			return 0;

		case WM_SIZE:
			if (fl_os.window == window) {
				WORD const width  = LOWORD(lparam);
				WORD const height = HIWORD(lparam);
				if (fl_os.width != width || fl_os.height != height) {
					fl_os.width = width;
					fl_os.height = height;
					if (fl_os.info.on_resize)
						fl_os.info.on_resize();
				}
				return 0;
			}
	}

	return DefWindowProc(window, message, wparam, lparam);
	// https://learn.microsoft.com/windows/win32/winmsg/window-notifications
}

AttrFileLocal()
void os_dispatch_messages(void) {
	for (MSG message; PeekMessageA(&message, NULL, 0, 0, PM_REMOVE); (void)0) {
		if (message.message == WM_QUIT) {
			fl_os.quit = true;
			continue;
		}
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

#if OS_MESSAGING == OS_MESSAGING_FIBER
AttrFileLocal()
VOID os_message_fiber_entry_point(LPVOID data) {
	for (;;) {
		os_dispatch_messages();
		SwitchToFiber(fl_os.main_fiber);
	}
}
#endif

// ---- ---- ---- ----
// API
// ---- ---- ---- ----

void os_init(struct OS_IInfo info) {
	fl_os.info = info;
	fl_os.module = GetModuleHandle(0);

	// -- init an exceptions handler
	fl_os.vector = AddVectoredExceptionHandler(0, os_vectored_exception_handler);

	// -- init signals
	signal(SIGABRT,  os_signal_handler);
	signal(SIGBREAK, os_signal_handler);
	signal(SIGFPE,   os_signal_handler);
	signal(SIGILL,   os_signal_handler);
	signal(SIGINT,   os_signal_handler);
	signal(SIGSEGV,  os_signal_handler);
	signal(SIGTERM,  os_signal_handler);

	// -- init timer
	QueryPerformanceFrequency(&fl_os.timer_frequency);
	QueryPerformanceCounter(&fl_os.timer_initial);

	// -- collect system info
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	g_os_info = (struct OS_Info){
		.page_size = system_info.dwPageSize,
	};

	str8 const processor_arch_text = os_to_string_processor_architecture(system_info.wProcessorArchitecture);
	str8 const processor_type_text = os_to_string_processor_type(system_info.dwProcessorType);

	fmt_print("[os] info:\n");
	fmt_print("- memory\n");
	fmt_print("  page size: %lu\n", system_info.dwPageSize);
	fmt_print("  addr min:  0x%p\n", system_info.lpMinimumApplicationAddress);
	fmt_print("  addr max:  0x%p\n", system_info.lpMaximumApplicationAddress);
	fmt_print("- CPU\n");
	fmt_print("  cores: %lu\n", system_info.dwNumberOfProcessors);
	fmt_print("  arch:  %.*s\n",  (int)processor_arch_text.count, processor_arch_text.buffer);
	fmt_print("  type:  %.*s\n",  (int)processor_type_text.count, processor_type_text.buffer);
	fmt_print("  revis: %#x\n", system_info.wProcessorRevision);
	fmt_print("  level: %u\n",  system_info.wProcessorLevel);
	fmt_print("\n");

	// -- init a growable heap
	fl_os.heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0, 0);

	// -- create fiber
	#if OS_MESSAGING == OS_MESSAGING_FIBER
	fl_os.main_fiber = ConvertThreadToFiber(NULL);
	fl_os.message_fiber = CreateFiber(0, os_message_fiber_entry_point, NULL);
	#endif

	// -- register a graphical window class
	{
		RegisterClassExA(&(WNDCLASSEX){
			.cbSize        = sizeof(WNDCLASSEX),
			.style         = CS_HREDRAW | CS_VREDRAW,
			.lpfnWndProc   = os_window_procedure,
			.hInstance     = fl_os.module,
			.hIcon         = LoadIcon(fl_os.module, MAIN_ICO),
			.hCursor       = LoadCursor(0, IDC_ARROW),
			.lpszClassName = fl_os_default_window_class_name,
		});
	}

	// -- create a graphical window
	{
		RECT target_rect = {.right = info.window_size_x, .bottom = info.window_size_y};
		DWORD const target_style    = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
		DWORD const target_ex_style = WS_EX_APPWINDOW;
		AdjustWindowRectExForDpi(
			&target_rect, target_style, FALSE, target_ex_style,
			GetDpiForSystem()
		);

		void * create_params = NULL;
		fl_os.window = CreateWindowExA(
			target_ex_style,
			fl_os_default_window_class_name,
			info.window_caption,
			target_style,
			CW_USEDEFAULT, CW_USEDEFAULT,
			target_rect.right - target_rect.left, target_rect.bottom - target_rect.top,
			HWND_DESKTOP, NULL, fl_os.module, create_params
		);
	}
}

void os_free(void) {
	// -- destroy the graphical window
	if (fl_os.window != NULL)
		DestroyWindow(fl_os.window);

	// -- unregister the graphical window class
	UnregisterClassA(fl_os_default_window_class_name, fl_os.module);

	// --destroy the heap
	{
		HEAP_SUMMARY summary = {.cb = sizeof(summary)};
		HeapSummary(fl_os.heap, 0, &summary);
		Assert(summary.cbAllocated == 0, "[os] heap memory leak\n");
	}
	HeapDestroy(fl_os.heap);

	// -- delete fiber
	#if OS_MESSAGING == OS_MESSAGING_FIBER
	DeleteFiber(fl_os.message_fiber);
	ConvertFiberToThread();
	#endif

	// -- deinit the exceptions handler
	RemoveVectoredContinueHandler(fl_os.vector);

	// -- zero the memory
	mem_zero(&fl_os, sizeof(fl_os));
}

void os_tick(void) {
	#if OS_MESSAGING == OS_MESSAGING_NONE
	os_dispatch_messages();
	#elif OS_MESSAGING == OS_MESSAGING_TICK
	os_dispatch_messages();
	os_syscommand_tick();
	#elif OS_MESSAGING == OS_MESSAGING_FIBER
	SwitchToFiber(fl_os.message_fiber);
	#endif
}

void os_sleep(u64 nanos) {
	u32 const millis = (u32)(nanos / (u64)1000000);
	if (millis == 0)
		YieldProcessor();
	else
		Sleep(millis);
}

bool os_should_quit(void) {
	return fl_os.quit;
}

bool os_exit(int code) {
	ExitProcess((UINT)code);
}

// ---- ---- ---- ----
// file
// ---- ---- ---- ----

struct OS_File {
	struct OS_File_IInfo info;
	HANDLE handle;
};

struct OS_File * os_file_init(struct OS_File_IInfo info) {
	HANDLE const handle = CreateFileA(info.name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return NULL;
	struct OS_File * ret = os_memory_heap(NULL, sizeof(*ret));
	ret->info = info;
	ret->handle = handle;
	return ret;
	// https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-createfilea
}

void os_file_free(struct OS_File * file) {
	BOOL const ok = CloseHandle(file->handle);
	Assert(ok == TRUE, "[os] `CloseHandle` failed\n");
	mem_zero(file, sizeof(*file));
	os_memory_heap(file, 0);
}

u64 os_file_get_size(struct OS_File const * file) {
	LARGE_INTEGER ret;
	BOOL const ok = GetFileSizeEx(file->handle, &ret);
	Assert(ok == TRUE, "[os] `GetFileSizeEx` failed\n");
	return (u64)ret.QuadPart;
	// https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-getfilesizeex
}

u64 os_file_get_write_nanos(struct OS_File const * file) {
	FILETIME write_time;
	BOOL const ok = GetFileTime(file->handle, NULL, NULL, &write_time);
	Assert(ok == TRUE, "[os] `GetFileTime` failed\n");
	// copy the low- and high-order parts
	// of the file time to a ULARGE_INTEGER structure,
	// perform 64-bit arithmetic on the QuadPart member
	ULARGE_INTEGER combined = {
		.LowPart = write_time.dwLowDateTime,
		.HighPart = write_time.dwHighDateTime,
	};
	// Contains a 64-bit value representing the number of
	// 100-nanosecond intervals since January 1, 1601 (UTC).
	return combined.QuadPart * 100;
	// https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-getfiletime
	// https://learn.microsoft.com/windows/win32/api/minwinbase/ns-minwinbase-filetime
}

u64 os_file_read(struct OS_File const * file, u64 offset_min, u64 offset_max, void * buffer) {
	AttrFuncLocal() u64 const chunk_limit = ~(DWORD)0;
	u64 ret = 0;
	for (u64 offset = offset_min; offset < offset_max; (void)0) {
		OVERLAPPED overlapped_offset = {
			.Offset     = (offset & 0x00000000ffffffffull),
			.OffsetHigh = (offset & 0xffffffff00000000ull) >> 32,
		};
		DWORD received_size;
		DWORD const requested_size = (DWORD)min_u64(offset_max - offset, chunk_limit);

		// @note for synchronous reads only (without `FILE_FLAG_OVERLAPPED` flag)
		// otherwise if `ok == FALSE` and `GetLastError() == ERROR_IO_PENDING`
		// overlapped offset should be kept intact until the response
		BOOL const ok = ReadFile(file->handle, (u8*)buffer + ret, requested_size, &received_size, &overlapped_offset);
		Assert(ok == TRUE, "[os] `ReadFile` failed\n");

		offset += received_size;
		ret += received_size;
		if (received_size < requested_size)
			break;
	}
	return ret;
	// https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-readfile
}

// ---- ---- ---- ----
// graphics
// ---- ---- ---- ----

void os_surface_get_size(u32 * width, u32 * height) {
	*width = fl_os.width;
	*height = fl_os.height;
}

// ---- ---- ---- ----
// time
// ---- ---- ---- ----

u64 os_timer_get_nanos(void) {
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	LONGLONG const elapsed = value.QuadPart - fl_os.timer_initial.QuadPart;
	return mul_div_u64((u64)elapsed, AsNanos(1), (u64)fl_os.timer_frequency.QuadPart);
}

// ---- ---- ---- ----
// memory
// ---- ---- ---- ----

void * os_memory_heap(void * ptr, size_t size) {
	if (size > 0)
	{
		if (ptr == NULL)
		{
			void * ret = HeapAlloc(fl_os.heap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, size);
			AssertF(ret != NULL, "[os] `HeapAlloc(%zu)` failed\n", size);
			return ret;
		}
		void * ret = HeapReAlloc(fl_os.heap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, ptr, size);
		AssertF(ret != NULL, "[os] `HeapReAlloc(0x%p, %zu)` failed\n", ptr, size);
		return ret ? ret : ptr;
	}
	if (ptr != NULL)
	{
		BOOL const ok = HeapFree(fl_os.heap, 0, ptr);
		AssertF(ok == TRUE, "[os] `HeapFree(0x%p)` failed\n", ptr);
		return ok ? NULL : ptr;
	}
	return NULL;
	// MEMORY_ALLOCATION_ALIGNMENT
	// https://learn.microsoft.com/windows/win32/api/heapapi/nf-heapapi-heapalloc
	// https://learn.microsoft.com/windows/win32/api/heapapi/nf-heapapi-heaprealloc
	// https://learn.microsoft.com/windows/win32/api/heapapi/nf-heapapi-heapfree
}

void * os_memory_reserve(size_t size) {
	// `size` is rounded up to the next page boundary
	void * ret = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
	AssertF(ret != NULL, "[os] `VirtualAlloc(NULL, %zu, MEM_RESERVE)` failed\n", size);
	return ret;
	// https://learn.microsoft.com/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
}

void os_memory_release(void * ptr, size_t size) {
	(void)size; // @note `size` is not used
	// release frees the entire region
	BOOL const ok = VirtualFree(ptr, 0, MEM_RELEASE);
	AssertF(ok == TRUE, "[os] `VirtualFree(0x%p, 0, MEM_RELEASE)` failed\n", ptr);
	// https://learn.microsoft.com/windows/win32/api/memoryapi/nf-memoryapi-virtualfree
}

void os_memory_commit(void * ptr, size_t size) {
	void const * mem = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
	AssertF(mem != NULL, "[os] `VirtualAlloc(0x%p, %zu, MEM_COMMIT)` failed\n", ptr, size);
	// https://learn.microsoft.com/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
}

void os_memory_decommit(void * ptr, size_t size) {
	// 1) if `ptr` is the base and `size == 0`, decommits the entire region
	// 2) otherwise decommits all memory pages that contain the range
	BOOL const ok = VirtualFree(ptr, size, MEM_DECOMMIT);
	AssertF(ok == TRUE, "[os] `VirtualFree(0x%p, %zu, MEM_DECOMMIT)` failed\n", ptr, size);
	// https://learn.microsoft.com/windows/win32/api/memoryapi/nf-memoryapi-virtualfree
}

// ---- ---- ---- ----
// shared
// ---- ---- ---- ----

void * os_shared_load(char * name) {
	void * ret = LoadLibraryA(name);
	AssertF(ret != NULL, "[os] `LoadLibraryA(\"%s\")` failed\n", name);
	return ret;
}

void os_shared_drop(void * inst) {
	BOOL const ok = FreeLibrary(inst);
	AssertF(ok == TRUE, "[os] `FreeLibrary(\"0x%p\")` failed\n", inst);
}

void * os_shared_find(void * inst, char * name) {
	void * ret = (void *)GetProcAddress(inst, name);
	AssertF(ret != NULL, "[os] `GetProcAddress(0x%p, \"%s\")` failed\n", inst, name);
	return ret;
}

// ---- ---- ---- ----
// thread
// ---- ---- ---- ----

struct OS_Thread {
	struct OS_Thread_IInfo info;
	DWORD  id;
	HANDLE handle;
};

AttrFileLocal()
DWORD os_thread_entry_point(LPVOID data) {
	struct OS_Thread const * thread = data;
	thread->info.function(thread->info.context);
	return 0;
}

struct OS_Thread * os_thread_init(struct OS_Thread_IInfo info) {
	struct OS_Thread * ret = os_memory_heap(NULL, sizeof(*ret));
	ret->info = info;
	ret->handle = CreateThread(NULL, 0, os_thread_entry_point, ret, 0, &ret->id);
	Assert(ret->handle != NULL, "[os] `CreateThread` failed\n");
	return ret;
	// https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread
}

void os_thread_free(struct OS_Thread * thread) {
	BOOL const ok = CloseHandle(thread->handle);
	Assert(ok == TRUE, "[os] `CloseHandle` failed\n");
	mem_zero(thread, sizeof(*thread));
	os_memory_heap(thread, 0);
}

void os_thread_join(struct OS_Thread * thread) {
	WaitForSingleObject(thread->handle, INFINITE);
}

// ---- ---- ---- ----
// internal
// ---- ---- ---- ----

#if BUILD_TARGET == BUILD_TARGET_GRAPHICAL
# include <stdlib.h>

int main(int argc, char * argv[]);

WINAPI
int WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
	return main(__argc, __argv);
}
#endif

// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
// Global Variable NvOptimusEnablement (new in Driver Release 302)
// Starting with the Release 302 drivers, application developers can direct the Optimus driver at runtime to use the High Performance Graphics to render any application–even those applications for which there is no existing application profile. They can do this by exporting a global variable named NvOptimusEnablement. The Optimus driver looks for the existence and value of the export. Only the LSB of the DWORD matters at this time. Avalue of 0x00000001 indicates that rendering should be performed using High Performance Graphics. A value of 0x00000000 indicates that this method should beignored.
__declspec(dllexport) extern DWORD NvOptimusEnablement;
DWORD NvOptimusEnablement = 1;

// https://gpuopen.com/learn/amdpowerxpressrequesthighperformance
// Many Gaming and workstation laptops are available with both (1) integrated power saving and (2) discrete high performance graphics devices. Unfortunately, 3D intensive application performance may suffer greatly if the best graphics device is not selected. For example, a game may run at 30 Frames Per Second (FPS) on the integrated GPU rather than the 60 FPS the discrete GPU would enable. As a developer you can easily fix this problem by adding only one line to your executable’s source code:
// Yes, it’s that easy. This line will ensure that the high-performance graphics device is chosen when running your application.
__declspec(dllexport) extern DWORD AmdPowerXpressRequestHighPerformance;
DWORD AmdPowerXpressRequestHighPerformance = 1;
