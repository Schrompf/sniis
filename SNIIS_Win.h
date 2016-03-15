/// @file SNIIS_Win.h
/// Windows-specific classes

#pragma once

#include "SNIIS.h"

#if SNIIS_SYSTEM_WINDOWS
#include <cstdint>
#include <cassert>
#include <map>

#define NOMINMAX
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIRECTINPUT_VERSION DIRECTINPUT_HEADER_VERSION
#include <dinput.h>
#include <wbemidl.h>
#include <oleauto.h>

class WinMouse;
class WinKeyboard;
class WinJoystick;

/// -------------------------------------------------------------------------------------------------------------------
/// Windows Input System
class WinInput : public SNIIS::InputSystem
{
  /// The window handle we are using
	HWND hWnd;

  /// The old WndProc before we intercepted it
  LONG_PTR mPreviousWndProc;

  /// workaround for buggy RawInput implementation: true if our process runs translated via WoW64 (Windows on Windows)
  bool mIsWorkAroundEnabled;

	/// Direct Input Interface for the devices being handled by DI
	IDirectInput8* mDirectInput;
  IDirectInputDevice8* mKeyboard;

  /// Devices associated with Handles, to map RawInput events to devices 
  std::map<HANDLE, WinMouse*> mMice;
  std::map<HANDLE, WinKeyboard*> mKeyboards;

public:
  /// Constructor
  WinInput( HWND wnd);
  /// Destructor
  ~WinInput();

  void EnumerateDevices();
  static int CALLBACK _DIEnumDevCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);
  void CheckXInputDevices();
  void RegisterForRawInput();

  void Update() override;

  static LRESULT CALLBACK WndProcHook( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  void InternSetFocus( bool pHasFocus) override;
  void InternSetMouseGrab( bool enabled) override;

  HWND GetWindowHandle() const { return hWnd; }
  bool IsWorkAroundEnabled() const { return mIsWorkAroundEnabled; }
};

/// -------------------------------------------------------------------------------------------------------------------
/// Win mouse uses both RawInput and Windows Messages
class WinMouse : public SNIIS::Mouse
{
  WinInput* mSystem;
  HANDLE mHandle;
  struct State 
  {
    float absX, absY, relX, relY;
    float wheel, prevWheel;
    uint32_t buttons, prevButtons;
  } mState;
  bool mIsInUpdate;
  float mOutOfUpdateRelX, mOutOfUpdateRelY;

public:
  WinMouse( WinInput* pSystem, size_t pId, HANDLE pHandle);

  void StartUpdate();
  void ParseMessage( const RAWINPUT& e, bool useWorkaround);
  void EndUpdate();
  void SetFocus(bool pHasFocus);

  size_t GetNumButtons() const override;
  std::string GetButtonText( size_t idx) const override;
  size_t GetNumAxes() const override;
  std::string GetAxisText( size_t idx) const override;
  bool IsButtonDown( size_t idx) const override;
  bool WasButtonPressed( size_t idx) const override;
  bool WasButtonReleased( size_t idx) const override;
  float GetAxisAbsolute( size_t idx) const override;
  float GetAxisDifference( size_t idx) const override;
  float GetMouseX() const override;
  float GetMouseY() const override;
  float GetRelMouseX() const override;
  float GetRelMouseY() const override;

private:
  void DoMouseWheel( float wheel);
  void DoMouseButton( size_t btnIndex, bool isPressed);
};

/// -------------------------------------------------------------------------------------------------------------------
/// Win keyboard uses both RawInput and Windows Messages, too
class WinKeyboard : public SNIIS::Keyboard
{
  static const size_t NumKeys = 256;
  WinInput* mSystem;
  HANDLE mHandle;
	IDirectInputDevice8* mKeyboard; ///< for retrieving the names of keys
	uint64_t mState[NumKeys/64], mPrevState[NumKeys/64]; ///< current and previous keystate
	WCHAR deadKey; ///< Stored dead key from last translation

public:
  WinKeyboard( WinInput* pSystem, size_t pId, HANDLE pHandle, IDirectInputDevice8* pDirectInputKeyboard);

  void StartUpdate();
  void ParseMessage( const RAWINPUT& e, bool useWorkaround);
  void SetFocus( bool pHasFocus);

  size_t GetNumButtons() const override;
  std::string GetButtonText( size_t idx) const override;
  bool IsButtonDown( size_t idx) const override;
  bool WasButtonPressed( size_t idx) const override;
  bool WasButtonReleased( size_t idx) const override;
protected:
  void DoKeyboardButton( SNIIS::KeyCode kc, size_t unicode, bool isPressed);
  void Set( size_t kc, bool set);
  bool IsSet( size_t kc) const;
  bool WasSet( size_t kc) const;
  size_t TranslateText( size_t kc);
};

/// -------------------------------------------------------------------------------------------------------------------
/// Win Joystick uses either XInput or DirectInput
class WinJoystick : public SNIIS::Joystick
{
  WinInput* mSystem;
	IDirectInput8* mDirectInput;
	IDirectInputDevice8* mJoystick;
  DIDEVCAPS mDIJoyCaps;
  GUID mGuidInstance, mGuidProduct;
  size_t mXInputPadIndex;
  size_t mNumButtons, mNumAxes;
  struct State {
    uint64_t buttons, prevButtons;
    float axes[16], diffs[16];
  } mState;
  size_t mEnumAxis;

public:
  WinJoystick( WinInput* pSystem, size_t pId, IDirectInput8* pDI, GUID pGuidInstance, GUID pGuidProduct);

  void StartUpdate();
  void SetFocus( bool pHasFocus);

  bool IsXInput() const { return mXInputPadIndex != SIZE_MAX; }
  GUID GetProductGuid() const { return mGuidProduct; }
  void SetXInput( size_t pXIndex);

  size_t GetNumButtons() const override;
  std::string GetButtonText( size_t idx) const override;
  size_t GetNumAxes() const override;
  std::string GetAxisText( size_t idx) const override;

  bool IsButtonDown( size_t idx) const override;
  bool WasButtonPressed( size_t idx) const override;
  bool WasButtonReleased( size_t idx) const override;
  float GetAxisAbsolute( size_t idx) const override;
  float GetAxisDifference( size_t idx) const override;
protected:
	static int CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);
};

#endif // SNIIS_SYSTEM_WINDOWS