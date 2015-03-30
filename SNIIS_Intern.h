/// @file SNIIS_Intern.h
/// Internal helper functions which are platform-agnostic but should not be part of the public interface
#pragma once

#include "SNIIS.h"

namespace SNIIS
{
  /// Platform-agnostic base implementation of InputSystem
  class BaseInputSystem : public InputSystem
  {
  };

  /// Platform-agnostic helper functions
  struct InputSystemHelper
  {
    static void AddDevice( Device* dev);
    static void DoMouseButton( Mouse* sender, size_t btnIndex, bool isPressed);
    static void DoMouseMove( Mouse* sender, int absx, int absy, int relx, int rely);
    static void DoKeyboardButton( Keyboard* sender, KeyCode kc, size_t unicode, bool isPressed);
    static void DoJoystickAxis( Joystick* sender, size_t axisIndex, float value);
    static void DoJoystickButton( Joystick* sender, size_t btnIndex, bool isPressed);
    static void DoDigitalEvent( Device* sender, size_t btnIndex, bool isPressed);
    static void DoAnalogEvent( Device* sender, size_t axisIndex, float value);
    static void MakeThisMouseFirst( Mouse* mouse);
    static void MakeThisKeyboardFirst( Keyboard* keyboard);
  };
}