/// @file SNIIS_Mac.cpp
/// Mac OSX implementation of input system

#include "SNIIS_Mac.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_MAC
using namespace SNIIS;

// --------------------------------------------------------------------------------------------------------------------
// Constructor
MacInput::MacInput(id pWindowId)
{
  mWindow = pWindowId;

  // create the manager
  mHidManager = IOHIDManagerCreate( kCFAllocatorDefault, 0);
  if( !mHidManager )
    throw std::runtime_error( "Failed to create HIDManager");

  // tell 'em we want it all
  IOHIDManagerSetDeviceMatching( mHidManager, nullptr);
  // register our enumeration callback
  IOHIDManagerRegisterDeviceMatchingCallback( mHidManager, &MacInput::HandleNewDeviceCallback, (void*) this);
  // register us for running the event loop
  IOHIDManagerScheduleWithRunLoop( mHidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

  // and open the manager, enumerating all devices along with it
  IOReturn res = IOHIDManagerOpen( mHidManager, 0);
  Log( "IOHIDManagerOpen() returned %d", res);
  if( res != kIOReturnSuccess )
    throw std::runtime_error( "Failed to open HIDManager / enumerate devices");

  // run the update loop to get the callbacks for new devices
  while( CFRunLoopRunInMode( kCFRunLoopDefaultMode, 0, TRUE) == kCFRunLoopRunHandledSource )
    /**/;

  // remove the manager from the callbacks and runloop
  IOHIDManagerRegisterDeviceMatchingCallback( mHidManager, nullptr, nullptr);
  // Since some OSX update the Unschedule() thingy also unschedules all devices, so we never get any event notifications
  // simply leaving it be should be fine, as we unregistered the callback
//  IOHIDManagerUnscheduleFromRunLoop( mHidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
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
// Updates the inputs, to be called before handling system messages
void MacInput::Update()
{
  // Basis work
  InputSystem::Update();

  // device work
  for( size_t a = 0; a < mMacDevices.size(); ++a )
  {
    MacDevice* d = mMacDevices[a];
    d->StartUpdate();
  }

  // then run the update loop
  while( CFRunLoopRunInMode( kCFRunLoopDefaultMode, 0, TRUE) == kCFRunLoopRunHandledSource )
    /**/;

  // Mice need postprocessing
  for( auto d : mDevices )
  {
    if( auto m = dynamic_cast<MacMouse*> (d) )
      m->EndUpdate();

    // from now on everything generates signals
    d->ResetFirstUpdateFlag();
  }
}

// --------------------------------------------------------------------------------------------------------------------
// Notifies the input system that the application has lost/gained focus.
void MacInput::InternSetFocus( bool pHasFocus)
{
  for( auto d : mMacDevices )
    d->SetFocus( pHasFocus);
}

// --------------------------------------------------------------------------------------------------------------------
void MacInput::InternSetMouseGrab( bool enabled)
{
  auto wr = MacHelper_GetWindowRect( mWindow);
  Pos p;
  if( enabled )
  {
    // if enabled, move system mouse to window center and start taking offsets from there
    p.x = wr.w / 2; p.y = wr.h / 2;
  } else
  {
    // if disabled, move mouse to last reported mouse position to achieve a smooth non-jumpy transition between the modes
    p.x = std::max( 0.0f, std::min( wr.w, float( GetMouseX())));
    p.y = std::max( 0.0f, std::min( wr.h, float( GetMouseY())));
  }
  Pos gp = MacHelper_WinToDisplay( mWindow, p);
  MacHelper_SetMousePos( gp);
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

  // get the names
  auto cfstr = (CFStringRef) IOHIDDeviceGetProperty( device, CFSTR(kIOHIDProductKey));
  auto cfstr2 = (CFStringRef) IOHIDDeviceGetProperty( device, CFSTR(kIOHIDManufacturerKey));

  std::vector<char> tmp( 500, 0);
  if( cfstr )
    CFStringGetCString( cfstr, &tmp[0], tmp.size(), kCFStringEncodingUTF8);
  else
    sprintf( &tmp[0], "(null)");

  size_t l = strlen( tmp.data());
  tmp[l++] = '|';
  if( cfstr2 )
    CFStringGetCString( cfstr2, &tmp[l], tmp.size() - l, kCFStringEncodingUTF8);
  else
    sprintf( &tmp[l], "(null)");

  Log( "HandleNewDevice \"%s\", page %d, usage %d", tmp.data(), usepage, usage);

  if( usepage != kHIDPage_GenericDesktop )
    return;

  Log( "New device \"%s\" at page %d, usage %d", tmp.data(), usepage, usage);

  // extra ugly: store last mouse because we might have to add the second trackpad to it
  static MacMouse* lastMouse = nullptr;
  switch( usage )
  {
    case kHIDUsage_GD_Mouse:
    case kHIDUsage_GD_Pointer:
    {
      try {
        bool isTrackpad = strncmp( tmp.data(), "Apple Internal Keyboard / Trackpad", 34) == 0;
        if( isTrackpad && lastMouse && lastMouse->IsTrackpad() )
        {
          Log( "-> second HID of internal trackpad, adding to Mouse %d (id %d)", lastMouse->GetCount(), lastMouse->GetId());
          lastMouse->AddDevice( device);
        }
        else
        {
          Log( "-> Mouse %d (id %d)", mNumMice, mDevices.size());
          auto m = new MacMouse( this, mDevices.size(), device, isTrackpad);
          InputSystemHelper::AddDevice( m);
          mMacDevices.push_back( m);
          if( isTrackpad )
            lastMouse = m;
        }
      } catch( std::exception& e)
      {
        Log( "Exception: %s", e.what());
      }
      break;
    }

    case kHIDUsage_GD_Keyboard:
    case kHIDUsage_GD_Keypad:
    {
      try {
        Log( "-> Keyboard %d (id %d)", mNumKeyboards, mDevices.size());
        auto k = new MacKeyboard( this, mDevices.size(), device);
        InputSystemHelper::AddDevice( k);
        mMacDevices.push_back( k);
      } catch( std::exception& e)
      {
        Log( "Exception: %s", e.what());
      }
      break;
    }

    case kHIDUsage_GD_Joystick:
    case kHIDUsage_GD_GamePad:
    case kHIDUsage_GD_MultiAxisController:
    {
      try {
        Log( "-> Controller %d (id %d)", mNumJoysticks, mDevices.size());
        auto j = new MacJoystick( this, mDevices.size(), device);
        InputSystemHelper::AddDevice( j);
        mMacDevices.push_back( j);
      } catch( std::exception& e)
      {
        Log( "Exception: %s", e.what());
      }
      break;
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
std::vector<MacControl> EnumerateDeviceControls( IOHIDDeviceRef devref)
{
  // enumerate all controls of that device
  std::vector<MacControl> controls;
  CFArrayRef elements = IOHIDDeviceCopyMatchingElements( devref, nullptr, kIOHIDOptionsTypeNone);
  for( size_t a = 0, count = CFArrayGetCount( elements); a < count; ++a )
  {
    auto elmref = (IOHIDElementRef) CFArrayGetValueAtIndex( elements, CFIndex( a));
    auto type = IOHIDElementGetType( elmref);
    auto usepage = IOHIDElementGetUsagePage( elmref), usage = IOHIDElementGetUsage( elmref);

    size_t prevSize = controls.size();
    if( type == kIOHIDElementTypeInput_Axis || type == kIOHIDElementTypeInput_Misc )
    {
      auto min = IOHIDElementGetLogicalMin( elmref), max = IOHIDElementGetLogicalMax( elmref);
      if( usage == kHIDUsage_GD_Hatswitch )
      {
        controls.push_back( MacControl{ devref, MacControl::Type_Hat, "", 0, usepage, usage, min, max });
        controls.push_back( MacControl{ devref, MacControl::Type_Hat_Second, "", 0, usepage, usage, min, max });
      }
      else
      {
        controls.push_back( MacControl{ devref, MacControl::Type_Axis, "", 0, usepage, usage, min, max });
      }
    }
    else if( type == kIOHIDElementTypeInput_Button )
    {
      controls.push_back( MacControl{ devref, MacControl::Type_Button, "", 0, usepage, usage, 0, 1 });
    }

    // add a few things afterwards if we got new controls
    for( size_t a = prevSize; a < controls.size(); ++a )
    {
      controls[a].mCookie = IOHIDElementGetCookie( elmref);
      auto name = IOHIDElementGetName( elmref);
      if( name )
      {
        std::vector<char> tmp( 500, 0);
        CFStringGetCString( name, &tmp[0], tmp.size(), kCFStringEncodingUTF8);
        controls[a].mName = &tmp[0];
      }
    }
  }

  return controls;
}

void InputElementValueChangeCallback( void* ctx, IOReturn res, void* sender, IOHIDValueRef val)
{
  SNIIS_UNUSED( sender);
  if( res != kIOReturnSuccess )
    return;
  //also ignore all events if we ain't focus'd. Did I use that word correctly?
  if( !gInstance->HasFocus() )
    return;

  auto dev = static_cast<MacDevice*> (ctx);
  auto elm = IOHIDValueGetElement( val);
  auto keksie = IOHIDElementGetCookie( elm);
  auto value = IOHIDValueGetIntegerValue( val);
  auto usepage = IOHIDElementGetUsagePage( elm), usage = IOHIDElementGetUsage( elm);
  auto hiddev = IOHIDElementGetDevice( elm);
  dev->HandleEvent( hiddev, keksie, usepage, usage, value);
}

std::pair<float, float> ConvertHatToAxes( long min, long max, long value)
{
  std::pair<float, float> axes;
  // Hats deliver a value that starts at North (x=0, y=-1) and goes a full circle clockwise within the value range
  // We need to decompose this into two axes from roughly 8 standard cases with a little safety each
  float v = float( value - min) / float( max - min);
  if( v > 0.1f && v < 0.4f )
    axes.first = 1.0f;
  else if( v > 0.6f && v < 0.9f )
    axes.first = -1.0f;
  else
    axes.first = 0.0f;

  if( v < 0.15f || v > 0.85f )
    axes.second = -1.0f;
  else if( v > 0.35f && v < 0.65f )
    axes.second = 1.0f;
  else
    axes.second = 0.0f;

  return axes;
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
    gInstance = new MacInput( pInitArg);
  } catch( std::exception& e)
  {
    // nope
    if( gLogCallback )
      gLogCallback( (std::string( "Exception while creating SNIIS instance: ") + e.what()).c_str());
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

