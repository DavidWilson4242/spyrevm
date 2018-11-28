#include "kernel.h"
#include "syscall.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>

static int term_x = 0;
static int term_y = 0; 

void term_puts(Spyre_State*, Spyre_Window*, const char*);
void term_putsyx(Spyre_State*, Spyre_Window*, const char*, int, int);
void term_putc(Spyre_State*, Spyre_Window*, char);
void term_print_location(Spyre_State*, Spyre_Window*, const char*, int, int);
void term_scroll_up(Spyre_State*, Spyre_Window*);
void term_backspace(Spyre_State*, Spyre_Window*);

void term_backspace(Spyre_State* S, Spyre_Window* win) {
    if (win->cx <= 0) {
        win->cx = 0;
        return;
    }
    win->screen_data[win->cy][win->cx] = ' ';
    mvwprintw(win->w, win->cy + 1, win->cx + 1, "%c", ' ');
    win->cx--;
    wrefresh(win->w);
}

void term_scroll_up(Spyre_State* S, Spyre_Window* win) {
    for (int i = 0; i < SPY_SCREEN_Y - 2; i++) {
        memcpy(win->screen_data[i], win->screen_data[i + 1], sizeof(char*)*(SPY_SCREEN_X - 1));
    }
    win->cx = 0;
    term_print_location(S, win, S->cur_dir, win->cy, 0);
}

void term_print_location(Spyre_State* S, Spyre_Window* win, const char* location, int y, int x) {
    size_t len_loc = strlen(location);
    char* buffer = malloc(len_loc + 3);
    strcpy(buffer, location);
    strcat(buffer, "$  ");
    term_putsyx(S, win, buffer, y, x);
    free(buffer);
}

void term_puts(Spyre_State* S, Spyre_Window* win, const char* s) {
    while (*s) {
        term_putc(S, win, *s++);
    }
}

void term_putc(Spyre_State* S, Spyre_Window* win, char c) {
    int max_x, max_y;
    
    getmaxyx(win->w, max_y, max_x);

    win->cx++;
    if (win->cx >= max_x - 3) {
        win->cx = 1;
        term_scroll_up(S, win);
        term_print_location(S, win, S->cur_dir, SPY_TERM_CURSOR_Y, SPY_TERM_CURSOR_X);
    }
    win->screen_data[win->cy][win->cx] = c;
    for (int i = 0; i < SPY_SCREEN_Y - 2; i++) {
        for (int j = 0; j < SPY_SCREEN_X - 2; j++) {
            mvwprintw(win->w, i + 1, j + 1, "%c", win->screen_data[i][j]);
        }
    }
    wrefresh(win->w);
   
}

void term_putsyx(Spyre_State* S, Spyre_Window* win, const char* s, int y, int x) {
    win->cy = y;
    win->cx = x;
    term_puts(S, win, s);
}

Spyre_State* Spyre_init(size_t bytes) {

    Spyre_State* vm_state = malloc(sizeof(Spyre_State));
    if (!vm_state) {
        printf("couldn't allocate Spyre VM state\n");
        return NULL;
    }

    vm_state->vm_memory = malloc(bytes);
    if (!vm_state->vm_memory) {
        printf("couldn't allocate vm_memory\n");
        free(vm_state);
        return NULL;
    }
    
    // initialize all processes to NULL
    memset(vm_state->processes, 0, sizeof(Spyre_Process*) * SPY_MAX_PROCESSES);

    // initialize memory map to empty
    vm_state->memory_map.head = NULL;
    vm_state->memory_map.tail = NULL;
    vm_state->memory_map.size = 0;

    vm_state->running = 0;
    vm_state->vm_memory_size = bytes;
    vm_state->focused_process = -1;
    strcpy(vm_state->cur_dir, "root");

    // initialize syscalls
    Spyre_init_syscalls(vm_state);

    // initialize disk
    Spyre_init_disk(vm_state);

    // initialize file descriptors
    for (int i = 0; i < SPY_FD_TABLE; i++) {
        Spyre_FileDescriptor* fd = &vm_state->fds[i];
        fd->descriptor = i;
        fd->index = 0;
        fd->start_index = 0;
        fd->is_open = 0;
    }
    
    // initialize ncurses
    vm_state->main_window.w = initscr();
    if (!vm_state->main_window.w) {
        fprintf(stderr, "Failed to initialize ncurses!\n");
        free(vm_state->vm_memory);
        free(vm_state);
        return NULL;
    }

    Spyre_Window* main_win = &vm_state->main_window;
    main_win->screen_data = malloc(sizeof(char**) * (SPY_SCREEN_Y - 1));
    for (int i = 0; i < SPY_SCREEN_Y - 1; i++) {
        main_win->screen_data[i] = malloc(sizeof(char*) * (SPY_SCREEN_X - 1));
        for (int j = 0; j < SPY_SCREEN_X - 1; j++) {
            main_win->screen_data[i][j] = ' ';
        }
    }
    
    resizeterm(SPY_SCREEN_Y, SPY_SCREEN_X);
    noecho(); 
    keypad(main_win->w, 1);
    
    box(main_win->w, 0, 0);
    wrefresh(main_win->w);

    // initialize terminal cursor
    main_win->cx = 0;
    main_win->cy = SPY_SCREEN_Y - 4;
    wmove(main_win->w, main_win->cx + 1, main_win->cy);

    // print directory
    term_print_location(vm_state, main_win, vm_state->cur_dir, SPY_TERM_CURSOR_Y, SPY_TERM_CURSOR_X);
    wrefresh(main_win->w);

    return vm_state;

}

