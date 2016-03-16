/// @file SNIIS_Linux.cpp
/// Linux implementation of input

#include "SNIIS_Linux.h"
#include "SNIIS_Intern.h"

#if SNIIS_SYSTEM_LINUX
using namespace SNIIS;

#include <cstring>
#include <sstream>
#include <fcntl.h>
#include <linux/input.h>

static bool IsBitSet( const uint8_t* bits, size_t i) { return (bits[i/8] & (1<<(i&7))) != 0; }

// --------------------------------------------------------------------------------------------------------------------
// Constructor
LinuxInput::LinuxInput( Window wnd)
{
  mWindow = wnd;
  mDisplay = nullptr;

  mDisplay = XOpenDisplay( nullptr);
  if( !mDisplay )
    throw std::runtime_error( "Failed to open XDisplay");

  int event = 0, error = 0;
  int major = 2, minor = 0;
  bool isAvailable = (XQueryExtension( mDisplay, "XInputExtension", &mXiOpcode, &event, &error) != False
               && XIQueryVersion( mDisplay, &major, &minor) != BadRequest);
  if( !isAvailable )
    throw std::runtime_error( "Failed to get XInputExtension");

  // Register for events
  XIEventMask evmask;
  uint8_t mask[] = { 0, 0, 0, 0 };
  XISetMask( mask, XI_HierarchyChanged);
  XISetMask( mask, XI_RawMotion);
  XISetMask( mask, XI_RawButtonPress);
  XISetMask( mask, XI_RawButtonRelease);
  XISetMask( mask, XI_RawKeyPress);
  XISetMask( mask, XI_RawKeyRelease);

  evmask.deviceid = XIAllDevices;
  evmask.mask_len = sizeof( mask);
  evmask.mask = mask;
  auto selwnd = DefaultRootWindow( mDisplay);
  if( XISelectEvents( mDisplay, selwnd, &evmask, 1) != 0 )
    throw std::runtime_error( "Failed to register for XInput2 events");

  int deviceCount = 0;
  XIDeviceInfo* devices = XIQueryDevice( mDisplay, XIAllDevices, &deviceCount);
  for( int i = 0; i < deviceCount; i++ )
  {
    const auto& dev = devices[i];
    /// We only look at "slave" devices. "Master" pointers are the logical cursors, "slave" pointers are the hardware
    /// that back them. "Floating slaves" are hardware that don't back a cursor.
    if( dev.use != XISlavePointer && dev.use != XISlaveKeyboard && dev.use != XIFloatingSlave )
      continue;
    // Ignore some common pffft cases
    if( strstr(devices[i].name, "XTEST") != nullptr )
      continue;

    // Turns out the use field is unreliable. I got reports from keyboards being reported as mice because they back
    // a pointer. I also got reports from mice exposing "keys", and XInput always reports >248 keys if at least one key
    // is present. So I see no other options but to register some devices as a mouse AND a keyboard.
    size_t numButtons = 0, numAxes = 0, numKeys = 0;
    bool isAxisPresent[2] = { false, false };
    for( int a = 0; a < dev.num_classes; ++a )
    {
      auto cl = dev.classes[a];
      switch( cl->type )
      {
        case XIButtonClass:
        {
          auto bcl = reinterpret_cast<const XIButtonClassInfo*> (cl);
          numButtons += bcl->num_buttons;
          break;
        }
        case XIKeyClass:
        {
          // keys are most probably on a keyboard. I've never seen less than 248 num_keycodes; I wonder what made them do this.
          auto kcl = reinterpret_cast<const XIKeyClassInfo*> (cl);
          numKeys += kcl->num_keycodes;
          break;
        }
        case XIValuatorClass:
        {
          // an axis
          auto vcl = reinterpret_cast<const XIValuatorClassInfo*> (cl);
          if( vcl->number < 2 )
            isAxisPresent[vcl->number] = true;
          numAxes++;
          break;
        }
        case XIScrollClass:
        {
          // probably a scroll wheel - might be on a mouse or a keyboard
          numAxes++;
          break;
        }
      }
    }

    static const char* sTypeName[6] = {
      "Invalid", "XIMasterPointer", "XIMasterKeyboard", "XISlavePointer", "XISlaveKeyboard", "XIFloatingSlave"
    };
    Log( "Input device of type %d - \"%s\" - %d axes, %d buttons, %d keys",
      dev.use < 6 ? sTypeName[dev.use] : "Unknown", devices[i].name, numAxes, numButtons, numKeys);

    // A mouse has at least two absolute axes, unfortunately we have no means to know if those are X and Y axes.
    // A mouse also has a few buttons, maybe a dozen at max.
    if( isAxisPresent[0] && isAxisPresent[1] )
    {
      Log( "-> register this as mouse %d (id %d)", mNumMice, mDevices.size());
      try {
        auto m = new LinuxMouse( this, mDevices.size(), devices[i]);
        InputSystemHelper::AddDevice( m);
        mMiceById[devices[i].deviceid] = m;
      } catch( std::exception& e)
      {
        Log( "Exception: %s", e.what());
      }
    }

    // A keyboard on the other hand has keys, but might also feature a few axes. So register a device as both if necessary.
    if( numKeys > 0 )
    {
      Log( "-> register this as keyboard %d (id %d)", mNumKeyboards, mDevices.size());
      try {
        auto k = new LinuxKeyboard( this, mDevices.size(), devices[i]);
        InputSystemHelper::AddDevice( k);
        mKeyboardsById[devices[i].deviceid] = k;
      } catch( std::exception& e)
      {
        Log( "Exception: %s", e.what());
      }
    }
  }

  XIFreeDeviceInfo( devices);

  // use a completely different API for controllers, because XInput would be perfectly capable of supporting
  // those, too, but refuses to do so. It enumerates my USB headset as a keyboard, but it does not expose
  // my XBox controller. Sometimes I wish to look into the coders' minds and learn what possessed them when
  // designing x separate APIs for the same purpose, but each with a different set of flaws.
	for( size_t a = 0; a < 64; ++a )
	{
		std::stringstream eventPath;
		eventPath << "/dev/input/event" << a;
		int fd = open( eventPath.str().c_str(), O_RDWR |O_NONBLOCK );
		if( fd == -1 )
			continue;

    char tmp[256] = "Unknown";
    if( ioctl( fd, EVIOCGNAME( sizeof( tmp)), tmp) < 0)
      throw std::runtime_error( "Could not read device name");

    Log( "Controller %d (id %d) - \"%s\"", mNumJoysticks, mDevices.size(), &tmp[0]);

    // check if it's a controller. If we're started with root privileges, we'd get mice and keyboards here, too,
    // but we can't rely on it, so we sort those out and only use it for controllers.
    // (Side note: mice and keyboards are root-only because you'd otherwise be able to write a keylogger with it.
    // It will take a few years until someone will notice that on SteamOS you enter your credit card credentials
    // and passwords with a controller. Then it's a viable approach to also spy on the controller, so they'll
    // probably make the controllers root-only, too, and this whole mess finally ends up
    // in the "Linux Hall Of Self-Digged Graves" where it belongs. Sorry for the rant.)
    uint8_t ev_bits[(EV_MAX+7)/8];
    memset( ev_bits, 0, sizeof(ev_bits) );

    //Read "all" (hence 0) components of the device
    if( ioctl( fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) == -1 )
      throw std::runtime_error( "Could not read device events features");

    if( IsBitSet( ev_bits, EV_KEY) )
    {
      uint8_t key_bits[(KEY_MAX+7)/8];
      memset( key_bits, 0, sizeof(key_bits) );

      if( ioctl( fd, EVIOCGBIT( EV_KEY, sizeof(key_bits)), key_bits) == -1 )
        throw std::runtime_error( "Could not read device buttons features");

      bool isController = false;
      for( size_t b = 0; b < KEY_MAX; b++ )
      {
        if( IsBitSet( key_bits, b) )
        {
          // Check to ensure we find at least one joy only button
          if( (b >= BTN_JOYSTICK && b < BTN_GAMEPAD) || (b >= BTN_GAMEPAD && b < BTN_DIGI) || (b >= BTN_WHEEL && b < KEY_OK) )
            isController = true;
        }
      }

      if( isController )
      {
        try {
          auto j = new LinuxJoystick( this, mDevices.size(), fd);
          InputSystemHelper::AddDevice( j);
        } catch( std::exception& e)
        {
          Log( "Exception: %s", e.what());
        }
      } else
      {
        close( fd);
      }
    }
	}
}

