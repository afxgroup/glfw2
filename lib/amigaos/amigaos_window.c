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

#include "internal.h"

extern BOOL VARARGS68K showErrorRequester(const char *errMsgRaw, ...);

//************************************************************************
//****                  GLFW internal functions                       ****
//************************************************************************

//========================================================================
// _glfwTranslateChar() - Translates an AmigaOS key to Unicode
//========================================================================

static int _glfwTranslateChar(struct IntuiMessage *msg)
{
    struct InputEvent event;
    TEXT buffer[8] = {0};
    int character;

    // Create input event
    event.ie_Class = IECLASS_RAWKEY;
    event.ie_Code = msg->Code;
    event.ie_Qualifier = msg->Qualifier;
    event.ie_EventAddress = msg->IAddress;

    // Map key event to text string
    if (IKeymap->MapRawKey(&event, buffer, 8, NULL) > 0)
    {
        // Valid Unicode character?
        character = (int)buffer[0];
        if ((character >= 32 && character <= 126) ||
            (character >= 160 && character <= 255))
        {
            return character;
        }
    }

    return -1;
}

//========================================================================
// _glfwTranslateKey() - Translates an AmigaOS key to internal coding
//========================================================================

static int _glfwTranslateKey(struct IntuiMessage *msg)
{
    int key = msg->Code & 0x7F;
    ULONG old_qualifier;

    // Special (non printable) keys
    switch (key)
    {
    // Modifier keys
    case 0x60:
        return GLFW_KEY_LSHIFT;
    case 0x61:
        return GLFW_KEY_RSHIFT;
    case 0x62:
        return GLFW_KEY_LCTRL; // ?
    case 0x63:
        return GLFW_KEY_RCTRL; // ?
    case 0x64:
        return GLFW_KEY_LALT;
    case 0x65:
        return GLFW_KEY_RALT;

    // Function keys
    case 0x50:
        return GLFW_KEY_F1;
    case 0x51:
        return GLFW_KEY_F2;
    case 0x52:
        return GLFW_KEY_F3;
    case 0x53:
        return GLFW_KEY_F4;
    case 0x54:
        return GLFW_KEY_F5;
    case 0x55:
        return GLFW_KEY_F6;
    case 0x56:
        return GLFW_KEY_F7;
    case 0x57:
        return GLFW_KEY_F8;
    case 0x58:
        return GLFW_KEY_F9;
    case 0x59:
        return GLFW_KEY_F10;

    // Other control keys
    case 0x45:
        return GLFW_KEY_ESC;
    case 0x42:
        return GLFW_KEY_TAB;
    case 0x44:
        return GLFW_KEY_ENTER;
    case 0x46:
        return GLFW_KEY_DEL;
    case 0x41:
        return GLFW_KEY_BACKSPACE;
    case 0x66:
        return GLFW_KEY_INSERT; // ?
    case 0x4F:
        return GLFW_KEY_LEFT;
    case 0x4E:
        return GLFW_KEY_RIGHT;
    case 0x4C:
        return GLFW_KEY_UP;
    case 0x4D:
        return GLFW_KEY_DOWN;

    // Keypad keys
    case 0x0F:
        return GLFW_KEY_KP_0;
    case 0x1D:
        return GLFW_KEY_KP_1;
    case 0x1E:
        return GLFW_KEY_KP_2;
    case 0x1F:
        return GLFW_KEY_KP_3;
    case 0x2D:
        return GLFW_KEY_KP_4;
    case 0x2E:
        return GLFW_KEY_KP_5;
    case 0x2F:
        return GLFW_KEY_KP_6;
    case 0x3D:
        return GLFW_KEY_KP_7;
    case 0x3E:
        return GLFW_KEY_KP_8;
    case 0x3F:
        return GLFW_KEY_KP_9;
    case 0x43:
        return GLFW_KEY_KP_ENTER;
    case 0x5E:
        return GLFW_KEY_KP_ADD;
    case 0x4A:
        return GLFW_KEY_KP_SUBTRACT;
    case 0x5D:
        return GLFW_KEY_KP_MULTIPLY;
    case 0x5C:
        return GLFW_KEY_KP_DIVIDE;
    case 0x3C:
        return GLFW_KEY_KP_DECIMAL;

    default:
        break;
    }

    // Printable keys (without modifiers!)
    old_qualifier = msg->Qualifier;
    msg->Qualifier = 0;
    key = _glfwTranslateChar(msg);
    msg->Qualifier = old_qualifier;
    if (key > 0)
    {
        // Make sure it is upper case
        key = IUtility->ToUpper(key);
    }

    return key;
}

