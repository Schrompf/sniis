/// @file SNIIS_Win_Joystick.cpp
/// Windows implementation of joysticks, controllers, quiffles, smoeffles

#include "SNIIS_Win.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_WINDOWS
#include <XInput.h>
#include <algorithm>

using namespace SNIIS;

static const size_t JOYSTICK_DX_BUFFERSIZE = 64;
static const int MIN_AXIS = -32768, MAX_AXIS = 32767;
static const size_t XINPUT_TRANSLATED_BUTTON_COUNT = 13;
static const size_t XINPUT_TRANSLATED_AXIS_COUNT = 8;

// --------------------------------------------------------------------------------------------------------------------
WinJoystick::WinJoystick( WinInput* pSystem, size_t pId, IDirectInput8* pDI, GUID pGuidInstance, GUID pGuidProduct)
  : Joystick( pId), mSystem( pSystem), mDirectInput( pDI), mGuidInstance( pGuidInstance), mGuidProduct( pGuidProduct)
{
  mXInputPadIndex = SIZE_MAX;
  mNumRealButtons = mNumButtons = mNumAxes = 0;
  memset( &mState, 0, sizeof( mState));

  DIPROPDWORD dipdw;
  dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
  dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  dipdw.diph.dwObj        = 0;
  dipdw.diph.dwHow        = DIPH_DEVICE;
  dipdw.dwData            = JOYSTICK_DX_BUFFERSIZE;

  if( FAILED( mDirectInput->CreateDevice( mGuidInstance, &mJoystick, NULL)) )
    throw std::runtime_error( "Could not initialize controller device");

  if( FAILED( mJoystick->SetDataFormat( &c_dfDIJoystick2)) )
    throw std::runtime_error( "Controller data format error");

  if( FAILED( mJoystick->SetCooperativeLevel( mSystem->GetWindowHandle(), DISCL_FOREGROUND | DISCL_EXCLUSIVE)) )
    throw std::runtime_error( "Controller failed to set cooperation level");

  if( FAILED( mJoystick->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph)) )
    throw std::runtime_error( "Controller failed to set buffer size property" );

  // Get joystick capabilities.
  mDIJoyCaps.dwSize = sizeof(DIDEVCAPS);
  if( FAILED( mJoystick->GetCapabilities( &mDIJoyCaps)) )
    throw std::runtime_error( "Controller failed to get capabilities");

  // TODO: handle POVs, maybe as two axes?
  //mPOVs = (short)mDIJoyCaps.dwPOVs;
  mNumRealButtons = mDIJoyCaps.dwButtons;
  mNumAxes = mDIJoyCaps.dwAxes;
  mNumButtons = mNumRealButtons + 2*mNumAxes;

  //Enumerate and set axis constraints
  mEnumAxis = 0;
  mJoystick->EnumObjects( DIEnumDeviceObjectsCallback, this, DIDFT_AXIS);
}

