/// @file SNIIS_Win.cpp
/// Windows implementation of input

#include "SNIIS_Win.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_WINDOWS
using namespace SNIIS;

#ifndef SAFE_RELEASE
  #define SAFE_RELEASE(x) if(x != nullptr) { x->Release(); x = nullptr; }
#endif

#pragma comment(lib, "xinput.lib")

// --------------------------------------------------------------------------------------------------------------------
// Constructor
WinInput::WinInput( HWND wnd)
{
  hWnd = wnd;
  mDirectInput = nullptr;
  mKeyboard = nullptr;
  mIsWorkAroundEnabled = false;

	if( IsWindow(hWnd) == 0 )
		throw std::runtime_error( "HWND is not valid");

  // workaround for GetRawInputBuffer() writing misaligned structures when being a 32bit exe running on a 64bit Windows
  {
    typedef BOOL (WINAPI *IsWow64Process) (HANDLE, PBOOL);
    IsWow64Process isWow64Process = nullptr;
    isWow64Process = (IsWow64Process) GetProcAddress( GetModuleHandle( TEXT("kernel32")), "IsWow64Process");
    if( nullptr != isWow64Process )
    {
      BOOL isit = FALSE;
      if( isWow64Process( GetCurrentProcess(), &isit) )
        mIsWorkAroundEnabled = (isit != FALSE);
    }
  }

  // reroute the WinProc through our function to catch ALL RawInput messages. It got obvious that using buffered reads
  // on RawInput is useless; the application's message loop still caught a few RawInput messages and discarded those.
  // This approach is now hopefully reliable enough to catch all messages without losses, while still not interfering
  // with the game's message loop

	HINSTANCE hInst = GetModuleHandle(0);
  mPreviousWndProc = (WNDPROC) SetWindowLongPtrW( hWnd, GWL_WNDPROC, (LONG) &WinInput::WndProcHook);

	//Create the device
	HRESULT hr = DirectInput8Create( hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&mDirectInput, nullptr );
  if (FAILED(hr))	
		throw std::runtime_error( "Unable to init DirectX8 Input");

  // create and configure a mouse and keyboard to ask for system control names
  if( FAILED(mDirectInput->CreateDevice(GUID_SysKeyboard, &mKeyboard, nullptr)) )
    throw std::runtime_error( "Could not init default keyboard");
  if( FAILED(mKeyboard->SetDataFormat(&c_dfDIKeyboard)) )
    throw std::runtime_error( "Keyboard format error");

	EnumerateDevices();
  CheckXInputDevices();
  RegisterForRawInput();
}

// --------------------------------------------------------------------------------------------------------------------
// Destructor
WinInput::~WinInput()
{
  for( auto d : mDevices )
    delete d;

  if( mKeyboard )
    mKeyboard->Release();
	if( mDirectInput )
		mDirectInput->Release();

  SetWindowLongPtrW( hWnd, GWL_WNDPROC, (LONG) mPreviousWndProc);
}

