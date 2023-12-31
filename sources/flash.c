// *** flash.c ********************************************************
// this file implements the low level flash control and access, as well
// as the "s19 upgrade" or "hex upgrade" functionality for upgrading
// firmware.

#include "main.h"

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#define NVMDATA  NVMDATA0
#endif

#define NVMOP_PAGE_ERASE        0x4      // Page erase operation
#define NVMOP_WORD_PGM          0x1      // Word program operation
#define NVMOP_QWORD_PGM         0x2      // Quad Word program operation

#undef NVMCON_WREN
#undef NVMCON_WR
#define NVMCON_WREN _NVMCON_WREN_MASK
#define NVMCON_WR  _NVMCON_WR_MASK

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    // NVMOP can be written only when WREN is zero. So, clear WREN.
    /* Clear and Set, as NVMCON contains status bits and hence need to be accessed atomically.
     * Using bit field access may erroneously cause status bits to get cleared */
    // Set WREN to enable writes to the WR bit and to prevent NVMOP modification
    // Write the unlock key sequence
    // Start the operation
    // Wait for WR bit to clear
        // NULL
    // assert no errors
#define FLASH_OPERATION(nvmop) \
    NVMCONCLR = _NVMCON_WREN_MASK; \
    NVMCONCLR = _NVMCON_NVMOP_MASK; \
    NVMCONSET = ( _NVMCON_NVMOP_MASK & (((uint32_t)nvmop) << _NVMCON_NVMOP_POSITION) ); \
    NVMCONSET = _NVMCON_WREN_MASK; \
    NVMKEY = 0x0; \
    NVMKEY = 0xAA996655; \
    NVMKEY = 0x556699AA; \
    NVMCONSET = _NVMCON_WR_MASK; \
    while (NVMCON & NVMCON_WR) { \
    } \
    assert_ram(! (NVMCON & NVMCON_WR)); \
    assert_ram(! (NVMCON & (_NVMCON_WRERR_MASK | _NVMCON_LVDERR_MASK)));
#else
#error
#endif

static
void
__attribute__((nomips16))
flash_operation(unsigned int nvmop)
{
    FLASH_OPERATION(nvmop);
}

void
flash_erase_pages(uint32 *addr_in, uint32 npages_in)
{
#if SODEBUG
    int i;
#endif
    int x;
    uint32 *addr;
    uint32 npages;

    addr = addr_in;
    npages = npages_in;

    x = splx(7);

    DMACONSET = _DMACON_SUSPEND_MASK;
    while (! DMACONbits.SUSPEND) {
        // NULL
    }

    // while there are more pages to erase...
    while (npages) {
        // Convert Address to Physical Address
        NVMADDR = KVA_TO_PA((unsigned int)addr);

        // Unlock and Erase Page
        flash_operation(NVMOP_PAGE_ERASE);

        npages--;
        addr += FLASH_PAGE_SIZE/sizeof(uint32);
    }

    DMACONCLR = _DMACON_SUSPEND_MASK;

    (void)splx(x);

#if SODEBUG
    for (i = 0; i < npages_in*FLASH_PAGE_SIZE/sizeof(uint32); i++) {
        assert(addr_in[i] == -1);
    }
#endif
}

void
flash_write_words(uint32 *addr_in, uint32 *data_in, uint32 nwords_in)
{
#if SODEBUG
    int i;
#endif
    int x;
    uint32 *addr;
    uint32 *data;
    uint32 nwords;

    addr = addr_in;
    data = data_in;
    nwords = nwords_in;

    x = splx(7);

    DMACONSET = _DMACON_SUSPEND_MASK;
    while (! DMACONbits.SUSPEND) {
        // NULL
    }

    while (nwords--) {
        // Convert Address to Physical Address
        NVMADDR = KVA_TO_PA((unsigned int)addr);

        // Load data into NVMDATA register
        NVMDATA = *data;

        // Unlock and Write Word
        flash_operation(NVMOP_WORD_PGM);

        addr++;
        data++;
    }

    DMACONCLR = _DMACON_SUSPEND_MASK;

    (void)splx(x);

#if SODEBUG
    for (i = 0; i < nwords_in; i++) {
        assert(addr_in[i] == data_in[i]);
    }
#endif
}