// --------------------------------------------------------------------------------------------------------------------
void WinJoystick::StartUpdate()
{
  mState.prevButtons = mState.buttons;
  memset( mState.diffs, 0, sizeof( mState.diffs));
  float prevAxes[16];
  static_assert( sizeof( prevAxes) == sizeof( mState.axes), "Hardcoded array size");
  memcpy( prevAxes, mState.axes, sizeof( mState.axes));

  // handle xbox controller differently
  if( IsXInput() )
  {
    XINPUT_STATE inputState;
    auto res = XInputGetState( (DWORD)mXInputPadIndex, &inputState);
    if( res != ERROR_SUCCESS )
      memset(&inputState, 0, sizeof(inputState));

    //Sticks and triggers
    mState.axes[0] = std::max( -1.0f, float( -inputState.Gamepad.sThumbLY) / 32767.0f);
    mState.axes[1] = std::max( -1.0f, float( inputState.Gamepad.sThumbLX) / 32767.0f);
    mState.axes[2] = std::max( -1.0f, float( inputState.Gamepad.bLeftTrigger) / 127.0f);
    mState.axes[3] = std::max( -1.0f, float( -inputState.Gamepad.sThumbRY) / 32767.0f);
    mState.axes[4] = std::max( -1.0f, float( inputState.Gamepad.sThumbRX) / 32767.0f);
    mState.axes[5] = std::max( -1.0f, float( inputState.Gamepad.bRightTrigger) / 127.0f);
    mState.axes[6] = ((inputState.Gamepad.wButtons & ) ? -1.0f : 0.0f)
        + ((inputState.Gamepad.wButtons & ) ? 1.0f : 0.0f);
    mState.axes[7] = ((inputState.Gamepad.wButtons & ) ? -1.0f : 0.0f)
        + ((inputState.Gamepad.wButtons & ) ? 1.0f : 0.0f);

    // Buttons, except the lowest four which are the DPad - we map those to axis 6 and 7 to
    // match the axes reported by the USB interface
    mState.buttons = inputState.Gamepad.wButtons;
  }
  else
  {
    //handle directinput based devices
    DIDEVICEOBJECTDATA diBuff[JOYSTICK_DX_BUFFERSIZE];
    DWORD entries = JOYSTICK_DX_BUFFERSIZE;

    // Poll the device to read the current state
    HRESULT hr = mJoystick->Poll();
    if( hr == DI_OK )
      hr = mJoystick->GetDeviceData( sizeof(DIDEVICEOBJECTDATA), diBuff, &entries, 0 );

    if( hr != DI_OK )
    {
      hr = mJoystick->Acquire();
      while( hr == DIERR_INPUTLOST )
        hr = mJoystick->Acquire();

      // Poll the device to read the current state
      mJoystick->Poll();
      hr = mJoystick->GetDeviceData( sizeof(DIDEVICEOBJECTDATA), diBuff, &entries, 0 );
      //Perhaps the user just tabbed away
      if( FAILED(hr) )
      {
        entries = 0;
        memset( &mState, 0, sizeof( mState));
      }
    }

    // Loop through all the events
    for( size_t i = 0; i < entries; ++i )
    {
      switch( diBuff[i].dwOfs )
      {
        // Max 4 POVs
        case DIJOFS_POV(0):
        case DIJOFS_POV(1):
        case DIJOFS_POV(2):
        case DIJOFS_POV(3):
          break;
        default:
          // handle button events easily using the DX Offset Macros
          if( diBuff[i].dwOfs >= DIJOFS_BUTTON(0) && diBuff[i].dwOfs < DIJOFS_BUTTON(128) )
          {
            size_t btnidx = (diBuff[i].dwOfs - DIJOFS_BUTTON(0));
            mState.buttons = (mState.buttons & (UINT64_MAX ^ (1ull << btnidx))) | (uint64_t((diBuff[i].dwData >> 7) & 1) << btnidx);
          }
          else if( ((diBuff[i].uAppData >> 16)&0xffff) == 0x1313 )
          {
            // if it was nothing else, might be axis enumerated earlier (determined by magic number)
            size_t axis = (0x0000FFFF & diBuff[i].uAppData);
            if( axis < mNumAxes )
              mState.axes[axis] = float( diBuff[i].dwData + MIN_AXIS) / float( MAX_AXIS - MIN_AXIS);
          }

          break;
      } // end case
    } // end for
  } // end if( DirectInput )

  // send events - Axes
  for( size_t i = 0; i < mNumAxes; i++ )
  {
    if( mState.axes[i] != prevAxes[i] )
    {
      mState.diffs[i] = mState.axes[i] - prevAxes[i];
      InputSystemHelper::DoJoystickAxis( this, i, mState.axes[i]);
    }
  }

  // also trigger plus and minus buttons for each axis if analog amplitude is large enough
  for( size_t a = 0; a < mNumAxes; ++a )
  {
    size_t buttonIndex = mNumRealButtons + 2*a;
    mState.buttons &= UINT64_MAX ^ (3 << buttonIndex);
    mState.buttons |= (mState.axes[a] > 0.5f ? 1 : 0) << buttonIndex;
    mState.buttons |= (mState.axes[a] < -0.5f ? 1 : 0) << (buttonIndex+1);
  }

  // send events - buttons
  for( size_t i = 0; i < mNumButtons; i++ )
  {
    if( (mState.buttons ^ mState.prevButtons) & (1ull << i) )
      InputSystemHelper::DoJoystickButton( this, i, (mState.buttons & (1ull << i)) != 0);
  }
}

