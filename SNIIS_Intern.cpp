/// @file SNIIS_Intern.cpp
/// System-agnostic implementation parts

#include "SNIIS_Intern.h"
#include <algorithm>
#include <cassert>

#include "../Traumklassen/TraumBasis.h"

using namespace SNIIS;

namespace SNIIS
{
  /// global Instance of the Input System if initialized, or Null
  InputSystem* gInstance = nullptr;

  /// globol log collbock
  LogCallback gLogCallback = nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
InputSystem::InputSystem()
{
  // I don't want to hand around the instance pointer in InputSystemHelper so make it public quickly and retract later
  SNIIS::gInstance = this;
  Log( "SNIIS instance created.");

  mFirstMouse = nullptr; mFirstKeyboard = nullptr; mFirstJoystick = nullptr;
  mNumMice = mNumKeyboards = mNumJoysticks = 0;
  mReorderMiceOnActivity = mReorderKeyboardsOnActivity = true;
  mHandler = nullptr;
  mHasFocus = true;
  mIsInMultiMouseMode = false;
  mIsMouseGrabEnabled = mIsMouseGrabbed = false;

  mKeyRepeatState.lasttick = clock();
  mKeyRepeatState.mKeyCode = KC_UNASSIGNED; mKeyRepeatState.mUnicodeChar = 0;
  mKeyRepeatState.mTimeTillRepeat = 0.0f;
  mKeyRepeatState.mSender = nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
InputSystem::~InputSystem()
{
  Log("SNIIS instance going down.");
}

// --------------------------------------------------------------------------------------------------------------------
// Updates the input system, to be called before handling system messages
void InputSystem::Update()
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
      InputSystemHelper::DoKeyboardButtonIntern( krs.mSender, krs.mKeyCode, krs.mUnicodeChar, false);
      InputSystemHelper::DoKeyboardButtonIntern( krs.mSender, krs.mKeyCode, krs.mUnicodeChar, true);
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
// Notifies the input system that the application has lost/gained focus.
void InputSystem::SetFocus( bool pHasFocus)
{
  if( pHasFocus == mHasFocus )
    return;
  Log( "SNIIS: %s focus", pHasFocus ? "got" : "lost");
  mHasFocus = pHasFocus;
  InternSetFocus( pHasFocus);
  InternGrabMouseIfNecessary();
}

// --------------------------------------------------------------------------------------------------------------------
// Enables or disables multi-mice mode.
void InputSystem::SetMultiDeviceMode( bool enabled)
{
  if( enabled == mIsInMultiMouseMode )
    return;
  Log("SNIIS: %s multi mouse mode", enabled ? "enabled" : "disabled");
  mIsInMultiMouseMode = enabled;
  InternGrabMouseIfNecessary();
}

// --------------------------------------------------------------------------------------------------------------------
// Enables or disables Mouse Grabbing.
void InputSystem::SetMouseGrab( bool enabled)
{
  if( enabled == mIsMouseGrabEnabled )
    return;
  Log("SNIIS: %s mouse grab", enabled ? "enabled" : "disabled");
  mIsMouseGrabEnabled = enabled;
  InternGrabMouseIfNecessary();
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystem::InternGrabMouseIfNecessary()
{
  bool necessary = mIsMouseGrabEnabled && mHasFocus && !mIsInMultiMouseMode;
  if( necessary == mIsMouseGrabbed )
    return;
  Log("SNIIS: %s mouse", necessary ? "grabbing" : "releasing");
  mIsMouseGrabbed = necessary;
  InternSetMouseGrab( necessary);
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
    p.second.mSources.clear();
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
    p.second.mSources.clear();
  }
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystem::Log(const char* msg, ...)
{
  if( !gLogCallback )
    return;

  static const size_t TEMPSIZE = 1024;
  va_list args;

  // extend to text
  va_start(args, msg);
  char temp[TEMPSIZE];
  int z = vsnprintf( temp, TEMPSIZE - 1, msg, args);
  va_end(args);

  // terminate with zero if not done yet
  if( z >= 0 )
  {
    z = std::min(z, int(TEMPSIZE - 1));
    temp[z] = 0;
  }

  // send to callback
  gLogCallback( temp);
}

// ********************************************************************************************************************
// --------------------------------------------------------------------------------------------------------------------
void DigitalChannel::AddDigitalSource( size_t pDeviceId, size_t pButtonId)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pButtonId && !s.mIsAnalog; });
  if( it != mSources.end() )
    return;
  mSources.push_back( DigitalChannel::Source{ pDeviceId, pButtonId, false, 0.0f });
}

