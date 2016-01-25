/// @file SNIIS_Mac_Keyboard.cpp
/// Mac implementation of keyboards

#include "SNIIS_Mac.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_MAC
using namespace SNIIS;
#include <Carbon/Carbon.h>

static SNIIS::KeyCode sKeyTable[] = {
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_A, SNIIS::KC_B, SNIIS::KC_C, SNIIS::KC_D,
  SNIIS::KC_E, SNIIS::KC_F, SNIIS::KC_G, SNIIS::KC_H,
  SNIIS::KC_I, SNIIS::KC_J, SNIIS::KC_K, SNIIS::KC_L,
  SNIIS::KC_M, SNIIS::KC_N, SNIIS::KC_O, SNIIS::KC_P,
  SNIIS::KC_Q, SNIIS::KC_R, SNIIS::KC_S, SNIIS::KC_T,
  SNIIS::KC_U, SNIIS::KC_V, SNIIS::KC_W, SNIIS::KC_X,
  SNIIS::KC_Y, SNIIS::KC_Z, SNIIS::KC_1, SNIIS::KC_2,
  SNIIS::KC_3, SNIIS::KC_4, SNIIS::KC_5, SNIIS::KC_6,
  SNIIS::KC_7, SNIIS::KC_8, SNIIS::KC_9, SNIIS::KC_0,
  SNIIS::KC_RETURN, SNIIS::KC_ESCAPE, SNIIS::KC_BACK, SNIIS::KC_TAB,
  SNIIS::KC_SPACE, SNIIS::KC_MINUS, SNIIS::KC_EQUALS, SNIIS::KC_LBRACKET,
  SNIIS::KC_RBRACKET, SNIIS::KC_BACKSLASH, /*#' key*/SNIIS::KC_UNASSIGNED, SNIIS::KC_SEMICOLON,
  SNIIS::KC_APOSTROPHE, SNIIS::KC_GRAVE, SNIIS::KC_COMMA, SNIIS::KC_PERIOD,
  SNIIS::KC_SLASH, SNIIS::KC_CAPITAL, SNIIS::KC_F1, SNIIS::KC_F2,
  SNIIS::KC_F3, SNIIS::KC_F4, SNIIS::KC_F5, SNIIS::KC_F6,
  SNIIS::KC_F7, SNIIS::KC_F8, SNIIS::KC_F9, SNIIS::KC_F10,
  SNIIS::KC_F11, SNIIS::KC_F12, SNIIS::KC_SYSRQ, SNIIS::KC_SCROLL,
  SNIIS::KC_PAUSE, SNIIS::KC_INSERT, SNIIS::KC_HOME, SNIIS::KC_PGUP,
  SNIIS::KC_DELETE, SNIIS::KC_END, SNIIS::KC_PGDOWN, SNIIS::KC_RIGHT,
  SNIIS::KC_LEFT, SNIIS::KC_DOWN, SNIIS::KC_UP, SNIIS::KC_NUMLOCK,
  SNIIS::KC_DIVIDE, SNIIS::KC_MULTIPLY, SNIIS::KC_SUBTRACT, SNIIS::KC_ADD,
  SNIIS::KC_NUMPADENTER, SNIIS::KC_NUMPAD1, SNIIS::KC_NUMPAD2, SNIIS::KC_NUMPAD3,
  SNIIS::KC_NUMPAD4, SNIIS::KC_NUMPAD5, SNIIS::KC_NUMPAD6, SNIIS::KC_NUMPAD7,
  SNIIS::KC_NUMPAD8, SNIIS::KC_NUMPAD9, SNIIS::KC_NUMPAD0, SNIIS::KC_DECIMAL,
  SNIIS::KC_OEM_102, SNIIS::KC_APPS, SNIIS::KC_POWER, SNIIS::KC_NUMPADEQUALS,
  SNIIS::KC_F13, SNIIS::KC_F14, SNIIS::KC_F15, /*F16*/SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, /*F24*/SNIIS::KC_UNASSIGNED,
  /*Execute*/SNIIS::KC_UNASSIGNED, /*Help*/SNIIS::KC_UNASSIGNED, /*Menu*/SNIIS::KC_UNASSIGNED, SNIIS::KC_MEDIASELECT,
  SNIIS::KC_MEDIASTOP, /*Again*/SNIIS::KC_UNASSIGNED, /*Undo*/SNIIS::KC_UNASSIGNED, /*Cut*/SNIIS::KC_UNASSIGNED,
  /*Copy*/SNIIS::KC_UNASSIGNED, /*Paste*/SNIIS::KC_UNASSIGNED, /*Find*/SNIIS::KC_UNASSIGNED, SNIIS::KC_MUTE,
  SNIIS::KC_VOLUMEUP, SNIIS::KC_VOLUMEDOWN, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_NUMPADCOMMA, SNIIS::KC_NUMPADEQUALS, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED, SNIIS::KC_UNASSIGNED,
  SNIIS::KC_LCONTROL, SNIIS::KC_LSHIFT, SNIIS::KC_LMENU, SNIIS::KC_LWIN,
  SNIIS::KC_RCONTROL, SNIIS::KC_RSHIFT, SNIIS::KC_RMENU, SNIIS::KC_RWIN
};

