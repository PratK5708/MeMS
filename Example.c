#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

typedef struct FreeBlock {
    size_t size;
    struct FreeBlock* next;
} FreeBlock;

FreeBlock* freeList = NULL;
void* mems_start = NULL;

void mems_init() {
    freeList = NULL;
    mems_start = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mems_start == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    freeList = (FreeBlock*)mems_start;
    freeList->size = PAGE_SIZE - sizeof(FreeBlock);
    freeList->next = NULL;
}

void mems_finish() {
    munmap(mems_start, PAGE_SIZE);
    printf("Unmapped: %zu bytes at %p\n", PAGE_SIZE, mems_start);
}

void* mems_malloc(size_t size) {
    size = (size + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;

    FreeBlock* current = freeList;
    FreeBlock* prev = NULL;

    while (current != NULL) {
        if (current->size >= size) {
            if (current->size > size) {
                FreeBlock* newFreeBlock = (FreeBlock*)((char*)current + size);
                newFreeBlock->size = current->size - size;
                newFreeBlock->next = current->next;

                if (prev == NULL) {
                    freeList = newFreeBlock;
                } else {
                    prev->next = newFreeBlock;
                }
            } else {
                if (prev == NULL) {
                    freeList = current->next;
                } else {
                    prev->next = current->next;
                }
            }

            printf("Allocated: %zu bytes at %p\n", size, current);
            return (void*)current;
        }

        prev = current;
        current = current->next;
    }

    void* memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("Allocated: %zu bytes at %p\n", size, memory);
    return memory;
}

void mems_free(void* v_ptr) {
    if (v_ptr == NULL) {
        return;
    }

    FreeBlock* newFreeBlock = (FreeBlock*)v_ptr;
    newFreeBlock->size = newFreeBlock->size;
    newFreeBlock->next = freeList;
    freeList = newFreeBlock;

    printf("Freed: %zu bytes at %p\n", newFreeBlock->size, v_ptr);
}

void mems_print_stats() {
    FreeBlock* current = freeList;
    int mainChainLength = 0;
    size_t unusedSpace = 0;

    printf("MeMS SYSTEM STATS\n");

    while (current != NULL) {
        mainChainLength++;
        size_t subChainLength = 0;
        size_t mainChainSize = 0;

        printf("MAIN[%p:%p]-> ", current, (char*)current + current->size - 1);

        while (current != NULL) {
            subChainLength++;
            mainChainSize += current->size;
            printf("P[%p:%p] <-> ", current, (char*)current + current->size - 1);
            current = current->next;
        }

        unusedSpace += mainChainSize;
        printf("H[%p:%p] <-> NULL\n", (char*)current, (char*)current + mainChainSize - 1);

        if (current != NULL) {
            current = current->next;
        }
    }

    printf("Pages used: %d\n", mainChainLength);
    printf("Space unused: %zu\n", unusedSpace);
    printf("Main Chain Length: %d\n", mainChainLength);
    printf("Sub-chain Length array: [");
    current = freeList;
    while (current != NULL) {
        printf("%zu, ", current->size / PAGE_SIZE);
        current = current->next;
    }
    printf("]\n");
}

void* mems_get(void* v_ptr) {
    return v_ptr;
}

int main(int argc, char const *argv[])
{
    // initialise the MeMS system 
    mems_init();
    int* ptr[10];

    /*
    This allocates 10 arrays of 250 integers each
    */
    printf("\n------- Allocated virtual addresses [mems_malloc] -------\n");
    for(int i=0;i<10;i++){
        ptr[i] = (int*)mems_malloc(sizeof(int)*250);
        printf("Virtual address: %lu\n", (unsigned long)ptr[i]);
    }

    /*
    In this section we are tring to write value to 1st index of array[0] (here it is 0 based indexing).
    We get get value of both the 0th index and 1st index of array[0] by using function mems_get.
    Then we write value to 1st index using 1st index pointer and try to access it via 0th index pointer.

    This section is show that even if we have allocated an array using mems_malloc but we can 
    retrive MeMS physical address of any of the element from that array using mems_get. 
    */
    printf("\n------ Assigning value to Virtual address [mems_get] -----\n");
    // how to write to the virtual address of the MeMS (this is given to show that the system works on arrays as well)
    int* phy_ptr= (int*) mems_get(&ptr[0][1]); // get the address of index 1
    phy_ptr[0]=200; // put value at index 1
    int* phy_ptr2= (int*) mems_get(&ptr[0][0]); // get the address of index 0
    printf("Virtual address: %lu\tPhysical Address: %lu\n",(unsigned long)ptr[0],(unsigned long)phy_ptr2);
    printf("Value written: %d\n", phy_ptr2[1]); // print the address of index 1 

    /*
    This shows the stats of the MeMS system.  
    */
    printf("\n--------- Printing Stats [mems_print_stats] --------\n");
    mems_print_stats();

    /*
    This section shows the effect of freeing up space on free list and also the effect of 
    reallocating the space that will be fullfilled by the free list.
    */
    printf("\n--------- Freeing up the memory [mems_free] --------\n");
    mems_free(ptr[3]);
    mems_print_stats();
    ptr[3] = (int*)mems_malloc(sizeof(int)*250);
    mems_print_stats();

    printf("\n--------- Unmapping all memory [mems_finish] --------\n\n");
    mems_finish();
    return 0;
}