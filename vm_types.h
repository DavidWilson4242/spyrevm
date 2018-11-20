#ifndef VM_TYPES
#define VM_TYPES

// this file specifies certain typedefs that will be used
// by the virtual machine itself, and the child processes

#include <stdint.h>
#include <stddef.h>

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef uint64_t SPY_ADDR;
typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;

typedef struct Spyre_Process         Spyre_Process;
typedef struct Spyre_State           Spyre_State;
typedef struct Spyre_MemoryChunk     Spyre_MemoryChunk;
typedef struct Spyre_MemoryChunkList Spyre_MemoryChunkList;
typedef struct Spyre_Window          Spyre_Window;

#define SPY_MEMORY_ALIGNMENT  8
#define SPY_NULL              ((SPY_ADDR)0x00)
#define SPY_FIRST_ADDR        ((SPY_ADDR)SPY_MEMORY_ALIGNMENT) // first address is 1 (addr 0 is invalid)

#endif
