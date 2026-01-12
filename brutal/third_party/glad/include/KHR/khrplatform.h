/*
** Copyright (c) 2008-2018 The Khronos Group Inc.
** Minimal version for Brutal Engine
*/
#ifndef __khrplatform_h_
#define __khrplatform_h_

// Khronos platform types - minimal definitions
typedef signed   char  khronos_int8_t;
typedef unsigned char  khronos_uint8_t;
typedef signed   short khronos_int16_t;
typedef unsigned short khronos_uint16_t;
typedef signed   int   khronos_int32_t;
typedef unsigned int   khronos_uint32_t;
typedef float          khronos_float_t;

#if defined(_WIN64) || defined(__LP64__)
typedef signed   long long int khronos_intptr_t;
typedef unsigned long long int khronos_uintptr_t;
typedef signed   long long int khronos_ssize_t;
typedef unsigned long long int khronos_usize_t;
#else
typedef signed   long  int     khronos_intptr_t;
typedef unsigned long  int     khronos_uintptr_t;
typedef signed   long  int     khronos_ssize_t;
typedef unsigned long  int     khronos_usize_t;
#endif

// Calling convention
#if defined(_WIN32) && !defined(__SCITECH_SNAP__)
#   define KHRONOS_APICALL __declspec(dllimport)
#   define KHRONOS_APIENTRY __stdcall
#else
#   define KHRONOS_APICALL
#   define KHRONOS_APIENTRY
#endif

#endif /* __khrplatform_h_ */