// --------------------------------------------------------------------------------------------------------------------
void DigitalChannel::AddAnalogSource( size_t pDeviceId, size_t pAxisId, float pLimit)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pAxisId && s.mIsAnalog; });
  if( it != mSources.end() )
    return;
  mSources.push_back( DigitalChannel::Source{ pDeviceId, pAxisId, true, pLimit });
}

// --------------------------------------------------------------------------------------------------------------------
void DigitalChannel::RemoveDigitalSource( size_t pDeviceId, size_t pButtonId)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pButtonId && !s.mIsAnalog; });
  if( it != mSources.end() )
    mSources.erase( it);
}

// --------------------------------------------------------------------------------------------------------------------
void DigitalChannel::RemoveAnalogSource( size_t pDeviceId, size_t pAxisId)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pAxisId && s.mIsAnalog; });
  if( it != mSources.end() )
    mSources.erase( it);
}

// --------------------------------------------------------------------------------------------------------------------
void DigitalChannel::ClearAllAssignments()
{
  mSources.clear();
}

// ********************************************************************************************************************
// --------------------------------------------------------------------------------------------------------------------
void AnalogChannel::AddAnalogSource( size_t pDeviceId, size_t pAxisId)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pAxisId && s.mType == Source_Analog; });
  if( it != mSources.end() )
    return;
  mSources.push_back( AnalogChannel::Source{ pDeviceId, pAxisId, Source_Analog, 0.0f, 0.0f });
}

// --------------------------------------------------------------------------------------------------------------------
void AnalogChannel::AddDigitalSource( size_t pDeviceId, size_t pButtonId, float pTranslatedValue)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pButtonId && s.mType == Source_Digital; });
  if( it != mSources.end() )
    return;
  mSources.push_back( AnalogChannel::Source{ pDeviceId, pButtonId, Source_Digital, pTranslatedValue, 0.0f });
}

// --------------------------------------------------------------------------------------------------------------------
void AnalogChannel::AddDigitalizedAnalogSource(size_t pDeviceId, size_t pAxisId, float pScale, float pLimitValue)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pAxisId && s.mType == Source_LimitedAnalog; });
  if( it != mSources.end() )
    return;
  mSources.push_back( AnalogChannel::Source{ pDeviceId, pAxisId, Source_LimitedAnalog, pLimitValue, pScale });
}

// --------------------------------------------------------------------------------------------------------------------
void AnalogChannel::RemoveAnalogSource( size_t pDeviceId, size_t pAxisId)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pAxisId && s.mType == Source_Analog; });
  if( it != mSources.end() )
    mSources.erase( it);
}

// --------------------------------------------------------------------------------------------------------------------
void AnalogChannel::RemoveDigitalSource( size_t pDeviceId, size_t pButtonId)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pButtonId && s.mType == Source_Digital; });
  if( it != mSources.end() )
    mSources.erase( it);
}

// --------------------------------------------------------------------------------------------------------------------
void AnalogChannel::RemoveDigitalizedAnalogSource( size_t pDeviceId, size_t pAxisId)
{
  auto it = std::find_if( mSources.begin(), mSources.end(),
    [=](const Source& s) { return s.mDeviceId == pDeviceId && s.mControlId == pAxisId && s.mType == Source_LimitedAnalog; });
  if( it != mSources.end() )
    mSources.erase( it);
}