// --------------------------------------------------------------------------------------------------------------------
void WinInput::EnumerateDevices()
{
	//Enumerate all attached devices to collect joysticks / gamepads
	mDirectInput->EnumDevices( 0, _DIEnumDevCallback, this, DIEDFL_ATTACHEDONLY);

  // enumerate again using RawInput to collect keyboards and mice
  uint32_t deviceCount = 0;
  uint32_t erg = GetRawInputDeviceList( nullptr, &deviceCount, sizeof( RAWINPUTDEVICELIST));
  if( erg == UINT32_MAX )
    throw std::runtime_error( "unable to get device count");

  std::vector<RAWINPUTDEVICELIST> devices( deviceCount);
  erg = GetRawInputDeviceList( devices.data(), &deviceCount, sizeof( RAWINPUTDEVICELIST));
  if( erg == UINT32_MAX )
    throw std::runtime_error( "unable to retrieve device list");

  for( size_t a = 0; a < devices.size(); ++a )
  {
    const RAWINPUTDEVICELIST& dev = devices[a];

    std::string name( 256, 0);
    uint32_t nameSize = (uint32_t) name.size();
    erg = GetRawInputDeviceInfo( dev.hDevice, RIDI_DEVICENAME, &name[0], &nameSize);

    RID_DEVICE_INFO info;
    uint32_t infoSize = sizeof( RID_DEVICE_INFO);
    erg = GetRawInputDeviceInfo( dev.hDevice, RIDI_DEVICEINFO, &info, &infoSize);

    switch( dev.dwType )
    {
      case RIM_TYPEMOUSE: 
        // on many systems there are ghost mice for Remote Desktop programs. We filter those out.
        if( name.find( "RDP_") == std::string::npos )
        {
          try {
            auto m = new WinMouse( this, mDevices.size(), dev.hDevice);
            InputSystemHelper::AddDevice( m);
            mMice[dev.hDevice] = m;
          } catch( std::exception& )
          {
            // TODO: invent logging
          }
        }
        break;
      case RIM_TYPEKEYBOARD: 
        // there are also ghost keyboards from Remote desktop programs
        if( name.find( "RDP_") == std::string::npos )
        {
          try {
            auto k = new WinKeyboard( this, mDevices.size(), dev.hDevice, mKeyboard);
            InputSystemHelper::AddDevice( k);
            mKeyboards[dev.hDevice] = k;
          } catch( std::exception& )
          {
            // TODO: invent logging
          }
        }
        break;
      case RIM_TYPEHID: 
        // laut USB-Usage-Spec ist Page 1 (Generic Desktop Page), Usage 4 (Joysticks) und 5 (GamePads) unser Ziel
//        if( info.hid.usUsagePage == 1 && (info.hid.usUsage == 4 || info.hid.usUsage == 5) )
//          mRestPad.push_back( dev.hDevice);
        break;
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
int CALLBACK WinInput::_DIEnumDevCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	auto _this_ = static_cast<WinInput*> (pvRef);

	// Register only game devices (keyboard and mouse are managed differently).
  auto dt = GET_DIDEVICE_TYPE(lpddi->dwDevType);
	if( dt == DI8DEVTYPE_JOYSTICK || dt == DI8DEVTYPE_GAMEPAD || dt == DI8DEVTYPE_1STPERSON || 
      dt == DI8DEVTYPE_DRIVING || dt == DI8DEVTYPE_FLIGHT )
	{
    try {
      auto j = new WinJoystick( _this_, _this_->mDevices.size(), _this_->mDirectInput, lpddi->guidInstance, lpddi->guidProduct);
      InputSystemHelper::AddDevice( j);
    } catch( std::exception& )
    {
      // TODO: invent logging
    }
	}

	return DIENUM_CONTINUE;
}

// --------------------------------------------------------------------------------------------------------------------
void WinInput::CheckXInputDevices()
{
  IEnumWbemClassObject* pEnumDevices = nullptr;
  IWbemClassObject* pDevices[20] = {0};
  IWbemServices* pIWbemServices = nullptr;
  IWbemLocator* pIWbemLocator  = nullptr;
  BSTR bstrNamespace = nullptr, bstrDeviceID = nullptr, bstrClassName = nullptr;
  size_t padCount = 0; 

  // CoInit if needed
  HRESULT hr = CoInitialize(nullptr);
  bool bCleanupCOM = SUCCEEDED(hr);

  // Create WMI
  hr = CoCreateInstance(__uuidof(WbemLocator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IWbemLocator), (LPVOID*)&pIWbemLocator);
  if( FAILED(hr) || pIWbemLocator == nullptr )
    goto LCleanup;

  bstrNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );
  bstrClassName = SysAllocString( L"Win32_PNPEntity" );
  bstrDeviceID  = SysAllocString( L"DeviceID" );
  if( !bstrNamespace || !bstrClassName || !bstrDeviceID )
    goto LCleanup;

  // Connect to WMI 
  hr = pIWbemLocator->ConnectServer( bstrNamespace, nullptr, nullptr, 0L, 0L, nullptr, nullptr, &pIWbemServices );
  if( FAILED(hr) || pIWbemServices == nullptr )
    goto LCleanup;

  // Switch security level to IMPERSONATE. 
  CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE );                    

  hr = pIWbemServices->CreateInstanceEnum( bstrClassName, 0, nullptr, &pEnumDevices ); 
  if( FAILED(hr) || pEnumDevices == nullptr )
    goto LCleanup;

  // Loop over all devices
  for( ;; )
  {
    // Get 20 at a time
    DWORD uReturned = 0;
    hr = pEnumDevices->Next(5000, 20, pDevices, &uReturned);
    if( FAILED(hr) || uReturned == 0 )
      break;

    for( DWORD iDevice = 0; iDevice < uReturned; iDevice++ )
    {
      // For each device, get its device ID
      VARIANT var;
      hr = pDevices[iDevice]->Get(bstrDeviceID, 0L, &var, nullptr, nullptr);
      if( SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != nullptr )
      {
        // Check if the device ID contains "IG_".  If it does, then it's an XInput device - This information can not be found from DirectInput 
        if( wcsstr(var.bstrVal, L"IG_") )
        {
          // If it does, then get the VID/PID from var.bstrVal
          DWORD dwPid = 0, dwVid = 0;
          WCHAR* strVid = wcsstr( var.bstrVal, L"VID_" );
          if(strVid && swscanf_s( strVid, L"VID_%4X", &dwVid ) != 1)
            dwVid = 0;

          WCHAR* strPid = wcsstr( var.bstrVal, L"PID_" );
          if(strPid && swscanf_s( strPid, L"PID_%4X", &dwPid ) != 1)
            dwPid = 0;

          // Compare the VID/PID to the DInput device.
          DWORD dwVidPid = MAKELONG(dwVid, dwPid);
          for( auto d : mDevices )
          {
            if( auto j = static_cast<WinJoystick*> (d) )
            {
              if( !j->IsXInput() && dwVidPid == j->GetProductGuid().Data1 )
              {
                j->SetXInput( padCount++);
                break;
              }
            }
          }
        }
      }

      SAFE_RELEASE(pDevices[iDevice]);
    }
  }

LCleanup:
  if(bstrNamespace) SysFreeString(bstrNamespace);
  if(bstrDeviceID) SysFreeString(bstrDeviceID);
  if(bstrClassName) SysFreeString(bstrClassName);

  for(size_t iDevice = 0; iDevice < 20; iDevice++ )
    SAFE_RELEASE(pDevices[iDevice]);

  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pIWbemLocator);
  SAFE_RELEASE(pIWbemServices);

  if(bCleanupCOM)
    CoUninitialize();
}