#if UPGRADE
// this function performs the final step of a firmware flash upgrade.
void
__longramfunc__
__attribute__((nomips16))
flash_upgrade_ram(void)
{
#if SODEBUG
    static int i;
    static int rd;
    static int wd;
#endif
    static uint32 *addr;
    static uint32 *data;
    static uint32 nwords;
    static uint32 npages;

    // N.B. this code generates no relocations so we can run it from RAM!!!
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    // un write protect boot memory
    NVMKEY = 0x0;
    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;
    NVMBWPCLR = 0x1f1f;
#endif

    // erase the program flash
    // flash_erase_pages()
    addr = (uint32 *)FLASH_START;
    npages = FLASH_BYTES/2/FLASH_PAGE_SIZE;
    while (npages) {
        NVMADDR = KVA_TO_PA((unsigned int)addr);
        FLASH_OPERATION(NVMOP_PAGE_ERASE);
        npages--;
        addr += FLASH_PAGE_SIZE/sizeof(uint32);
    }
#if SODEBUG
    addr = (uint32 *)FLASH_START;
    npages = FLASH_BYTES/2/FLASH_PAGE_SIZE;
    for (i = 0; i < npages*FLASH_PAGE_SIZE/sizeof(uint32); i++) {
        assert_ram((rd = addr[i]) == -1);
    }
#endif

    // and re-flash the program flash from the staging area
    // flash_write_words()
    addr = (uint32 *)FLASH_START;
    data = (uint32 *)(FLASH_START+FLASH_BYTES/2);
    nwords = (FLASH_BYTES/2 - FLASH2_BYTES)/sizeof(uint32);
    while (nwords) {
        NVMADDR = KVA_TO_PA((unsigned int)addr);
        NVMDATA = *data;
        FLASH_OPERATION(NVMOP_WORD_PGM);
        nwords--;
        addr++;
        data++;
    }
#if SODEBUG
    addr = (uint32 *)FLASH_START;
    data = (uint32 *)(FLASH_START+FLASH_BYTES/2);
    nwords = (FLASH_BYTES/2 - FLASH2_BYTES)/sizeof(uint32);
    for (i = 0; i < nwords; i++) {
        assert_ram((wd = addr[i]) == (rd = data[i]));
    }
#endif

    // erase the boot flash
    // flash_erase_pages()
    addr = (uint32 *)FLASH2_START;
    npages = FLASH2_BYTES/FLASH_PAGE_SIZE;
    while (npages) {
        NVMADDR = KVA_TO_PA((unsigned int)addr);
        FLASH_OPERATION(NVMOP_PAGE_ERASE);
        npages--;
        addr += FLASH_PAGE_SIZE/sizeof(uint32);
    }
#if SODEBUG
    addr = (uint32 *)FLASH2_START;
    npages = FLASH2_BYTES/FLASH_PAGE_SIZE;
    for (i = 0; i < npages*FLASH_PAGE_SIZE/sizeof(uint32); i++) {
        //assert_ram((rd = addr[i]) == -1);
    }
#endif

    // and re-flash the boot flash from the staging area
    // flash_write_words()
    addr = (uint32 *)FLASH2_START;
    data = (uint32 *)(FLASH_START+FLASH_BYTES-FLASH2_BYTES);
    nwords = FLASH2_BYTES/sizeof(uint32);
    while (nwords) {
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
        assert_ram((nwords & 3) == 0);
        // Note: Use only Quad Word program operation (NVMOP<3:0> = 0010) when programming data into the sequence and configuration spaces.
        NVMADDR = KVA_TO_PA((unsigned int)addr);
        NVMDATA0 = *data++;
        addr++;
        nwords--;
        NVMDATA1 = *data++;
        addr++;
        nwords--;
        NVMDATA2 = *data++;
        addr++;
        nwords--;
        NVMDATA3 = *data++;
        addr++;
        nwords--;
        FLASH_OPERATION(NVMOP_QWORD_PGM);
#else
#error
#endif
    }
#if SODEBUG
    addr = (uint32 *)FLASH2_START;
    data = (uint32 *)(FLASH_START+FLASH_BYTES-FLASH2_BYTES);
    nwords = FLASH2_BYTES/sizeof(uint32);
    for (i = 0; i < nwords; i++) {
        assert_ram((wd = addr[i]) == (rd = data[i]));
    }
#endif

    // erase the staging area
    // flash_erase_pages(FLASH_BYTES/2, FLASH_BYTES/2/FLASH_PAGE_SIZE)
    addr = (uint32 *)(FLASH_START+FLASH_BYTES/2);
    npages = FLASH_BYTES/2/FLASH_PAGE_SIZE;
    while (npages) {
        NVMADDR = KVA_TO_PA((unsigned int)addr);
        FLASH_OPERATION(NVMOP_PAGE_ERASE);
        npages--;
        addr += FLASH_PAGE_SIZE/sizeof(uint32);
    }
#if SODEBUG
    addr = (uint32 *)(FLASH_START+FLASH_BYTES/2);
    npages = FLASH_BYTES/2/FLASH_PAGE_SIZE;
    for (i = 0; i < npages*FLASH_PAGE_SIZE/sizeof(uint32); i++) {
        assert_ram((rd = addr[i]) == -1);
    }
#endif

    // reset the MCU
    SYSKEY = 0;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    RSWRSTSET = _RSWRST_SWRST_MASK;
    while (RSWRST, true) {
        // NULL
    }
}
#endif

