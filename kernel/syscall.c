#include "syscall.h"

// SYSCALL TABLE
// |------------------------------------------------------------------------------------|
// | OPCODE     | NAME       | RI0       | RI1       | RI2       | RI3                  |
// | 1          | open       | desc      | name      | mode      |                      |
// | 2          | close      |           | desc      |           |                      |
// | 3          | putc       |           | char      |           |                      |

void __sys_open(Spyre_State*);
void __sys_close(Spyre_State*);
void __sys_putc(Spyre_State*);

void __sys_open(Spyre_State* S) {

}

void __sys_close(Spyre_State* S) {

}

void __sys_putc(Spyre_State* S) {

}

void Spyre_init_syscalls(Spyre_State* S) {
    S->syscalls[0] = __sys_open;
    S->syscalls[1] = __sys_close;
    S->syscalls[2] = __sys_putc;
}
