/** @file SNIIS.h
 * Basic C interface. Currently very lacking.
 */

#pragma once

#include <stdint.h>

/// -------------------------------------------------------------------------------------------------------------------
#if defined(_WIN32) || defined(USE_WINE)
  #define SNIIS_SYSTEM_WINDOWS 1
#elif defined(__linux__)
  #define SNIIS_SYSTEM_LINUX 1
#elif defined(__MACH__) && defined(__APPLE__)
  #define SNIIS_SYSTEM_MAC 1
#else
  #error Platform not implemented
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Creates the global input instance. Returns zero if successful or non-zero on error.
/// Windows: pass the HWND window handle
/// Linux: pass the X Window handle
/// Mac OSX: pass the Cocoa window id
int SNIIS_Initialize( void* pInitArgs);
/// Shuts down the global input instance
void SNIIS_Shutdown();

/// Per-frame update cycle: does the input processing. To be called before the message loop
void SNIIS_InputSystem_Update();
/// Notifies SNIIS about focus loss/gain. Non-Zero for focus gain, zero for focus loss
void SNIIS_InputSystem_SetFocus( int pFocus);

#ifdef __cplusplus
}
#endif
