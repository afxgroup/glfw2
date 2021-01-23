//========================================================================
// GLFW - An OpenGL framework
// Platform:    Any
// API version: 2.7
// WWW:         http://www.glfw.org/
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2010 Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#ifndef _platform_h_
#define _platform_h_

// Include files
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/timer.h>
#include <proto/keymap.h>
#include <proto/utility.h>

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <graphics/displayinfo.h>
#include <graphics/rastport.h>
#include <devices/timer.h>
#include <devices/keymap.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <devices/gameport.h>

#include <proto/minigl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#define MIN_MINIGLVERSION 2

// GLFW+GL+GLU defines
#include "../../include/GL/glfw.h"

// Stack size for each thread (in bytes)
#define _GLFW_TASK_STACK_SIZE 500000

//========================================================================
// Global variables (GLFW internals)
//========================================================================

//------------------------------------------------------------------------
// Shared libraries
//------------------------------------------------------------------------

extern struct Library *DOSBase;
extern struct DOSIFace *IDOS;

struct GraphicsIFace *IGraphics;
struct Library *GfxBase;

struct IntuitionIFace *IIntuition;
struct Library *IntuitionBase;

struct KeymapIFace *IKeymap;
struct Library *KeymapBase;

struct UtilityIFace *IUtility;
struct Library *UtilityBase;

struct Library *MiniGLBase;
struct MiniGLIFace *IMiniGL;

struct Device *TimerBase;
struct TimerIFace *ITimer;

typedef unsigned long GLFWintptr;

//------------------------------------------------------------------------
// Window structure
//------------------------------------------------------------------------
typedef struct _GLFWwin_struct _GLFWwin;

struct _GLFWwin_struct
{
    // ========= PLATFORM INDEPENDENT MANDATORY PART =========================

    // User callback functions
    GLFWwindowsizefun    windowSizeCallback;
    GLFWwindowclosefun   windowCloseCallback;
    GLFWwindowrefreshfun windowRefreshCallback;
    GLFWmousebuttonfun   mouseButtonCallback;
    GLFWmouseposfun      mousePosCallback;
    GLFWmousewheelfun    mouseWheelCallback;
    GLFWkeyfun           keyCallback;
    GLFWcharfun          charCallback;

    // User selected window settings
    int       fullscreen;      // Fullscreen flag
    int       mouseLock;       // Mouse-lock flag
    int       autoPollEvents;  // Auto polling flag
    int       sysKeysDisabled; // System keys disabled flag
    int       windowNoResize;  // Resize- and maximize gadgets disabled flag
    int       refreshRate;     // Vertical monitor refresh rate

    // Window status & parameters
    int       opened;          // Flag telling if window is opened or not
    int       active;          // Application active flag
    int       iconified;       // Window iconified flag
    int       width, height;   // Window width and heigth
    int       accelerated;     // GL_TRUE if window is HW accelerated

    // Framebuffer attributes
    int       redBits;
    int       greenBits;
    int       blueBits;
    int       alphaBits;
    int       depthBits;
    int       stencilBits;
    int       accumRedBits;
    int       accumGreenBits;
    int       accumBlueBits;
    int       accumAlphaBits;
    int       auxBuffers;
    int       stereo;
    int       samples;

    // OpenGL extensions and context attributes
    int       has_GL_SGIS_generate_mipmap;
    int       has_GL_ARB_texture_non_power_of_two;
    int       glMajor, glMinor, glRevision;
    int       glForward, glDebug, glProfile;

    PFNGLGETSTRINGIPROC GetStringi;

    // ========= PLATFORM SPECIFIC PART ======================================

    // Platform specific window resources
    struct Screen *Screen;    // Screen handle
    struct Window *Window;    // Window handle
    ULONG modeID;             // ModeID
    APTR PointerSprite;       // Memory for blank pointer sprite
    int PointerHidden;        // Is pointer hidden?
    struct MsgPort *InputMP;  // Message port (pointer movement)
    struct IOStdReq *InputIO; // I/O request (pointer movement)

    // OpenGL flavour specific
    struct GLContextIFace *Context; //GL context handle

    // Platform specific extensions

    // Various platform specific internal variables
};

GLFWGLOBAL _GLFWwin _glfwWin;

