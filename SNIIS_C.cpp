/** @file SNIIS.cpp
 * Implementation of C interface using the CPP lib
 */

#include "SNIIS.h"
#include "SNIIS_C.h"

// Creates the global input instance. Returns non-zero if successful or 0 on error
extern "C" int SNIIS_Initialize( void* pInitArgs)
{
  bool success = SNIIS::InputSystem::Initialize( pInitArgs);
  return success ? 0 : 1;
}

// Shuts down the global input instance
extern "C" void SNIIS_Shutdown()
{
  SNIIS::InputSystem::Shutdown();
}

// Per-frame update cycle: starts the input processing
extern "C" void SNIIS_StartUpdate()
{
  if( SNIIS::gInstance )
    SNIIS::gInstance->StartUpdate();
}

// Per-frame update cycle: ends the input processing
extern "C" void SNIIS_EndUpdate()
{
  if( SNIIS::gInstance )
    SNIIS::gInstance->EndUpdate();
}

/// System-specific handling of data gathered outside of SNIIS. Call StartUpdate() first, then call this when handling 
/// system messages, then finally call EndUpdate();
#if SNIIS_SYSTEM_WINDOWS
extern "C" void SNIIS_HandleWinMessage( uint32_t message, size_t lParam, size_t wParam)
{
  if( SNIIS::gInstance )
    SNIIS::gInstance->HandleWinMessage( message, lParam, wParam);
}
#elif SNIIS_SYSTEM_LINUX
#endif
