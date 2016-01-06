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
        // reserve axis 2 for mouse wheel
        if( num >= 2 ) ++num;
        if( mAxes.size() <= num )
          mAxes.resize( num+1);
        mAxes[num] = Axis{ vcl->label, vcl->min, vcl->max, 0.0, 0.0, vcl->mode == XIModeAbsolute };
      }
    }
  }

  // insert dummy mouse wheel axis
  if( mAxes.size() < 3 )
    mAxes.emplace_back();
  mAxes[2] = Axis{ 0, 0, 256, 0.0, 0.0f, false };
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxMouse::StartUpdate()
{
  mState.prevButtons = mState.buttons;

  for( auto& a : mAxes )
    a.prevValue = a.value;
  // wheel axis is relative, shows movements only. So zero out for each frame and start accumulating differences anew
  mAxes[2].value = 0.0;
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
          if( !mIsFirstUpdate )
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
      if( !mIsFirstUpdate )
        InputSystemHelper::MakeThisMouseFirst( this);

      size_t button = size_t( ev.detail);
      bool isPressed = (ev.evtype == XI_RawButtonPress);
      // Mouse wheel. There seem to be two of those, we treat them the same
      if( button >= 4 && button <= 7 )
      {
        if( isPressed )
          mState.wheel += ((button&1) == 0 ? 1 : -1);
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
  if( !mIsFirstUpdate )
  {
    // send the mouse move
    if( mAxes[0].prevValue != mAxes[0].value || mAxes[1].prevValue != mAxes[1].value )
      InputSystemHelper::DoMouseMove( this, mAxes[0].value, mAxes[1].value, mAxes[0].value - mAxes[0].prevValue, mAxes[1].value - mAxes[1].prevValue);
    // send the mouse wheel axis
    if( mAxes[2].prevValue != mAxes[2].value )
      InputSystemHelper::DoMouseWheel( this, mAxes[2].value);
  }
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxMouse::SetFocus( bool pHasFocus)
{
  if( pHasFocus )
  {
    // get current mouse position when in SingleMouseMode
    if( !mSystem->IsInMultiMouseMode() )
    {
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
void LinuxMouse::DoMouseClick( int mouseButton, bool isDown )
{
  if( isDown )
	  mState.buttons |= 1 << mouseButton;
  else
	  mState.buttons &= ~(1 << mouseButton); //turn the bit flag off

  if( !mIsFirstUpdate )
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
