/// @file SNIIS_Win_Mouse.cpp
/// Windows implementation of mice

#include "SNIIS_Win.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_WINDOWS
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
WinMouse::WinMouse( WinInput* pSystem, size_t pId, HANDLE pHandle) 
  : Mouse( pId), mSystem( pSystem), mHandle( pHandle)
{
  mState.absX = mState.absY = mState.absWheel = 0;
  mState.relX = mState.relY = mState.relWheel = 0;
  mState.buttons = mState.prevButtons = 0;

  // read initial position, assume single mouse mode
  POINT point;
  GetCursorPos(&point);
  ScreenToClient( mSystem->GetWindowHandle(), &point);
  mState.absX = point.x; mState.absY = point.y;
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::StartUpdate()
{
  mState.relX = mState.relY = mState.relWheel = 0;
  mState.prevButtons = mState.buttons;
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::ParseMessage( const RAWINPUT& e)
{
  // not a mouse or not our mouse - just a safety, such a message shouldn't even reach us
  assert( e.header.dwType == RIM_TYPEMOUSE && e.header.hDevice == mHandle );
  const RAWMOUSE& mouse = e.data.mouse;

  // any message counts as activity, so treat this mouse as primary if it's not decided, yet
  InputSystemHelper::MakeThisMouseFirst( this);

  // Mouse buttons - Raw Input only supports five
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN ) DoMouseClick( MB_Left, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) DoMouseClick( MB_Left, false);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN ) DoMouseClick( MB_Right, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) DoMouseClick( MB_Right, false);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN ) DoMouseClick( MB_Middle, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP) DoMouseClick( MB_Middle, false);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN ) DoMouseClick( MB_Button3, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) DoMouseClick( MB_Button3, false);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN ) DoMouseClick( MB_Button4, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) DoMouseClick( MB_Button4, false);

  // Mouse wheel
  if( mouse.usButtonFlags & RI_MOUSE_WHEEL )
  {
    mState.relWheel += (short) mouse.usButtonData;
    mState.absWheel += mState.relWheel;
  }

  // Movement
  if( mSystem->IsInMultiMouseMode() )
  {
    if( (mouse.usFlags & 1) == MOUSE_MOVE_RELATIVE )
    {
      if( mouse.lLastX != 0 || mouse.lLastY != 0 )
      {
        mState.relX += mouse.lLastX; mState.absX += mouse.lLastX;
        mState.relY += mouse.lLastY; mState.absY += mouse.lLastY;
      }
    }
    else
    {
      int preX = mState.absX, preY = mState.absY;
      if( mState.absX != mouse.lLastX || mState.absY != mouse.lLastY )
      {
        mState.absX = mouse.lLastX; mState.absY = mouse.lLastY;
        mState.relX += mState.absX - preX;
        mState.relY += mState.absY - preY;
      }
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::EndUpdate()
{
  bool mw_down = mState.relWheel < 0;
  if( mw_down != ((mState.buttons & (1 << MB_WheelDown)) != 0) )
    DoMouseClick( MB_WheelDown, mw_down);
  bool mw_up = mState.relWheel > 0;
  if( mw_up != ((mState.buttons & (1 << MB_WheelUp)) != 0) )
    DoMouseClick( MB_WheelUp, mw_up);

  // in Single Mouse Mode we discard all mouse movements and replace it by the global mouse position.
  // Otherwise the movement would feel weird to the user because RawInput is missing all mouse acceleration and such
  if( !mSystem->IsInMultiMouseMode() )
  {
    POINT point;
    GetCursorPos(&point);
    ScreenToClient( mSystem->GetWindowHandle(), &point);
    int prevAbsX = mState.absX, prevAbsY = mState.absY;
    mState.absX = point.x; mState.absY = point.y;
    mState.relX = mState.absX - prevAbsX;
    mState.relY = mState.absY - prevAbsY;
  }

  // send the mouse move
  if( mState.relX != 0 || mState.relY != 0 )
    InputSystemHelper::DoMouseMove( this, mState.absX, mState.absY, mState.relX, mState.relY);
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::DoMouseClick( int mouseButton, bool isDown )
{
  // in single mouse mode, redirect click to main mouse
  // actually: don't. It would only allow clicking with all mice the user has, which is an unlikely usecase IMO. But
  // it opens a new range of state mixup when switching Single/Multi Mouse mode while a button is down. Isn't worth it.
//  if( !mSystem->IsInMultiMouseMode() && mCount != 0 )
//    return static_cast<WinMouse*> (mSystem->GetMouseByCount( 0))->DoMouseClick( mouseButton, isDown);

  if( isDown )
	  mState.buttons |= 1 << mouseButton; 
  else
	  mState.buttons &= ~(1 << mouseButton); //turn the bit flag off

  InputSystemHelper::DoMouseButton( this, mouseButton, isDown);
}

// --------------------------------------------------------------------------------------------------------------------
size_t WinMouse::GetNumButtons() const
{
  return MB_Count;
}
// --------------------------------------------------------------------------------------------------------------------
std::string WinMouse::GetButtonText( size_t idx) const
{
  return ""; // TODO: query DirectInput for names?
}
// --------------------------------------------------------------------------------------------------------------------
size_t WinMouse::GetNumAxis() const
{
  return 2; // two axis: axis0 is horizontal mouse pos, axis1 is vertical mouse pos.
}
// --------------------------------------------------------------------------------------------------------------------
std::string WinMouse::GetAxisText( size_t idx) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
bool WinMouse::IsButtonDown( size_t idx) const
{
  if( idx < MB_Count )
    return (mState.buttons & (1 << idx)) != 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool WinMouse::WasButtonPressed( size_t idx) const
{
  if( idx < MB_Count )
    return IsButtonDown( idx) && (mState.prevButtons & (1 << idx)) == 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool WinMouse::WasButtonReleased( size_t idx) const
{
  if( idx < MB_Count )
    return !IsButtonDown( idx) && (mState.prevButtons & (1 << idx)) != 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
float WinMouse::GetAxisAbsolute( size_t idx) const
{
  switch( idx )
  {
    case 0: return float( mState.absX);
    case 1: return float( mState.absY);
    default: return 0.0f;
  }
}
// --------------------------------------------------------------------------------------------------------------------
float WinMouse::GetAxisDifference( size_t idx) const
{
  switch( idx )
  {
    case 0: return float( mState.relX);
    case 1: return float( mState.relY);
    default: return 0.0f;
  }
}
// --------------------------------------------------------------------------------------------------------------------
int WinMouse::GetMouseX() const { return mState.absX; }
// --------------------------------------------------------------------------------------------------------------------
int WinMouse::GetMouseY() const { return mState.absY; }
// --------------------------------------------------------------------------------------------------------------------
int WinMouse::GetRelMouseX() const { return mState.relX; }
// --------------------------------------------------------------------------------------------------------------------
int WinMouse::GetRelMouseY() const { return mState.relY; }

#endif // SNIIS_SYSTEM_WINDOWS