// --------------------------------------------------------------------------------------------------------------------
void WinInput::RegisterForRawInput() 
{
  // register ourself to receive RawInput. For anything, we can't convert the findings above to something useful here.
  RAWINPUTDEVICE ids[3];
  ids[0].usUsagePage = 1; // generic desktop page, according to USB HID Usage Tables 1.12
  ids[0].usUsage = 6; // keyboard
  ids[0].hwndTarget = 0;
  ids[0].dwFlags = 0;

  ids[1].usUsagePage = 1; // generic desktop page
  ids[1].usUsage = 7; // keypads... whatever
  ids[1].hwndTarget = 0;
  ids[1].dwFlags = 0;

  ids[2].usUsagePage = 1; // generic desktop page
  ids[2].usUsage = 2; // mouse
  ids[2].hwndTarget = 0;
  ids[2].dwFlags = 0;

  if( !RegisterRawInputDevices( &ids[0], 3, sizeof( RAWINPUTDEVICE)) )
    throw std::runtime_error( "failed to register for input messages");
}

// --------------------------------------------------------------------------------------------------------------------
// Starts the update, to be called before handling system messages
void WinInput::Update()
{
  // Basis work
  InputSystem::Update();

  // begin updating all devices
  for( auto d : mDevices )
  {
    if( auto mouse = dynamic_cast<WinMouse*> (d) )
      mouse->StartUpdate();
    else if( auto keyboard = dynamic_cast<WinKeyboard*> (d) )
      keyboard->StartUpdate();
    else if( auto joy = dynamic_cast<WinJoystick*> (d) )
      joy->StartUpdate();
  }

  // read raw input 
  while( true )
  {
    uint8_t buf[800];
    // align it to 16 bytes. Probably useless, the actual bug was in the RAWINPUT structure itsself
    RAWINPUT* rib = (RAWINPUT*) (&buf[0] + ((16 - (size_t(&buf[0]) & 15)) & 15));
    uint32_t size = sizeof( buf) - 16;
    auto num = GetRawInputBuffer( rib, &size, sizeof( RAWINPUTHEADER));
    if( num == 0 )
      break;
    for( uint32_t a = 0; a < num; ++a )
    {
      if( rib->header.dwType == RIM_TYPEKEYBOARD )
      {
        auto it = mKeyboards.find( rib->header.hDevice);
        if( it != mKeyboards.end() )
          it->second->ParseMessage( *rib, mIsWorkAroundEnabled);
      }
      else if( rib->header.dwType == RIM_TYPEMOUSE )
      {
        auto it = mMice.find( rib->header.hDevice);
        if( it != mMice.end() )
          it->second->ParseMessage( *rib, mIsWorkAroundEnabled);
      }

      rib = NEXTRAWINPUTBLOCK( rib);
    }
  }

  // update postprocessing, currently mice only
  for( auto d : mDevices )
  {
    if( auto mouse = dynamic_cast<WinMouse*> (d) )
      mouse->EndUpdate();

    // from now on everything generates signals
    d->ResetFirstUpdateFlag();
  }
}

