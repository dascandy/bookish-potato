#pragma once

typedef signed char int8_t;
typedef signed short int int16_t;
typedef signed int int32_t;
typedef signed long int int64_t;
typedef signed __int128 int128_t;

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long int uint64_t;
typedef unsigned __int128 uint128_t;

typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

typedef signed char int_fast8_t;
#ifdef __x86_64__
typedef signed short int int_fast16_t;
typedef signed int int_fast32_t;
#else
typedef long int int_fast16_t;
typedef long int int_fast32_t;
#endif
typedef long int int_fast64_t;

typedef unsigned char uint_fast8_t;
#ifdef __x86_64__
typedef unsigned short int uint_fast16_t;
typedef unsigned int uint_fast32_t;
#else
typedef unsigned long int uint_fast16_t;
typedef unsigned long int uint_fast32_t;
#endif
typedef unsigned long int uint_fast64_t;

typedef long int intptr_t;
typedef unsigned long int uintptr_t;

typedef long int intmax_t;
typedef unsigned long int uintmax_t;

