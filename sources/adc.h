// *** adc.h **********************************************************

enum {
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    adc_num_channel = 8
#else
#error
#endif
};

void
adc_timer_poll(bool debouncing);

void
adc_sleep();

int
adc_get_value(int offset, int pin_qual);

void
adc_initialize(void);

