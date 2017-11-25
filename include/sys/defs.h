#ifndef _DEFS_H
#define _DEFS_H

#define NULL ((void*)0)
#define O_RDONLY	0x0000
#define O_WRONLY	0x0001
#define O_RDWR		0x0002
#define PRINT_BUF_ADDRESS 0xFFFFFFFD000B8000
//#define PRINT_BUF_ADDRESS 0xB8000

typedef unsigned long  uint64_t;
typedef          long   int64_t;
typedef unsigned int   uint32_t;
typedef          int    int32_t;
typedef unsigned short uint16_t;
typedef          short  int16_t;
typedef unsigned char   uint8_t;
typedef          char    int8_t;

typedef uint64_t size_t;
typedef int64_t ssize_t;

typedef uint64_t off_t;

typedef uint32_t pid_t;

typedef uint16_t mode_t;

#endif
