/// @file SNIIS_Intern.h
/// Internal helper functions which are platform-agnostic but should not be part of the public interface
#pragma once

#include "SNIIS.h"

namespace SNIIS
{
  /// Platform-agnostic helper functions
  struct InputSystemHelper
  {
    /// The optional 'assembled' parameter names an assembled device whose state changed along with the sender's.
    /// Handler callbacks are only invoked once, with the sender; channels sourced from either device are updated together.
    static void AddDevice( Device* dev);
    static void DoMouseButton( Mouse* sender, size_t btnIndex, bool isPressed, Device* assembled = nullptr);
    static void DoMouseMove( Mouse* sender, float absx, float absy, float relx, float rely);
    static void DoMouseWheel(Mouse* sender, float diff, Device* assembled = nullptr);
    static void DoKeyboardButton( Keyboard* sender, KeyCode kc, size_t unicode, bool isPressed, Device* assembled = nullptr);
    static void DoKeyboardButtonIntern( Keyboard* sender, KeyCode kc, size_t unicode, bool isPressed, Device* assembled = nullptr);
    static void DoJoystickAxis( Joystick* sender, size_t axisIndex, float value);
    static void DoJoystickButton( Joystick* sender, size_t btnIndex, bool isPressed);
    static void DoDigitalEvent( Device* sender, size_t btnIndex, bool isPressed, Device* assembled = nullptr);
    static void DoAnalogEvent( Device* sender, size_t axisIndex, float value, Device* assembled = nullptr);
    static void UpdateChannels( Device* sender, size_t ctrlIndex, bool isAnalog, Device* assembled = nullptr);
  };
}
