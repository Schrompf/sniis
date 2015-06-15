/// @file SNIIS_Mac_Helper.m
/// Helper stuff for the Mac implementation which require ObjectiveC calls.

#include "SNIIS_Mac_Helper.h"
#import <Cocoa/Cocoa.h>

struct WindowRect MacHelper_GetWindowRect(id wid)
{
  const NSRect r = [wid frame];
  struct WindowRect wr = { r.origin.x, r.origin.y, r.size.width, r.size.height };
  return wr;
}

struct Pos MacHelper_GetMousePos()
{
  // fetch the mouse position - the internet tells me to do it like this
  CGEventRef event = CGEventCreate(nil);
  CGPoint loc = CGEventGetLocation(event);
  CFRelease(event);
  struct Pos p = { loc.x, loc.y };
  return p;
}

void MacHelper_SetMousePos( struct Pos p)
{
  CGWarpMouseCursorPosition( CGPointMake( p.x, p.y));
  // http://stackoverflow.com/questions/8215413/why-is-cgwarpmousecursorposition-causing-a-delay-if-it-is-not-what-is
  CGAssociateMouseAndMouseCursorPosition( true);
}

struct Pos MacHelper_WinToDisplay( id wid, struct Pos p)
{
  struct Pos nop = { -10000.0f, -10000.0f };
  const NSRect r = [wid frame];
  if( r.size.width < 1.0f || r.size.height < 1.0f )
    return nop;

  const NSRect localPoint = NSMakeRect( p.x, r.size.height - p.y - 1, 1, 1);
  const NSRect globalPoint = [wid convertRectToScreen:localPoint];

  const float height = CGDisplayBounds( CGMainDisplayID()).size.height;
  struct Pos rp = { globalPoint.origin.x, height - globalPoint.origin.y };
  return rp;
}

struct Pos MacHelper_DisplayToWin( id wid, struct Pos p)
{
  struct Pos nop = { -10000.0f, -10000.0f };
  const NSRect r = [wid frame];
  if( r.size.width < 1.0f || r.size.height < 1.0f )
    return nop;

  const float h = CGDisplayBounds( CGMainDisplayID()).size.height;
  const NSPoint gp = NSMakePoint( p.x, h - p.y);
  const NSPoint lp = [wid convertScreenToBase:gp];

  struct Pos rp = { lp.x, r.size.height - lp.y - 1 };
  return rp;
}
