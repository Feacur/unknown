#include "os.h"
#include "gfx.h"

int main(int argc, char * argv[]) {
	fmt_print("[main] args:\n");
	for (int i = 0; i < argc; i++) {
		fmt_print("- %s\n", argv[i]);
	}
	fmt_print("\n");

	os_init((struct OS_IInfo){
		.window_size_x = 900,
		.window_size_y = 600,
		.window_caption = "unknown",
	});
	gfx_init();

	u64 const nanos_fixed_delta           = AsNanos(0.020000000);
	u64 const nanos_variable_delta_limit  = AsNanos(0.050000000);
	u64 const nanos_variable_delta_target = AsNanos(0.016666666);
	Assert(nanos_variable_delta_limit >= nanos_variable_delta_target, "timer limit should be >= target\n");

	u64 nanos_variable_delta = nanos_variable_delta_target;
	u64 nanos_fixed_accumulator = 0;

	while (!os_should_quit()) {
		u64 const nanos_frame_start = os_timer_get_nanos();

		// -- poll events
		os_tick();

		// -- fixed timestep
		nanos_fixed_accumulator += nanos_variable_delta;
		while (nanos_fixed_accumulator > nanos_fixed_delta) {
			nanos_fixed_accumulator -= nanos_fixed_delta;
		}

		// -- variable timestep
		gfx_tick();

		// -- wait for the frame to end
		u64 const nanos_payload_end = os_timer_get_nanos();
		u64 const nanos_payload_time = nanos_payload_end - nanos_frame_start;
		u64 const nanos_frame_delta_left = nanos_variable_delta_target > nanos_payload_time
			? nanos_variable_delta_target - nanos_payload_time
			: 0;
		os_sleep(nanos_frame_delta_left);

		// -- update variable delta
		u64 const nanos_frame_end = os_timer_get_nanos();
		u64 const nanos_frame_delta = nanos_frame_end - nanos_frame_start;
		nanos_variable_delta = clamp_u64(nanos_frame_delta, 1, nanos_variable_delta_limit);
	}

	gfx_free();
	os_free();

	return 0;
}
