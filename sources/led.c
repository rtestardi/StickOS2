// *** led.c **********************************************************
// this file implements basic LED controls.

#include "main.h"

enum led {
    led_red = 0,  // dtin3
    led_blue = 0,  // dtin3
    led_max
};

static int led_count[led_max];
static int last_led_count[led_max];

static enum led led_blink;


void
led_unknown_progress(void)
{
    led_count[led_blue]++;
}

// this function turns an LED on or off.
static
void
led_set(enum led n, int on)
{
    assert (n >= 0 && n < led_max);

#if ! STICK_GUEST
    pin_set(pin_assignments[pin_assignment_heartbeat], pin_type_digital_output, 0, on);
#endif // ! STICK_GUEST
}

void
led_timer_poll()
{
    static int calls;

    calls++;

    // normal led processing
    if (led_count[led_blink] != last_led_count[led_blink]) {
        led_set(led_blink, calls%2);  // blink fast
        last_led_count[led_blink] = led_count[led_blink];
    } else {
        led_set(led_blink, (calls/4)%2);  // blink slow
    }
}

#if ! STICK_GUEST

// this function displays a diagnostic code on a LED.
void
led_line(int line)
{
    int i;
    int j;
    int n;
    int inv;

    if (debugger_attached) {
        asm_halt();
    } else {
        splx(7);

        n = 0;
        inv = 0;
        for (;;) {
            if (n++ >= 10) {
            }

            for (i = 1000; i > 0; i /= 10) {
                if (line < i) {
                    continue;
                }
                j = (line/i)%10;
                if (! j) {
                    j = 10;
                }
                while (j--) {
                    led_set(led_red, 1^inv);
                    delay(200);
                    led_set(led_red, 0^inv);
                    delay(200);
                }
                delay(1000);
            }
            delay(3000);
            inv = ! inv;
            led_set(led_red, 0^inv);
            delay(3000);
        }
    }
}

// this function displays a diagnostic code on a LED.
void
led_hex(int hex)
{
    int i;
    int j;
    int n;
    int inv;

    if (debugger_attached) {
        asm_halt();
    } else {
        splx(7);

        n = 0;
        inv = 0;
        for (;;) {
            if (n++ >= 10) {
            }

            for (i = 0x10000000; i > 0; i /= 16) {
                if (hex < i) {
                    continue;
                }
                j = (hex/i)%16;
                if (! j) {
                    j = 16;
                }
                while (j--) {
                    led_set(led_red, 1^inv);
                    delay(200);
                    led_set(led_red, 0^inv);
                    delay(200);
                }
                delay(1000);
            }
            delay(3000);
            inv = ! inv;
            led_set(led_red, 0^inv);
            delay(3000);
        }
    }
}

#endif // ! STICK_GUEST

// this function initializes the led module.
void
led_initialize(void)
{
#if ! STICK_GUEST
    pin_assign(pin_assignment_heartbeat, pin_assignments[pin_assignment_heartbeat]);
    pin_set(pin_assignments[pin_assignment_heartbeat], pin_type_digital_output, 0, true);
#endif // ! STICK_GUEST
}

