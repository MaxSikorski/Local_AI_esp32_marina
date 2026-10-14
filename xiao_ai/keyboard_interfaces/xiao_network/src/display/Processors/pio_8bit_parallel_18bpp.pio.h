 
 
 

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

 
 
 

#define tft_io_wrap_target 11
#define tft_io_wrap 31

#define tft_io_offset_block_fill 0u
#define tft_io_offset_start_tx 11u
#define tft_io_offset_start_8 18u
#define tft_io_offset_set_addr_window 21u

static const uint16_t tft_io_program_instructions[] = {
    0x98a0,  
    0xa027,  
    0x80a0,  
    0xa047,  
    0xb8e1,  
    0x7110,  
    0xb942,  
    0x7108,  
    0xb942,  
    0x7108,  
    0x1884,  
             
    0x98a0,  
    0x7110,  
    0xb942,  
    0x7108,  
    0xb942,  
    0x7108,  
    0x180b,  
    0x98a0,  
    0x7100,  
    0x180b,  
    0xf822,  
    0xe000,  
    0x80a0,  
    0x7000,  
    0x003e,  
    0x98a0,  
    0xe001,  
    0x7108,  
    0x19fc,  
    0x1856,  
    0xe001,  
             
};

#if !PICO_NO_HARDWARE
static const struct pio_program tft_io_program = {
    .instructions = tft_io_program_instructions,
    .length = 32,
    .origin = -1,
};

static inline pio_sm_config tft_io_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + tft_io_wrap_target, offset + tft_io_wrap);
    sm_config_set_sideset(&c, 2, true, false);
    return c;
}
#endif
