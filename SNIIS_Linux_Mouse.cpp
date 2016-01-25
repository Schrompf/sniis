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
      static std::vector<double> diffs;
      if( diffs.size() < numReportedAxes )
        diffs.resize( numReportedAxes);
      std::fill( diffs.begin(), diffs.end(), 0.0);

      for( size_t a = 0; a < numReportedAxes; ++a )
      {
        if( XIMaskIsSet( ev.valuators.mask, a) )
        {
          if( !mIsFirstUpdate )
            InputSystemHelper::SortThisMouseToFront( this);
          double v = *values++;
          if( mAxes[a].isAbsolute )
            diffs[a] = v - mAxes[a].value;
          else
            diffs[a] = v;
        }
      }

      DoMouseMove( diffs.data(), diffs.size());
      break;
    }

    case XI_RawButtonPress:
    case XI_RawButtonRelease:
    {
      if( !mIsFirstUpdate )
        InputSystemHelper::SortThisMouseToFront( this);

      size_t button = size_t( ev.detail);
      bool isPressed = (ev.evtype == XI_RawButtonPress);
      // Mouse wheel. There seem to be two of those, we treat them the same
      if( button >= 4 && button <= 7 )
      {
        if( isPressed && !mIsFirstUpdate )
          DoMouseWheel( (button&1) == 0 ? 1.0 : -1.0);
      } else
      {
        --button;
        // we do "Left, Right, Middle", they do "Left, Middle, Right" - remap that
        if( button == 1 ) button = 2; else if( button == 2 ) button = 1;
        // announce
        if( button < mButtons.size() && !mIsFirstUpdate )
          DoMouseButton( button, isPressed);
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
    // send the mouse move if we're primary or separate
    if( mSystem->IsInMultiDeviceMode() || GetCount() == 0 )
    {
      if( mAxes[0].prevValue != mAxes[0].value || mAxes[1].prevValue != mAxes[1].value )
        InputSystemHelper::DoMouseMove( this, mAxes[0].value, mAxes[1].value, mAxes[0].value - mAxes[0].prevValue, mAxes[1].value - mAxes[1].prevValue);
      // send the wheel
      if( mAxes[2].prevValue != mAxes[2].value )
        InputSystemHelper::DoMouseWheel( this, float( mAxes[2].value));
      // send the other axes, if there are any
      for( size_t a = 3; a < mAxes.size(); ++a )
        if( mAxes[a].prevValue != mAxes[a].value )
          InputSystemHelper::DoAnalogEvent( this, a, mAxes[a].value);
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxMouse::DoMouseMove( double* diffs, size_t diffcount)
{
  // apply to our values. Necessary to make the difference calculation in HandleEvent() work correctly
  for( size_t a = 0; a < std::min( diffcount, mAxes.size()); ++a )
    mAxes[a].value += diffs[a];

  // also reroute to primary mouse if we're in SingleDeviceMode
  if( !mSystem->IsInMultiDeviceMode() && GetCount() != 0 )
    dynamic_cast<LinuxMouse*> (mSystem->GetMouseByCount( 0))->DoMouseMove( diffs, diffcount);

  // callbacks are triggered from EndUpdate()
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxMouse::DoMouseWheel( double wheel)
{
  // reroute to primary mouse if we're in SingleDeviceMode
  if( !mSystem->IsInMultiDeviceMode() && GetCount() != 0 )
    return dynamic_cast<LinuxMouse*> (mSystem->GetMouseByCount( 0))->DoMouseWheel( wheel);

  // store change
  mAxes[2].value += wheel;

  // callbacks are triggered from EndUpdate()
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxMouse::DoMouseButton( size_t btnIndex, bool isPressed)
{
  // reroute to primary mouse if we're in SingleDeviceMode
  if( !mSystem->IsInMultiDeviceMode() && GetCount() != 0 )
    return dynamic_cast<LinuxMouse*> (mSystem->GetMouseByCount( 0))->DoMouseButton( btnIndex, isPressed);

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
void LinuxMouse::SetFocus( bool pHasFocus)
{
  if( pHasFocus )
  {
    // get current mouse position when in SingleMouseMode
    if( !mSystem->IsInMultiDeviceMode() && GetCount() == 0 )
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
        DoMouseButton( a, false);
        mState.prevButtons |= (1 << a);
      }
    }
  }
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
float LinuxMouse::GetMouseX() const { return GetAxisAbsolute( 0); }
// --------------------------------------------------------------------------------------------------------------------
float LinuxMouse::GetMouseY() const { return GetAxisAbsolute( 1); }
// --------------------------------------------------------------------------------------------------------------------
float LinuxMouse::GetRelMouseX() const { return GetAxisDifference( 0); }
// --------------------------------------------------------------------------------------------------------------------
float LinuxMouse::GetRelMouseY() const { return GetAxisDifference( 1); }

#endif // SNIIS_SYSTEM_LINUX
