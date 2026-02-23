#if !defined (UKWN_OS_H)
#define UKWN_OS_H

#include "base.h" // IWYU pragma: keep

AttrGlobal() AttrExternal()
struct OS_Info {
	size_t page_size;
} g_os_info;

struct OS_IInfo {
	u16          window_size_x;
	u16          window_size_y;
	char const * window_caption;
	void (* on_resize)(void);
};

void os_init(struct OS_IInfo info);
void os_free(void);

void os_tick(void);
void os_sleep(u64 nanos);

bool os_should_quit(void);

bool os_exit(int code);

// ---- ---- ---- ----
// graphics
// ---- ---- ---- ----

void os_surface_get_size(u32 * width, u32 * height);

void os_vulkan_push_extensions(u32 * counter, char const ** buffer);
void * os_vulkan_create_surface(void * instance, void const * allocator);

// ---- ---- ---- ----
// file
// ---- ---- ---- ----

struct OS_File;
struct OS_File_IInfo {
	char const * name;
};

struct OS_File * os_file_init(struct OS_File_IInfo info);
void os_file_free(struct OS_File * file);

u64 os_file_get_size(struct OS_File const * file);
u64 os_file_get_write_nanos(struct OS_File const * file);

u64 os_file_read(struct OS_File const * file, u64 offset_min, u64 offset_max, void * buffer);

// ---- ---- ---- ----
// time
// ---- ---- ---- ----

u64 os_timer_get_nanos(void);

// ---- ---- ---- ----
// memory
// ---- ---- ---- ----

void * os_memory_heap(void * ptr, size_t size);

void * os_memory_reserve(size_t size);
void   os_memory_release(void * ptr, size_t size);

void   os_memory_commit(void * ptr, size_t size);
void   os_memory_decommit(void * ptr, size_t size);

// ---- ---- ---- ----
// shared
// ---- ---- ---- ----

void * os_shared_load(char * name);
void   os_shared_drop(void * inst);
void * os_shared_find(void * inst, char * name);

// ---- ---- ---- ----
// thread
// ---- ---- ---- ----

struct OS_Thread;
struct OS_Thread_IInfo {
	void (* function)(void * context);
	void * context;
};

struct OS_Thread * os_thread_init(struct OS_Thread_IInfo info);
void os_thread_free(struct OS_Thread * thread);

void os_thread_join(struct OS_Thread * thread);

#endif
