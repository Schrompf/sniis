/// @file SNIIS_Mac.cpp
/// Mac OSX implementation of input system

#include "SNIIS_Mac.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_MAC
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
// Constructor
MacInput::MacInput()
{
  // create the manager
  mHidManager = IOHIDManagerCreate( kCFAllocatorDefault, 0);
  if( !mHidManager )
    throw std::runtime_error( "Failed to create HIDManager");

  // register our enumeration callback
  IOHIDManagerRegisterDeviceMatchingCallback( mHidManager, &MacInput::HandleNewDeviceCallback, (void*) this);
  // register us for running the event loop
  IOHIDManagerScheduleWithRunLoop( mHidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
  // and open the manager, enumerating all devices along with it
  IOReturn res = IOHIDManagerOpen( mHidManager, 0);
  if( res != kIOReturnSuccess )
    throw std::runtime_error( "Failed to open HIDManager / enumerate devices");
}

// --------------------------------------------------------------------------------------------------------------------
// Destructor
MacInput::~MacInput()
{
  for( auto d : mDevices )
    delete d;

  if( mHidManager )
    IOHIDManagerClose( mHidManager, 0);
  CFRelease( mHidManager);
}

// --------------------------------------------------------------------------------------------------------------------
// Starts the update, to be called before handling system messages
void MacInput::StartUpdate()
{
  // Basis work
  InputSystem::StartUpdate();

  // device work
  for( auto d : mMacDevices )
    d->StartUpdate();
}

// --------------------------------------------------------------------------------------------------------------------
// Ends the update, to be called after handling system messages
void MacInput::EndUpdate()
{
  // base work
  InputSystem::EndUpdate();

}

// --------------------------------------------------------------------------------------------------------------------
// Notifies the input system that the application has lost/gained focus.
void MacInput::SetFocus( bool pHasFocus)
{
  if( pHasFocus == mHasFocus )
    return;

  mHasFocus = pHasFocus;
}

// --------------------------------------------------------------------------------------------------------------------
// Callback for each enumerated device
void MacInput::HandleNewDeviceCallback( void* context, IOReturn result, void* sender, IOHIDDeviceRef device)
{
  SNIIS_UNUSED( sender);
  if( result == kIOReturnSuccess )
  {
    auto inp = reinterpret_cast<MacInput*> (context);
    inp->HandleNewDevice( device);
  }
}

// --------------------------------------------------------------------------------------------------------------------
// Handle a newly detected device
void MacInput::HandleNewDevice( IOHIDDeviceRef device)
{
  // get usage page and usage
  int32_t usepage = 0, usage = 0;
  auto ref = IOHIDDeviceGetProperty( device, CFSTR( kIOHIDPrimaryUsagePageKey));
  if( ref )
    CFNumberGetValue( (CFNumberRef) ref, kCFNumberSInt32Type, &usepage);
  ref = IOHIDDeviceGetProperty( device, CFSTR( kIOHIDPrimaryUsageKey));
  if( ref )
    CFNumberGetValue( (CFNumberRef) ref, kCFNumberSInt32Type, &usage);

  if( usepage != kHIDPage_GenericDesktop )
    return;

  // get the names
  auto cfstr = (CFStringRef) IOHIDDeviceGetProperty( device, CFSTR(kIOHIDProductKey));
  auto cfstr2 = (CFStringRef) IOHIDDeviceGetProperty( device, CFSTR(kIOHIDManufacturerKey));

  std::vector<char> tmp( 500, 0);
  CFStringGetCString( cfstr, &tmp[0], tmp.size(), kCFStringEncodingUTF8);
  size_t l = strlen( tmp.data());
  tmp[l++] = '|';
  CFStringGetCString( cfstr2, &tmp[l], tmp.size() - l, kCFStringEncodingUTF8);

  switch( usage )
  {
    case kHIDUsage_GD_Mouse:
    case kHIDUsage_GD_Pointer:
    {
      auto m = new MacMouse( this, mDevices.size(), device);
      InputSystemHelper::AddDevice( m);
      mMacDevices.push_back( m);
      break;
    }

    case kHIDUsage_GD_Keyboard:
    case kHIDUsage_GD_Keypad:
    {
      auto k = new MacKeyboard( this, mDevices.size(), device);
      InputSystemHelper::AddDevice( k);
      mMacDevices.push_back( k);
      break;
    }

    case kHIDUsage_GD_Joystick:
    case kHIDUsage_GD_GamePad:
    case kHIDUsage_GD_MultiAxisController:
    {
      auto j = new MacJoystick( this, mDevices.size(), device);
      InputSystemHelper::AddDevice( j);
      mMacDevices.push_back( j);
      break;
    }
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
    gInstance = new MacInput;
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

#endif // SNIIS_SYSTEM_MAC
