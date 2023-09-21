// *** qspi.h *********************************************************

extern void
qspi_transfer(bool cs, byte *buffer, int length);

extern void
qspi_baud_fast(void);

extern void
qspi_uninitialize(void);

extern void
qspi_initialize(void);

