#ifndef SNIIS_MAC_HELPER_H
#define SNIIS_MAC_HELPER_H

/// Window id, or AnyID in ObjC. GLFW does it like this
typedef void* id;

/// Helper stuff in need of ObjectiveC calls. Grmpf.
struct Pos { float x, y; };
struct WindowRect { float x, y, w, h; };

#if __cplusplus
extern "C" {
#endif

struct WindowRect MacHelper_GetWindowRect( id wid);
struct Pos MacHelper_GetMousePos();
void MacHelper_SetMousePos( struct Pos p);
struct Pos MacHelper_WinToDisplay( id wid, struct Pos p);
struct Pos MacHelper_DisplayToWin( id wid, struct Pos p);

#if __cplusplus
}
#endif

#endif // SNIIS_MAC_HELPER_H
