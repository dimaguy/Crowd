#include <idt.h>
#include <stdint.h>
#include <stddef.h>

idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr = {
  .limit = sizeof(idt_entries) - 1,
  .base = (uint64_t)&idt_entries
};
extern uint64_t isrs[256];

void load_idt() {
    for (int i = 0; i < 256; i++) {
        idt_entries[i] = (idt_entry_t) {
            .offset_low = (uint16_t)(isrs[i] & 0xFFFF),
            .selector = 0x28,
            .ist = 0,
            .type_attributes = 0x8E,
            .offset_mid = (uint16_t)((isrs[i] >> 16) & 0xFFFF),
            .offset_high = (uint32_t)((isrs[i] >> 32) & 0xFFFFFFFF),
            .zero = 0
        };
    }
    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}

void interrupt_handler(void) {
    for (;;) {
        asm ("hlt");
    }
}