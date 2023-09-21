// *** i2c.h **********************************************************

extern void
i2c_start(int address);

extern void
i2c_read_write(bool write, byte *buffer, int length);

extern void
i2c_stop(void);

extern void
i2c_uninitialize(void);

extern void
i2c_initialize(void);

