#include <serial.h>
#include <IO.h>

uint16_t port = 0xe9;

void serial_print (char *str) {
    while (*str) {
        serial_putchar(*str);
        str++;
    }
}

void serial_putchar(char c) {
    outb(port, (uint8_t) c);
}