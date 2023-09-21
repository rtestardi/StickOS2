// *** serial.h ************************************************************

#define SERIAL_UART  1

extern bool serial_active;

extern int serial_baudrate;

void
serial_disable(void);

void
serial_command_ack(void);

void
serial_send(const byte *buffer, int length);

void
serial_initialize(void);

void
serial_uninitialize(void);
