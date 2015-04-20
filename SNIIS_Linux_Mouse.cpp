/// @file SNIIS_Linux_Mouse.cpp
/// Linux implementation of mice

#include "SNIIS_Linux.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_LINUX
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
LinuxMouse::LinuxMouse( LinuxInput* pSystem, size_t pId, const XIDeviceInfo& pDeviceInfo)
  : Mouse( pId), mSystem( pSystem), mDeviceId( pDeviceInfo.deviceid)
{
  mState.absWheel = mState.relWheel = 0;
  mState.buttons = mState.prevButtons = 0;

  // enumerate all controls on that device
  for( int a = 0; a < pDeviceInfo.num_classes; ++a )
  {
    auto cl = pDeviceInfo.classes[a];
    switch( cl->type )
    {
      case XIButtonClass:
      {
        auto bcl = reinterpret_cast<const XIButtonClassInfo*> (cl);
        for( int b = 0; b < bcl->num_buttons; ++b )
          mButtons.emplace_back( Button{ bcl->labels[b] });
        break;
      }
      case XIValuatorClass:
      {
        auto vcl = reinterpret_cast<const XIValuatorClassInfo*> (cl);
        size_t num = size_t( vcl->number);
        if( mAxes.size() <= num )
          mAxes.resize( num+1);
        mAxes[num] = Axis{ vcl->label, vcl->min, vcl->max, 0.0, 0.0, vcl->mode == XIModeAbsolute };
      }
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxMouse::StartUpdate()
{
  mState.relWheel = 0;
  mState.prevButtons = mState.buttons;

  for( auto& a : mAxes )
    a.prevValue = a.value;
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxMouse::HandleEvent( const XIRawEvent& ev)
{
  switch( ev.evtype )
  {
    case XI_RawMotion:
    {
      size_t numReportedAxes = std::min( mAxes.size(), size_t( ev.valuators.mask_len*8));
      const double* values = ev.valuators.values;
      for( size_t a = 0; a < numReportedAxes; ++a )
      {
        if( XIMaskIsSet( ev.valuators.mask, a) )
        {
          InputSystemHelper::MakeThisMouseFirst( this);
          double v = *values++;
          if( mAxes[a].isAbsolute )
            mAxes[a].value = v;
          else
            mAxes[a].value += v;
        }
      }
      break;
    }

    case XI_RawButtonPress:
    case XI_RawButtonRelease:
    {
      InputSystemHelper::MakeThisMouseFirst( this);

      size_t button = size_t( ev.detail);
      bool isPressed = (ev.evtype == XI_RawButtonPress);
      // Mouse wheel. There seem to be two of those, we treat them the same
      if( button >= 4 && button <= 7 )
      {
        if( isPressed )
          mState.relWheel += ((button&1) == 0 ? 1 : -1);
      } else
      {
        --button;
        // we do "Left, Right, Middle", they do "Left, Middle, Right" - remap that
        if( button == 1 ) button = 2; else if( button == 2 ) button = 1;
        // announce
        if( button < mButtons.size() )
          DoMouseClick( button, isPressed);
      }
      break;
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxMouse::EndUpdate()
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
/*    POINT point;
    GetCursorPos(&point);
    ScreenToClient( mSystem->GetWindowHandle(), &point);
    int prevAbsX = mState.absX, prevAbsY = mState.absY;
    mState.absX = point.x; mState.absY = point.y;
    mState.relX = mState.absX - prevAbsX;
    mState.relY = mState.absY - prevAbsY;
    */
  }

  // send the mouse move
  if( mAxes[0].prevValue != mAxes[0].value || mAxes[1].prevValue != mAxes[1].value )
    InputSystemHelper::DoMouseMove( this, mAxes[0].value, mAxes[1].value, mAxes[0].value - mAxes[0].prevValue, mAxes[1].value - mAxes[1].prevValue);
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxMouse::DoMouseClick( int mouseButton, bool isDown )
{
  if( isDown )
	  mState.buttons |= 1 << mouseButton;
  else
	  mState.buttons &= ~(1 << mouseButton); //turn the bit flag off

  InputSystemHelper::DoMouseButton( this, mouseButton, isDown);
}

// --------------------------------------------------------------------------------------------------------------------
size_t LinuxMouse::GetNumButtons() const
{
  return std::max( size_t( MB_Count), mButtons.size());
}
// --------------------------------------------------------------------------------------------------------------------
std::string LinuxMouse::GetButtonText( size_t idx) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
size_t LinuxMouse::GetNumAxes() const
{
  return mAxes.size();
}
// --------------------------------------------------------------------------------------------------------------------
std::string LinuxMouse::GetAxisText( size_t idx) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
bool LinuxMouse::IsButtonDown( size_t idx) const
{
  if( idx < GetNumButtons() )
    return (mState.buttons & (1 << idx)) != 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool LinuxMouse::WasButtonPressed( size_t idx) const
{
  if( idx < GetNumButtons() )
    return IsButtonDown( idx) && (mState.prevButtons & (1 << idx)) == 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool LinuxMouse::WasButtonReleased( size_t idx) const
{
  if( idx < GetNumButtons() )
    return !IsButtonDown( idx) && (mState.prevButtons & (1 << idx)) != 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
float LinuxMouse::GetAxisAbsolute( size_t idx) const
{
  if( idx < mAxes.size() )
    return float( mAxes[idx].value);
  else
    return 0.0f;
}
// --------------------------------------------------------------------------------------------------------------------
float LinuxMouse::GetAxisDifference( size_t idx) const
{
  if( idx < mAxes.size() )
    return float( mAxes[idx].value - mAxes[idx].prevValue);
  else
    return 0.0f;
}
// --------------------------------------------------------------------------------------------------------------------
int LinuxMouse::GetMouseX() const { return int( GetAxisAbsolute( 0)); }
// --------------------------------------------------------------------------------------------------------------------
int LinuxMouse::GetMouseY() const { return int( GetAxisAbsolute( 1)); }
// --------------------------------------------------------------------------------------------------------------------
int LinuxMouse::GetRelMouseX() const { return int( GetAxisDifference( 0)); }
// --------------------------------------------------------------------------------------------------------------------
int LinuxMouse::GetRelMouseY() const { return int( GetAxisDifference( 1)); }

#endif // SNIIS_SYSTEM_LINUX
