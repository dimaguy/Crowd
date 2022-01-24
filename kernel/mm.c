#include <mm.h>
#include <limits.h>
#include <printf.h>
#include <stivale2.h>
#include <stddef.h> // size_t
#include <stdbool.h>
#include <kernel.h>

word_t *bitmap;
uint64_t bitmap_size;
page_table *kernel_pt;

//todo: move memset from here
void *memset(void *_dst, int _val, size_t len)
{
  uint8_t *dst = _dst;
  uint8_t val  = (uint8_t)_val;
  while (len--)
    *dst++ = val;
  return _dst;
}

void set_bit(word_t *words, int n) { 
    words[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
}

void clear_bit(word_t *words, int n) {
    words[WORD_OFFSET(n)] &= ~(1 << BIT_OFFSET(n)); 
}

int get_bit(word_t *words, int n) {
    word_t bit = words[WORD_OFFSET(n)] & (1 << BIT_OFFSET(n));
    return bit != 0; 
}
/*
uint64_t find_empty_bit(word_t *words, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        if (!get_bit(words, i)) {
            return i;
        }
    }
    return -1;
}*/
uint64_t find_empty_contiguous_bits(word_t *words, uint64_t size, uint64_t count) {
    for (uint64_t i = 0; i < size * 8; i++) {
        if (!get_bit(words, i)) {
            bool found = 1;
            for (uint64_t j = 0; found && j < count; j++) {
                if (get_bit(words, i + j)) {
                    found = 0;
                }
            }
            if (found)
                return i;
        }
    }
    return -1;
}

void init_mm(struct stivale2_struct *stivale2_struct) {
    uint64_t last_usable_addr_base = 0;
    uint64_t last_usable_addr_length = 0;
    struct stivale2_struct_tag_memmap *memmap_str_tag = stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);

    for (uint64_t i = 0; i < memmap_str_tag->entries; i++) {
        if (memmap_str_tag->memmap[i].type == STIVALE2_MMAP_USABLE) {
            last_usable_addr_base = memmap_str_tag->memmap[i].base;
            last_usable_addr_length = memmap_str_tag->memmap[i].length;
        }
    }
    // Calculate size of bitmap
    bitmap_size = (((last_usable_addr_base + last_usable_addr_length) / 4096 / BITS_PER_WORD) + 4095) & -4096;
    printf("Last usable address: %u\n", last_usable_addr_base + last_usable_addr_length);
    for (uint64_t i = 0; i < memmap_str_tag->entries; i++) {
        if (memmap_str_tag->memmap[i].type == STIVALE2_MMAP_USABLE) {
            if ((memmap_str_tag->memmap[i].length) > bitmap_size) {
                bitmap = (word_t *)memmap_str_tag->memmap[i].base;
                memmap_str_tag->memmap[i].base += bitmap_size;
                memmap_str_tag->memmap[i].length -= bitmap_size;
            }
        }
    }
    printmmap(memmap_str_tag);
    printf("Memory map size: %u\n", bitmap_size);
    // Initialize the bitmap via memset
    memset(bitmap, 0xff, bitmap_size);

    // Mark free entries on mmap
    for (uint64_t i = 0; i < memmap_str_tag->entries; i++) {
        if (memmap_str_tag->memmap[i].type == STIVALE2_MMAP_USABLE) {
            for (uint64_t j = memmap_str_tag->memmap[i].base/4096; j < (memmap_str_tag->memmap[i].base + memmap_str_tag->memmap[i].length)/4096; j++) {
                clear_bit(bitmap, j);
            }
        }
    }

    // Print the bitmap
    for (uint64_t i = 0; i < bitmap_size; i++) {
        //printf("%d", get_bit(mmap, i));
    }
    printf("\n");
    //set_bit(bitmap, 0);

    struct stivale2_struct_tag_kernel_base_address *kernel_base_addr_tag = stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID);

    struct stivale2_struct_tag_pmrs *pmrs_tag = stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_PMRS_ID);
    
    // Create Kernel Page Table
    kernel_pt = ptalloc();

    //pmrvaddr: virtual address
    //pmrpaddr: physical address
    for (uint64_t i = 0; i < pmrs_tag->entries; i++) {
        struct stivale2_pmr pmr = pmrs_tag->pmrs[i];
        for (uint64_t pmrvaddr = pmr.base; pmrvaddr < pmr.base + pmr.length; pmrvaddr += 4096) {
            uint64_t pmrpaddr = kernel_base_addr_tag->physical_base_address + (pmrvaddr - kernel_base_addr_tag->virtual_base_address);
            pt_map(kernel_pt,pmrvaddr, pmrpaddr, PTE_PRESENT | PTE_WRITABLE);
        }
    }

    // Switch to kernel page table
    __asm__("mov %0, %%cr3" :: "r"(kernel_pt));
}

