#include "os.h"

#include "_experiments/tloz_oot_entities/type.h"

// define subtype
#include "en_test.h"

AttrFileLocal()
void en_test_init(struct Entity * inst) {
	struct En_Test * this = (struct En_Test *)inst;
	this->counter = 0;
}

AttrFileLocal()
void en_test_free(struct Entity * inst) {
	struct En_Test * this = (struct En_Test *)inst;
	mem_zero(this, sizeof(*this));
}

AttrFileLocal()
void en_test_tick(struct Entity * inst) {
	struct En_Test * this = (struct En_Test *)inst;
	this->counter++;
	u64 const time = os_timer_get_nanos();
	fmt_print("[en_test_tick] %f %d\n", (double)time / AsNanos(1), this->counter);
}

AttrFileLocal()
void en_test_draw(struct Entity * inst) {
	struct En_Test * this = (struct En_Test *)inst;
	u64 const time = os_timer_get_nanos();
	fmt_print("[en_test_draw] %f %d\n", (double)time / AsNanos(1), this->counter);
}

// define meta
#include "_experiments/tloz_oot_entities/meta.h"

struct Entity_Meta const ENTITY_META_TEST = {
	.type = ENTITY_TYPE_TEST,
	.size = sizeof(struct En_Test),
	.vtable = {
		.init = &en_test_init,
		.free = &en_test_free,
		.tick = &en_test_tick,
		.draw = &en_test_draw,
	},
};
