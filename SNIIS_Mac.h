/// @file SNIIS_Linux.h
/// Linux-specific classes

#pragma once

#include "SNIIS.h"

#if SNIIS_SYSTEM_MAC

#include <IOKit/hid/IOHIDLib.h>
#include <CoreGraphics/CoreGraphics.h>

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

  void Update() override;
  void SetFocus(bool pHasFocus) override;
protected:
  static void HandleNewDeviceCallback( void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
  void HandleNewDevice( IOHIDDeviceRef device);
};

/// -------------------------------------------------------------------------------------------------------------------
/// Describes a control of a HID - it's the same for all kinds of devices on Mac, which makes things confortable
struct MacControl
{
  enum Type { Type_Axis, Type_Hat, Type_Hat_Second, Type_Button };
  Type mType;
  std::string mName;
  IOHIDElementCookie mCookie;
  uint32_t mUsePage, mUsage;
  long mMin, mMax; // only for Type_Axis
};

std::vector<MacControl> EnumerateDeviceControls( IOHIDDeviceRef devref);
void InputElementValueChangeCallback( void* ctx, IOReturn res, void* sender, IOHIDValueRef val);
std::pair<float, float> ConvertHatToAxes( long min, long max, long value);

/// -------------------------------------------------------------------------------------------------------------------
/// common Mac device interface, because they all work on USB HID data
class MacDevice
{
  friend class MacInput;
public:
  MacDevice( MacInput* pSystem, IOHIDDeviceRef pDeviceRef) : mSystem( pSystem), mDevice( pDeviceRef) { }
  /// Starts the update
  virtual void StartUpdate() = 0;
  /// Handles an input event coming from the USB HID callback
  virtual void HandleEvent( IOHIDElementCookie cookie, uint32_t usepage, uint32_t usage, CFIndex value) = 0;
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
  struct State
  {
    float axes[16], prevAxes[16];
    uint32_t buttons, prevButtons;
  } mState;

  std::vector<MacControl> mButtons, mAxes;

public:
  MacMouse( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef);
  ~MacMouse();

  void StartUpdate() override;
  void HandleEvent( IOHIDElementCookie cookie, uint32_t usepage, uint32_t usage, CFIndex value) override;
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

  unsigned long mDeadKeyState; // "Uint32" - lol

public:
  MacKeyboard( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef);

  void StartUpdate() override;
  void HandleEvent( IOHIDElementCookie cookie, uint32_t usepage, uint32_t usage, CFIndex value) override;
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
  std::vector<MacControl> mButtons, mAxes;
  struct State {
    uint64_t buttons, prevButtons;
    float axes[16], diffs[16];
  } mState;

public:
  MacJoystick( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef);

  void StartUpdate() override;
  void HandleEvent( IOHIDElementCookie cookie, uint32_t usepage, uint32_t usage, CFIndex value) override;
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