static const size_t sKeyTableSize = sizeof( sKeyTable) / sizeof( SNIIS::KeyCode);

// translates a HID usage code to a OSX virtual key. Copied from http://jens.ayton.se/code/keynaming/
static const uint16_t	sOsxKeyTable[] =
{
  0xffff,		/* Reserved (no event indicated) */
  0xffff,		/* ErrorRollOver */
  0xffff,		/* POSTFail */
  0xffff,		/* ErrorUndefined */
  0x00,				/* a and A */
  0x0B,				/* b and B */
  0x08,				/* ... */
  0x02,
  0x0E,
  0x03,
  0x05,
  0x04,
  0x22,
  0x26,
  0x28,
  0x25,
  0x2E,
  0x2D,
  0x1F,
  0x23,
  0x0C,
  0x0F,
  0x01,
  0x11,
  0x20,
  0x09,
  0x0D,
  0x07,
  0x10,
  0x06,				/* z and Z */
  0x12,				/* 1 */
  0x13,				/* 2 */
  0x14,				/* ... */
  0x15,
  0x17,
  0x16,
  0x1A,
  0x1C,
  0x19,				/* 9 */
  0x1D,				/* 0 */
  36,		/* Keyboard Return (ENTER) */
  53,			/* Escape */
  51,		/* Delete (Backspace) */
  0x30,				/* Tab */
  49,			/* Space bar */
  0x1B,				/* - and _ */
  0x18,				/* = and + */
  0x21,				/* [ and { */
  0x1E,				/* ] and } */
  0x2A,				/* \ and | */
  0xffff,		/* "Non-US # and ~" ?? */
  0x29,				/* ; and : */
  0x27,				/* ' and " */
  0x32,				/* ` and ~ */
  0x2B,				/* , and < */
  0x2F,				/* . and > */
  0x2C,				/* / and ? */
  57,		/* Caps Lock */
  122,			/* F1 */
  120,			/* ... */
  99,
  118,
  96,
  97,
  98,
  100,
  101,
  109,
  103,
  111,			/* F12 */
  0xffff,		/* Print Screen */
  0xffff,		/* Scroll Lock */
  0xffff,		/* Pause */
  0xffff,		/* Insert */
  115,			/* Home */
  116,			/* Page Up */
  117,		/* Delete Forward */
  119,			/* End */
  121,			/* Page Down */
  124,		/* Right Arrow */
  123,		/* Left Arrow */
  125,		/* Down Arrow */
  126,		/* Up Arrow */
  71,			/* Keypad Num Lock and Clear */
  0x4B,				/* Keypad / */
  0x43,				/* Keypad * */
  0x4E,				/* Keypad - */
  0x45,				/* Keypad + */
  76,		/* Keypad ENTER */
  0x53,				/* Keypad 1 */
  0x54,				/* Keypad 2 */
  0x55,				/* Keypad 3 */
  0x56,				/* Keypad 4 */
  0x57,				/* Keypad 5 */
  0x58,				/* Keypad 6 */
  0x59,				/* Keypad 7 */
  0x5B,				/* Keypad 8 */
  0x5C,				/* Keypad 9 */
  0x52,				/* Keypad 0 */
  0x41,				/* Keypad . */
  0x32,				/* "Keyboard Non-US \ and |" */
  0xffff,		/* "Keyboard Application" (Windows key for Windows 95, and "Compose".) */
  0x6005,			/* Keyboard Power (status, not key... but Apple doesn't seem to have read the spec properly) */
  0x51,				/* Keypad = */
  105,			/* F13 */
  107,			/* ... */
  113,
  106,
  64,
  79,
  80,
  90,
  0xffff,
  0xffff,
  0xffff,
  0xffff,			/* F24 */
  0xffff,		/* Keyboard Execute */
  114,			/* Keyboard Help */
  0xffff,		/* Keyboard Menu */
  0xffff,		/* Keyboard Select */
  0xffff,		/* Keyboard Stop */
  0xffff,		/* Keyboard Again */
  0xffff,		/* Keyboard Undo */
  0xffff,		/* Keyboard Cut */
  0xffff,		/* Keyboard Copy */
  0xffff,		/* Keyboard Paste */
  0xffff,		/* Keyboard Find */
  74,			/* Keyboard Mute */
  72,		/* Keyboard Volume Up */
  73,	/* Keyboard Volume Down */
  57,		/* Keyboard Locking Caps Lock */
  0xffff,		/* Keyboard Locking Num Lock */
  0xffff,		/* Keyboard Locking Scroll Lock */
  0x41,				/*	Keypad Comma ("Keypad Comma is the appropriate usage for the Brazilian
              keypad period (.) key. This represents the closest possible  match, and
              system software should do the correct mapping based on the current locale
              setting." If strange stuff happens on a (physical) Brazilian keyboard,
              I'd like to know about it. */
  0x51,				/* Keypad Equal Sign ("Used on AS/400 Keyboards.") */
  0xffff,		/* Keyboard International1 (Brazilian / and ? key? Kanji?) */
  0xffff,		/* Keyboard International2 (Kanji?) */
  0xffff,		/* Keyboard International3 (Kanji?) */
  0xffff,		/* Keyboard International4 (Kanji?) */
  0xffff,		/* Keyboard International5 (Kanji?) */
  0xffff,		/* Keyboard International6 (Kanji?) */
  0xffff,		/* Keyboard International7 (Kanji?) */
  0xffff,		/* Keyboard International8 (Kanji?) */
  0xffff,		/* Keyboard International9 (Kanji?) */
  0xffff,		/* Keyboard LANG1 (Hangul/English toggle) */
  0xffff,		/* Keyboard LANG2 (Hanja conversion key) */
  0xffff,		/* Keyboard LANG3 (Katakana key) */		// kVKC_Kana?
  0xffff,		/* Keyboard LANG4 (Hirigana key) */
  0xffff,		/* Keyboard LANG5 (Zenkaku/Hankaku key) */
  0xffff,		/* Keyboard LANG6 */
  0xffff,		/* Keyboard LANG7 */
  0xffff,		/* Keyboard LANG8 */
  0xffff,		/* Keyboard LANG9 */
  0xffff,		/* Keyboard Alternate Erase ("Example, Erase-Eaze™ key.") */
  0xffff,		/* Keyboard SysReq/Attention */
  0xffff,		/* Keyboard Cancel */
  0xffff,		/* Keyboard Clear */
  0xffff,		/* Keyboard Prior */
  0xffff,		/* Keyboard Return */
  0xffff,		/* Keyboard Separator */
  0xffff,		/* Keyboard Out */
  0xffff,		/* Keyboard Oper */
  0xffff,		/* Keyboard Clear/Again */
  0xffff,		/* Keyboard CrSel/Props */
  0xffff,		/* Keyboard ExSel */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  0xffff,		/* Keypad 00 */
  0xffff,		/* Keypad 000 */
  0xffff,		/* Thousands Separator */
  0xffff,		/* Decimal Separator */
  0xffff,		/* Currency Unit */
  0xffff,		/* Currency Sub-unit */
  0xffff,		/* Keypad ( */
  0xffff,		/* Keypad ) */
  0xffff,		/* Keypad { */
  0xffff,		/* Keypad } */
  0xffff,		/* Keypad Tab */
  0xffff,		/* Keypad Backspace */
  0xffff,		/* Keypad A */
  0xffff,		/* Keypad B */
  0xffff,		/* Keypad C */
  0xffff,		/* Keypad D */
  0xffff,		/* Keypad E */
  0xffff,		/* Keypad F */
  0xffff,		/* Keypad XOR */
  0xffff,		/* Keypad ^ */
  0xffff,		/* Keypad % */
  0xffff,		/* Keypad < */
  0xffff,		/* Keypad > */
  0xffff,		/* Keypad & */
  0xffff,		/* Keypad && */
  0xffff,		/* Keypad | */
  0xffff,		/* Keypad || */
  0xffff,		/* Keypad : */
  0xffff,		/* Keypad # */
  0xffff,		/* Keypad Space */
  0xffff,		/* Keypad @ */
  0xffff,		/* Keypad ! */
  0xffff,		/* Keypad Memory Store */
  0xffff,		/* Keypad Memory Recall */
  0xffff,		/* Keypad Memory Clear */
  0xffff,		/* Keypad Memory Add */
  0xffff,		/* Keypad Memory Subtract */
  0xffff,		/* Keypad Memory Multiply */
  0xffff,		/* Keypad Memory Divide */
  0xffff,		/* Keypad +/- */
  0xffff,		/* Keypad Clear */
  0xffff,		/* Keypad Clear Entry */
  0xffff,		/* Keypad Binary */
  0xffff,		/* Keypad Octal */
  0xffff,		/* Keypad Decimal */
  0xffff,		/* Keypad Hexadecimal */
  0xffff,		/* Reserved */
  0xffff,		/* Reserved */
  59,		/* Keyboard LeftControl */
  56,			/* Keyboard LeftShift */
  58,		/* Keyboard LeftAlt */
  55,		/* Keyboard LeftGUI */
  62,		/* Keyboard RightControl */
  60,		/* Keyboard RightShift */
  61,		/* Keyboard RightAlt */
  0xffff		/* Keyboard RightGUI */
};