#if UPGRADE
// this function downloads a new HEX firmware file to a staging
// area, and then installs it by calling a RAM copy of flash_upgrade_ram().
void
flash_upgrade()
{
    int i;
    int n;
    int x;
    int y;
    char c;
    int sum;
    int addr;
    int paddr;
    int vaddr;
    int taddr;
    int type;
    int count;
    int extend;
    char *hex;
    bool done;
    bool error;
    uint32 data;
    uint32 zero;
    bool begun;

    if ((int)end_of_static > FLASH_START+FLASH_BYTES/2-FLASH2_BYTES) {
        printf("code exceeds half of flash\n");
        return;
    }

    // erase the staging area
    flash_erase_pages((uint32 *)(FLASH_START+FLASH_BYTES/2), FLASH_BYTES/2/FLASH_PAGE_SIZE);

    printf("paste HEX upgrade file now...\n");
    terminal_echo = false;

    y = 0;
    done = false;
    error = false;
    begun = false;

    do {
        // wait for an hex command line
        if (main_command) {
            main_command = NULL;
            terminal_command_ack(false);
        }

        while (! main_command) {
            terminal_poll();
        }

        hex = main_command;
        while (isspace(*hex)) {
            hex++;
        }

        if (! *hex) {
            continue;
        }

        sum = 0;

        // parse hex header
        if (*hex++ != ':') {
            printf("\nbad record\n");
            break;
        }

        // 1 byte of count
        n = get2hex(&hex);
        if (n == -1) {
            printf("\nbad count\n");
            break;
        }
        sum += n;
        count = n;

        // 2 bytes of address
        addr = 0;
        for (i = 0; i < 2; i++) {
            n = get2hex(&hex);
            if (n == -1) {
                printf("\nbad address\n");
                break;
            }
            sum += n;
            addr = addr*256+n;
        }
        if (i != 2) {
            break;
        }
        addr = extend<<16 | addr;
        paddr = (int)KVA_TO_PA(addr);
        vaddr = (int)PA_TO_KVA1(paddr);

        // 1 byte of record type
        n = get2hex(&hex);
        if (n == -1) {
           printf("\nbad type\n");
            break;
        }
        sum += n;
        type = n;

        if (type == 1 && count == 0) {
            // eof
            done = true;
        } else if (type == 4 && count == 2) {
            // get the extended address
            extend = 0;
            for (i = 0; i < 2; i++) {
                n = get2hex(&hex);
                if (n == -1) {
                    printf("\nbad address\n");
                    break;
                }
                sum += n;
                extend = extend*256+n;
            }
            if (i != 2) {
                break;
            }
            begun = true;
        } else if (type == 0) {
            // we flash 4 bytes at a time!
            if (count % 4) {
                printf("\nbad count\n");
                break;
            }

            // while there is more data
            while (count) {
                assert(count % 4 == 0);

                // get 4 bytes of data
                data = 0;
                for (i = 0; i < 4; i++) {
                    n = get2hex(&hex);
                    if (n == -1) {
                        printf("\nbad data\n");
                        break;
                    }
                    sum += n;
                    data = data|(n<<(8*i));  // endian
                }
                if (i != 4) {
                    break;
                }

                if (paddr >= KVA_TO_PA(FLASH_START) && paddr < KVA_TO_PA(FLASH_START)+FLASH_BYTES/2-FLASH2_BYTES) {
                    if (*(uint32 *)(vaddr+FLASH_BYTES/2) == -1) {
                        // program the words
                        flash_write_words((uint32 *)(vaddr+FLASH_BYTES/2), &data, 1);
                    } else if (! error) {
                        printf("\nduplicate address 0x%x\n", paddr);
                        error = true;
                    }
                } else if (paddr >= KVA_TO_PA(FLASH2_START) && paddr < KVA_TO_PA(FLASH2_START)+FLASH2_BYTES) {
                    taddr = FLASH_START + FLASH_BYTES - FLASH2_BYTES + (vaddr - FLASH2_START);
                    if (*(uint32 *)taddr == -1) {
                        // this is bootflash data; program the words in the last FLASH2_BYTES of the staging area
                        flash_write_words((uint32 *)taddr, &data, 1);
                    } else if (! error) {
                        printf("\nduplicate address 0x%x\n", paddr);
                        error = true;
                    }
                } else if (! error) {
                    printf("\nbad address 0x%x\n", paddr);
                    error = true;
                }

                addr += 4;
                vaddr += 4;
                paddr += 4;
                assert(count >= 4);
                count -= 4;
            }
            if (count) {
                break;
            }
        } else {
            printf("\nbad record\n");
            break;
        }

        // verify 1 byte of checksum
        n = get2hex(&hex);
        if (n == -1) {
            printf("\nbad checksum\n");
            break;
        }
        sum += n;
        if ((sum & 0xff) != 0x00) {
            printf("\nbad checksum 0x%x\n", sum & 0xff);
            break;
        }

        if (++y%4 == 0) {
            printf(".");
        }
    } while (! done);

    if (! begun || ! done || error) {
        if (main_command) {
            main_command = NULL;
            terminal_command_ack(false);
        }

        // we're in trouble!
        if (! begun) {
            printf(":04 record not found\n");
        }
        if (! done) {
            printf(":01 record not found\n");
        }
        printf("upgrade failed\n");
        terminal_echo = true;

        // erase the staging area
        code_new();
        return;
    }

    // we're committed to upgrade!
    printf("\npaste done!\n");

    printf("programming flash for upgrade...\n");
    printf("wait for heartbeat LED to blink!\n");
    delay(100);

    // N.B. this is an incompatible upgrade; if we crash before we are through,
    // we might be in trouble.

    // disable interrupts
    x = splx(7);

    DMACONSET = _DMACON_SUSPEND_MASK;
    while (! DMACONbits.SUSPEND) {
        // NULL
    }

    delay(100);

    // N.B. the flash upgrade routine is already copied to RAM
    // run it!
    flash_upgrade_ram();

    // we should not come back!
    ASSERT(0);  // stop!
}
#endif

// this function initializes the flash module.
void
flash_initialize(void)
{
}

