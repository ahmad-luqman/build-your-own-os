#ifndef TYPES_MINIMAL_H
#define TYPES_MINIMAL_H

// Minimal types header for testing file system headers
// This provides basic types without depending on system headers

// Basic integer types
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;

// Size types
typedef unsigned long size_t;
typedef signed long ssize_t;
typedef signed long off_t;
typedef unsigned long uintptr_t;
typedef signed long intptr_t;

// Pointer types
typedef void *void_ptr;

// Null pointer constant
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif // TYPES_MINIMAL_H