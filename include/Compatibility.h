// Cross platform compatibility definitions
// Stanley Bak (June 2016)

#pragma once

#include <cstdint>
typedef int64_t i64;
typedef uint64_t u64;
typedef int32_t i32;
typedef uint32_t u32;
typedef int16_t i16;
typedef uint16_t u16;
typedef int8_t i8;
typedef uint8_t u8;

// strcasecmp
#ifdef WIN32

#include <string.h>
#define strcasecmp(a, b) _stricmp((a), (b))
#else
#include <strings.h>
#endif

// snprintf
#include <stdio.h>

#ifdef MSVC
#ifndef snprintf
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif
#endif
