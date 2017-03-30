/**
 ******************************************************************************
 * @file       types.h
 * @brief      系统类型定义
 * @details    本文件定义系统中用到的所有使用的基本类型
 * @copyright Copyright(C), 2015-2020,Sanxing Medical & Electric Co.,Ltd.
 ******************************************************************************
 */

#ifndef _MYTYPES_H_
#define _MYTYPES_H_

/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdint.h>

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
#ifndef OK
#define OK      0
#endif

#ifndef ERROR
#define ERROR   (-1)
#endif

#ifndef BOOL
#define BOOL    int8
#endif

#ifndef bool
#define bool    uint8
#endif

#ifndef IMPORT
#define IMPORT  extern
#endif

#ifndef LOCAL
#define LOCAL   static
#endif

#ifndef FAST
#define FAST    register
#endif

#ifndef NULL
#define NULL    0
#endif

/**
 * IO definitions
 *
 * define access restrictions to peripheral registers
 */
#define __I     volatile const  /*!< defines 'read only' permissions      */
#define __O     volatile        /*!< defines 'write only' permissions     */
#define __IO    volatile        /*!< defines 'read / write' permissions   */

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
typedef char                char_t;
#if 0 //stdint
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
#endif
typedef float               float32_t;
typedef double              float64_t;
typedef long double         float128_t;

typedef int                 status_t;
typedef int                 STATUS;

typedef int8_t              int8;
typedef int16_t             int16;
typedef int32_t             int32;
typedef int64_t             int64;

typedef uint8_t             uint8;
typedef uint16_t            uint16;
typedef uint32_t            uint32;
typedef uint64_t            uint64;

typedef uint8_t             byte;
typedef uint8_t             bits;
typedef uint8_t             uchar;
typedef float               float32;

typedef uint32              size_t;
//typedef __SIZE_TYPE__     size_t;

typedef unsigned char       tBoolean;

typedef enum boolean
{
   E_FALSE  = 0,
   E_TRUE   = 1
} bool_e;

//typedef enum Boolean
//{
//   false = 0,
//   true  = 1
//} boolean;
typedef signed long time_t;



#define WAIT_FOREVER  ((uint32)0)

//#define FOREVER for (;;)

#define FAR


#ifdef __cplusplus
typedef void    (*OSFUNCPTR) (void *);  /* ptr to function returning int */
typedef int     (*FUNCPTR) (...);       /* ptr to function returning int */
typedef void    (*VOIDFUNCPTR) (...);   /* ptr to function returning void */
typedef double  (*DBLFUNCPTR) (...);    /* ptr to function returning double*/
typedef float   (*FLTFUNCPTR) (...);    /* ptr to function returning float */
typedef void    (*VOIDFUNCPTRBOOL)(boolean);

#else
typedef void    (*OSFUNCPTR) (void *);  /* ptr to function returning int */
typedef int     (*FUNCPTR) ();          /* ptr to function returning int */
typedef void    (*VOIDFUNCPTR) ();      /* ptr to function returning void */
typedef double  (*DBLFUNCPTR) ();       /* ptr to function returning double*/
typedef float   (*FLTFUNCPTR) ();       /* ptr to function returning float */
typedef void    (*VOIDFUNCPTRBOOL)(bool_e b);

#endif          /* _cplusplus */

#ifdef __CC_ARM                         /* ARM Compiler */
#define WEAK __weak
#elif defined (__IAR_SYSTEMS_ICC__)     /* for IAR Compiler */
#define WEAK __weak
#elif defined (__GNUC__)                /* GNU GCC Compiler */
#define WEAK __attribute__ ((weak))
#endif

typedef union
{
    unsigned long longValue;
    unsigned char array[4];
    struct
    {
        unsigned short high, low;
    } shortValue;
    struct
    {
        unsigned char highest, higher, middle, low;
    } charValue;
} U_UINT32;

typedef union
{
    unsigned long LongValue;
    unsigned char Array[4];
    struct
    {
        unsigned short High, Low;
    } IntValue;
    struct
    {
        unsigned char Highest, Higher, Middle, Low;
    } CharValue;
} Long_Char;

#endif /* _MYTYPES_H_ */

/*-----------------------------End of types.h--------------------------------*/
