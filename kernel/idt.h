#ifndef IDT_H_INCLUDED
#define IDT_H_INCLUDED
#include <stdint.h>

typedef struct idt_ptr_t {
  uint16_t limit;               // Size of the IDT 
  uint64_t base;                // Start of the IDT
} __attribute__((packed)) idt_ptr_t;

typedef struct idt_entry_t {
  uint16_t offset_low;          // offset bits 0..15
  uint16_t selector;            // a code segment selector in GDT or LDT
  uint8_t  ist;                 // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
  uint8_t  type_attributes;     // gate type, dpl, and p fields
  uint16_t offset_mid;          // offset bits 16..31
  uint32_t offset_high;         // offset bits 32..63
  uint32_t zero;                // reserved
} __attribute__((packed)) idt_entry_t;

void load_idt();
void interrupt_handler(void);
#endif
