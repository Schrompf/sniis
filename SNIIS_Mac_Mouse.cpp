/// @file SNIIS_Mac_Mouse.cpp
/// Mac OSX implementation of mice

#include "SNIIS_Mac.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_MAC
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
MacMouse::MacMouse( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef, bool isTrackpad)
  : Mouse( pId), MacDevice( pSystem, pDeviceRef)
{
  mIsTrackpad = isTrackpad;
  memset( &mState, 0, sizeof( mState));
  mAxes.resize( 3, MacControl{ nullptr, MacControl::Type_Axis, "", 0, 0, 0, -1, 1 });
  mSecondaryAxes.resize( 3, MacControl{ nullptr, MacControl::Type_Axis, "", 0, 0, 0, -1, 1 });

  AddDevice( pDeviceRef);
}

// The MacBook shows up as two separate mice, where one does the movement and the other does clicks and wheel... unite those
void MacMouse::AddDevice( IOHIDDeviceRef pRef)
{
  if( IOHIDDeviceOpen( pRef, kIOHIDOptionsTypeNone) != kIOReturnSuccess )
    throw std::runtime_error( "Failed to open HID device");

  auto& btns = pRef == mDevice ? mButtons : mSecondaryButtons;
  auto& axes = pRef == mDevice ? mAxes : mSecondaryAxes;

  // get all controls, sort axes to fit 0:X, 1:Y, 2:MouseWheel, sort buttons to fit 0:Left, 1:Right, 2:Middle
  auto ctrls = EnumerateDeviceControls( pRef);
  for( const auto& c : ctrls )
  {
//    Log( "Control: \"%s\" Typ %d, keks %d, usage %d/%d, bereich %d..%d", c.mName.c_str(), c.mType, c.mCookie, c.mUsePage, c.mUsage, c.mMin, c.mMax);
    if( c.mType == MacControl::Type_Axis )
    {
      // add every axis only once, because both trackpads supply the same basic controls
      switch( c.mUsage )
      {
        case kHIDUsage_GD_X: axes[0] = c; break;
        case kHIDUsage_GD_Y: axes[1] = c; break;
        case kHIDUsage_GD_Wheel: axes[2] = c; break;
        default: axes.push_back( c); break;
      }
    }
    else if( c.mType == MacControl::Type_Button )
    {
      // there are no Usage codes to describe mouse buttons, but ManyMouse says the usage is a 1-based button index
      if( c.mUsePage == kHIDPage_Button )
      {
        size_t idx = size_t( c.mUsage) - 1;
        if( btns.size() <= idx )
          btns.resize( idx+1);
        btns[idx] = c;
      }
    }
  }

  // call us if something moves
  IOHIDDeviceRegisterInputValueCallback( pRef, &InputElementValueChangeCallback, static_cast<MacDevice*> (this));
  IOHIDDeviceScheduleWithRunLoop( pRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
}

MacMouse::~MacMouse()
{
  if( mDevice )
  {
    IOHIDDeviceUnscheduleFromRunLoop( mDevice, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceClose( mDevice, kIOHIDOptionsTypeNone);
  }
}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::StartUpdate()
{
  mState.prevButtons = mState.buttons;

  for( size_t a = 0; a < mAxes.size(); ++a )
    mState.prevAxes[a] = mState.axes[a];
  // axis 2 is always mouse wheel. this one always starts at zero on every update.
  mState.axes[2] = 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::HandleEvent(IOHIDDeviceRef dev, IOHIDElementCookie cookie, uint32_t usepage, uint32_t usage, CFIndex value)
{
  SNIIS_UNUSED( usepage);
  SNIIS_UNUSED( usage);

  if( value != 0 && !mIsFirstUpdate )
    InputSystemHelper::SortThisMouseToFront( this);

  const auto& axes = dev == mDevice ? mAxes : mSecondaryAxes;
  auto axit = std::find_if( axes.cbegin(), axes.cend(), [&](const MacControl& c) { return c.mCookie == cookie; });
  if( axit != axes.cend() )
  {
    size_t idx = std::distance( axes.cbegin(), axit);
    if( axit->mType == MacControl::Type_Hat )
    {
      // a hat always generates two axes
      assert( axes.size() > idx+2 );
      std::tie( mState.axes[idx], mState.axes[idx+1]) = ConvertHatToAxes( axit->mMin, axit->mMax, value );
    }
    else
    {
      // first two axes are pixel-sized position, which we care for only in multi-mouse mode
      if( idx < 2 )
      {
        // in single mouse mode the position will be queried from the OS in EndUpdate()
        if( mSystem->IsInMultiDeviceMode() )
          mState.axes[idx] += float( value);
      }
      else if( idx < 3 )
      {
        DoMouseWheel( float( value));
      }
      else
      {
        // try to tell one-sided axes apart from symmetric axes and act accordingly
        if( abs( axit->mMin) <= abs( axit->mMax) / 10 )
          mState.axes[idx] = float( value - axit->mMin) / float( axit->mMax - axit->mMin);
        else
          mState.axes[idx] = (float( value - axit->mMin) / float( axit->mMax - axit->mMin)) * 2.0f - 1.0f;
      }
    }
  }

  {
    auto& btns = dev == mDevice ? mButtons : mSecondaryButtons;
    auto buttit = std::find_if( btns.cbegin(), btns.cend(), [&](const MacControl& c) { return c.mCookie == cookie; });
    if( buttit != btns.cend() )
    {
      size_t idx = std::distance( btns.cbegin(), buttit);
      bool isDown = (value != 0);
      if( !mIsFirstUpdate )
        DoMouseButton( idx, isDown);
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::EndUpdate()
{
  // get global mouse position if single mouse mode, because the HID mouse movements lack acceleration and such
  // just like RawInput on Windows
  if( !mSystem->IsInMultiDeviceMode() && mSystem->HasFocus() && mCount == 0 )
  {
    // convert to local coordinates
    Pos mp = MacHelper_GetMousePos();
    Pos lp = MacHelper_DisplayToWin( mSystem->GetWindowId(), mp);
    if( lp.x > -10000.0f && lp.y > -10000.0f )
    {
      if( mSystem->IsMouseGrabbed() )
      {
        WindowRect rect = MacHelper_GetWindowRect( mSystem->GetWindowId());
        Pos wndCenterPos = { rect.w / 2, rect.h / 2 };
        float dx = lp.x - wndCenterPos.x, dy = lp.y - wndCenterPos.y;
        if( dx*dx + dy*dy > 0.7f )
        {
          mState.axes[0] += dx; mState.axes[1] += dy;
          Pos globalWndCenter = MacHelper_WinToDisplay( mSystem->GetWindowId(), wndCenterPos);
          MacHelper_SetMousePos( globalWndCenter);
        }
      } else
      {
        // single mouse mode without grabbing: we're not allowed to lock the mouse like this, so we simply mirror the
        // global mouse movement to get expected mouse acceleration and such at least.
        mState.axes[0] = lp.x; mState.axes[1] = lp.y;
      }
    }
  }

  if( !mIsFirstUpdate )
  {
    // send the mouse move
    if( mState.prevAxes[0] != mState.axes[0] || mState.prevAxes[1] != mState.axes[1] )
      InputSystemHelper::DoMouseMove( this, mState.axes[0], mState.axes[1], mState.axes[0] - mState.prevAxes[0], mState.axes[1] - mState.prevAxes[1]);
    // send the mouse wheel.
    if( mState.prevAxes[2] != mState.axes[2] )
      InputSystemHelper::DoMouseWheel( this, mState.axes[2]);
    // send the other axes, if there are any
    for( size_t a = 3; a < mAxes.size(); ++a )
      if( mState.axes[a] != mState.prevAxes[a] )
        InputSystemHelper::DoAnalogEvent( this, a, mState.axes[a]);
  }
}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::DoMouseWheel( float wheel)
{
  // reroute to primary mouse if we're in SingleDeviceMode
  if( !mSystem->IsInMultiDeviceMode() && GetCount() != 0 )
    return dynamic_cast<MacMouse*> (mSystem->GetMouseByCount( 0))->DoMouseWheel( wheel);

  // store change
  mState.axes[2] += wheel;

  // callbacks are triggered from EndUpdate()
}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::DoMouseButton( size_t btnIndex, bool isPressed)
{
  // reroute to primary mouse if we're in SingleDeviceMode
  if( !mSystem->IsInMultiDeviceMode() && GetCount() != 0 )
    return dynamic_cast<MacMouse*> (mSystem->GetMouseByCount( 0))->DoMouseButton( btnIndex, isPressed);

  // don't signal if it isn't an actual state change
  if( !!(mState.buttons & (1u << btnIndex)) == isPressed )
    return;

  // store state change
  uint32_t bitmask = (1 << btnIndex);
  mState.buttons = (mState.buttons & ~bitmask) | (isPressed ? bitmask : 0);

  // and notify everyone interested
  InputSystemHelper::DoMouseButton( this, btnIndex, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void MacMouse::SetFocus( bool pHasFocus)
{
  if( pHasFocus )
  {
    // get current mouse position when in SingleMouseMode
    if( !mSystem->IsInMultiDeviceMode() )
    {
      Pos mp = MacHelper_GetMousePos();
      float px = mState.axes[0], py = mState.axes[1];
      mState.axes[0] = mp.x; mState.axes[1] = mp.y;
      if( px != mState.axes[0] || py != mState.axes[1] )
        InputSystemHelper::DoMouseMove( this, mState.axes[0], mState.axes[1], mState.axes[0] - px, mState.axes[1] - py);
    }
  }
  else
  {
    for( size_t a = 0; a < mButtons.size(); ++a )
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
  return mButtons.size();
}
// --------------------------------------------------------------------------------------------------------------------
std::string MacMouse::GetButtonText( size_t idx) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
size_t MacMouse::GetNumAxes() const
{
  return mAxes.size();
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
  if( idx < GetNumAxes() )
    return float( mState.axes[idx]);
  else
    return 0.0f;
}
// --------------------------------------------------------------------------------------------------------------------
float MacMouse::GetAxisDifference( size_t idx) const
{
  if( idx < GetNumAxes() )
    return float( mState.axes[idx] - mState.prevAxes[idx]);
  else
    return 0.0f;
}
// --------------------------------------------------------------------------------------------------------------------
float MacMouse::GetMouseX() const { return GetAxisAbsolute( 0); }
// --------------------------------------------------------------------------------------------------------------------
float MacMouse::GetMouseY() const { return GetAxisAbsolute( 1); }
// --------------------------------------------------------------------------------------------------------------------
float MacMouse::GetRelMouseX() const { return GetAxisDifference( 0); }
// --------------------------------------------------------------------------------------------------------------------
float MacMouse::GetRelMouseY() const { return GetAxisDifference( 1); }

#endif // SNIIS_SYSTEM_MAC
