/** @file SNIIS.cpp
 * Implementation of C interface using the CPP lib
 */

#include "SNIIS.h"
#include "SNIIS_C.h"

// Creates the global input instance. Returns zero if successful or non-zero on error
extern "C" int SNIIS_Initialize( void* pInitArgs)
{
  bool success = SNIIS::InputSystem::Initialize( pInitArgs);
  return success ? 1 : 0;
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

// System-specific handling of data gathered outside of SNIIS. Call StartUpdate() first, then call this when handling 
// system messages, then finally call EndUpdate();
#if SNIIS_SYSTEM_WINDOWS
extern "C" void SNIIS_HandleWinMessage( uint32_t message, size_t lParam, size_t wParam)
{
  if( SNIIS::gInstance )
    SNIIS::gInstance->HandleWinMessage( message, lParam, wParam);
}
#elif SNIIS_SYSTEM_LINUX
#endif

// Notifies SNIIS about focus loss/gain. Non-Zero for focus gain, zero for focus loss
extern "C" void SNIIS_SetFocus( int pFocus)
{
  if( SNIIS::gInstance )
    SNIIS::gInstance->SetFocus( pFocus != 0);
}
