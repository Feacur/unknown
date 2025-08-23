#if !defined (UKWN_ENTITY_META_H)
#define UKWN_ENTITY_META_H

#include "base.h"

struct Entity;
typedef void Entity_Func(struct Entity * base);

struct Entity_VTable {
	Entity_Func * init;
	Entity_Func * free;
	Entity_Func * tick;
	Entity_Func * draw;
};

struct Entity_Meta {
	u16 type; // `enum Entity_Type`
	u16 size;
	struct Entity_VTable vtable;
};

struct Entity_Meta const * entity_meta_get(u16 type);

#endif
