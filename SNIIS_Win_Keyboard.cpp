/// @file SNIIS_Win_Keyboard.cpp
/// Windows implementation of keyboards

#include "SNIIS_Win.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_WINDOWS
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
WinKeyboard::WinKeyboard( WinInput* pSystem, size_t pId, HANDLE pHandle, IDirectInputDevice8* pDirectInputKeyboard)
  : Keyboard( pId), mSystem( pSystem), mHandle( pHandle), mKeyboard( pDirectInputKeyboard)
{
  memset( mState, 0, sizeof( mState));
  memset( mPrevState, 0, sizeof( mPrevState));
  deadKey = 0;
}

// --------------------------------------------------------------------------------------------------------------------
void WinKeyboard::StartUpdate()
{
  memcpy( mPrevState, mState, sizeof( mState));
}

void WinKeyboard::ParseMessage( const RAWINPUT& e, bool useWorkaround)
{
  // safety measure - should not reach us if it's not for us
  assert( e.header.dwType == RIM_TYPEKEYBOARD && e.header.hDevice == mHandle );
  // work around for GetRawInputBuffer() bug on WoW64
  auto ptr = reinterpret_cast<const uint8_t*> (&e.data.keyboard);
  if( useWorkaround )
    ptr += 8;
  const auto& kbd = *(reinterpret_cast<const RAWKEYBOARD*> (ptr));

  size_t scanCode = kbd.MakeCode;
  size_t virtualKey = kbd.VKey;
  size_t flags = kbd.Flags;
  bool pressed = !(flags & RI_KEY_BREAK);

  // all logic gratefully taken from http://molecularmusings.wordpress.com/2011/09/05/properly-handling-keyboard-input/
  if( virtualKey == 255 )
    return;
  else if( virtualKey == VK_SHIFT )
  {
    // correct left-hand / right-hand SHIFT
    virtualKey = MapVirtualKey( scanCode, MAPVK_VSC_TO_VK_EX);
    scanCode = (virtualKey == VK_RSHIFT) ? KC_RSHIFT : KC_LSHIFT;
  }
  else if( virtualKey == VK_NUMLOCK )
  {
    // correct PAUSE/BREAK and NUM LOCK silliness, and set the extended bit
    scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC); //  | 0x100;
  }

  // e0 and e1 are escape sequences used for certain special keys, such as PRINT and PAUSE/BREAK.
  // see http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
  const bool isE0 = ((flags & RI_KEY_E0) != 0);
  const bool isE1 = ((flags & RI_KEY_E1) != 0);

  if (isE1)
  {
    // for escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
    // however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
    if (virtualKey == VK_PAUSE )
      scanCode = KC_PAUSE; // 0x45;
    else
      scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
  }


  switch (virtualKey)
  {
      // right-hand CONTROL and ALT have their e0 bit set
    case VK_CONTROL: scanCode = isE0 ? KC_RCONTROL : KC_LCONTROL; break;
    case VK_MENU: scanCode = isE0 ? KC_RMENU : KC_LMENU; break;
    // NUMPAD ENTER has its e0 bit set
    case VK_RETURN: scanCode = isE0 ? KC_NUMPADENTER : KC_RETURN; break;

    // the standard INSERT, DELETE, HOME, END, PRIOR and NEXT keys will always have their e0 bit set, but the
    // corresponding keys on the NUMPAD will not.
    case VK_INSERT: scanCode = !isE0 ? KC_NUMPAD0 : KC_INSERT; break;
    case VK_DELETE: scanCode = !isE0 ? KC_NUMPADCOMMA : KC_DELETE; break;
    case VK_HOME: scanCode = !isE0 ? KC_NUMPAD7 : KC_HOME; break;
    case VK_END: scanCode = !isE0 ? KC_NUMPAD1 : KC_END; break;
    case VK_PRIOR: scanCode = !isE0 ? KC_NUMPAD9 : KC_PGUP; break;
    case VK_NEXT: scanCode = !isE0 ? KC_NUMPAD3 : KC_PGDOWN; break;

    // the standard arrow keys will always have their e0 bit set, but the
    // corresponding keys on the NUMPAD will not.
    case VK_LEFT: scanCode = !isE0 ? KC_NUMPAD4 : KC_LEFT; break;
    case VK_RIGHT: scanCode = !isE0 ? KC_NUMPAD6 : KC_RIGHT; break;
    case VK_UP: scanCode = !isE0 ? KC_NUMPAD8 : KC_UP; break;
    case VK_DOWN: scanCode = !isE0 ? KC_NUMPAD2 : KC_DOWN; break;

    // NUMPAD 5 doesn't have its e0 bit set
    case VK_CLEAR: scanCode = !isE0 ? KC_NUMPAD5 : KC_NUMPAD5; break; // was isn VK_CLEAR?
  }

  // any message counts as activity, so treat this mouse as primary if it's not decided, yet
  InputSystemHelper::MakeThisKeyboardFirst( this);

  if( IsSet( scanCode) != pressed )
  {
    Set( scanCode, pressed);
    InputSystemHelper::DoKeyboardButton( this, (KeyCode) scanCode, TranslateText( (KeyCode) scanCode), pressed);
  }
}