void printmmap(struct stivale2_struct_tag_memmap *memmap_str_tag) {
    printf("Memory map:\n");
    printf("|-------------------------------------------\n");
    for (uint64_t i = 0; i < memmap_str_tag->entries; i++) {
        switch (memmap_str_tag->memmap[i].type) {
            case STIVALE2_MMAP_USABLE:
                printf("| 0x%X Available memory: %lu KB\n", memmap_str_tag->memmap[i].base, memmap_str_tag->memmap[i].length / 1024);
            break;
            case STIVALE2_MMAP_RESERVED:
                printf("| 0x%X Reserved memory: %lu KB\n",memmap_str_tag->memmap[i].base, memmap_str_tag->memmap[i].length / 1024);
            break;
            case STIVALE2_MMAP_KERNEL_AND_MODULES:
                printf("| 0x%X Kernel and modules: %lu KB\n", memmap_str_tag->memmap[i].base, memmap_str_tag->memmap[i].length / 1024);
            break;
            case STIVALE2_MMAP_ACPI_RECLAIMABLE:
                printf("| 0x%X ACPI reclaimable: %lu KB\n", memmap_str_tag->memmap[i].base, memmap_str_tag->memmap[i].length / 1024);
            break;
            case STIVALE2_MMAP_ACPI_NVS:
                printf("| 0x%X ACPI NVS: %lu KB\n", memmap_str_tag->memmap[i].base, memmap_str_tag->memmap[i].length / 1024);
            break;
            case STIVALE2_MMAP_BAD_MEMORY:
                printf("| 0x%X Bad memory: %lu\n KB\n", memmap_str_tag->memmap[i].base, memmap_str_tag->memmap[i].length / 1024);
            break;
            case STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE:
                printf("| 0x%X Bootloader reclaimable: %lu KB\n", memmap_str_tag->memmap[i].base, memmap_str_tag->memmap[i].length / 1024);
            break;
            case STIVALE2_MMAP_FRAMEBUFFER:
                printf("| 0x%X Framebuffer: %lu KB\n", memmap_str_tag->memmap[i].base, memmap_str_tag->memmap[i].length / 1024);
            break;
            default:
                printf("| 0x%X Unknown: %lu KB\n", memmap_str_tag->memmap[i].base, memmap_str_tag->memmap[i].length / 1024);
            break;
        }
    printf("|-------------------------------------------\n");
    }
}

uint64_t pmalloc(uint64_t pages) {
    uint64_t index = find_empty_contiguous_bits(bitmap, bitmap_size, pages);
    if (index == (uint64_t)-1) {
        printf("No free memory\n");
        return 0;
    }
    for (uint64_t j = 0; j < pages; j++) {
        if(get_bit(bitmap, (index) + j)) {
                printf("Error: pmalloc failed\n");
                return 0;
            }
            set_bit(bitmap, index + j);
        }
    return index * 4096;
}

void pmfree(uint64_t addr, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        if (get_bit(bitmap, (addr / 4096) + i)) {
            clear_bit(bitmap, (addr / 4096) + i);
            //printf("Freeing: %lu\n", addr + i);
        }
    }
}

page_table *ptalloc() {
    page_table *pt = (page_table*)pmalloc(1);
    memset(pt, 0, 4096);
    return pt;
}

void ptfree(page_table *pt) {
    pmfree((uint64_t)pt, 1);
}

// struct page_table{
//     uint64_t entries[512];
// }; //pml4

page_table *get_pt(page_table *parent, uint64_t index, bool create) {
    uint64_t *entry = &parent->entries[index];
    // check if entry is present
    if ((*entry & PTE_PRESENT)) {
        // mask out flags
        return (page_table*)(*entry & 0xfffffffffffff000); // get rid of everything until bit 12 (aka. bit mask)
    } else if (create) {
        // allocate new page table
        page_table *pt = ptalloc();
        // set entry address
        *entry = (uint64_t)pt;
        // mark entry as present and writable
        *entry |= PTE_WRITABLE | PTE_PRESENT;
        // return the allocated page table
        return pt;
    }
    return 0;
}

void pt_map(page_table *pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    uint64_t pml4_index = (vaddr >> 39) & 0x1FF; //these aren't magic numbers.
    uint64_t pml3_index = (vaddr >> 30) & 0x1FF; //they're just the bit shifts.
    uint64_t pml2_index = (vaddr >> 21) & 0x1FF; //All fresh from intel SDM 
    uint64_t pml1_index = (vaddr >> 12) & 0x1FF; //Volume 3 Chapter 4.5.4 Figure 4-11.
    
    page_table *pml3 = get_pt(pml4, pml4_index, 1);
    page_table *pml2 = get_pt(pml3, pml3_index, 1);
    page_table *pml1 = get_pt(pml2, pml2_index, 1);
    uint64_t *pml1_entry = &pml1->entries[pml1_index];
    *pml1_entry = (paddr & 0xfffffffffffff000) | flags;
    //skipping checking for page table present
}

void pt_unmap(page_table *pml4, uint64_t vaddr) {
    uint64_t pml4_index = (vaddr >> 39) & 0x1FF; //same as above
    uint64_t pml3_index = (vaddr >> 30) & 0x1FF;
    uint64_t pml2_index = (vaddr >> 21) & 0x1FF;
    uint64_t pml1_index = (vaddr >> 12) & 0x1FF;
    
    page_table *pml3 = get_pt(pml4, pml4_index, 0),
        *pml2 = 0,
        *pml1 = 0;
    
    if (pml3) {
        pml2 = get_pt(pml3, pml3_index, 0);
    }
    if (pml2) {
        pml1 = get_pt(pml2, pml2_index, 0);
    }
    if (pml1) {
        uint64_t *pml1_entry = &pml1->entries[pml1_index];
        if (*pml1_entry & PTE_PRESENT) {
            *pml1_entry = 0;
        } else {
            // TODO: use panic instead :)
            printf("Error: pml1 entry not present\n");
        }
    }
}