// --------------------------------------------------------------------------------------------------------------------
void WinJoystick::SetXInput( size_t pXIndex)
{
  mXInputPadIndex = pXIndex;
  mNumRealButtons = XINPUT_TRANSLATED_BUTTON_COUNT;
  mNumAxes = XINPUT_TRANSLATED_AXIS_COUNT;
  mNumButtons = mNumRealButtons + 2*mNumAxes;
}

// --------------------------------------------------------------------------------------------------------------------
int CALLBACK WinJoystick::DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
  WinJoystick* _this = (WinJoystick*) pvRef;

  //Setup mappings
  DIPROPPOINTER diptr;
  diptr.diph.dwSize       = sizeof(DIPROPPOINTER);
  diptr.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  diptr.diph.dwHow        = DIPH_BYID;
  diptr.diph.dwObj        = lpddoi->dwType;
  //Add a magic number to recognise we set seomthing
  diptr.uData             = 0x13130000 | _this->mEnumAxis;

  if( FAILED( _this->mJoystick->SetProperty( DIPROP_APPDATA, &diptr.diph)) )
    // If for some reason we could not set needed user data, just ignore this axis
    return DIENUM_CONTINUE;

  //Increase for next time through
  _this->mEnumAxis++;

  //Set range
  DIPROPRANGE diprg;
  diprg.diph.dwSize       = sizeof(DIPROPRANGE);
  diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  diprg.diph.dwHow        = DIPH_BYID;
  diprg.diph.dwObj        = lpddoi->dwType;
  diprg.lMin              = MIN_AXIS;
  diprg.lMax              = MAX_AXIS;

  if( FAILED( _this->mJoystick->SetProperty( DIPROP_RANGE, &diprg.diph)) )
    throw std::runtime_error( "Controller failed to set min/max range property");

  return DIENUM_CONTINUE;
}

// --------------------------------------------------------------------------------------------------------------------
size_t WinJoystick::GetNumButtons() const
{
  return mNumButtons;
}
// --------------------------------------------------------------------------------------------------------------------
std::string WinJoystick::GetButtonText( size_t idx) const
{
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
size_t WinJoystick::GetNumAxis() const
{
  return mNumAxes;
}
// --------------------------------------------------------------------------------------------------------------------
std::string WinJoystick::GetAxisText( size_t idx) const
{
  return "";
}

// --------------------------------------------------------------------------------------------------------------------
bool WinJoystick::IsButtonDown( size_t idx) const
{
  if( idx < mNumButtons )
    return (mState.buttons & (1ull << idx)) != 0;
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool WinJoystick::WasButtonPressed( size_t idx) const
{
  if( idx < mNumButtons )
    return IsButtonDown( idx) && ((mState.prevButtons & (1ull << idx)) == 0);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool WinJoystick::WasButtonReleased( size_t idx) const
{
  if( idx < mNumButtons )
    return !IsButtonDown( idx) && ((mState.prevButtons & (1ull << idx)) != 0);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
float WinJoystick::GetAxisAbsolute( size_t idx) const
{
  if( idx < mNumAxes )
    return mState.axes[idx];
  else
    return 0.0f;
}
// --------------------------------------------------------------------------------------------------------------------
float WinJoystick::GetAxisDifference( size_t idx) const
{
  if( idx < mNumAxes )
    return mState.diffs[idx];
  else
    return 0.0f;
}

#endif // SNIIS_SYSTEM_WINDOWS
