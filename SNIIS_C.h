/** @file SNIIS.h
 * Basic C interface. Currently only implements creating and feeding it, but lacks any support for actual input processing
 */

#pragma once

#include <stdint.h>

/// -------------------------------------------------------------------------------------------------------------------
#if defined(_WIN32) || defined(USE_WINE)
  #define SNIIS_SYSTEM_WINDOWS 1
#elif defined(__linux__)
  #define SNIIS_SYSTEM_LINUX 1
#else
  #error Platform not implemented
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Creates the global input instance. Returns zero if successful or non-zero on error
int SNIIS_Initialize( void* pInitArgs);
/// Shuts down the global input instance
void SNIIS_Shutdown();
/// Per-frame update cycle: starts the input processing
void SNIIS_StartUpdate();
/// Per-frame update cycle: ends the input processing
void SNIIS_EndUpdate();

/// System-specific handling of data gathered outside of SNIIS. Call StartUpdate() first, then call this when handling 
/// system messages, then finally call EndUpdate();
#if SNIIS_SYSTEM_WINDOWS
  void SNIIS_HandleWinMessage( uint32_t message, size_t lParam, size_t wParam);
#elif SNIIS_SYSTEM_LINUX
#endif

#ifdef __cplusplus
}
#endif
