// *** led.h **********************************************************

void
led_unknown_progress(void);  // blue fast (ignored if happy)

void
led_timer_poll(void);

void
led_line(int line);  // red crashed

void
led_hex(int hex);

void
led_initialize(void);

