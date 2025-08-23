#include "os.h"
#include "meta.h"

//
#include "entity.h"

AttrFileLocal()
struct Entities {
	u16 count;
} fl_entities;

struct Entity * entity_create(u16 type) {
	struct Entity_Meta const * meta = entity_meta_get(type);
	struct Entity * inst = os_memory_heap(NULL, meta->size);
	if (inst == NULL)
		return NULL;

	mem_zero(inst, meta->size);
	inst->type = type;

	if (meta->vtable.init != NULL)
		meta->vtable.init(inst);
	else DbgPrint("[entity_create] `meta->init` is `NULL`\n");

	fl_entities.count++;

	return inst;
}

void entity_delete(struct Entity * inst) {
	struct Entity_Meta const * meta = entity_meta_get(inst->type);

	fl_entities.count--;

	if (meta->vtable.free != NULL)
		meta->vtable.free(inst);
	else DbgPrint("[entity_delete] `meta->free` is `NULL`\n");

	os_memory_heap(inst, 0);
}

void entity_tick(struct Entity * inst) {
	struct Entity_Meta const * meta = entity_meta_get(inst->type);
	if (meta->vtable.tick != NULL)
		meta->vtable.tick(inst);
}

void entity_draw(struct Entity * inst) {
	struct Entity_Meta const * meta = entity_meta_get(inst->type);
	if (meta->vtable.draw != NULL)
		meta->vtable.draw(inst);
}
