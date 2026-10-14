 
 
 
 

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

 
 
 

#define tft_io_wrap_target 27
#define tft_io_wrap 31

#define tft_io_offset_start_8 0u
#define tft_io_offset_set_addr_window 3u
#define tft_io_offset_block_fill 17u
#define tft_io_offset_start_tx 27u

static const uint16_t tft_io_program_instructions[] = {
    0x90a0,  
    0x6019,  
    0x181e,  
    0xf022,  
    0xe000,  
    0x90a0,  
    0x6019,  
    0xb842,  
    0x7001,  
    0x18e8,  
    0xf001,  
    0x003b,  
    0x80a0,  
    0x7001,  
    0x18ed,  
    0x1044,  
    0x001b,  
    0x90a0,  
    0xa027,  
    0x80a0,  
    0xa047,  
    0xb0e1,  
    0x7011,  
    0xb842,  
    0x7001,  
    0x18f8,  
    0x1095,  
             
    0x90a0,  
    0x7011,  
    0xb842,  
    0x7001,  
    0x18fe,  
             
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
