/** @file SNIIS.h
 * C++ interface to SNIIS. Include this.
 * SNIIS stands for Simple NonIntrusive Input System. The bad word joke is fully intended. It's heavily based on
 * Object-oriented Input System (OIS), but without the RegisterAbstractFactoryHandlerPattern crust. It also expects
 * YOU to provide the required system messages because it does not take over the central message loop. Which is the
 * reason why I wrote it in the first place.
 * LICENSE: Do whatever you want with it, but don't blame me for anything. It would be nice if you state somewhere in
 * your Credits/Readme that you used SNIIS, but you're not required to do so.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cstdint>
#include <algorithm>

/// -------------------------------------------------------------------------------------------------------------------
#if defined(_WIN32) || defined(USE_WINE)
#define SNIIS_SYSTEM_WINDOWS 1
#elif defined(__linux__)
#define SNIIS_SYSTEM_LINUX 1
#elif defined(__MACH__) && defined(__APPLE__)
#define SNIIS_SYSTEM_MAC 1
#else
#error Platform not implemented
#endif

#define SNIIS_UNUSED( a) (void) a

namespace SNIIS
{
/// -------------------------------------------------------------------------------------------------------------------
/// Keyboard scan codes
enum KeyCode
{
  KC_UNASSIGNED = 0x00,
  KC_ESCAPE = 0x01,
  KC_1 = 0x02,
  KC_2 = 0x03,
  KC_3 = 0x04,
  KC_4 = 0x05,
  KC_5 = 0x06,
  KC_6 = 0x07,
  KC_7 = 0x08,
  KC_8 = 0x09,
  KC_9 = 0x0A,
  KC_0 = 0x0B,
  KC_MINUS = 0x0C,    // - on main keyboard
  KC_EQUALS = 0x0D,
  KC_BACK = 0x0E,    // backspace
  KC_TAB = 0x0F,
  KC_Q = 0x10,
  KC_W = 0x11,
  KC_E = 0x12,
  KC_R = 0x13,
  KC_T = 0x14,
  KC_Y = 0x15,
  KC_U = 0x16,
  KC_I = 0x17,
  KC_O = 0x18,
  KC_P = 0x19,
  KC_LBRACKET = 0x1A,
  KC_RBRACKET = 0x1B,
  KC_RETURN = 0x1C,    // Enter on main keyboard
  KC_LCONTROL = 0x1D,
  KC_A = 0x1E,
  KC_S = 0x1F,
  KC_D = 0x20,
  KC_F = 0x21,
  KC_G = 0x22,
  KC_H = 0x23,
  KC_J = 0x24,
  KC_K = 0x25,
  KC_L = 0x26,
  KC_SEMICOLON = 0x27,
  KC_APOSTROPHE = 0x28,
  KC_GRAVE = 0x29,    // accent
  KC_LSHIFT = 0x2A,
  KC_BACKSLASH = 0x2B,
  KC_Z = 0x2C,
  KC_X = 0x2D,
  KC_C = 0x2E,
  KC_V = 0x2F,
  KC_B = 0x30,
  KC_N = 0x31,
  KC_M = 0x32,
  KC_COMMA = 0x33,
  KC_PERIOD = 0x34,    // . on main keyboard
  KC_SLASH = 0x35,    // / on main keyboard
  KC_RSHIFT = 0x36,
  KC_MULTIPLY = 0x37,    // * on numeric keypad
  KC_LMENU = 0x38,    // left Alt
  KC_SPACE = 0x39,
  KC_CAPITAL = 0x3A,
  KC_F1 = 0x3B,
  KC_F2 = 0x3C,
  KC_F3 = 0x3D,
  KC_F4 = 0x3E,
  KC_F5 = 0x3F,
  KC_F6 = 0x40,
  KC_F7 = 0x41,
  KC_F8 = 0x42,
  KC_F9 = 0x43,
  KC_F10 = 0x44,
  KC_NUMLOCK = 0x45,
  KC_SCROLL = 0x46,    // Scroll Lock
  KC_NUMPAD7 = 0x47,
  KC_NUMPAD8 = 0x48,
  KC_NUMPAD9 = 0x49,
  KC_SUBTRACT = 0x4A,    // - on numeric keypad
  KC_NUMPAD4 = 0x4B,
  KC_NUMPAD5 = 0x4C,
  KC_NUMPAD6 = 0x4D,
  KC_ADD = 0x4E,    // + on numeric keypad
  KC_NUMPAD1 = 0x4F,
  KC_NUMPAD2 = 0x50,
  KC_NUMPAD3 = 0x51,
  KC_NUMPAD0 = 0x52,
  KC_DECIMAL = 0x53,    // . on numeric keypad
  KC_OEM_102 = 0x56,    // < > | on UK/Germany keyboards
  KC_F11 = 0x57,
  KC_F12 = 0x58,
  KC_F13 = 0x64,    //                     (NEC PC98)
  KC_F14 = 0x65,    //                     (NEC PC98)
  KC_F15 = 0x66,    //                     (NEC PC98)
  KC_KANA = 0x70,    // (Japanese keyboard)
  KC_ABNT_C1 = 0x73,    // / ? on Portugese (Brazilian) keyboards
  KC_CONVERT = 0x79,    // (Japanese keyboard)
  KC_NOCONVERT = 0x7B,    // (Japanese keyboard)
  KC_YEN = 0x7D,    // (Japanese keyboard)
  KC_ABNT_C2 = 0x7E,    // Numpad . on Portugese (Brazilian) keyboards
  KC_NUMPADEQUALS = 0x8D,    // = on numeric keypad (NEC PC98)
  KC_PREVTRACK = 0x90,    // Previous Track (KC_CIRCUMFLEX on Japanese keyboard)
  KC_AT = 0x91,    //                     (NEC PC98)
  KC_COLON = 0x92,    //                     (NEC PC98)
  KC_UNDERLINE = 0x93,    //                     (NEC PC98)
  KC_KANJI = 0x94,    // (Japanese keyboard)
  KC_STOP = 0x95,    //                     (NEC PC98)
  KC_AX = 0x96,    //                     (Japan AX)
  KC_UNLABELED = 0x97,    //                        (J3100)
  KC_NEXTTRACK = 0x99,    // Next Track
  KC_NUMPADENTER = 0x9C,    // Enter on numeric keypad
  KC_RCONTROL = 0x9D,
  KC_MUTE = 0xA0,    // Mute
  KC_CALCULATOR = 0xA1,    // Calculator
  KC_PLAYPAUSE = 0xA2,    // Play / Pause
  KC_MEDIASTOP = 0xA4,    // Media Stop
  KC_VOLUMEDOWN = 0xAE,    // Volume -
  KC_VOLUMEUP = 0xB0,    // Volume +
  KC_WEBHOME = 0xB2,    // Web home
  KC_NUMPADCOMMA = 0xB3,    // , on numeric keypad (NEC PC98)
  KC_DIVIDE = 0xB5,    // / on numeric keypad
  KC_SYSRQ = 0xB7,
  KC_RMENU = 0xB8,    // right Alt
  KC_PAUSE = 0xC5,    // Pause
  KC_HOME = 0xC7,    // Home on arrow keypad
  KC_UP = 0xC8,    // UpArrow on arrow keypad
  KC_PGUP = 0xC9,    // PgUp on arrow keypad
  KC_LEFT = 0xCB,    // LeftArrow on arrow keypad
  KC_RIGHT = 0xCD,    // RightArrow on arrow keypad
  KC_END = 0xCF,    // End on arrow keypad
  KC_DOWN = 0xD0,    // DownArrow on arrow keypad
  KC_PGDOWN = 0xD1,    // PgDn on arrow keypad
  KC_INSERT = 0xD2,    // Insert on arrow keypad
  KC_DELETE = 0xD3,    // Delete on arrow keypad
  KC_LWIN = 0xDB,    // Left Windows key
  KC_RWIN = 0xDC,    // Right Windows key
  KC_APPS = 0xDD,    // AppMenu key
  KC_POWER = 0xDE,    // System Power
  KC_SLEEP = 0xDF,    // System Sleep
  KC_WAKE = 0xE3,    // System Wake
  KC_WEBSEARCH = 0xE5,    // Web Search
  KC_WEBFAVORITES = 0xE6,    // Web Favorites
  KC_WEBREFRESH = 0xE7,    // Web Refresh
  KC_WEBSTOP = 0xE8,    // Web Stop
  KC_WEBFORWARD = 0xE9,    // Web Forward
  KC_WEBBACK = 0xEA,    // Web Back
  KC_MYCOMPUTER = 0xEB,    // My Computer
  KC_MAIL = 0xEC,    // Mail
  KC_MEDIASELECT = 0xED,  // Media Select
  KC_FIRST_CUSTOM = 0x100 // custom keycodes not mapped to the above list start with this code
};

/// Mouse Button IDs
enum MouseButtonID
{
  MB_Left, MB_Right, MB_Middle,
  MB_Button3, MB_Button4, MB_Button5, MB_Button6, MB_Button7,
  MB_Count
};

/// -------------------------------------------------------------------------------------------------------------------
/// Key Repetition config
struct KeyRepeatCfg
{
  bool enable; ///< is Key Repetition enabled
  float delay; ///< delay until first repeated KeyPress event is sent
  float interval; ///< interval in which repeated KeyPress events are sent after initial delay
  KeyRepeatCfg() { enable = true; delay = 0.7f; interval = 0.1f; }
};

/// -------------------------------------------------------------------------------------------------------------------
/// An abstract input device has a number of buttons and axes. You can ask for a human-readable description of it via
/// the Get*Text() set of functions, but the string will be empty if the operating system does not offer such a text.
/// You can also query the current state of each control. Each axis will also be exposed by a set of buttons for negative
/// and positive directions, so each analogue control also adds two digital controls.
class Device
{
  friend struct InputSystemHelper;

protected:
  size_t mId;   ///< Device ID
  size_t mCount; ///< we're the n-th device of our specific kind
  bool mIsFirstUpdate; ///< true if the device is queried for the first time. First state does not trigger updates to evade devices with perm_on controls
  bool mWasSortedOnActivity; ///< true if the device has been moved to the front after activity, false if activity is yet to be seen.

public:
  Device(size_t pId) : mId(pId), mCount( 0), mIsFirstUpdate( true), mWasSortedOnActivity( false) { }
  virtual ~Device() { }

  /// ID
  size_t GetId() const { return mId; }
  /// Count - our index in the sequence of devices of our kind, zero-based. Like: we're the 0th mouse, or the 2nd controller
  size_t GetCount() const { return mCount; }

  /// Query controls of that device
  virtual size_t GetNumButtons() const { return 0; }
  virtual std::string GetButtonText(size_t idx) const { SNIIS_UNUSED( idx); return std::string(); }
  virtual size_t GetNumAxes() const { return 0; }
  virtual std::string GetAxisText(size_t idx) const { SNIIS_UNUSED( idx); return std::string(); }

  /// Query current state
  virtual bool IsButtonDown(size_t idx) const { SNIIS_UNUSED( idx); return false; }
  virtual bool WasButtonPressed(size_t idx) const { SNIIS_UNUSED( idx); return false; }
  virtual bool WasButtonReleased(size_t idx) const { SNIIS_UNUSED( idx); return false; }
  virtual float GetAxisAbsolute(size_t idx) const { SNIIS_UNUSED( idx); return 0.0f; }
  virtual float GetAxisDifference(size_t idx) const { SNIIS_UNUSED( idx); return 0.0f; }

  /// Bookkeeping. Sorry for the public
  void ResetFirstUpdateFlag() { mIsFirstUpdate = false; }
};

/// a mouse is an abstract input device
class Mouse : public Device
{
public:
  Mouse(size_t pId) : Device(pId) { }

  virtual float GetMouseX() const { return 0; }
  virtual float GetMouseY() const { return 0; }
  virtual float GetRelMouseX() const { return 0; }
  virtual float GetRelMouseY() const { return 0; }
};

/// a keyboard is, too
class Keyboard : public Device
{
public:
  Keyboard(size_t pId) : Device(pId) { }

  virtual bool IsKeyDown(KeyCode key) const { return IsButtonDown(size_t(key)); }
  virtual bool WasKeyReleased(KeyCode key) const { return WasButtonPressed(size_t(key)); }
  virtual bool WasKeyPressed(KeyCode key) const { return WasButtonReleased(size_t(key)); }
};

/// A joystick is also a Device, but a pretty useless one except for the type.
class Joystick : public Device
{
public:
  Joystick(size_t pId) : Device(pId) { }
};

/// -------------------------------------------------------------------------------------------------------------------
/// Digital event channel - zero to multiple event sources connected to an digital input event which turns out either ON or OFF
struct DigitalChannel
{
  size_t mId;
  /// A source to trigger a digital channel is either digital or an analog source beyond a specific limit value.
  /// In case it's an analog source, the digital channel is assumed to be ON if the analog source is above the positive
  /// limit or below the negative limit value.
  struct Source { size_t mDeviceId, mControlId; bool mIsAnalog; float mAnalogLimit; };
  std::vector<Source> mSources;

  /// current state and change since last Update()
  bool mIsPressed, mIsModified;

  DigitalChannel() { mId = SIZE_MAX; mIsPressed = mIsModified = false; }
  size_t GetId() const { return mId; }
  void AddDigitalSource( size_t pDeviceId, size_t pButtonId);
  void AddAnalogSource( size_t pDeviceId, size_t pAxisId, float pLimit);
  void RemoveDigitalSource( size_t pDeviceId, size_t pButtonId);
  void RemoveAnalogSource( size_t pDeviceId, size_t pAxisId);
  void ClearAllAssignments();

  bool IsOn() const { return mIsPressed; }
  bool WasSwitchedOn() const { return mIsPressed && mIsModified; }
  bool WasSwitchedOff() const { return !mIsPressed && mIsModified; }
  void Update() { mIsModified = false; }
};

/// Analog event channel - zero to multiple event sources connected to an analog input event
struct AnalogChannel
{
  size_t mId;
  /// A source affecting an analog channel is analog, a digitalized analog source or a digital source, with digital
  /// sources translating to a specific value if ON.
  enum SourceType { Source_Analog, Source_Digital, Source_LimitedAnalog };
  struct Source { size_t mDeviceId, mControlId; SourceType mType; float mDigitalAmountOrAnalogLimit; float mAnalogScale; };
  std::vector<Source> mSources;

  /// current state and change since last Update()
  float mValue, mDiff;

  AnalogChannel() { mId = SIZE_MAX; mValue = mDiff = 0.0f; }
  size_t GetId() const { return mId; }
  void AddAnalogSource( size_t pDeviceId, size_t pAxisId);
  void AddDigitalSource( size_t pDeviceId, size_t pButtonId, float pTranslatedValue);
  void AddDigitalizedAnalogSource( size_t pDeviceId, size_t pAxisId, float pScale, float pLimitValue);
  void RemoveAnalogSource( size_t pDeviceId, size_t pAxisId);
  void RemoveDigitalSource( size_t pDeviceId, size_t pButtonId);
  void RemoveDigitalizedAnalogSource( size_t pDeviceId, size_t pAxisId);
  void ClearAllAssignments();

  float GetAbsolute() const { return mValue; }
  float GetRelative() const { return mDiff; }
  void Update() { mDiff = 0.0f; }
};

/// -------------------------------------------------------------------------------------------------------------------
/// Input handler - implement this interface to be notified about input events from within InputSystem::Update()
/// Every method can return if it handled the event. If not, it will be converted to the next level of abstraction.
/// Keyboard key: OnKey() -> OnUnicode() -> OnDigitalEvent() -> OnDigitalChannel()
/// Mouse button: OnMouseButton() -> OnDigitalEvent() -> OnDigitalChannel()
/// Mouse move: OnMouseMove() -> OnAnalogEvent() -> OnAnalogChannel()
/// Mouse wheel: OnMouseWheel() -> OnAnalogEvent() -> OnAnalogChannel()
/// Controller button: OnJoystickButton() -> OnDigitalEvent() -> OnDigitalChannel()
/// Controller stick/pad: OnJoystickAxis() -> OnAnalogEvent() -> OnAnalogChannel()
class InputHandler
{
public:
  virtual ~InputHandler() { }

  virtual bool OnKey( Keyboard*, KeyCode, bool) { return false; }
  virtual bool OnMouseMoved( Mouse*, float, float) { return false; }
  virtual bool OnMouseButton( Mouse*, size_t, bool) { return false; }
  virtual bool OnMouseWheel( Mouse*, float) { return false; }

  virtual bool OnJoystickButton( Joystick*, size_t, bool) { return false; }
  virtual bool OnJoystickAxis( Joystick*, size_t, float) { return false; }

  virtual bool OnUnicode( Keyboard*, size_t) { return false; }

  virtual bool OnDigitalEvent( Device*, size_t, bool) { return false; }
  virtual bool OnAnalogEvent( Device*, size_t, float) { return false; }
  virtual void OnDigitalChannel( const DigitalChannel&) { }
  virtual void OnAnalogChannel( const AnalogChannel &) { }
};

/// -------------------------------------------------------------------------------------------------------------------
class InputSystem
{
  friend struct InputSystemHelper;

protected:
  /// Construction private, use static Initialize() method
  InputSystem();
  virtual ~InputSystem();

public:
  /// Initializes the input system with the given InitArgs. When successful, gInstance is not Null.
  /// Windows: pass in your HWND. Linux: pass in your X Window handle. Mac: unused, pass nullptr.
  static bool Initialize( void* pInitArg);
  /// Destroys the input system. After returning gInstance is Null again
  static void Shutdown();

  /// Updates the inputs, to be called before handling system messages
  virtual void Update();

  /// Notifies the input system that the application has lost/gained focus. This avoids sticky keys where the PRESS msg
  /// was received but the RELEASE msg was not. Plus some OSes just keep sending input events regardless of focus; we
  /// want to avoid interacting with things in the game while it's in background.
  void SetFocus( bool pHasFocus);
  /// Returns if the input system is acting as if it's focused. Strange wording, I know, but it's set from the outside only.
  bool HasFocus() const { return mHasFocus; }

  /// Enables or disables multi-device mode. In Single mode all mice draw their state from the global OS state and thus
  /// match the expected movement exactly. Also all keyboards and mice will reroute their events to the primary device
  /// to match the user's expectation. In Multi Device Mode, some local API is used instead which usually lacks any Mouse 
  /// speed and acceleration setting, therefore the mouse might move differently than what the user expects.
  /// Also, there's usually a whole bunch of strange HIDs showing up in the enumeration and some of those behave weirdly,
  /// so exactly determining which device is under the user's hand is difficult at times.
  /// Default: disabled
  void SetMultiDeviceMode( bool enabled);
  /// Returns whether MultiMouseMode is currently enabled.
  bool IsInMultiDeviceMode() const { return mIsInMultiMouseMode; }

  /// Enables or disables Mouse Grabbing. If not grabbed, the mouse can leave the window or maybe activate things 
  /// in the background on Linux. This also activates the per-frame center code which keeps the mouse in the middle of
  /// the window. This is necessary to get correct relative mouse movements in SingleMouseMode because the mouse would
  /// otherwise hit the screen border and would therefore stop reporting further relative movements.
  /// Mouse grabbing is automatically disabled when 1) Focus is lost or 2) MultiMouseMode is enabled; and is reenabled
  /// automatically in those cases.
  void SetMouseGrab( bool enabled);
  /// Returns if MouseGrabMode is currently enabled. This does NOT imply that the mouse is actually grabbed.
  bool IsMouseGrabEnabled() const { return mIsMouseGrabEnabled; }
  /// Returns if the mouse is currently grabbed.
  bool IsMouseGrabbed() const { return mIsMouseGrabbed; }

  /// Event handler to be called on input events.
  InputHandler* GetHandler() const { return mHandler; }
  void SetHandler( InputHandler* handler) { mHandler = handler; }

  /// Returns all devices currently present
  const std::vector<Device*>& GetDevices() const { return mDevices; }
  /// Get the total number of devices of that specific kind
  size_t GetNumMice() const { return mNumMice; }
  size_t GetNumKeyboards() const { return mNumKeyboards; }
  size_t GetNumJoysticks() const { return mNumJoysticks; }
  /// Gets the nth device of that specific kind
  Mouse* GetMouseByCount( size_t pNumber) const;
  Keyboard* GetKeyboardByCount( size_t pNumber) const;
  Joystick* GetJoystickByCount( size_t pNumber) const;

  /// Comfort functions to query the current state of the input system and the changes since the last call to Update()
  /// All functions query the first device of its kind only.
  float GetMouseX() const { return mFirstMouse ? mFirstMouse->GetMouseX() : 0; }
  float GetMouseY() const { return mFirstMouse ? mFirstMouse->GetMouseY() : 0; }
  float GetRelMouseX() const { return mFirstMouse ? mFirstMouse->GetRelMouseX() : 0; }
  float GetRelMouseY() const { return mFirstMouse ? mFirstMouse->GetRelMouseY() : 0; }

  bool IsKeyDown( KeyCode key) const { return mFirstKeyboard ? mFirstKeyboard->IsKeyDown( key) : false; }
  bool WasKeyReleased( KeyCode key) const { return mFirstKeyboard ? mFirstKeyboard->WasKeyReleased( key) : false; }
  bool WasKeyPressed( KeyCode key) const { return mFirstKeyboard ? mFirstKeyboard->WasKeyPressed( key) : false; }

  bool IsMouseDown( size_t btnId) const { return mFirstMouse ? mFirstMouse->IsButtonDown( btnId) : false; }
  bool WasMouseReleased( size_t btnId) const { return mFirstMouse ? mFirstMouse->WasButtonReleased( btnId) : false; }
  bool WasMousePressed( size_t btnId) const { return mFirstMouse ? mFirstMouse->WasButtonPressed( btnId) : false; }
  float GetMouseWheelDiff() const { return mFirstMouse ? mFirstMouse->GetAxisAbsolute( 2) : 0.0f; }

  bool IsJoyDown( size_t btnId) const { return mFirstJoystick ? mFirstJoystick->IsButtonDown( btnId) : false; }
  bool WasJoyReleased( size_t btnId) const { return mFirstJoystick ? mFirstJoystick->WasButtonReleased( btnId) : false; }
  bool WasJoyPressed( size_t btnId) const { return mFirstJoystick ? mFirstJoystick->WasButtonPressed( btnId) : false; }
  float GetJoyAxisAbsolute( size_t axisId) const { return mFirstJoystick ? mFirstJoystick->GetAxisAbsolute( axisId) : 0.0f; }
  float GetJoyAxisDifference( size_t axisId) const { return mFirstJoystick ? mFirstJoystick->GetAxisDifference( axisId) : 0.0f; }

  /// Key repetition config
  void SetKeyRepeatCfg(const KeyRepeatCfg& cfg) { mKeyRepeatCfg = cfg; }
  const KeyRepeatCfg& GetKeyRepeatCfg() const { return mKeyRepeatCfg; }
  bool IsInKeyRepeat() const { return mKeyRepeatState.mTimeTillRepeat > 0.0f; }

  /// Returns the digital channel associated with the given number, or creates it if it doesn't exist, yet
  DigitalChannel& GetDigital( size_t id);
  /// Returns the analog channel associated with the given number, or creates it if it doesn't exist, yet
  AnalogChannel& GetAnalog( size_t id);
  /// Returns the ids of all digital channels
  std::vector<size_t> GetDigitalIds() const;
  /// Returns the ids of all analog channels
  std::vector<size_t> GetAnalogIds() const;
  /// Clears all channel assignments, both digital and analog
  void ClearChannelAssignments();

protected:
  virtual void InternSetFocus( bool pHasFocus) = 0;
  void InternGrabMouseIfNecessary();
  virtual void InternSetMouseGrab( bool enabled) = 0;
  void Log(const char* msg, ...);

protected:
  std::vector<Device*> mDevices;
  Mouse* mFirstMouse; Keyboard* mFirstKeyboard; Joystick* mFirstJoystick;
  size_t mNumMice, mNumKeyboards, mNumJoysticks;
  bool mReorderMiceOnActivity, mReorderKeyboardsOnActivity;
  InputHandler* mHandler;
  bool mHasFocus;
  bool mIsInMultiMouseMode;
  bool mIsMouseGrabEnabled, mIsMouseGrabbed;

  KeyRepeatCfg mKeyRepeatCfg;
  struct KeyRepeatState {
    Keyboard* mSender; KeyCode mKeyCode; size_t mUnicodeChar; clock_t lasttick; float mTimeTillRepeat;
  } mKeyRepeatState;

  std::map<size_t, DigitalChannel> mDigitalChannels;
  std::map<size_t, AnalogChannel> mAnalogChannels;
};

/// global Instance of the Input System if initialized, or Null
extern InputSystem* gInstance;

/// Log callback to receive logging messages.
typedef void (*LogCallback)( const char* message);
extern LogCallback gLogCallback;

} // namespace SNIIS