// --------------------------------------------------------------------------------------------------------------------
// Notifies the input system that the application has lost/gained focus.
void WinInput::InternSetFocus( bool pHasFocus)
{
  for( auto d : mDevices )
  {
    if( auto k = dynamic_cast<WinKeyboard*> (d) )
      k->SetFocus( mHasFocus);
    else if( auto m = dynamic_cast<WinMouse*> (d) )
      m->SetFocus( mHasFocus);
    else if( auto j = dynamic_cast<WinJoystick*> (d) )
      j->SetFocus( mHasFocus);
  }
}

// --------------------------------------------------------------------------------------------------------------------
void WinInput::InternSetMouseGrab( bool enabled)
{
  RECT rect;
  GetWindowRect( hWnd, &rect);
  POINT pt;
  if( enabled )
  {
    // if enabled, move system mouse to window center and start taking offsets from there
    pt = POINT{ (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
  } else
  {
    // if disabled, move mouse to last reported mouse position to achieve a smooth non-jumpy transition between the modes
    pt = POINT{ std::max( 0, std::min( int( rect.right - rect.left), GetMouseX())), std::max( 0, std::min( int( rect.bottom - rect.top), GetMouseY())) };
  }
  ClientToScreen( hWnd, &pt);
  SetCursorPos( pt.x, pt.y);
}

LRESULT CALLBACK WinInput::WndProcHook( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  auto that = reinterpret_cast<WinInput*> (SNIIS::gInstance);
  if( msg == WM_INPUT )
  {
    uint8_t buf[256];
    UINT sz = sizeof( buf);
    sz = GetRawInputData( (HRAWINPUT) lparam, RID_INPUT, buf, &sz, sizeof( RAWINPUTHEADER));
    if( sz != std::numeric_limits<decltype(sz)>::max() )
    {
      auto inp = reinterpret_cast<const RAWINPUT*> (&buf[0]);
      if( inp->header.dwType == RIM_TYPEKEYBOARD )
      {
        auto it = that->mKeyboards.find( inp->header.hDevice);
        if( it != that->mKeyboards.end() )
          it->second->ParseMessage( *inp, false);
      }
      else if( inp->header.dwType == RIM_TYPEMOUSE )
      {
        auto it = that->mMice.find( inp->header.hDevice);
        if( it != that->mMice.end() )
          it->second->ParseMessage( *inp, false);
      }
    }
    return 0;
  }
  else
  {
    return CallWindowProcW( that->mPreviousWndProc, hwnd, msg, wparam, lparam);
  }
}

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// Initializes the input system with the given InitArgs. When successful, gInstance is not Null.
bool InputSystem::Initialize(void* pInitArg)
{
  if( gInstance )
    throw std::runtime_error( "Input already initialized");

  try 
  {
    gInstance = new WinInput( (HWND) pInitArg);
  } catch( std::exception& )
  {
    // nope
    gInstance = nullptr;
    return false;
  }

  return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Destroys the input system. After returning gInstance is Null again
void InputSystem::Shutdown()
{
  delete gInstance;
  gInstance = nullptr;
}

#endif // SNIIS_SYSTEM_WINDOWS