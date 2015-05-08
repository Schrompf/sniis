/// @file SNIIS_Mac_Mouse.cpp
/// Mac OSX implementation of mice

#include "SNIIS_Mac.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_MAC
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
MacMouse::MacMouse( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef)
  : Mouse( pId), MacDevice( pSystem, pDeviceRef)
{
  mState.absWheel = mState.relWheel = 0;
  mState.buttons = mState.prevButtons = 0;

}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::StartUpdate()
{
  mState.relWheel = 0;
  mState.prevButtons = mState.buttons;

  for( size_t a = 0; a < mNumAxes; ++a )
    mState.prevAxes[a] = mState.axes[a];
}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::HandleEvent( uint32_t page, uint32_t usage, int value)
{
}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::EndUpdate()
{
  bool mw_down = mState.relWheel < 0;
  if( mw_down != ((mState.buttons & (1 << MB_WheelDown)) != 0) )
  {
    mState.buttons = (mState.buttons & ~(1 << MB_WheelDown)) | ((mw_down ? 1 : 0) << MB_WheelDown);
    InputSystemHelper::DoMouseButton( this, MB_WheelDown, mw_down);
  }
  bool mw_up = mState.relWheel > 0;
  if( mw_up != ((mState.buttons & (1 << MB_WheelUp)) != 0) )
  {
    mState.buttons = (mState.buttons & ~(1 << MB_WheelUp)) | ((mw_down ? 1 : 0) << MB_WheelUp);
    InputSystemHelper::DoMouseButton( this, MB_WheelUp, mw_down);
  }

  // send the mouse move
  if( mState.prevAxes[0] != mState.axes[0] || mState.prevAxes[1] != mState.axes[1] )
    InputSystemHelper::DoMouseMove( this, mState.axes[0], mState.axes[1], mState.axes[0] - mState.prevAxes[0], mState.axes[1] - mState.prevAxes[1]);
}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::SetFocus( bool pHasFocus)
{
  if( pHasFocus )
  {
    // get current mouse position when in SingleMouseMode
    if( !mSystem->IsInMultiMouseMode() )
    {
/*
      Window wa, wb;
      int rootx, rooty, childx, childy;
      unsigned int mask;
      if( XQueryPointer( mSystem->GetDisplay(), DefaultRootWindow( mSystem->GetDisplay()), &wa, &wb, &rootx, &rooty, &childx, &childy, &mask) != 0 )
      {
        mAxes[0].prevValue = mAxes[0].value; mAxes[1].prevValue = mAxes[1].value;
        mAxes[0].value = rootx; mAxes[1].value = rooty;
        if( mAxes[0].value != mAxes[0].prevValue || mAxes[1].value != mAxes[1].prevValue )
          InputSystemHelper::DoMouseMove( this, mAxes[0].value, mAxes[1].value, mAxes[0].value - mAxes[0].prevValue, mAxes[1].value - mAxes[1].prevValue);
      }
     */
    }
  }
  else
  {
    for( size_t a = 0; a < MB_Count; ++a )
    {
      if( mState.buttons & (1 << a) )
      {
        mState.prevButtons |= (1 << a);
        mState.buttons &= std::numeric_limits<decltype( mState.buttons)>::max() ^ (1 << a);
        InputSystemHelper::DoMouseButton( this, a, false);
      }
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
size_t MacMouse::GetNumButtons() const
{
  return mNumButtons;
}
// --------------------------------------------------------------------------------------------------------------------
std::string MacMouse::GetButtonText( size_t idx) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
size_t MacMouse::GetNumAxes() const
{
  return mNumAxes;
}
// --------------------------------------------------------------------------------------------------------------------
std::string MacMouse::GetAxisText( size_t idx) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
bool MacMouse::IsButtonDown( size_t idx) const
{
  if( idx < GetNumButtons() )
    return (mState.buttons & (1 << idx)) != 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool MacMouse::WasButtonPressed( size_t idx) const
{
  if( idx < GetNumButtons() )
    return IsButtonDown( idx) && (mState.prevButtons & (1 << idx)) == 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool MacMouse::WasButtonReleased( size_t idx) const
{
  if( idx < GetNumButtons() )
    return !IsButtonDown( idx) && (mState.prevButtons & (1 << idx)) != 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
float MacMouse::GetAxisAbsolute( size_t idx) const
{
  if( idx < mNumAxes )
    return float( mState.axes[idx]);
  else
    return 0.0f;
}
// --------------------------------------------------------------------------------------------------------------------
float MacMouse::GetAxisDifference( size_t idx) const
{
  if( idx < mNumAxes )
    return float( mState.axes[idx] - mState.prevAxes[idx]);
  else
    return 0.0f;
}
// --------------------------------------------------------------------------------------------------------------------
int MacMouse::GetMouseX() const { return int( GetAxisAbsolute( 0)); }
// --------------------------------------------------------------------------------------------------------------------
int MacMouse::GetMouseY() const { return int( GetAxisAbsolute( 1)); }
// --------------------------------------------------------------------------------------------------------------------
int MacMouse::GetRelMouseX() const { return int( GetAxisDifference( 0)); }
// --------------------------------------------------------------------------------------------------------------------
int MacMouse::GetRelMouseY() const { return int( GetAxisDifference( 1)); }

#endif // SNIIS_SYSTEM_MAC
