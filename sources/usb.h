// *** usb ******************************************************************

#define SETUP_SIZE  8

#define SETUP_TYPE_STANDARD  0
#define SETUP_TYPE_CLASS  1
#define SETUP_TYPE_VENDOR  2

#define SETUP_RECIP_DEVICE  0
#define SETUP_RECIP_INTERFACE  1
#define SETUP_RECIP_ENDPOINT  2

#define REQUEST_GET_STATUS  0x00
#define REQUEST_CLEAR_FEATURE  0x01
#define REQUEST_SET_ADDRESS  0x05
#define REQUEST_GET_DESCRIPTOR  0x06
#define REQUEST_GET_CONFIGURATION  0x08
#define REQUEST_SET_CONFIGURATION  0x09

#define DEVICE_DESCRIPTOR  1
#define CONFIGURATION_DESCRIPTOR  2
#define STRING_DESCRIPTOR  3
#define INTERFACE_DESCRIPTOR  4
#define ENDPOINT_DESCRIPTOR  5
#define DEVICE_CAPABILITY_DESCRIPTOR  16

#define PLATFORM_CAPABILITY  5

#define TOKEN_STALL  0x0e  // for mst stall disaster

struct setup {
    byte requesttype;  // 7:in, 6-5:type, 4-0:recip
    byte request;
    short value;
    short index;
    short length;
};

extern volatile bool usb_in_isr;  // set when in isr

extern bool cdcacm_attached;  // set when cdcacm host is attached
extern uint32 cdcacm_attached_count;

extern char usb_serial[33];

extern byte bulk_in_ep;
extern byte bulk_out_ep;
extern byte int_ep;

// *** host ***

// initialize a setup data0 buffer
void
usb_setup(int in, int type, int recip, byte request, short value, short index, short length, struct setup *setup);

// perform a usb host/device control transfer
int
usb_control_transfer(struct setup *setup, byte *buffer, int length);

// perform a usb host/device bulk transfer
int
usb_bulk_transfer(int in, byte *buffer, int length, bool null_or_short);

// detach from the device and prepare to re-attach
void
usb_host_detach(void);

// *** device ***

// enqueue a packet to the usb engine for transfer to/from the host
void
usb_device_enqueue(int endpoint, bool tx, byte *buffer, int length);

typedef void (*usb_reset_cbfn)(void);
typedef int (*usb_control_cbfn)(struct setup *setup, byte *buffer, int length);
typedef int (*usb_bulk_cbfn)(bool in, byte *buffer, int length);
typedef void (*usb_interrupt_cbfn)(void);

void
usb_register(usb_reset_cbfn reset, usb_control_cbfn control_transfer, usb_bulk_cbfn bulk_transfer);

void
usb_device_descriptor(const byte *descriptor, int length);

void
usb_configuration_descriptor(const byte *descriptor, int length);

void
usb_string_descriptor(const byte *descriptor, int length);

// *** init ***

void
usb_initialize(void);

