/// @file SNIIS_Linux.h
/// Linux-specific classes

#pragma once

#include "SNIIS.h"

#if SNIIS_SYSTEM_MAC

#include <IOKit/hid/IOHIDLib.h>

class MacDevice;
class MacMouse;
class MacKeyboard;
class MacJoystick;

/// -------------------------------------------------------------------------------------------------------------------
/// Mac OSX Input System
class MacInput : public SNIIS::InputSystem
{
  IOHIDManagerRef mHidManager;
  std::vector<MacDevice*> mMacDevices;

public:
  MacInput();
  ~MacInput();

  void StartUpdate() override;
  void EndUpdate() override;
  void SetFocus(bool pHasFocus) override;
protected:
  static void HandleNewDeviceCallback( void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
  void HandleNewDevice( IOHIDDeviceRef device);
};

/// -------------------------------------------------------------------------------------------------------------------
/// common Mac device interface, because they all work on USB HID data
class MacDevice
{
public:
  MacDevice( MacInput* pSystem, IOHIDDeviceRef pDeviceRef) : mSystem( pSystem), mDevice( pDeviceRef) { }
  /// Starts the update
  virtual void StartUpdate() = 0;
  /// Handles an input event coming from the USB HID callback
  virtual void HandleEvent( uint32_t page, uint32_t usage, int value) = 0;
  /// Notifies the input device that the application has lost/gained focus.
  virtual void SetFocus( bool pHasFocus) = 0;
protected:
  MacInput* mSystem;
  IOHIDDeviceRef mDevice;
};

/// -------------------------------------------------------------------------------------------------------------------
/// Mac mouse, fed by USB HID events
class MacMouse : public SNIIS::Mouse, public MacDevice
{
  size_t mNumAxes, mNumButtons;
  struct State
  {
    int absWheel, relWheel;
    float axes[16], prevAxes[16];
    uint32_t buttons, prevButtons;
  } mState;

public:
  MacMouse( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef);

  void StartUpdate() override;
  void HandleEvent( uint32_t page, uint32_t usage, int value) override;
  void EndUpdate();
  void SetFocus( bool pHasFocus) override;

  size_t GetNumButtons() const override;
  std::string GetButtonText( size_t idx) const override;
  size_t GetNumAxes() const override;
  std::string GetAxisText( size_t idx) const override;
  bool IsButtonDown( size_t idx) const override;
  bool WasButtonPressed( size_t idx) const override;
  bool WasButtonReleased( size_t idx) const override;
  float GetAxisAbsolute( size_t idx) const override;
  float GetAxisDifference( size_t idx) const override;
  int GetMouseX() const override;
  int GetMouseY() const override;
  int GetRelMouseX() const override;
  int GetRelMouseY() const override;
};

/// -------------------------------------------------------------------------------------------------------------------
/// OSX keyboard, aswell fed by USB
class MacKeyboard : public SNIIS::Keyboard, public MacDevice
{
  size_t mNumKeys;
  std::vector<uint32_t> mExtraButtons;
  std::vector<uint64_t> mState, mPrevState; ///< current and previous keystate

public:
  MacKeyboard( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef);

  void StartUpdate() override;
  void HandleEvent( uint32_t page, uint32_t usage, int value) override;
  void SetFocus( bool pHasFocus) override;

  size_t GetNumButtons() const override;
  std::string GetButtonText( size_t idx) const override;
  bool IsButtonDown( size_t idx) const override;
  bool WasButtonPressed( size_t idx) const override;
  bool WasButtonReleased( size_t idx) const override;
protected:
  void Set( size_t kc, bool set);
  bool IsSet( size_t kc) const;
  bool WasSet( size_t kc) const;
  size_t TranslateText( size_t kc);
};

/// -------------------------------------------------------------------------------------------------------------------
/// OSX joystick
class MacJoystick : public SNIIS::Joystick, public MacDevice
{
  struct Axis { size_t idx; bool isAbsolute; int32_t min, max, flat; };
  std::vector<Axis> mAxes;
  struct Button { size_t idx; };
  std::vector<Button> mButtons;
  struct State {
    uint64_t buttons, prevButtons;
    float axes[16], diffs[16];
  } mState;

public:
  MacJoystick( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef);

  void StartUpdate() override;
  void HandleEvent( uint32_t page, uint32_t usage, int value) override;
  void SetFocus( bool pHasFocus) override;

  size_t GetNumButtons() const override;
  std::string GetButtonText( size_t idx) const override;
  size_t GetNumAxes() const override;
  std::string GetAxisText( size_t idx) const override;

  bool IsButtonDown( size_t idx) const override;
  bool WasButtonPressed( size_t idx) const override;
  bool WasButtonReleased( size_t idx) const override;
  float GetAxisAbsolute( size_t idx) const override;
  float GetAxisDifference( size_t idx) const override;
};

#endif // SNIIS_SYSTEM_MAC