// --------------------------------------------------------------------------------------------------------------------
// Destructor
LinuxInput::~LinuxInput()
{
  for( auto d : mDevices )
    delete d;

  if( mDisplay != nullptr )
    XCloseDisplay( mDisplay);
}

// --------------------------------------------------------------------------------------------------------------------
// Updates the inputs, to be called before handling system messages
void LinuxInput::Update()
{
  // Basis work
  InputSystem::Update();

  // begin updating all devices
  for( auto d : mDevices )
  {
    if( auto mouse = dynamic_cast<LinuxMouse*> (d) )
      mouse->StartUpdate();
    else if( auto keyboard = dynamic_cast<LinuxKeyboard*> (d) )
      keyboard->StartUpdate();
    else if( auto joy = dynamic_cast<LinuxJoystick*> (d) )
      joy->StartUpdate();
  }

  // process XEvents
  XEvent event;
	while( XPending( mDisplay) > 0 )
	{
		XNextEvent( mDisplay, &event);

    if( event.xcookie.type != GenericEvent )
      continue;
    else if( event.xcookie.extension != mXiOpcode )
      continue;
    else if( !XGetEventData( mDisplay, &event.xcookie) )
      continue;

    switch( event.xcookie.evtype )
    {
      case XI_RawMotion:
      case XI_RawButtonPress:
      case XI_RawButtonRelease:
      {
        auto rawev = *((const XIRawEvent *) event.xcookie.data);
        auto mit = mMiceById.find( rawev.deviceid);
        if( mit == mMiceById.end() )
          break;
        mit->second->HandleEvent( rawev);
        break;
      }

      case XI_RawKeyPress:
      case XI_RawKeyRelease:
      {
        auto rawev = *((const XIRawEvent *) event.xcookie.data);
        auto kit = mKeyboardsById.find( rawev.deviceid);
        if( kit == mKeyboardsById.end() )
          break;
        kit->second->HandleEvent( rawev);
        break;
      }
    }

    XFreeEventData( mDisplay, &event.xcookie);
  }

  // update postprocessing
  for( auto d : mDevices )
  {
    if( auto mouse = dynamic_cast<LinuxMouse*> (d) )
      mouse->EndUpdate();

    // from now on everything generates signals
    d->ResetFirstUpdateFlag();
  }
}

// --------------------------------------------------------------------------------------------------------------------
// Notifies the input system that the application has lost/gained focus.
void LinuxInput::InternSetFocus( bool pHasFocus)
{
  for( auto d : mDevices )
  {
    if( auto k = dynamic_cast<LinuxKeyboard*> (d) )
      k->SetFocus( mHasFocus);
    else if( auto m = dynamic_cast<LinuxMouse*> (d) )
      m->SetFocus( mHasFocus);
    else if( auto j = dynamic_cast<LinuxJoystick*> (d) )
      j->SetFocus( mHasFocus);
  }
}

// --------------------------------------------------------------------------------------------------------------------
void LinuxInput::InternSetMouseGrab( bool enabled)
{
  // The Linux XInput2 API is nice to already provide mouse acceleration and all, so all that mouse coordinate cheating
  // is simply not necessary. Just grab the mouse and be happy
  if( enabled )
  {
    XGrabPointer( mDisplay, mWindow, True, 0, GrabModeAsync, GrabModeAsync, mWindow, None, CurrentTime);
  } else
  {
    XUngrabPointer( mDisplay, CurrentTime);
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
    gInstance = new LinuxInput( (Window) pInitArg);
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

#endif // SNIIS_SYSTEM_LINUX
