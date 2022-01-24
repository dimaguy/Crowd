#include <gdt.h>
#include <stdint.h>
uint64_t GDT[9] ={
    0x0000000000000000,
    0x00009a000000ffff,
    0x000093000000ffff,
    0x00cf9a000000ffff,
    0x00cf93000000ffff,
    0x00af9b000000ffff,
    0x00af93000000ffff,
    0x00affb000000ffff,
    0x00aff3000000ffff
};

gdt_ptr_t gdt_ptr = {
  .limit = sizeof(GDT) - 1,
  .base = (uint64_t)&GDT
};
void load_gdt() {__asm__ __volatile__("lgdt %0" : : "m" (gdt_ptr));}