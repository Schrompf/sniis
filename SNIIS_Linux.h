/// @file SNIIS_Linux.h
/// Linux-specific classes

#pragma once

#include "SNIIS.h"

#if SNIIS_SYSTEM_LINUX
#include <cstdint>
#include <cassert>

#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>


class LinuxMouse;
class LinuxKeyboard;
class LinuxJoystick;

/// -------------------------------------------------------------------------------------------------------------------
/// Linux Input System
class LinuxInput : public SNIIS::InputSystem
{
  /// Window and Display
	Window mWindow;
  Display* mDisplay;
  /// XInput2 extension opcode
  int mXiOpcode;
  /// Devices by DeviceID
  std::map<int, LinuxMouse*> mMiceById;
  std::map<int, LinuxKeyboard*> mKeyboardsById;

public:
  /// Constructor
  LinuxInput( Window wnd);
  /// Destructor
  ~LinuxInput();

  /// Updates the inputs, to be called before handling system messages
  void Update() override;
  /// Notifies the input system that the application has lost/gained focus.
  void InternSetFocus( bool pHasFocus) override;
  void InternSetMouseGrab( bool enabled) override;

  Display* GetDisplay() const { return mDisplay; }
};

/// -------------------------------------------------------------------------------------------------------------------
/// Linux mouse, fed by XEvents passed in from the outside
class LinuxMouse : public SNIIS::Mouse
{
  LinuxInput* mSystem;
  int mDeviceId;
  struct Button { Atom label; };
  std::vector<Button> mButtons;
  struct Axis { Atom label; double min, max; double value, prevValue; bool isAbsolute; };
  std::vector<Axis> mAxes;
  struct State
  {
    int wheel, prevWheel;
    uint32_t buttons, prevButtons;
  } mState;

public:
  LinuxMouse( LinuxInput* pSystem, size_t pId, const XIDeviceInfo& pDeviceInfo);

  void StartUpdate();
  void HandleEvent( const XIRawEvent& ev);
  void EndUpdate();
  void SetFocus( bool pHasFocus);

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
protected:
  void DoMouseClick( int mouseButton, bool isDown );
};

/// -------------------------------------------------------------------------------------------------------------------
/// Linux keyboard, aswell fed by XEvents
class LinuxKeyboard : public SNIIS::Keyboard
{
  LinuxInput* mSystem;
  int mDeviceId;
  size_t mNumKeys;
  std::vector<uint32_t> mExtraButtons;
	std::vector<uint64_t> mState, mPrevState; ///< current and previous keystate

public:
  LinuxKeyboard(LinuxInput* pSystem, size_t pId, const XIDeviceInfo& pDeviceInfo);

  void StartUpdate();
  void HandleEvent( const XIRawEvent& ev);
  void SetFocus( bool pHasFocus);

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
/// Linux joystick
class LinuxJoystick : public SNIIS::Joystick
{
  LinuxInput* mSystem;
  int mFileDesc;
  struct Axis { size_t idx; bool isAbsolute; int32_t min, max, flat; };
  std::vector<Axis> mAxes;
  struct Button { size_t idx; };
  std::vector<Button> mButtons;
  struct State {
    uint64_t buttons, prevButtons;
    float axes[16], diffs[16];
  } mState;

public:
  LinuxJoystick( LinuxInput* pSystem, size_t pId, int pFileDesc);

  void StartUpdate();
  void SetFocus( bool pHasFocus);

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

#endif // SNIIS_SYSTEM_LINUX
