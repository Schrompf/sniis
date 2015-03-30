/// @file SNIIS_Intern.cpp
/// System-agnostic implementation parts

#include "SNIIS_Intern.h"
#include <algorithm>
#include <cassert>

using namespace SNIIS;

/// global Instance of the Input System if initialized, or Null
namespace SNIIS 
{
  InputSystem* gInstance = nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
InputSystem::InputSystem()
{
  // I don't want to hand around the instance pointer in InputSystemHelper so make it public quickly and retract later
  SNIIS::gInstance = this;

  mFirstMouse = nullptr; mFirstKeyboard = nullptr; mFirstJoystick = nullptr;
  mNumMice = mNumKeyboards = mNumJoysticks = 0;
  mReorderMiceOnActivity = mReorderKeyboardsOnActivity = true;
  mHandler = nullptr;
  mIsInMultiMouseMode = false;

  mKeyRepeatState.lasttick = clock();
  mKeyRepeatState.mKeyCode = KC_UNASSIGNED; mKeyRepeatState.mUnicodeChar = 0;
  mKeyRepeatState.mTimeTillRepeat = 0.0f;
  mKeyRepeatState.mSender = nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
InputSystem::~InputSystem()
{
}

// --------------------------------------------------------------------------------------------------------------------
// Starts the update, to be called before handling system messages
void InputSystem::StartUpdate()
{
  // do the key repeat. yeah.
  auto& krs = gInstance->mKeyRepeatState;
  auto currtime = clock();
  float dt = std::min( 0.1f, std::max( 0.0f, float( currtime - krs.lasttick) / float( CLOCKS_PER_SEC)));
  krs.lasttick = currtime;

  if( krs.mTimeTillRepeat > 0.0f )
  {
    krs.mTimeTillRepeat -= dt;
    if( krs.mTimeTillRepeat <= 0.0f )
    {
      InputSystemHelper::DoKeyboardButton( krs.mSender, krs.mKeyCode, krs.mUnicodeChar, false);
      InputSystemHelper::DoKeyboardButton( krs.mSender, krs.mKeyCode, krs.mUnicodeChar, true);
      krs.mTimeTillRepeat = std::max( 0.00001f, krs.mTimeTillRepeat + gInstance->mKeyRepeatCfg.interval);
    }
  }

  // reset all channel modifications
  for( auto& dch : mDigitalChannels )
    dch.second.Update();
  for( auto& ach : mAnalogChannels )
    ach.second.Update();
}

// --------------------------------------------------------------------------------------------------------------------
// Ends the update, to be called after handling system messages
void InputSystem::EndUpdate()
{
}

// --------------------------------------------------------------------------------------------------------------------
// Gets the nth device of that specific kind
Mouse* InputSystem::GetMouseByCount(size_t pNumber) const
{
  for( auto d : mDevices )
  {
    if( auto m = dynamic_cast<Mouse*> (d) )
    {
      if( pNumber == 0 )
        return m;
      --pNumber;
    }
  }
  return nullptr;
}
// --------------------------------------------------------------------------------------------------------------------
Keyboard* InputSystem::GetKeyboardByCount(size_t pNumber) const
{
  for( auto d : mDevices )
  {
    if( auto k = dynamic_cast<Keyboard*> (d) )
    {
      if( pNumber == 0 )
        return k;
      --pNumber;
    }
  }
  return nullptr;
}
// --------------------------------------------------------------------------------------------------------------------
Joystick* InputSystem::GetJoystickByCount(size_t pNumber) const
{
  for( auto d : mDevices )
  {
    if( auto j = dynamic_cast<Joystick*> (d) )
    {
      if( pNumber == 0 )
        return j;
      --pNumber;
    }
  }
  return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
DigitalChannel& InputSystem::GetDigital(size_t id)
{
  auto it = mDigitalChannels.find( id);
  if( it != mDigitalChannels.end() )
    return it->second;
  // not found -> create new channel
  it = mDigitalChannels.insert( std::make_pair( id, DigitalChannel())).first;
  it->second.mId = id;
  return it->second;
}
// --------------------------------------------------------------------------------------------------------------------
AnalogChannel& InputSystem::GetAnalog(size_t id)
{
  auto it = mAnalogChannels.find( id);
  if( it != mAnalogChannels.end() )
    return it->second;
  // not found -> create new channel
  it = mAnalogChannels.insert( std::make_pair( id, AnalogChannel())).first;
  it->second.mId = id;
  return it->second;
}
// --------------------------------------------------------------------------------------------------------------------
std::vector<size_t> InputSystem::GetDigitalIds() const
{
  std::vector<size_t> ids;
  ids.reserve( mDigitalChannels.size());
  for( const auto& p : mDigitalChannels )
    ids.push_back( p.first);
  return ids;
}
// --------------------------------------------------------------------------------------------------------------------
std::vector<size_t> InputSystem::GetAnalogIds() const
{
  std::vector<size_t> ids;
  ids.reserve( mAnalogChannels.size());
  for( const auto& p : mAnalogChannels )
    ids.push_back( p.first);
  return ids;
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystem::ClearChannelAssignments()
{
  // disable all digital channels
  for( auto& p : mDigitalChannels )
  {
    if( p.second.mIsPressed )
    {
      p.second.mIsPressed = false; p.second.mIsModified = true;
      if( gInstance->mHandler )
        gInstance->mHandler->OnDigitalChannel( p.second);
    }
    p.second.mEvents.clear();
  }

  // and all analog channels
  for( auto& p : mAnalogChannels )
  {
    if( p.second.mValue != 0.0f )
    {
      p.second.mDiff = -p.second.mValue; p.second.mValue = 0.0f;
      if( gInstance->mHandler )
        gInstance->mHandler->OnAnalogChannel( p.second);
    }
    p.second.mEvents.clear();
  }
}

// ********************************************************************************************************************
// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::AddDevice( Device* dev)
{
  gInstance->mDevices.push_back( dev);
  if( auto m = dynamic_cast<Mouse*> (dev) )
  {
    dev->mCount = gInstance->mNumMice++;
    if( !gInstance->mFirstMouse )
      gInstance->mFirstMouse = m;
  }

  if( auto k = dynamic_cast<Keyboard*> (dev) )
  {
    dev->mCount = gInstance->mNumKeyboards++;
    if( !gInstance->mFirstKeyboard )
      gInstance->mFirstKeyboard = k;
  }

  if( auto j = dynamic_cast<Joystick*> (dev) )
  {
    dev->mCount = gInstance->mNumJoysticks++;
    if( !gInstance->mFirstJoystick )
      gInstance->mFirstJoystick = j;
  }
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoMouseButton( Mouse* sender, size_t btnIndex, bool isPressed)
{
  // swap mouse to front if this mouse was used first
  MakeThisMouseFirst( sender);

  if( !gInstance->mHandler )
    return;
  if( gInstance->mHandler->OnMouseButton( sender, btnIndex, isPressed) ) 
    return;
  DoDigitalEvent( sender, btnIndex, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoMouseMove( Mouse* sender, int absx, int absy, int relx, int rely)
{
  // swap mouse to front if this mouse was used first
  MakeThisMouseFirst( sender);

  if( !gInstance->mHandler )
    return;
  if( gInstance->mHandler->OnMouseMoved( sender, absx, absy) )
    return;
  if( relx != 0 )
    DoAnalogEvent( sender, 0, float( absx));
  if( rely != 0 )
    DoAnalogEvent( sender, 1, float( absy));
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoKeyboardButton( Keyboard* sender, KeyCode kc, size_t unicode, bool isPressed)
{
  if( !gInstance->mHandler )
    return;
  if( gInstance->mHandler->OnKey( sender, kc, isPressed) ) 
    return;
  if( isPressed && unicode && gInstance->mHandler->OnUnicode( sender, unicode) )
    return;
  DoDigitalEvent( sender, (size_t) kc, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoJoystickAxis( Joystick* sender, size_t axisIndex, float value)
{
  if( !gInstance->mHandler )
    return;
  if( gInstance->mHandler->OnJoystickAxis( sender, axisIndex, value) ) 
    return;
  DoAnalogEvent( sender, axisIndex, value);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoJoystickButton( Joystick* sender, size_t btnIndex, bool isPressed)
{
  if( !gInstance->mHandler )
    return;
  if( gInstance->mHandler->OnJoystickButton( sender, btnIndex, isPressed) ) 
    return;
  DoDigitalEvent( sender, btnIndex, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoDigitalEvent( Device* sender, size_t btnIndex, bool isPressed)
{
  auto evs = EventSource{ sender->GetId(), btnIndex };
  if( gInstance->mHandler->OnDigitalEvent( evs, isPressed) ) 
    return;

  for( auto& ch : gInstance->mDigitalChannels )
  {
    auto& dch = ch.second;
    if( std::find( dch.mEvents.begin(), dch.mEvents.end(), evs) != dch.mEvents.end() )
    {
      // derive new state of that channel
      bool wasPressed = dch.mIsPressed;
      dch.mIsPressed = false;
      for( auto e : dch.mEvents )
        dch.mIsPressed = dch.mIsPressed || gInstance->mDevices[e.deviceId]->IsButtonDown( e.ctrlId);
      dch.mIsModified = (dch.mIsPressed != wasPressed);
      if( dch.mIsModified )
        gInstance->mHandler->OnDigitalChannel( dch);
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoAnalogEvent( Device* sender, size_t axisIndex, float value)
{
  auto evs = EventSource{ sender->GetId(), axisIndex };
  if( gInstance->mHandler->OnAnalogEvent( evs, value) ) 
    return;

  for( auto& ch : gInstance->mAnalogChannels )
  {
    auto& ach = ch.second;
    if( std::find( ach.mEvents.begin(), ach.mEvents.end(), evs) != ach.mEvents.end() )
    {
      // accumulate new state of that channel
      float prevValue = ach.mValue;
      ach.mValue = 0.0f;
      for( auto e : ach.mEvents )
        ach.mValue += gInstance->mDevices[e.deviceId]->GetAxisAbsolute( e.ctrlId);
//      ach.mValue = std::min( 1.0f, std::max( -1.0f, ach.mValue));
      ach.mDiff = ach.mValue - prevValue;
      if( ach.mDiff != 0.0f )
        gInstance->mHandler->OnAnalogChannel( ach);
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::MakeThisMouseFirst( Mouse* mouse)
{
  if( gInstance->mReorderMiceOnActivity && mouse != gInstance->mFirstMouse )
  {
    gInstance->mReorderMiceOnActivity = false;
    std::swap( mouse->mId, gInstance->mFirstMouse->mId);
    std::swap( mouse->mCount, gInstance->mFirstMouse->mCount);
    gInstance->mFirstMouse = mouse;
    auto fit = std::find( gInstance->mDevices.begin(), gInstance->mDevices.end(), static_cast<Device*> (gInstance->mFirstMouse));
    auto mit = std::find( gInstance->mDevices.begin(), gInstance->mDevices.end(), static_cast<Device*> (mouse));
    assert( fit != gInstance->mDevices.end() && mit != gInstance->mDevices.end());
    std::swap( *fit, *mit);
    // the sender mouse should now be the first mouse by any means, all channel mappings should adapt automatically
  }
}
// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::MakeThisKeyboardFirst( Keyboard* keyboard)
{
  if( gInstance->mReorderKeyboardsOnActivity && keyboard != gInstance->mFirstKeyboard )
  {
    gInstance->mReorderKeyboardsOnActivity = false;
    std::swap( keyboard->mId, gInstance->mFirstKeyboard->mId);
    std::swap( keyboard->mCount, gInstance->mFirstKeyboard->mCount);
    gInstance->mFirstKeyboard = keyboard;
    auto fit = std::find( gInstance->mDevices.begin(), gInstance->mDevices.end(), static_cast<Device*> (gInstance->mFirstKeyboard));
    auto mit = std::find( gInstance->mDevices.begin(), gInstance->mDevices.end(), static_cast<Device*> (keyboard));
    assert( fit != gInstance->mDevices.end() && mit != gInstance->mDevices.end());
    std::swap( *fit, *mit);
  }
}
