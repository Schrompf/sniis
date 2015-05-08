/// @file SNIIS_Mac_Joystick.cpp
/// Mac implementation of anything which doesn't name itsself a Mouse or Keyboard

#include "SNIIS_Mac.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_MAC
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
MacJoystick::MacJoystick( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef)
  : Joystick( pId), MacDevice( pSystem, pDeviceRef)
{
  memset( &mState, 0, sizeof( mState));
}

// --------------------------------------------------------------------------------------------------------------------
void MacJoystick::StartUpdate()
{
  mState.prevButtons = mState.buttons;
  memset( mState.diffs, 0, sizeof( mState.diffs));
  float prevAxes[16];
  static_assert( sizeof( prevAxes) == sizeof( mState.axes), "Hardcoded array size");
  memcpy( prevAxes, mState.axes, sizeof( mState.axes));
}

// --------------------------------------------------------------------------------------------------------------------
void MacJoystick::HandleEvent( uint32_t page, uint32_t usage, int value)
{
}

// --------------------------------------------------------------------------------------------------------------------
void MacJoystick::SetFocus( bool pHasFocus)
{
  if( pHasFocus )
  {
    // don't do anything. Buttons currently pressed are lost then, all other controls will be handled correctly at next update
    // TODO: relative axes? If anyone ever spots one in the wilds, please let me know
  }
  else
  {
    // zero all buttons and axes
    for( size_t a = 0; a < mAxes.size(); ++a )
    {
      if( mState.axes[a] != 0.0f )
      {
        mState.diffs[a] = -mState.axes[a];
        mState.axes[a] = 0.0f;
        InputSystemHelper::DoJoystickAxis( this, a, 0.0f);
      }
    }

    for( size_t a = 0; a < mButtons.size(); ++a )
    {
      if( mState.buttons & (1ull << a) )
      {
        mState.buttons &= UINT64_MAX ^ (1ull << a);
        mState.prevButtons |= 1ull << a;
        InputSystemHelper::DoJoystickButton( this, a, false);
      }
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
size_t MacJoystick::GetNumButtons() const
{
  return mButtons.size();
}
// --------------------------------------------------------------------------------------------------------------------
std::string MacJoystick::GetButtonText( size_t idx) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
size_t MacJoystick::GetNumAxes() const
{
  return mAxes.size();
}
// --------------------------------------------------------------------------------------------------------------------
std::string MacJoystick::GetAxisText( size_t idx) const
{
  return "";
}

// --------------------------------------------------------------------------------------------------------------------
bool MacJoystick::IsButtonDown( size_t idx) const
{
  if( idx < mButtons.size() )
    return (mState.buttons & (1ull << idx)) != 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool MacJoystick::WasButtonPressed( size_t idx) const
{
  if( idx < mButtons.size() )
    return IsButtonDown( idx) && ((mState.prevButtons & (1ull << idx)) == 0);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool MacJoystick::WasButtonReleased( size_t idx) const
{
  if( idx < mButtons.size() )
    return !IsButtonDown( idx) && ((mState.prevButtons & (1ull << idx)) != 0);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
float MacJoystick::GetAxisAbsolute( size_t idx) const
{
  if( idx < mAxes.size() )
    return mState.axes[idx];
  else
    return 0.0f;
}
// --------------------------------------------------------------------------------------------------------------------
float MacJoystick::GetAxisDifference( size_t idx) const
{
  if( idx < mAxes.size() )
    return mState.diffs[idx];
  else
    return 0.0f;
}

#endif // SNIIS_SYSTEM_Mac

