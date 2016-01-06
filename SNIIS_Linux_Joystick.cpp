/// @file SNIIS_Linux_Joystick.cpp
/// Linux implementation of anything that does not fit the description of a "mouse" or a "keyboard"

#include "SNIIS_Linux.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_LINUX
#include <cstring>
#include <linux/input.h>

using namespace SNIIS;

static bool IsBitSet( const uint8_t* bits, size_t i) { return (bits[i/8] & (1<<(i&7))) != 0; }

// --------------------------------------------------------------------------------------------------------------------
LinuxJoystick::LinuxJoystick( LinuxInput* pSystem, size_t pId, int pFileDesc)
  : Joystick( pId), mSystem( pSystem), mFileDesc( pFileDesc)
{
  memset( &mState, 0, sizeof( mState));

  // enumerate, now for real
  uint8_t ev_bits[(EV_MAX+7)/8];
	memset( ev_bits, 0, sizeof(ev_bits) );

	//Read "all" (hence 0) components of the device
	if( ioctl( mFileDesc, EVIOCGBIT( 0, sizeof(ev_bits)), ev_bits) == -1 )
		throw std::runtime_error( "Could not read device events features");

  // Absolute axes
  if( IsBitSet( ev_bits, EV_ABS) )
  {
    uint8_t abs_bits[(ABS_MAX+7)/8];
    memset( abs_bits, 0, sizeof(abs_bits) );

    if( ioctl( mFileDesc, EVIOCGBIT( EV_ABS, sizeof(abs_bits)), abs_bits) == -1 )
      throw std::runtime_error( "Could not read device absolute axis features");

    for( size_t a = 0; a < ABS_MAX; a++ )
    {
      if( IsBitSet( abs_bits, a) )
      {
        input_absinfo abInfo;
        if( ioctl( mFileDesc, EVIOCGABS(a), &abInfo ) == -1 )
          continue;

        mAxes.push_back( Axis{ a, true, abInfo.minimum, abInfo.maximum, abInfo.flat });
      }
    }
  }

  // Relative axes
  if( IsBitSet( ev_bits, EV_REL) )
  {
    uint8_t rel_bits[(REL_MAX+7)/8];
    memset( rel_bits, 0, sizeof(rel_bits) );

    if( ioctl( mFileDesc, EVIOCGBIT( EV_REL, sizeof(rel_bits)), rel_bits) == -1 )
      throw std::runtime_error( "Could not read device relative axis features");

    for( size_t a = 0; a < REL_MAX; a++ )
    {
      if( IsBitSet( rel_bits, a) )
      {
        mAxes.push_back( Axis{ a, false, 0, 0, 0 });
      }
    }
  }

  // Buttons
  if( IsBitSet( ev_bits, EV_KEY) )
  {
    uint8_t key_bits[(KEY_MAX+7)/8];
    memset( key_bits, 0, sizeof(key_bits) );

    if( ioctl( mFileDesc, EVIOCGBIT( EV_KEY, sizeof(key_bits)), key_bits) == -1 )
      throw std::runtime_error( "Could not read device button features");

    for( size_t a = 0; a < KEY_MAX; a++ )
    {
      if( IsBitSet( key_bits, a) )
      {
        mButtons.push_back( Button{ a });
      }
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxJoystick::StartUpdate()
{
  mState.prevButtons = mState.buttons;
  memset( mState.diffs, 0, sizeof( mState.diffs));
  float prevAxes[16];
  static_assert( sizeof( prevAxes) == sizeof( mState.axes), "Hardcoded array size");
  memcpy( prevAxes, mState.axes, sizeof( mState.axes));

  // read events from file descriptor
	input_event js[64];
	while( true )
	{
		int ret = read( mFileDesc, &js, sizeof(js));
    if( ret <= 0 )
			break;

		size_t numEvents = size_t( ret) / sizeof(struct input_event);
		for( size_t a = 0; a < numEvents; ++a )
		{
      const auto& ev = js[a];
			switch( ev.type )
			{
        case EV_KEY: // Button
        {
          size_t bt = ev.code;
          auto it = std::find_if( mButtons.cbegin(), mButtons.cend(), [=](const Button& b) { return b.idx == bt; });
          if( it == mButtons.cend() )
            break;

          size_t btidx = std::distance( mButtons.cbegin(), it);
          bool isPressed = (ev.value != 0);
          if( isPressed )
            mState.buttons |= 1ull << btidx;
          else
            mState.buttons &= (UINT64_MAX ^ (1ull << btidx));

          break;
        }

        case EV_ABS: // Absolute Axis
        {
          size_t ax = ev.code;
          auto it = std::find_if( mAxes.cbegin(), mAxes.cend(), [=](const Axis& c) { return c.isAbsolute && c.idx == ax; });
          if( it == mAxes.cend() )
            break;

          size_t axidx = std::distance( mAxes.cbegin(), it);
          float v = 0.0f;
          // try to tell one-sided axes apart from symmetric axes and act accordingly
          if( std::abs( it->min) <= std::abs( it->max) / 10 )
            v = float( ev.value - it->min) / float( it->max - it->min);
          else
            v = (float( ev.value - it->min) / float( it->max - it->min)) * 2.0f - 1.0f;
          mState.diffs[axidx] = v - mState.axes[axidx];
          mState.axes[axidx] = v;

          break;
        }

        case EV_REL: // Relative Axis.
        {
          size_t ax = ev.code;
          auto it = std::find_if( mAxes.cbegin(), mAxes.cend(), [=](const Axis& c) { return !c.isAbsolute && c.idx == ax; });
          if( it == mAxes.cend() )
            break;

          size_t axidx = std::distance( mAxes.cbegin(), it);
          float d = float( ev.value);
          mState.diffs[axidx] += d;
          mState.axes[axidx] += d;

          break;
        }

        default:
          break;
			}
		}
	}

  // send events - Axes
  for( size_t i = 0; i < mAxes.size(); i++ )
  {
    if( mState.axes[i] != prevAxes[i] )
    {
      mState.diffs[i] = mState.axes[i] - prevAxes[i];
      if( !mIsFirstUpdate )
        InputSystemHelper::DoJoystickAxis( this, i, mState.axes[i]);
    }
  }

  // send events - buttons
  for( size_t i = 0; i < mButtons.size(); i++ )
  {
    if( !mIsFirstUpdate )
      if( (mState.buttons ^ mState.prevButtons) & (1ull << i) )
        InputSystemHelper::DoJoystickButton( this, i, (mState.buttons & (1ull << i)) != 0);
  }
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxJoystick::SetFocus( bool pHasFocus)
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
size_t LinuxJoystick::GetNumButtons() const
{
  return mButtons.size();
}
// --------------------------------------------------------------------------------------------------------------------
std::string LinuxJoystick::GetButtonText( size_t idx) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
size_t LinuxJoystick::GetNumAxes() const
{
  return mAxes.size();
}
// --------------------------------------------------------------------------------------------------------------------
std::string LinuxJoystick::GetAxisText( size_t idx) const
{
  return "";
}

// --------------------------------------------------------------------------------------------------------------------
bool LinuxJoystick::IsButtonDown( size_t idx) const
{
  if( idx < mButtons.size() )
    return (mState.buttons & (1ull << idx)) != 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool LinuxJoystick::WasButtonPressed( size_t idx) const
{
  if( idx < mButtons.size() )
    return IsButtonDown( idx) && ((mState.prevButtons & (1ull << idx)) == 0);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool LinuxJoystick::WasButtonReleased( size_t idx) const
{
  if( idx < mButtons.size() )
    return !IsButtonDown( idx) && ((mState.prevButtons & (1ull << idx)) != 0);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
float LinuxJoystick::GetAxisAbsolute( size_t idx) const
{
  if( idx < mAxes.size() )
    return mState.axes[idx];
  else
    return 0.0f;
}
// --------------------------------------------------------------------------------------------------------------------
float LinuxJoystick::GetAxisDifference( size_t idx) const
{
  if( idx < mAxes.size() )
    return mState.diffs[idx];
  else
    return 0.0f;
}

#endif // SNIIS_SYSTEM_LINUX
