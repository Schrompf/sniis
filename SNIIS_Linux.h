/// @file SNIIS_Linux.h
/// Linux-specific classes

#pragma once

#include "SNIIS.h"

#if SNIIS_SYSTEM_LINUX
#include <cstdint>
#include <cassert>

#include <unistd.h>
#include <X11/Xlib.h>


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

public:
  /// Constructor
  LinuxInput( Window wnd);
  /// Destructor
  ~LinuxInput();

  /// Starts the update, to be called before handling system messages
  void StartUpdate() override;
  /// Handles an XEvent - pass a pointer to the XEvent structure read by XNextEvent()
  void HandleXEvent( void* xevent) override;
  /// Ends the update, to be called after handling system messages
  void EndUpdate() override;

  HWND GetWindowHandle() const { return hWnd; }
};

/// -------------------------------------------------------------------------------------------------------------------
/// Linux mouse, fed by XEvents passed in from the outside
class LinuxMouse : public SNIIS::Mouse
{
  LinuxInput* mSystem;
  struct State 
  {
    int absX, absY, absWheel, relX, relY, relWheel;
    uint32_t buttons, prevButtons;
  } mState;

public:
  LinuxMouse( WinInput* pSystem, size_t pId);

  void StartUpdate();
  void HandleXEvent( const XEvent& ev) override;
  void EndUpdate();

  size_t GetNumButtons() const override;
  std::string GetButtonText( size_t idx) const override;
  size_t GetNumAxis() const override;
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
  static const size_t NumKeys = 256;
  LinuxInput* mSystem;
	uint64_t mState[NumKeys/64], mPrevState[NumKeys/64]; ///< current and previous keystate

  /// constant table to map linux key codes to our key codes
  static const SNIIS::KeyCode sKeyTable[256];

public:
  LinuxKeyboard( WinInput* pSystem, size_t pId);

  void StartUpdate();
  void HandleXEvent( const XEvent& ev) override;
  void EndUpdate();

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
  WinInput* mSystem;
  int mDeviceId;
  size_t mNumRealButtons, mNumButtons, mNumAxes;
  struct State {
    uint64_t buttons, prevButtons;
    float axes[16], diffs[16];
  } mState;

public:
  LinuxJoystick( WinInput* pSystem, size_t pId, int pDeviceId);

  void StartUpdate();

  size_t GetNumButtons() const override;
  std::string GetButtonText( size_t idx) const override;
  size_t GetNumAxis() const override;
  std::string GetAxisText( size_t idx) const override;

  bool IsButtonDown( size_t idx) const override;
  bool WasButtonPressed( size_t idx) const override;
  bool WasButtonReleased( size_t idx) const override;
  float GetAxisAbsolute( size_t idx) const override;
  float GetAxisDifference( size_t idx) const override;
};

#endif // SNIIS_SYSTEM_LINUX