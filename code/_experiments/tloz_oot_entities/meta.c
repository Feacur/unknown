#include "meta.h"
#include "type.h"

AttrFileLocal()
struct Entity_Meta const fl_meta_entity_default;
#define DEFINE_ENTITY(type) extern struct Entity_Meta const ENTITY_META_ ## type;
#include "_generator.h"

AttrFileLocal()
struct Entity_Meta const * fl_meta_entity[] = {
	&fl_meta_entity_default,
	#define DEFINE_ENTITY(type) &ENTITY_META_ ## type,
	#include "_generator.h"
};

struct Entity_Meta const * entity_meta_get(u16 type) {
	if (type >= ENTITY_TYPE_MAX) {
		DbgPrintF("[entity_get_meta] `type` %u is out of range of %u\n", type, ENTITY_TYPE_MAX);
		return &fl_meta_entity_default;
	}
	return fl_meta_entity[type];
}
