#ifndef __NATIVE_CODER_H
#define __NATIVE_CODER_H

unsigned long NBS_GetHighResolutionTime();

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE    1
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SKP_Silk_SDK_API.h"
#include "SKP_Silk_SigProc_FIX.h"

/* Define codec specific settings should be moved to h file */
#define MAX_BYTES_PER_FRAME     1024
#define MAX_INPUT_FRAMES        5
#define MAX_FRAME_LENGTH        480
#define FRAME_LENGTH_MS         20
#define MAX_API_FS_KHZ          48
#define MAX_LBRR_DELAY          2

#ifdef _SYSTEM_IS_BIG_ENDIAN
/* Function to convert a little endian int16 to a */
/* big endian int16 or vica verca                 */
void swap_endian(
    SKP_int16       vec[],
    SKP_int         len
);
#endif

#if (defined(_WIN32) || defined(_WINCE))

#include <windows.h>    /* timer */

#else    // Linux or Mac
#include <sys/time.h>
#endif



#endif //__NATIVE_CODER_H
