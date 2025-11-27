/// @file SNIIS_Win_Mouse.cpp
/// Windows implementation of mice

#include "SNIIS_Win.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_WINDOWS
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
WinMouse::WinMouse( WinInput* pSystem, size_t pId, HANDLE pHandle)
  : Mouse( pId, pHandle == nullptr), mSystem( pSystem), mHandle( pHandle)
{
  mState.absX = mState.absY = 0.0f;
  mState.relX = mState.relY = 0.0f;
  mState.wheel = mState.prevWheel = 0.0f;
  mState.buttons = mState.prevButtons = 0;
  mIsInUpdate = false;
  mOutOfUpdateRelX = mOutOfUpdateRelY = 0;

  // read initial position
  POINT point;
  GetCursorPos(&point);
  ScreenToClient( mSystem->GetWindowHandle(), &point);
  mState.absX = float( point.x); mState.absY = float( point.y);
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::StartUpdate()
{
  // we're not going to reset the relative state here. It turns out we sometimes miss RawInput messages because they
  // occur in the small gap between our GetRawInputBuffer() and the end of the game's message loop. So we hooked into
  // the wndproc to get those messages, too, but those will now call our ParseMessage() *after* our EndUpdate() is done.
  // So in order to collect and broadcast those messages, too, we reset the state in EndUpdate() and thus carry any
  // lost RawInput messages over to the Update() of next frame.

  // except for the buttons and wheel, because that is calculated and broadcasted immediatly
  mState.prevButtons = mState.buttons;
  mState.prevWheel = mState.wheel; mState.wheel = 0;
  // that plops back the wheel to zero. Broadcast this as well
  if( mState.prevWheel != 0 )
    DoMouseWheel( 0.0f);

  // take over the OutOfUpdate differences
  mState.relX = mOutOfUpdateRelX; mState.relY = mOutOfUpdateRelY;
  mOutOfUpdateRelX = mOutOfUpdateRelY = 0;
  mIsInUpdate = true;
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::ParseMessage( const RAWINPUT& e, bool useWorkaround)
{
  // not a mouse or not our mouse - just a safety, such a message shouldn't even reach us
  assert( e.header.dwType == RIM_TYPEMOUSE && e.header.hDevice == mHandle );
  // work around for GetRawInputBuffer() bug on WoW64
  auto ptr = reinterpret_cast<const uint8_t*> (&e.data.mouse);
  if( useWorkaround )
    ptr += 8;
  const RAWMOUSE& mouse = *(reinterpret_cast<const RAWMOUSE*> (ptr));

  // Mouse buttons - Raw Input only supports five
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN ) DoMouseButton( MB_Left, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) DoMouseButton( MB_Left, false);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN ) DoMouseButton( MB_Right, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) DoMouseButton( MB_Right, false);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN ) DoMouseButton( MB_Middle, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP) DoMouseButton( MB_Middle, false);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN ) DoMouseButton( MB_Button3, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) DoMouseButton( MB_Button3, false);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN ) DoMouseButton( MB_Button4, true);
  if( mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) DoMouseButton( MB_Button4, false);

  // Mouse wheel
  if( mouse.usButtonFlags & RI_MOUSE_WHEEL )
    DoMouseWheel( float( short( mouse.usButtonData)));

  // Movement
  float& relx = mIsInUpdate ? mState.relX : mOutOfUpdateRelX;
  float& rely = mIsInUpdate ? mState.relY : mOutOfUpdateRelY;

  if( (mouse.usFlags & 1) == MOUSE_MOVE_RELATIVE )
  {
    if( mouse.lLastX != 0 || mouse.lLastY != 0 )
    {
      relx += float( mouse.lLastX); mState.absX += float( mouse.lLastX);
      rely += float( mouse.lLastY); mState.absY += float( mouse.lLastY);
    }
  }
  else
  {
    float preX = mState.absX, preY = mState.absY;
    if( mState.absX != float( mouse.lLastX) || mState.absY != float( mouse.lLastY) )
    {
      mState.absX = float( mouse.lLastX); mState.absY = float( mouse.lLastY);
      relx += mState.absX - preX;
      rely += mState.absY - preY;
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::EndUpdate()
{
  mIsInUpdate = false;
  mOutOfUpdateRelX = mOutOfUpdateRelY = 0;

  // Assembled Mouse gets the global mouse position.
  if( IsAssembled() )
  {
    POINT currMousePos;
    GetCursorPos( &currMousePos);
    ScreenToClient( mSystem->GetWindowHandle(), &currMousePos);
    // if mouse is grabbed, accumulate offset from center and then move pointer back to that center
    if( mSystem->IsMouseGrabbed() )
    {
      RECT rect;
      GetWindowRect( mSystem->GetWindowHandle(), &rect);
      POINT wndCenterPos = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
      mState.relX = float( currMousePos.x - wndCenterPos.x);
      mState.relY = float( currMousePos.y - wndCenterPos.y);
      mState.absX += mState.relX; mState.absY += mState.relY;
      ClientToScreen( mSystem->GetWindowHandle(), &wndCenterPos);
      SetCursorPos( wndCenterPos.x, wndCenterPos.y);
    } else
    {
      // single mouse mode without grabbing: we're not allowed to lock the mouse like this, so we simply mirror the
      // global mouse movement to get expected mouse acceleration and such at least.
      float prevAbsX = mState.absX, prevAbsY = mState.absY;
      mState.absX = float( currMousePos.x); mState.absY = float( currMousePos.y);
      mState.relX = mState.absX - prevAbsX;
      mState.relY = mState.absY - prevAbsY;
    }
  }

  // send the mouse move
  if( mState.relX != 0 || mState.relY != 0 )
    InputSystemHelper::DoMouseMove( this, mState.absX, mState.absY, mState.relX, mState.relY);
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::DoMouseWheel( float wheel)
{
  // also apply to primary mouse 
  if( !IsAssembled() )
    dynamic_cast<WinMouse*> (mSystem->GetMouseByCount(0))->DoMouseWheel( wheel);

  // store change
  mState.wheel += wheel;
  InputSystemHelper::DoMouseWheel( this, wheel);
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::DoMouseButton(size_t btnIndex, bool isPressed)
{
  InputSystem::Log("Maus %zd, button %zd %s", mId, btnIndex, isPressed ? "gedrückt" : "losgelassen");
  // also apply to primary mouse
  if( !IsAssembled() )
    dynamic_cast<WinMouse*>(mSystem->GetMouseByCount(0))->DoMouseButton(btnIndex, isPressed);

  // don't signal if it isn't an actual state change
  if (!!(mState.buttons & (1u << btnIndex)) == isPressed)
    return;

  // store state change
  uint32_t bitmask = (1 << btnIndex);
  mState.buttons = (mState.buttons & ~bitmask) | (isPressed ? bitmask : 0);

  // and notify everyone interested
  InputSystemHelper::DoMouseButton(this, btnIndex, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void WinMouse::SetFocus( bool pHasFocus)
{
  if( pHasFocus )
  {
    // get current mouse position when we're the assembled mouse
    if( IsAssembled() )
    {
      POINT point;
      GetCursorPos(&point);
      ScreenToClient( mSystem->GetWindowHandle(), &point);
      float prevAbsX = mState.absX, prevAbsY = mState.absY;
      mState.absX = float( point.x); mState.absY = float( point.y);
      mState.relX = mState.absX - prevAbsX;
      mState.relY = mState.absY - prevAbsY;
      if( !mIsFirstUpdate )
        if( mState.relX != 0 || mState.relY != 0 )
          InputSystemHelper::DoMouseMove( this, mState.absX, mState.absY, mState.relX, mState.relY);
    }
  }
  else
  {
    for( size_t a = 0; a < MB_Count; ++a )
    {
      if( mState.buttons & (1 << a) )
      {
        DoMouseButton( a, false);
        mState.prevButtons |= (1 << a);
      }
    }
  }
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
size_t WinMouse::GetNumAxes() const
{
  return 3; // axis0 is horizontal mouse pos, axis1 is vertical mouse pos, axis2 is mouse wheel
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
    case 2: return float( mState.wheel); 
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
    case 2: return float( mState.wheel - mState.prevWheel);
    default: return 0.0f;
  }
}
// --------------------------------------------------------------------------------------------------------------------
float WinMouse::GetMouseX() const { return mState.absX; }
// --------------------------------------------------------------------------------------------------------------------
float WinMouse::GetMouseY() const { return mState.absY; }
// --------------------------------------------------------------------------------------------------------------------
float WinMouse::GetRelMouseX() const { return mState.relX; }
// --------------------------------------------------------------------------------------------------------------------
float WinMouse::GetRelMouseY() const { return mState.relY; }

#endif // SNIIS_SYSTEM_WINDOWS