Spyre_Process* Spyre_request_process(Spyre_State* S, size_t bytes) {
    
    for (int i = 0; i < SPY_MAX_PROCESSES; i++) {
        if (!S->processes[i]) {
            // found a valid place to store the process
            Spyre_Process* process = malloc(sizeof(Spyre_Process));
            if (!process) {
                return NULL;
            }
            SPY_ADDR process_memory = Spyre_request_memory(S, bytes);
            if (process_memory == SPY_NULL) {
                // couldn't allocate memory for the program ...
                // in the future, perhaps give the user less
                // memory than they asked for, instead of just bailing
                free(process);
                return NULL;
            }
            process->process_memory = process_memory;
            process->pid = i;
            process->master = S;
            S->processes[i] = process;
            return process;
        }
    }
    
    // no valid remaining spots for the process
    return NULL;

}

SPY_ADDR Spyre_request_memory(Spyre_State* S, size_t bytes) {

    // pad bytes to SPY_MEMORY_ALIGNMENT
    bytes += SPY_MEMORY_ALIGNMENT - (bytes % SPY_MEMORY_ALIGNMENT);

    // initialize the memory chunk that will be added to the memory map
    Spyre_MemoryChunk* chunk = malloc(sizeof(Spyre_MemoryChunk));
    chunk->next = NULL; // TBD
    chunk->prev = NULL; // TBD
    chunk->addr = 0;    // TBD
    chunk->bytes = bytes;

    // get a handle to the memory map
    Spyre_MemoryChunkList* map = &S->memory_map;

    // if the list is empty, add it at the head
    if (map->size == 0) {
        map->head = chunk;
        map->tail = chunk;
        map->size = 1;
        chunk->addr = SPY_FIRST_ADDR; // here, we use FIRST_ADDR because 0 is SPY_NULL
        return chunk->addr;
    }

    // otherwise, check if there is space at the back
    if (map->tail) {
        SPY_ADDR end_of_tail = map->tail->addr + map->tail->bytes;
        if (S->vm_memory_size - end_of_tail > bytes) {
            chunk->addr = end_of_tail;
            map->tail->next = chunk;
            chunk->prev = map->tail;
            map->tail = chunk;
            return chunk->addr;
        }
    }

    // final check, iterate to see if there is space between
    for (Spyre_MemoryChunk* scan = map->head; scan; scan = scan->next) {
        if (!scan->next) {
            break;
        }
        SPY_ADDR finish_at  = scan->addr + scan->bytes;
        SPY_ADDR start_next = scan->next->addr;
        size_t space = start_next - finish_at;
        if (space >= bytes) {
            chunk->addr = finish_at;
            // insert chunk between scan and scan->next
            Spyre_MemoryChunk* old_next = scan->next;
            scan->next = chunk;
            chunk->prev = scan;
            chunk->next = old_next;
            old_next->prev = chunk;
            return chunk->addr;
        }
    }

    free(chunk);
    return SPY_NULL;
}

void Spyre_defragment_memory(Spyre_State* S) {

}

void Spyre_delete_process(Spyre_State* S, Spyre_Process* proc) {

}

void Spyre_delete_memory(Spyre_State* S, SPY_ADDR addr) {

}

int Spyre_request_focus(Spyre_State* S, U8 pid) {
    if (S->focused_process == SPY_NO_PROCESS) {
        S->focused_process = pid;
        return SPY_SUCCESS;
    }
    return SPY_FAILURE;
}

void Spyre_take_control(Spyre_State* S) {
    
    S->running = 1;

    // clear getch buffer
    fseek(stdin, 0, SEEK_END);

    // main virtual machine loop
    // TODO add better scheduling algorithm
    while (S->running) {

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        
        // select allows us to only call getch() when we know there
        // is a character waiting to be read... this allows us to 
        // continue running programs while also getting user input
        int status = select(STDIN_FILENO, &fds, NULL, NULL, &tv);
        
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            // if reached, we know getch() has a char for us
            char c = getchar();
            if (c == 13) {
                term_scroll_up(S, &S->main_window);
            } else if (c == 8 || c == 127) {
                term_backspace(S, &S->main_window);
            } else {
                term_putc(S, &S->main_window, c);
            }
        }
        
        for (int i = 0; i < SPY_MAX_PROCESSES; i++) {
            Spyre_Process* proc = S->processes[i];
            if (!proc) {
                continue;
            }
            for (int i = 0; proc->running && i < SPY_PROCESS_STEPS; i++) {
                // TODO check for interrupts
                Process_step(proc);
            }
        }
    }

}
