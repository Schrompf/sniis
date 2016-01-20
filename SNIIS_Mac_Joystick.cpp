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

  if( IOHIDDeviceOpen( mDevice, kIOHIDOptionsTypeNone) != kIOReturnSuccess )
    throw std::runtime_error( "Failed to open HID device");

  // get all controls
  auto ctrls = EnumerateDeviceControls( mDevice);
  for( const auto& c : ctrls )
  {
//    Log( "Control: \"%s\" Typ %d, keks %d, usage %d/%d, bereich %d..%d", c.mName.c_str(), c.mType, c.mCookie, c.mUsePage, c.mUsage, c.mMin, c.mMax);
    if( c.mType != MacControl::Type_Button )
      mAxes.push_back( c);
    else
      mButtons.push_back( c);
  }

  // sort both lists by usage, because at least for the XBox360 controller I have here I got a reliable layout that
  // matches the list under Linux and Windows
  std::sort( mAxes.begin(), mAxes.end(), [](MacControl& c1, MacControl& c2) { return c1.mUsage < c2.mUsage; });
  std::sort( mButtons.begin(), mButtons.end(), [](MacControl& c1, MacControl& c2) { return c1.mUsage < c2.mUsage; });

  // call us if something moves
  IOHIDDeviceRegisterInputValueCallback( mDevice, &InputElementValueChangeCallback, static_cast<MacDevice*> (this));
  IOHIDDeviceScheduleWithRunLoop( mDevice, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
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
void MacJoystick::HandleEvent( IOHIDDeviceRef dev, IOHIDElementCookie cookie, uint32_t usepage, uint32_t usage, CFIndex value)
{
  SNIIS_UNUSED( dev);
  SNIIS_UNUSED( usepage);
  SNIIS_UNUSED( usage);

  auto axit = std::find_if( mAxes.cbegin(), mAxes.cend(), [&](const MacControl& c) { return c.mCookie == cookie; });
  auto buttit = std::find_if( mButtons.cbegin(), mButtons.cend(), [&](const MacControl& c) { return c.mCookie == cookie; });

  if( axit != mAxes.cend() )
  {
    size_t idx = std::distance( mAxes.cbegin(), axit);
    if( axit->mType == MacControl::Type_Hat )
    {
      // a hat always generates two axes
      assert( mAxes.size() > idx+2 );
      std::tie( mState.axes[idx], mState.axes[idx+1]) = ConvertHatToAxes( axit->mMin, axit->mMax, value );
      if( !mIsFirstUpdate )
      {
        InputSystemHelper::DoJoystickAxis( this, idx, mState.axes[idx]);
        InputSystemHelper::DoJoystickAxis( this, idx+1, mState.axes[idx+1]);
      }
    }
    else
    {
      // try to tell one-sided axes apart from symmetric axes and act accordingly
      if( abs( axit->mMin) <= abs( axit->mMax) / 10 )
        mState.axes[idx] = float( value - axit->mMin) / float( axit->mMax - axit->mMin);
      else
        mState.axes[idx] = (float( value - axit->mMin) / float( axit->mMax - axit->mMin)) * 2.0f - 1.0f;
      if( !mIsFirstUpdate )
        InputSystemHelper::DoJoystickAxis( this, idx, mState.axes[idx]);
    }
  }

  if( buttit != mButtons.cend() )
  {
    size_t idx = std::distance( mButtons.cbegin(), buttit);
    bool isDown = (value != 0);
    mState.buttons = (mState.buttons & (UINT64_MAX ^ (1ull << idx))) | ((isDown ? 1ull : 0u) << idx);
    if( !mIsFirstUpdate )
      InputSystemHelper::DoJoystickButton( this, idx, isDown);
  }
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
std::string MacJoystick::GetButtonText( size_t /*idx*/) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
size_t MacJoystick::GetNumAxes() const
{
  return mAxes.size();
}
// --------------------------------------------------------------------------------------------------------------------
std::string MacJoystick::GetAxisText( size_t /*idx*/) const
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

