#pragma once

#include <stdint.h>

struct IDT {
    uint16_t offset_1;  // Offset Bits 0 -> 15
    uint16_t selector;  // Code Segment Selector in GDT
    uint8_t ist;        // Bits 0 -> 2 Hold Interrupt Stack Table Offset (Rest are 0)
    uint8_t type_attr;  // Gate Type (DPL, P Fields)
    uint16_t offset_2;  // Offset Bits 16 -> 31
    uint32_t offset_3;  // Offset Bits 32 -> 63
    uint32_t reserved;  // Reserved
}