// --------------------------------------------------------------------------------------------------------------------
void WinKeyboard::SetFocus( bool pHasFocus)
{
  if( pHasFocus )
  {
    // don't do anything. Keys currently pressed are lost then, all other controls will be handled correctly at next update
    // TODO: maybe query current keyboard state? To capture that Alt+Tab? Naaah.
  }
  else
  {
    for( size_t a = 0; a < NumKeys; ++a )
    {
      if( IsSet( a) )
      {
        Set( a,  false);
        mPrevState[a/64] |= (1ull << (a&63));
        InputSystemHelper::DoKeyboardButton( this, (SNIIS::KeyCode) a, 0, false);
      }
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
size_t WinKeyboard::TranslateText( size_t kc)
{
	BYTE keyState[256];
	HKL  layout = GetKeyboardLayout(0);
	if( GetKeyboardState(keyState) == 0 )
		return 0;

	unsigned int vk = MapVirtualKeyEx(kc, 3, layout);
	if( vk == 0 )
		return 0;

	WCHAR buff[3] = {0};
	int ascii = ToUnicodeEx(vk, kc, keyState, buff, 3, 0, layout);

	if(ascii == 1 && deadKey != '\0' )
	{
		// A dead key is stored and we have just converted a character key
		// Combine the two into a single character
		WCHAR wcBuff[3] = {buff[0], deadKey, '\0'};
		WCHAR out[3];

		deadKey = '\0';
		if(FoldStringW(MAP_PRECOMPOSED, (LPWSTR)wcBuff, 3, (LPWSTR)out, 3))
			return out[0];
	}
	else if (ascii == 1)
	{	// We have a single character
		deadKey = '\0';
		return buff[0];
	}
	else if(ascii == 2)
	{	// Convert a non-combining diacritical mark into a combining diacritical mark
		// Combining versions range from 0x300 to 0x36F; only 5 (for French) have been mapped below
		// http://www.fileformat.info/info/unicode/block/combining_diacritical_marks/images.htm
		switch(buff[0])	{
		case 0x5E: // Circumflex accent: â
			deadKey = 0x302; break;
		case 0x60: // Grave accent: à
			deadKey = 0x300; break;
		case 0xA8: // Diaeresis: ü
			deadKey = 0x308; break;
		case 0xB4: // Acute accent: é
			deadKey = 0x301; break;
		case 0xB8: // Cedilla: ç
			deadKey = 0x327; break;
		default:
			deadKey = buff[0]; break;
		}
	}

	return 0;
}

void WinKeyboard::Set( size_t kc, bool set)
{
  if( set )
    mState[kc/64] |= (1ull << (kc&63));
  else
    mState[kc/64] &= UINT64_MAX ^ (1ull << (kc&63));
}
bool WinKeyboard::IsSet( size_t kc) const
{
  return (mState[kc/64] & (1ull << (kc&63))) != 0;
}
bool WinKeyboard::WasSet( size_t kc) const
{
  return (mPrevState[kc/64] & (1ull << (kc&63))) != 0;
}

// --------------------------------------------------------------------------------------------------------------------
size_t WinKeyboard::GetNumButtons() const
{
  return NumKeys;
}
// --------------------------------------------------------------------------------------------------------------------
std::string WinKeyboard::GetButtonText( size_t idx) const
{
	DIPROPSTRING prop;
	prop.diph.dwSize = sizeof(DIPROPSTRING);
	prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	prop.diph.dwObj = static_cast<DWORD>(idx); // the KeyCode enum was carefully crafted to fit the DirectInput keys
	prop.diph.dwHow = DIPH_BYOFFSET;

	if( FAILED( mKeyboard->GetProperty( DIPROP_KEYNAME, &prop.diph)) )
    return "";

  // convert the WCHAR in "wsz" to multibyte
	char temp[256];
	if( WideCharToMultiByte( CP_UTF8, 0, prop.wsz, -1, temp, sizeof(temp), NULL, NULL) <= 0 )
		return "";
  
  return std::string( &temp[0]);
}

// --------------------------------------------------------------------------------------------------------------------
bool WinKeyboard::IsButtonDown( size_t idx) const
{
  if( idx < NumKeys)
    return IsSet( idx);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool WinKeyboard::WasButtonPressed( size_t idx) const
{
  if( idx < NumKeys )
    return IsSet( idx) && !WasSet( idx);
  else
    return false;
}
// --------------------------------------------------------------------------------------------------------------------
bool WinKeyboard::WasButtonReleased( size_t idx) const
{
  if( idx < NumKeys )
    return !IsSet( idx) && WasSet( idx);
  else
    return false;
}

#endif // SNIIS_SYSTEM_WINDOWS