//========================================================================
// _glfwProcessEvents() - Process all pending AmigaOS events
//========================================================================

static int _glfwProcessEvents(void)
{
    // Just in case..
    if (_glfwWin.Window == NULL)
        return GL_TRUE;

    struct IntuiMessage message, *tmp_message = NULL;
    struct MsgPort *msg_port;
    int win_closed = GL_FALSE, needsRedraw = GL_FALSE, action;
    int x, y;

    // Examine pending messages
    msg_port = _glfwWin.Window->UserPort;
    while ((tmp_message = (struct IntuiMessage *)IExec->GetMsg(msg_port)))
    {
        // Copy contents of message structure
        message = *tmp_message;

        // Now reply to the message (we don't need it anymore)
        IExec->ReplyMsg((struct Message *)tmp_message);

        // Handle different messages
        switch (message.Class)
        {
            // Was the window activated?
            case IDCMP_ACTIVEWINDOW:
                _glfwWin.active = GL_TRUE;
                break;

            // Was the window deactivated?
            case IDCMP_INACTIVEWINDOW:
                _glfwWin.active = GL_FALSE;
                break;

            // Did we get a keyboard press or release?
            case IDCMP_RAWKEY:
                action = (message.Code & 0x80) ? GLFW_RELEASE : GLFW_PRESS;
                message.Code &= 0x7F;
                _glfwInputKey(_glfwTranslateKey(&message), action);
                _glfwInputChar(_glfwTranslateChar(&message), action);
                break;

            // Was the mouse moved?
            case IDCMP_MOUSEMOVE:
                x = message.MouseX;
                y = message.MouseY;
                if (_glfwWin.PointerHidden)
                {
                    // When pointer is hidden, we get delta moves
                    x += _glfwInput.MousePosX;
                    y += _glfwInput.MousePosY;
                }
                else if (x < 0 || x >= _glfwWin.width ||
                        y < 0 || y >= _glfwWin.height)
                {
                    // Only report mouse moves that are INSIDE client area
                    break;
                }
                if (x != _glfwInput.MousePosX || y != _glfwInput.MousePosY)
                {
                    _glfwInput.MousePosX = x;
                    _glfwInput.MousePosY = y;
                    if (_glfwWin.mousePosCallback)
                    {
                        _glfwWin.mousePosCallback(x, y);
                    }
                }
                break;

            // Did we get a mouse button event?
            case IDCMP_MOUSEBUTTONS:
                switch (message.Code)
                {
                case SELECTUP:
                    _glfwInputMouseClick(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
                    break;
                case SELECTDOWN:
                    _glfwInputMouseClick(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
                    break;
                case MENUUP:
                    _glfwInputMouseClick(GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE);
                    break;
                case MENUDOWN:
                    _glfwInputMouseClick(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
                    break;
                default:
                    break;
                }
                break;

            // Was the window size changed?
            case IDCMP_NEWSIZE:
                _glfwWin.width = _glfwWin.Window->Width - _glfwWin.Window->BorderLeft - _glfwWin.Window->BorderRight;
                _glfwWin.height = _glfwWin.Window->Height - _glfwWin.Window->BorderTop - _glfwWin.Window->BorderBottom;
                if (_glfwWin.windowSizeCallback)
                {
                    _glfwWin.windowSizeCallback(_glfwWin.width, _glfwWin.height);
                }
                needsRedraw = GL_TRUE;
                break;

            // Was the window contents damaged?
            case IDCMP_REFRESHWINDOW:
                // Intuition wants us to do this...
                IIntuition->BeginRefresh(_glfwWin.Window);
                IIntuition->EndRefresh(_glfwWin.Window, TRUE);

                // Call user callback function
                if (_glfwWin.windowRefreshCallback)
                {
                    _glfwWin.windowRefreshCallback();
                }
                needsRedraw = GL_TRUE;
                break;

            // Was the window closed?
            case IDCMP_CLOSEWINDOW:
                win_closed = GL_TRUE;
                break;

            default:
                break;
        }
    }

    if (needsRedraw) {
        if (_glfwWin.Context->SetDisplayMode(GL_TRUE, _glfwWin.width, _glfwWin.height, 0) == 0) {
            const char errMsgRaw[] = "Error: Out of graphics memory.\nThis program will now close\n";
            if (!showErrorRequester(errMsgRaw))
            {
                // Use printf() as a backup
                printf(errMsgRaw);
            } 
            exit(10);
        }
    }

    // Return GL_TRUE if window was closed
    return (win_closed);
}

//************************************************************************
//****               Platform implementation functions                ****
//************************************************************************

//========================================================================
// _glfwPlatformOpenWindow() - Here is where the window is created, and
// the OpenGL rendering context is created
//========================================================================

int _glfwPlatformOpenWindow(int width, int height, const _GLFWwndconfig *wndconfig, const _GLFWfbconfig *fbconfig)
{
    // Clear platform specific GLFW window state
    _glfwWin.refreshRate      = wndconfig->refreshRate;
    _glfwWin.windowNoResize   = wndconfig->windowNoResize;

    // Clear window state
    _glfwWin.Screen = NULL;
    _glfwWin.Window = NULL;
    _glfwWin.Context = NULL;
    _glfwWin.PointerHidden = 0;
    _glfwWin.PointerSprite = NULL;
    _glfwWin.InputMP = NULL;
    _glfwWin.InputIO = NULL;

    // Create input.device message port
    if (!(_glfwWin.InputMP = IExec->AllocSysObjectTags(ASOT_PORT, TAG_END)))
    {
        _glfwPlatformCloseWindow();
        return GL_FALSE;
    }

    // Create input.device I/O request
    if (!(_glfwWin.InputIO = (struct IOStdReq *)IExec->AllocSysObjectTags(ASOT_IOREQUEST, ASOIOR_Size, sizeof(struct IOStdReq), ASOIOR_ReplyPort, _glfwWin.InputMP, TAG_END)))
    {
        _glfwPlatformCloseWindow();
        return GL_FALSE;
    }

    // Open input.device (for pointer position manipulation)
    if (IExec->OpenDevice("input.device", 0, (struct IORequest *)_glfwWin.InputIO, 0))
    {
        IExec->FreeSysObject(ASOT_IOREQUEST, (struct IORequest *)_glfwWin.InputIO);
        _glfwWin.InputIO = NULL;
        _glfwPlatformCloseWindow();
        return GL_FALSE;
    }

    // If we are in fullscreen mode find the correct mode
    if (_glfwWin.fullscreen) {
        _glfwGetClosestVideoMode(&width, &height, &fbconfig->accumRedBits, &fbconfig->accumGreenBits, &fbconfig->accumBlueBits, _glfwWin.refreshRate);
    }

    // Create GL context
    _glfwWin.Context = IMiniGL->CreateContextTags(
                            MGLCC_Width,            width,
                            MGLCC_Height,           height,
                            MGLCC_Windowed,         _glfwWin.fullscreen ? FALSE : TRUE,
                            MGLCC_CloseGadget,      _glfwWin.fullscreen ? FALSE : TRUE,
                            MGLCC_SizeGadget,       _glfwWin.fullscreen ? FALSE : TRUE,
                            MGLCC_Buffers,          2,
                            MGLCC_PixelDepth,       16,
                            //MGLCC_StencilBuffer,	TRUE,
                            TAG_DONE);

    if (!_glfwWin.Context)
    {
        _glfwPlatformCloseWindow();
        return GL_FALSE;
    }

    mglMakeCurrent(_glfwWin.Context);
    mglLockMode(MGL_LOCK_SMART);

    // Get Window Handle
    _glfwWin.Window = mglGetWindowHandle();

    if (!IIntuition->ModifyIDCMP(_glfwWin.Window, IDCMP_REFRESHWINDOW |
                                                      IDCMP_CLOSEWINDOW |
                                                      IDCMP_NEWSIZE |
                                                      IDCMP_ACTIVEWINDOW |
                                                      IDCMP_INACTIVEWINDOW |
                                                      IDCMP_RAWKEY |
                                                      IDCMP_MOUSEMOVE |
                                                      IDCMP_MOUSEBUTTONS))
    {
        _glfwPlatformCloseWindow();
        return GL_FALSE;
    }
    //mglUnlockDisplay();

    // Get screen handle from window
    _glfwWin.Screen = _glfwWin.Window->WScreen;
    
    // fullscreen/windowed post fixups
    if (_glfwWin.fullscreen)
    {
        // Don't show screen title
        IIntuition->ShowTitle(_glfwWin.Screen, FALSE);
    }
    else
    {
        // Get ModeID for the current video mode
        _glfwWin.modeID = IGraphics->GetVPModeID(&_glfwWin.Screen->ViewPort);
    }
    // Remember window size
    _glfwWin.width = _glfwWin.Window->Width;
    _glfwWin.height = _glfwWin.Window->Height;

    // Put window on top
    IIntuition->WindowToFront(_glfwWin.Window);

    return GL_TRUE;
}

//========================================================================
// _glfwPlatformCloseWindow() - Properly kill the window/video display
//========================================================================

void _glfwPlatformCloseWindow(void)
{
    if (_glfwWin.Window == NULL)
        return;

    struct MsgPort *msg_port = _glfwWin.Window->UserPort;
    struct IntuiMessage *tmp_message = NULL;
    // Flush pending messages
    while ((tmp_message = (struct IntuiMessage *)IExec->GetMsg(msg_port)))
    {
        // Now reply to the message (we don't need it anymore)
        IExec->ReplyMsg((struct Message *)tmp_message);
    }

    // Restore mouse pointer (if hidden)
    _glfwPlatformShowMouseCursor();

    // Close input device I/O request
    if (_glfwWin.InputIO)
    {
        IExec->CloseDevice((struct IORequest *)_glfwWin.InputIO);
        IExec->FreeSysObject(ASOT_IOREQUEST, _glfwWin.InputIO);
        _glfwWin.InputIO = NULL;
    }

    // Close input device message port
    if (_glfwWin.InputMP)
    {
        IExec->FreeSysObject(ASOT_PORT, _glfwWin.InputMP);
        _glfwWin.InputMP = NULL;
    }

    // Destroy OpenGL context
    if (_glfwWin.Context)
    {
        _glfwWin.Context->DeleteContext();
        _glfwWin.Context = NULL;
    }

    // Close window
    if (_glfwWin.Window)
    {
        _glfwWin.Window = NULL;
    }
}

//========================================================================
// _glfwPlatformSetWindowTitle() - Set the window title.
//========================================================================

void _glfwPlatformSetWindowTitle(const char *title)
{
    if (!_glfwWin.fullscreen)
    {
        IIntuition->SetWindowTitles(_glfwWin.Window, (char *)title, (char *)title);
    }
}

//========================================================================
// _glfwPlatformSetWindowSize() - Set the window size.
//========================================================================

void _glfwPlatformSetWindowSize(int width, int height)
{
    if (!_glfwWin.fullscreen)
    {
        IIntuition->SizeWindow(_glfwWin.Window, width - _glfwWin.width, height - _glfwWin.height);
    }
}

//========================================================================
// _glfwPlatformSetWindowPos() - Set the window position.
//========================================================================

void _glfwPlatformSetWindowPos(int x, int y)
{
    if (!_glfwWin.fullscreen)
    {
        IIntuition->ChangeWindowBox(_glfwWin.Window, x, y, _glfwWin.Window->Width, _glfwWin.Window->Height);
    }
}

//========================================================================
// _glfwPlatformIconfyWindow() - Window iconification
//========================================================================

void _glfwPlatformIconifyWindow(void)
{
    if (_glfwWin.fullscreen)
    {
        IIntuition->ScreenToBack(_glfwWin.Screen);
        IIntuition->WBenchToFront();
        _glfwWin.iconified = GL_TRUE;
    }
}

//========================================================================
// _glfwPlatformRestoreWindow() - Window un-iconification
//========================================================================

void _glfwPlatformRestoreWindow(void)
{
    if (_glfwWin.fullscreen)
    {
        IIntuition->ScreenToFront(_glfwWin.Screen);
    }
    IIntuition->WindowToFront(_glfwWin.Window);
    IIntuition->ActivateWindow(_glfwWin.Window);
    _glfwWin.iconified = GL_FALSE;
}

//========================================================================
// _glfwPlatformSwapBuffers() - Swap buffers (double-buffering) and poll
// any new events.
//========================================================================

void _glfwPlatformSwapBuffers(void)
{
    _glfwWin.Context->SwitchDisplay();
}

//========================================================================
// _glfwPlatformSwapInterval() - Set double buffering swap interval
//========================================================================

void _glfwPlatformSwapInterval(int interval)
{
    // Not supported
}

//========================================================================
// _glfwPlatformRefreshWindowParams()
//========================================================================

void _glfwPlatformRefreshWindowParams(void)
{
    int refresh;
    GLint x;
    GLboolean b;

    // This function is not proerly implemented yet. We use OpenGL for
    // getting framebuffer format information - we should use some
    // alternate interface (such as glX under the X Window System), but
    // StormMesa does not seem to provide this.

    // Fill out information
    _glfwWin.accelerated = GL_TRUE;
    glGetIntegerv(GL_RED_BITS, &x);
    _glfwWin.redBits = x;
    glGetIntegerv(GL_GREEN_BITS, &x);
    _glfwWin.greenBits = x;
    glGetIntegerv(GL_BLUE_BITS, &x);
    _glfwWin.blueBits = x;
    glGetIntegerv(GL_ALPHA_BITS, &x);
    _glfwWin.alphaBits = x;
    glGetIntegerv(GL_DEPTH_BITS, &x);
    _glfwWin.depthBits = x;
    glGetIntegerv(GL_STENCIL_BITS, &x);
    _glfwWin.stencilBits = x;
    glGetIntegerv(GL_ACCUM_RED_BITS, &x);
    _glfwWin.accumRedBits = x;
    glGetIntegerv(GL_ACCUM_GREEN_BITS, &x);
    _glfwWin.accumGreenBits = x;
    glGetIntegerv(GL_ACCUM_BLUE_BITS, &x);
    _glfwWin.accumBlueBits = x;
    glGetIntegerv(GL_ACCUM_ALPHA_BITS, &x);
    _glfwWin.accumAlphaBits = x;
    glGetIntegerv(GL_AUX_BUFFERS, &x);
    _glfwWin.auxBuffers = x;
    glGetBooleanv(GL_AUX_BUFFERS, &b);
    _glfwWin.stereo = b ? GL_TRUE : GL_FALSE;

    // Get ModeID information (refresh rate)
    _glfwGetModeIDInfo(_glfwWin.modeID, NULL, NULL, NULL, NULL, NULL, &refresh);
    _glfwWin.refreshRate = refresh;
}

//========================================================================
// _glfwPlatformPollEvents() - Poll for new window and input events
//========================================================================

void _glfwPlatformPollEvents(void)
{
    int winclosed;

    // Process all pending window events
    winclosed = _glfwProcessEvents();

    // Was there a window close request?
    if (winclosed && _glfwWin.windowCloseCallback)
    {
        // Check if the program wants us to close the window
        winclosed = _glfwWin.windowCloseCallback();
    }
    if (winclosed)
    {
        glfwCloseWindow();
    }
}

//========================================================================
// _glfwPlatformWaitEvents() - Wait for new window and input events
//========================================================================

void _glfwPlatformWaitEvents(void)
{
    // Wait for new events
    IExec->Wait(1L << _glfwWin.Window->UserPort->mp_SigBit);

    // Poll new events
    _glfwPlatformPollEvents();
}

//========================================================================
// _glfwPlatformHideMouseCursor() - Hide mouse cursor (lock it)
//========================================================================

void _glfwPlatformHideMouseCursor(void)
{
    // We only allow this under fullscreen right now, since we can't rely
    // on the pointer position in windowed mode! Perhaps it's possible to
    // "steal" the mouse with input.device or something...?
    if (!_glfwWin.PointerHidden && _glfwWin.fullscreen)
    {
        // Allocate chip memory for the blank mouse pointer
        _glfwWin.PointerSprite = IExec->AllocVec(128, MEMF_ANY | MEMF_CLEAR);
        if (_glfwWin.PointerSprite)
        {
            // Switch to blank/transparent pointer
            IIntuition->SetPointer(_glfwWin.Window, (UWORD *)_glfwWin.PointerSprite, 1, 1, 0, 0);
            _glfwWin.PointerHidden = 1;

            // Switch to mouse delta movement
            _glfwWin.Window->IDCMPFlags |= IDCMP_DELTAMOVE;
        }
    }
}

//========================================================================
// _glfwPlatformShowMouseCursor() - Show mouse cursor (unlock it)
//========================================================================

void _glfwPlatformShowMouseCursor(void)
{
    if (_glfwWin.PointerHidden)
    {
        // Switch to absolute mouse movement
        _glfwWin.Window->IDCMPFlags &= (0xFFFFFFFF ^ IDCMP_DELTAMOVE);

        // Change back to normal pointer
        IIntuition->ClearPointer(_glfwWin.Window);
        if (_glfwWin.PointerSprite)
        {
            IExec->FreeVec(_glfwWin.PointerSprite);
            _glfwWin.PointerSprite = NULL;
        }
        _glfwWin.PointerHidden = 0;
    }
}

//========================================================================
// _glfwPlatformSetMouseCursorPos() - Set physical mouse cursor position
//========================================================================

void _glfwPlatformSetMouseCursorPos(int x, int y)
{
    struct IEPointerPixel ppxl;
    struct InputEvent event;

    // Adjust coordinates to window client area upper left corner
    x += _glfwWin.Window->LeftEdge;
    y += _glfwWin.Window->TopEdge;

    /* Set up IEPointerPixel fields */
    ppxl.iepp_Screen = _glfwWin.Screen;
    ppxl.iepp_Position.X = x;
    ppxl.iepp_Position.Y = y;

    /* Set up InputEvent fields */
    event.ie_EventAddress = (APTR)&ppxl; /* IEPointerPixel */
    event.ie_NextEvent = NULL;
    event.ie_Class = IECLASS_NEWPOINTERPOS; /* new mouse pos */
    event.ie_SubClass = IESUBCLASS_PIXEL;   /* on pixel */
    event.ie_Code = IECODE_NOBUTTON;
    event.ie_Qualifier = 0; /* absolute pos */

    /* Set up I/O request */
    _glfwWin.InputIO->io_Data = (APTR)&event;
    _glfwWin.InputIO->io_Length = sizeof(struct InputEvent);
    _glfwWin.InputIO->io_Command = IND_WRITEEVENT;

    /* Perform I/O (move mouse cursor) */
    IExec->DoIO((struct IORequest *)_glfwWin.InputIO);
}

