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

// Per-frame update cycle: does the input processing. To be called before the message loop
extern "C" void SNIIS_InputSystem_Update()
{
  if( SNIIS::gInstance )
    SNIIS::gInstance->Update();
}

// Notifies SNIIS about focus loss/gain. Non-Zero for focus gain, zero for focus loss
extern "C" void SNIIS_InputSystem_SetFocus( int pFocus)
{
  if( SNIIS::gInstance )
    SNIIS::gInstance->SetFocus( pFocus != 0);
}
