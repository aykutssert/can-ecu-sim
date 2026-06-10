/*
 * Priority scheduling / timing demo.
 *
 * A high-priority periodic task (simulating a hard real-time control loop,
 * e.g. "read sensor and send CAN frame every 5ms") runs alongside a
 * low-priority thread that never voluntarily yields (simulating background
 * work like logging or diagnostics). The periodic task measures its own
 * wake-up jitter against an absolute deadline to show that priority-based
 * preemption keeps it on schedule despite the load thread hogging the CPU.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define PERIOD_MS 5
#define ITERATIONS 500

#define HIGH_PRIO 1
#define LOAD_PRIO 5

K_THREAD_STACK_DEFINE(load_stack, 1024);
static struct k_thread load_thread_data;

static void load_thread_entry(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	/* Continuous low-priority CPU load: never blocks voluntarily. */
	while (1) {
		k_busy_wait(1000);
	}
}

int main(void)
{
	const k_ticks_t period = k_ms_to_ticks_ceil32(PERIOD_MS);
	k_ticks_t worst = 0;
	int64_t total = 0;
	k_ticks_t deadline;

	k_thread_create(&load_thread_data, load_stack, K_THREAD_STACK_SIZEOF(load_stack),
			load_thread_entry, NULL, NULL, NULL, LOAD_PRIO, 0, K_NO_WAIT);

	k_thread_priority_set(k_current_get(), HIGH_PRIO);

	deadline = k_uptime_ticks();
	for (int i = 0; i < ITERATIONS; i++) {
		deadline += period;
		k_sleep(K_TIMEOUT_ABS_TICKS(deadline));

		k_ticks_t now = k_uptime_ticks();
		k_ticks_t jitter = now - deadline;

		if (jitter < 0) {
			jitter = 0;
		}
		total += jitter;
		if (jitter > worst) {
			worst = jitter;
		}
	}

	printk("periodic task: %d iterations, period %dms, high prio %d vs load prio %d\n",
	       ITERATIONS, PERIOD_MS, HIGH_PRIO, LOAD_PRIO);
	printk("wakeup jitter: avg=%lldus worst=%lldus (tick=%lldus)\n",
	       (long long)k_ticks_to_us_floor64(total) / ITERATIONS,
	       (long long)k_ticks_to_us_floor64(worst),
	       (long long)k_ticks_to_us_floor64(1));

	return 0;
}
