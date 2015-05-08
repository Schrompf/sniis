/// @file SNIIS_Mac_Keyboard.cpp
/// Mac implementation of keyboards

#include "SNIIS_Mac.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_MAC
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
MacKeyboard::MacKeyboard( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef)
  : Keyboard( pId), MacDevice( pSystem, pDeviceRef)
{
  mNumKeys = 256;

  mState.resize( (mNumKeys+63) / 64); mPrevState.resize( mState.size());
}

// --------------------------------------------------------------------------------------------------------------------
void MacKeyboard::StartUpdate()
{
  mPrevState = mState;
}

// --------------------------------------------------------------------------------------------------------------------
void MacKeyboard::HandleEvent( uint32_t page, uint32_t usage, int value)
{
}

// --------------------------------------------------------------------------------------------------------------------
void MacKeyboard::SetFocus( bool pHasFocus)
{
  if( pHasFocus )
  {
    // Don't do anything. Keys currently pressed are lost then, all other controls will be handled correctly at next update
    // TODO: maybe query current keyboard state? To capture that Alt+Tab? Naaah.
  }
  else
  {
    for( size_t a = 0; a < mNumKeys; ++a )
    {
      if( IsSet( a) )
      {
        Set( a,  false);
        mPrevState[a/64] |= (1ull << (a&63));
        InputSystemHelper::DoKeyboardButton( this, (SNIIS::KeyCode) a, 0, false);
      }
    }
  }
}

void MacKeyboard::Set( size_t kc, bool set)
{
  if( set )
    mState[kc/64] |= (1ull << (kc&63));
  else
    mState[kc/64] &= UINT64_MAX ^ (1ull << (kc&63));
}
bool MacKeyboard::IsSet( size_t kc) const
{
  return (mState[kc/64] & (1ull << (kc&63))) != 0;
}
bool MacKeyboard::WasSet( size_t kc) const
{
  return (mPrevState[kc/64] & (1ull << (kc&63))) != 0;
}

// --------------------------------------------------------------------------------------------------------------------
size_t MacKeyboard::GetNumButtons() const
{
  return mNumKeys;
}
// --------------------------------------------------------------------------------------------------------------------
std::string MacKeyboard::GetButtonText( size_t idx) const
{
/*
  // search backwards to map our keycode to the Mac Keysym it originated from
  size_t ks = SIZE_MAX;
  if( idx >= KC_FIRST_CUSTOM )
  {
    idx -= KC_FIRST_CUSTOM;
    if( idx < mExtraButtons.size() )
      ks = mExtraButtons[idx];
  } else
  {
    for( auto it = sKeyTable.cbegin(); it != sKeyTable.cend() && ks == SIZE_MAX; ++it )
      if( it->second == idx )
        ks = it->first;
  }

  // still there - haven't found the Keysym
  if( ks == SIZE_MAX )
    return "";
*/
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
bool MacKeyboard::IsButtonDown( size_t idx) const
{
  if( idx < mNumKeys )
    return IsSet( idx);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool MacKeyboard::WasButtonPressed( size_t idx) const
{
  if( idx < mNumKeys )
    return IsSet( idx) && !WasSet( idx);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool MacKeyboard::WasButtonReleased( size_t idx) const
{
  if( idx < mNumKeys )
    return !IsSet( idx) && WasSet( idx);
  else
    return false;
}

#endif // SNIIS_SYSTEM_MAC