// --------------------------------------------------------------------------------------------------------------------
void AnalogChannel::ClearAllAssignments()
{
  mSources.clear();
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
  if( gInstance->mHandler )
    if( gInstance->mHandler->OnMouseButton( sender, btnIndex, isPressed) )
      return;

  DoDigitalEvent( sender, btnIndex, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoMouseMove( Mouse* sender, float absx, float absy, float relx, float rely)
{
  if( gInstance->mHandler )
    if( gInstance->mHandler->OnMouseMoved( sender, absx, absy) )
      return;

  if( relx != 0 )
    DoAnalogEvent( sender, 0, float( absx));
  if( rely != 0 )
    DoAnalogEvent( sender, 1, float( absy));
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoMouseWheel( Mouse* sender, float diff)
{
  if( gInstance->mHandler )
    if( gInstance->mHandler->OnMouseWheel( sender, diff) )
      return;
  DoAnalogEvent( sender, 2, diff);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoKeyboardButton( Keyboard* sender, KeyCode kc, size_t unicode, bool isPressed)
{
  // store for key repetition
  if( isPressed && gInstance->mKeyRepeatCfg.enable )
  {
    gInstance->mKeyRepeatState.mKeyCode = kc;
    gInstance->mKeyRepeatState.mSender = sender;
    gInstance->mKeyRepeatState.mUnicodeChar = unicode;
    gInstance->mKeyRepeatState.mTimeTillRepeat = gInstance->mKeyRepeatCfg.delay;
  }
  else if( !isPressed && gInstance->mKeyRepeatState.mKeyCode == kc )
  {
    gInstance->mKeyRepeatState.mKeyCode = KC_UNASSIGNED;
    gInstance->mKeyRepeatState.mSender = nullptr;
    gInstance->mKeyRepeatState.mUnicodeChar = 0;
    gInstance->mKeyRepeatState.mTimeTillRepeat = 0.0f;
  }

  // and execute
  DoKeyboardButtonIntern( sender, kc, unicode, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoKeyboardButtonIntern( Keyboard* sender, KeyCode kc, size_t unicode, bool isPressed)
{
  if( gInstance->mHandler )
  {
    if( gInstance->mHandler->OnKey( sender, kc, isPressed) )
      return;
    if( isPressed && unicode && gInstance->mHandler->OnUnicode( sender, unicode) )
      return;
  }

  DoDigitalEvent( sender, (size_t) kc, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoJoystickAxis( Joystick* sender, size_t axisIndex, float value)
{
  if( gInstance->mHandler )
    if( gInstance->mHandler->OnJoystickAxis( sender, axisIndex, value) )
      return;

  DoAnalogEvent( sender, axisIndex, value);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoJoystickButton( Joystick* sender, size_t btnIndex, bool isPressed)
{
  if( gInstance->mHandler )
    if( gInstance->mHandler->OnJoystickButton( sender, btnIndex, isPressed) )
      return;

  DoDigitalEvent( sender, btnIndex, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::UpdateChannels( Device* sender, size_t ctrlIndex, bool isAnalog)
{
  // update digital channels using this as a source
  for( auto& ch : gInstance->mDigitalChannels )
  {
    auto& dch = ch.second;
    auto it = std::find_if( dch.mSources.begin(), dch.mSources.end(),
      [=](const DigitalChannel::Source& s) { return s.mDeviceId == sender->GetId() && s.mControlId == ctrlIndex && s.mIsAnalog == isAnalog; });

    if( it != dch.mSources.end() )
    {
      // derive new state of that channel by combining the states of all sources
      bool wasPressed = dch.mIsPressed;
      dch.mIsPressed = false;
      for( const auto& s : dch.mSources )
      {
        auto dev = s.mDeviceId < gInstance->mDevices.size() ? gInstance->mDevices[s.mDeviceId] : nullptr;
        if( !dev )
          continue;
        if( s.mIsAnalog )
        {
          float v = dev->GetAxisAbsolute( s.mControlId);
          dch.mIsPressed = (s.mAnalogLimit < 0.0f ? (v < s.mAnalogLimit) : (v > s.mAnalogLimit));
        } else
        {
          dch.mIsPressed = dch.mIsPressed || dev->IsButtonDown( s.mControlId);
        }
      }

      dch.mIsModified = (dch.mIsPressed != wasPressed);
      if( gInstance->mHandler && dch.mIsModified )
        gInstance->mHandler->OnDigitalChannel( dch);
    }
  }

  // and update analog channels using this as an input
  for( auto& ch : gInstance->mAnalogChannels )
  {
    // style guides, take cover. It's getting ugly.
    auto& ach = ch.second;
    auto it = std::find_if( ach.mSources.begin(), ach.mSources.end(),
      [=](const AnalogChannel::Source& s) { return s.mDeviceId == sender->GetId() && s.mControlId == ctrlIndex
          && ((isAnalog && (s.mType == AnalogChannel::Source_Analog || s.mType == AnalogChannel::Source_LimitedAnalog))
              || (!isAnalog && s.mType == AnalogChannel::Source_Digital)); });

    if( it != ach.mSources.end() )
    {
      // accumulate new state of that channel
      float prevValue = ach.mValue;
      ach.mValue = 0.0f;
      for( const auto& s : ach.mSources )
      {
        auto dev = s.mDeviceId < gInstance->mDevices.size() ? gInstance->mDevices[s.mDeviceId] : nullptr;
        if( !dev )
          continue;
        switch( s.mType )
        {
          case AnalogChannel::Source_Digital:
            ach.mValue += dev->IsButtonDown( s.mControlId) ? s.mDigitalAmountOrAnalogLimit : 0.0f;
            break;
          case AnalogChannel::Source_Analog:
            ach.mValue += dev->GetAxisAbsolute( s.mControlId);
            break;
          case AnalogChannel::Source_LimitedAnalog:
          {
            float v = dev->GetAxisAbsolute( s.mControlId);
            float l = s.mDigitalAmountOrAnalogLimit;
            bool isActive = (l != 0.0f ? (l < 0.0f ? (v < l) : (v > l)) : true);
            ach.mValue += isActive ? v * s.mAnalogScale : 0.0f;
            break;
          }
        }
      }
      ach.mDiff += ach.mValue - prevValue;
      if( gInstance->mHandler && ach.mValue != prevValue )
        gInstance->mHandler->OnAnalogChannel( ach);
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoDigitalEvent( Device* sender, size_t btnIndex, bool isPressed)
{
  if( gInstance->mHandler )
    if( gInstance->mHandler->OnDigitalEvent( sender, btnIndex, isPressed) )
      return;

  UpdateChannels( sender, btnIndex, false);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::DoAnalogEvent( Device* sender, size_t axisIndex, float value)
{
  if( gInstance->mHandler )
    if( gInstance->mHandler->OnAnalogEvent( sender, axisIndex, value) )
      return;

  UpdateChannels( sender, axisIndex, true);
}

// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::SortThisMouseToFront( Mouse* mouse)
{
  if( !mouse->mWasSortedOnActivity )
  {
    mouse->mWasSortedOnActivity = true;
    // find the first mouse in sequence which has not yet shown activity.
    Device* frontmouse = nullptr;
    for( size_t c = 0; c < mouse->GetCount(); ++c )
      if( gInstance->GetMouseByCount( c)->mWasSortedOnActivity == false )
        frontmouse = gInstance->GetMouseByCount( c);

    // if we found a (yet) silent mouse in front of our active mouse, swap the two
    if( frontmouse )
    {
      gInstance->Log( "Swap mice due to activity: %d,%d and %d,%d", mouse->mId, mouse->mCount, frontmouse->mId, frontmouse->mCount);

      std::swap( mouse->mId, frontmouse->mId);
      std::swap( mouse->mCount, frontmouse->mCount);
      auto fit = std::find( gInstance->mDevices.begin(), gInstance->mDevices.end(), static_cast<Device*> (frontmouse));
      auto mit = std::find( gInstance->mDevices.begin(), gInstance->mDevices.end(), static_cast<Device*> (mouse));
      assert( fit != gInstance->mDevices.end() && mit != gInstance->mDevices.end());
      std::swap( *fit, *mit);

      // if the new front mouse is the primary mouse now, also store it like this
      if( frontmouse->GetCount() == 0 )
        gInstance->mFirstMouse = dynamic_cast<Mouse*> (frontmouse);
      assert( gInstance->mFirstMouse != nullptr );

      // all channel mappings should adapt automatically
    }
  }
}
// --------------------------------------------------------------------------------------------------------------------
void InputSystemHelper::SortThisKeyboardToFront( Keyboard* keyboard)
{
  if( !keyboard->mWasSortedOnActivity )
  {
    keyboard->mWasSortedOnActivity = true;
    // find the first mouse in sequence which has not yet shown activity.
    Device* frontkeyboard = nullptr;
    for( size_t c = 0; c < keyboard->GetCount(); ++c )
      if( gInstance->GetKeyboardByCount( c)->mWasSortedOnActivity == false )
        frontkeyboard = gInstance->GetKeyboardByCount( c);

    // if we found a (yet) silent mouse in front of our active mouse, swap the two
    if( frontkeyboard )
    {
      gInstance->Log( "Swap mice due to activity: %d,%d and %d,%d", keyboard->mId, keyboard->mCount, frontkeyboard->mId, frontkeyboard->mCount);

      std::swap( keyboard->mId, frontkeyboard->mId);
      std::swap( keyboard->mCount, frontkeyboard->mCount);
      auto fit = std::find( gInstance->mDevices.begin(), gInstance->mDevices.end(), static_cast<Device*> (frontkeyboard));
      auto kit = std::find( gInstance->mDevices.begin(), gInstance->mDevices.end(), static_cast<Device*> (keyboard));
      assert( fit != gInstance->mDevices.end() && kit != gInstance->mDevices.end());
      std::swap( *fit, *kit);

      // if the new front keyboard is the primary keyboard now, also store it like this
      if( frontkeyboard->GetCount() == 0 )
        gInstance->mFirstKeyboard = dynamic_cast<Keyboard*> (frontkeyboard);
      assert( gInstance->mFirstKeyboard != nullptr );

      // all channel mappings should adapt automatically
    }
  }
}