static const size_t sOsxKeyTableSize = sizeof( sOsxKeyTable) / sizeof( uint16_t);

// --------------------------------------------------------------------------------------------------------------------
MacKeyboard::MacKeyboard( MacInput* pSystem, size_t pId, IOHIDDeviceRef pDeviceRef)
  : Keyboard( pId), MacDevice( pSystem, pDeviceRef)
{
  mNumKeys = 0;
  mDeadKeyState = 0;

  if( IOHIDDeviceOpen( mDevice, kIOHIDOptionsTypeNone) != kIOReturnSuccess )
    throw std::runtime_error( "Failed to open HID device");

  // get all controls
  auto ctrls = EnumerateDeviceControls( mDevice);
  for( const auto& c : ctrls )
  {
//    Log( "Control: \"%s\" Typ %d, keks %d, usage %d/%d, bereich %d..%d", c.mName.c_str(), c.mType, c.mCookie, c.mUsePage, c.mUsage, c.mMin, c.mMax);
    if( c.mType == MacControl::Type_Button )
    {
      if( c.mUsage > sKeyTableSize || sKeyTable[c.mUsage] == SNIIS::KC_UNASSIGNED )
        mExtraButtons.push_back( c.mUsage);
      else
        mNumKeys = std::max( mNumKeys, size_t( sKeyTable[c.mUsage]) + 1);
    }
  }

  if( !mExtraButtons.empty() )
    mNumKeys = SNIIS::KC_FIRST_CUSTOM + mExtraButtons.size();

  // call us if something moves
  IOHIDDeviceRegisterInputValueCallback( mDevice, &InputElementValueChangeCallback, static_cast<MacDevice*> (this));
  IOHIDDeviceScheduleWithRunLoop( mDevice, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

  mState.resize( (mNumKeys+63) / 64); mPrevState.resize( mState.size());
}

// --------------------------------------------------------------------------------------------------------------------
void MacKeyboard::StartUpdate()
{
  mPrevState = mState;
}

// --------------------------------------------------------------------------------------------------------------------
void MacKeyboard::HandleEvent(IOHIDDeviceRef dev, IOHIDElementCookie cookie, uint32_t usepage, uint32_t usage, CFIndex value)
{
  SNIIS_UNUSED( cookie);
  SNIIS_UNUSED( dev);

  if( usepage != kHIDPage_KeyboardOrKeypad )
    return;
  // also ignore events with invalid usage - I got a whole bunch of those with every keystroke
  if( usage < 4 || usage == UINT32_MAX )
    return;

  // translate to KeyCode - if not in the table, it's an extra key
  auto kc = usage < sKeyTableSize ? sKeyTable[usage] : SNIIS::KC_UNASSIGNED;
  if( kc == SNIIS::KC_UNASSIGNED )
  {
    auto it = std::find( mExtraButtons.cbegin(), mExtraButtons.cend(), usage);
    if( it != mExtraButtons.cend() )
      kc = SNIIS::KeyCode( SNIIS::KC_FIRST_CUSTOM + std::distance( mExtraButtons.cbegin(), it));
    else
      return;
  }

  // we're still there - it's an actual key stroke
  if( !mIsFirstUpdate )
    InputSystemHelper::SortThisKeyboardToFront( this);

  bool isDown = (value != 0);

  // translate to unicode char
  size_t uc = 0;
  if( isDown )
  {
    // translate to OSX keycode.
    uint16_t osxc = usage < sOsxKeyTableSize ? sOsxKeyTable[usage] : 0xffff;
    if( osxc != 0xffff )
    {
      TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
      CFDataRef uchr = (CFDataRef) TISGetInputSourceProperty( currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
      auto keyboardLayout = (const UCKeyboardLayout*) CFDataGetBytePtr( uchr);

      if( keyboardLayout )
      {
        UniCharCount maxlen = 8;
        UniCharCount actlen = 0;
        UniChar ustr[maxlen];
        uint32_t modifiers = 0;

        if( IsKeyDown( SNIIS::KC_LSHIFT) || IsKeyDown( SNIIS::KC_RSHIFT) ) modifiers |= 0x4;
        if( IsKeyDown( SNIIS::KC_LCONTROL) || IsKeyDown( SNIIS::KC_RCONTROL) ) modifiers |= 0x10;
        if( IsKeyDown( SNIIS::KC_LMENU) || IsKeyDown( SNIIS::KC_RMENU) ) modifiers |= 0x8;

        UCKeyTranslate( keyboardLayout, osxc, kUCKeyActionDown, modifiers, LMGetKbdType(), 0, &mDeadKeyState, maxlen, &actlen, ustr);

        // only translate the first UTF16 character - either one or two code points
        if( actlen > 0 )
          uc = (ustr[0] & 0x8000) != 0 ? ((size_t( ustr[0] & 0x3ff) << 10) | size_t( ustr[1] & 0x3ff)) : size_t( ustr[0]);
      }
    }
  }

  if( !mIsFirstUpdate )
    DoKeyboardKey( kc, uc, isDown);
}

// --------------------------------------------------------------------------------------------------------------------
void MacKeyboard::DoKeyboardKey( SNIIS::KeyCode kc, size_t unicode, bool isPressed)
{
  // reroute to primary keyboard if we're in SingleDeviceMode
  if( !mSystem->IsInMultiDeviceMode() && GetCount() != 0 )
  {
    dynamic_cast<MacKeyboard*> (mSystem->GetKeyboardByCount( 0))->DoKeyboardKey( kc, unicode, isPressed);
    return;
  }

  // small issue prevention: some additional keyboard might have buttons that this keyboard doesn't
  if( kc >= mNumKeys )
    return;

  // don't signal if it isn't an actual state change
  if( IsSet( kc) == isPressed )
    return;
  Set( kc, isPressed);

  InputSystemHelper::DoKeyboardButton( this, kc, unicode, isPressed);
}

// --------------------------------------------------------------------------------------------------------------------
void MacKeyboard::SetFocus( bool pHasFocus)
{
  if( pHasFocus )
  {
    // Don't do anything. Keys currently pressed are lost then, all other controls will be handled correctly at next update
    // TODO: maybe query current keyboard state? To capture that Alt+Tab? Naaah.
  }
  else
  {
    for( size_t a = 0; a < mNumKeys; ++a )
    {
      if( IsSet( a) )
      {
        DoKeyboardKey( (SNIIS::KeyCode) a, 0, false);
        mPrevState[a/64] |= (1ull << (a&63));
      }
    }
  }
}

void MacKeyboard::Set( size_t kc, bool set)
{
  if( set )
    mState[kc/64] |= (1ull << (kc&63));
  else
    mState[kc/64] &= UINT64_MAX ^ (1ull << (kc&63));
}
bool MacKeyboard::IsSet( size_t kc) const
{
  return (mState[kc/64] & (1ull << (kc&63))) != 0;
}
bool MacKeyboard::WasSet( size_t kc) const
{
  return (mPrevState[kc/64] & (1ull << (kc&63))) != 0;
}

// --------------------------------------------------------------------------------------------------------------------
size_t MacKeyboard::GetNumButtons() const
{
  return mNumKeys;
}
// --------------------------------------------------------------------------------------------------------------------
std::string MacKeyboard::GetButtonText( size_t /*idx*/) const
{
/*
  // search backwards to map our keycode to the Mac Keysym it originated from
  size_t ks = SIZE_MAX;
  if( idx >= KC_FIRST_CUSTOM )
  {
    idx -= KC_FIRST_CUSTOM;
    if( idx < mExtraButtons.size() )
      ks = mExtraButtons[idx];
  } else
  {
    for( auto it = sKeyTable.cbegin(); it != sKeyTable.cend() && ks == SIZE_MAX; ++it )
      if( it->second == idx )
        ks = it->first;
  }

  // still there - haven't found the Keysym
  if( ks == SIZE_MAX )
    return "";
*/
  return "";
}
// --------------------------------------------------------------------------------------------------------------------
bool MacKeyboard::IsButtonDown( size_t idx) const
{
  if( idx < mNumKeys )
    return IsSet( idx);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool MacKeyboard::WasButtonPressed( size_t idx) const
{
  if( idx < mNumKeys )
    return IsSet( idx) && !WasSet( idx);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool MacKeyboard::WasButtonReleased( size_t idx) const
{
  if( idx < mNumKeys )
    return !IsSet( idx) && WasSet( idx);
  else
    return false;
}

#endif // SNIIS_SYSTEM_MAC