//------------------------------------------------------------------------
// Library global data
//------------------------------------------------------------------------
GLFWGLOBAL struct {

// ========= PLATFORM INDEPENDENT MANDATORY PART =========================

    // Window opening hints
    _GLFWhints      hints;

    // Initial desktop mode
    GLFWvidmode     desktopMode;

// ========= PLATFORM SPECIFIC PART ======================================

    // Timer data
    struct {
        double base;
        double resolution;
    } timer;

    // dlopen handle for dynamically-loading extension function pointers
    void *OpenGLFramework;

    //int originalMode;

    //int autoreleasePool;

    //CGEventSourceRef eventSource;

} _glfwLibrary;

//------------------------------------------------------------------------
// User input status (most of this should go in _GLFWwin)
//------------------------------------------------------------------------
GLFWGLOBAL struct
{

    // ========= PLATFORM INDEPENDENT MANDATORY PART =========================

    // Mouse status
    int MousePosX, MousePosY;
    int WheelPos;
    char MouseButton[GLFW_MOUSE_BUTTON_LAST + 1];

    // Keyboard status
    char Key[GLFW_KEY_LAST + 1];
    int LastChar;

    // User selected settings
    int StickyKeys;
    int StickyMouseButtons;
    int KeyRepeat;

    // ========= PLATFORM SPECIFIC PART ======================================

    // Platform specific internal variables
    int MouseMoved, OldMouseX, OldMouseY;

} _glfwInput;

//------------------------------------------------------------------------
// Timer status
//------------------------------------------------------------------------
GLFWGLOBAL struct
{
    struct MsgPort *TimerMP;
    struct timerequest *TimerIO;
    double Resolution;
    long long t0;
} _glfwTimer;

//------------------------------------------------------------------------
// Thread record (one for each thread)
//------------------------------------------------------------------------
typedef struct _GLFWthread_struct _GLFWthread;

struct _GLFWthread_struct
{

    // ========= PLATFORM INDEPENDENT MANDATORY PART =========================

    // Pointer to previous and next threads in linked list
    _GLFWthread *Previous, *Next;

    // GLFW user side thread information
    GLFWthread ID;

    // ========= PLATFORM SPECIFIC PART ======================================

    // System side thread information
    GLFWthreadfun Function;
    void *Arg;
    struct Process *AmiProc;
    struct Task *AmiTask;

    // "Wait for" object. Can be a thread, condition variable or NULL.
    void *WaitFor;
    int WaitSig;
};

//------------------------------------------------------------------------
// General thread information
//------------------------------------------------------------------------
GLFWGLOBAL struct
{
    // ========= PLATFORM INDEPENDENT MANDATORY PART =========================

    // Next thread ID to use (increments for every created thread)
    GLFWthread NextID;

    // First thread in linked list (always the main thread)
    _GLFWthread First;

    // ========= PLATFORM SPECIFIC PART ======================================

    // Critical section lock
    struct SignalSemaphore CriticalSection;

    // Next condition variable ID (decrements for every created cond)
    unsigned int NextCondID;

} _glfwThrd;

//------------------------------------------------------------------------
// Joystick information & state
//------------------------------------------------------------------------
GLFWGLOBAL struct
{
    int Present;
    int GameDeviceOpen;
    struct IOStdReq *GameIO;
    struct MsgPort *GameMP;
    struct InputEvent GameEvent;
    float Axis[2];
    unsigned char Button[2];
} _glfwJoy;

//========================================================================
// Macros for encapsulating critical code sections (i.e. making parts
// of GLFW thread safe)
//========================================================================

// Thread list management
#define ENTER_THREAD_CRITICAL_SECTION IExec->ObtainSemaphore(&_glfwThrd.CriticalSection);
#define LEAVE_THREAD_CRITICAL_SECTION IExec->ReleaseSemaphore(&_glfwThrd.CriticalSection);

//========================================================================
// Prototypes for platform specific internal functions
//========================================================================

// Time
int _glfwInitTimer(void);
void _glfwTerminateTimer(void);
 
// Fullscreen
int _glfwGetClosestVideoMode(int *w, int *h, int *r, int *g, int *b, int refresh);
void _glfwGetModeIDInfo(ULONG modeID, int *w, int *h, int *r, int *g, int *b, int *refresh);
// Joystick
void _glfwInitJoysticks(void);
void _glfwTerminateJoysticks(void);

#endif // _platform_h_
