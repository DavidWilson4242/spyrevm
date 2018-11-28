#ifndef VM_H
#define VM_H

#include <ncurses.h>
#include "../vm_types.h"
#include "process.h"

#define SPY_MAX_PROCESSES 16
#define SPY_PROCESS_STEPS 100

#define SPY_SUCCESS 1
#define SPY_FAILURE 0

#define SPY_SCREEN_X 50 
#define SPY_SCREEN_Y 40

#define SPY_NO_PROCESS (-1)

#define SPY_TERM_CURSOR_Y (SPY_SCREEN_Y - 4)
#define SPY_TERM_CURSOR_X 0

#define SPY_SYSCALL_TABLE   128
#define SPY_FD_TABLE        128
#define SPY_INT_REGISTERS   16
#define SPY_FLOAT_REGISTERS 16

struct Spyre_MemoryChunk {
    Spyre_MemoryChunk* next;
    Spyre_MemoryChunk* prev;
    SPY_ADDR           addr;
    size_t             bytes;
};

struct Spyre_MemoryChunkList {
    Spyre_MemoryChunk* head;
    Spyre_MemoryChunk* tail;
    U32                size;
};

struct Spyre_Window {
    WINDOW* w;
    int     cx; // cursor x
    int     cy; // cursor y
    char**  screen_data;
};

struct Spyre_FileDescriptor {
    U64     descriptor;
    U64     index;
    U64     start_index;
    U8      is_open;
};

struct Spyre_State {
    int                   running;
    U8*                   vm_memory;
    size_t                vm_memory_size;
    Spyre_Process*        processes[SPY_MAX_PROCESSES];
    Spyre_MemoryChunkList memory_map;
    Spyre_Window          main_window;
    U8                    focused_process;
    char                  cur_dir[128];
    
    // "hard disk"
    FILE*                 disk;

    // registers
    I64                   RI[SPY_INT_REGISTERS];
    F64                   RF[SPY_FLOAT_REGISTERS];

    // tables
    void                  (*syscalls[SPY_SYSCALL_TABLE])(Spyre_State*);
    Spyre_FileDescriptor  fds[SPY_FD_TABLE];
};

Spyre_State*   Spyre_init(size_t);
Spyre_Process* Spyre_request_process(Spyre_State*, size_t);
SPY_ADDR       Spyre_request_memory(Spyre_State*, size_t);
void           Spyre_defragment_memory(Spyre_State*);
void           Spyre_delete_process(Spyre_State*, Spyre_Process*);
void           Spyre_delete_memory(Spyre_State*, SPY_ADDR);
void           Spyre_take_control(Spyre_State*);
int            Spyre_request_focus(Spyre_State*, U8);

#endif
