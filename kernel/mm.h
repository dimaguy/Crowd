#ifndef MM_H_INCLUDED
#define MM_H_INCLUDED
#include <stdint.h>
#include <limits.h>
#include <stddef.h> // size_t
#include <stivale2.h>
#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)
#define ALIGN_UP(x, a) ({ \
    typeof(x) value = x; \
    typeof(a) align = a; \
    value = DIV_ROUNDUP(value, align) * align; \
    value; \
})
#define DIV_ROUNDUP(a, b) (((a) + ((b) - 1)) / (b))
#define PTE_PRESENT 0x1
#define PTE_WRITABLE 0x2
typedef uint8_t word_t;
enum { BITS_PER_WORD = sizeof(word_t) * CHAR_BIT };
typedef struct page_table_t {
    uint64_t entries[512];
} page_table; //pml4
extern page_table *kernel_pt;
void *memset(void *_dst, int _val, size_t len); //TODO: move it from here
void set_bit(word_t *words, int n);
void clear_bit(word_t *words, int n);
int get_bit(word_t *words, int n);
uint64_t find_free_bit(word_t *words, int size);
void init_mm(struct stivale2_struct *stivale2_struct);
void printmmap(struct stivale2_struct_tag_memmap *memmap_str_tag);
uint64_t pmalloc(uint64_t size);
void pmfree(uint64_t addr, uint64_t size);
page_table *ptalloc();
void ptfree(page_table *pt);
void pt_map(page_table *pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags);
void pt_unmap(page_table *pml4, uint64_t vaddr);
#endif