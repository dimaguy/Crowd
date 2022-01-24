#ifndef GDT_H_INCLUDED
#define GDT_H_INCLUDED
#include <stdint.h>

typedef struct gdt_ptr_t {
  uint16_t limit;               /* Size of the GDT */
  uint64_t base;                /* Start of the GDT */
} __attribute__((packed)) gdt_ptr_t;

void load_gdt();
#endif