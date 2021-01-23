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

//************************************************************************
//****                  GLFW internal functions                       ****
//************************************************************************

//========================================================================
// _glfwInitTimer() - Initialize timer
//========================================================================

int _glfwInitTimer(void)
{
    ULONG freq;
    struct EClockVal t;

    // Start by clearing all handles
    TimerBase = NULL;
    _glfwTimer.TimerMP = NULL;
    _glfwTimer.TimerIO = NULL;

    // Open timer.device (used as a library for ReadEClock)
    if ((_glfwTimer.TimerMP = IExec->CreateMsgPort()))
    {
        // Create the I/O request
        if ((_glfwTimer.TimerIO = (struct timerequest *)IExec->CreateIORequest(_glfwTimer.TimerMP, sizeof(struct timerequest))))
        {
            // Open the timer device
            if (!(IExec->OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)_glfwTimer.TimerIO, 0)))
            {
                // Set up pointer for timer functions
                TimerBase = (struct Device *)_glfwTimer.TimerIO->tr_node.io_Device;
                ITimer = (struct TimerIFace *)IExec->GetInterface((struct Library*) TimerBase, "main", 1, 0);
                if (ITimer == NULL)
                {
                    IExec->CloseDevice((struct IORequest *)_glfwTimer.TimerIO);
                }
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }

    // Get current time
    freq = ITimer->ReadEClock(&t);

    // Calculate resolution
    _glfwTimer.Resolution = 1.0 / (double)freq;

    // Convert to 64-bit integer
    _glfwTimer.t0 = (long long)t.ev_hi * (long long)4294967296 +
                    (long long)t.ev_lo;

    return 1;
}

//========================================================================
// _glfwTerminateTimer() - Terminate timer
//========================================================================

void _glfwTerminateTimer(void)
{
    // Empty the timer.device message port queue
    if (_glfwTimer.TimerMP)
    {
        struct Message *msg;
        while (NULL != (msg = IExec->GetMsg(_glfwTimer.TimerMP)))
        {
            IExec->ReplyMsg(msg);
        }
    }

    // Close timer.device
    if (TimerBase)
    {
        IExec->CloseDevice((struct IORequest *)_glfwTimer.TimerIO);
        TimerBase = NULL;
    }
    if (ITimer != NULL)
    {
        IExec->DropInterface((struct Interface *)ITimer);
    }

    // Delete timer.device I/O request
    if (_glfwTimer.TimerIO)
    {
        IExec->DeleteIORequest((struct IORequest *)_glfwTimer.TimerIO);
        _glfwTimer.TimerIO = NULL;
    }

    // Delete timer.device message port
    if (_glfwTimer.TimerMP)
    {
        IExec->DeletePort(_glfwTimer.TimerMP);
        _glfwTimer.TimerMP = NULL;
    }
}

//************************************************************************
//****               Platform implementation functions                ****
//************************************************************************

//========================================================================
// _glfwPlatformGetTime() - Return timer value in seconds
//========================================================================

double _glfwPlatformGetTime(void)
{
    struct EClockVal t;
    long long t64;

    // Get current time
    (void)ITimer->ReadEClock(&t);

    // Convert to 64-bit integer
    t64 = (long long)t.ev_hi * (long long)4294967296 +
          (long long)t.ev_lo;

    return (double)(t64 - _glfwTimer.t0) * _glfwTimer.Resolution;
}

//========================================================================
// _glfwPlatformSetTime() - Set timer value in seconds
//========================================================================

void _glfwPlatformSetTime(double t)
{
    struct EClockVal t0;
    long long t64;

    // Get current time
    (void)ITimer->ReadEClock(&t0);

    // Convert to 64-bit integer
    t64 = (long long)t0.ev_hi * (long long)4294967296 +
          (long long)t0.ev_lo;

    // Calulate new starting time
    _glfwTimer.t0 = t64 - (long long)(t / _glfwTimer.Resolution);
}

//========================================================================
// _glfwPlatformSleep() - Put a thread to sleep for a specified amount of
// time
//========================================================================

void _glfwPlatformSleep(double time)
{
    ULONG ticks;

    // Too short time?
    if (time <= 0.0)
    {
        struct Task *me = IExec->FindTask(NULL);
        IExec->SetTaskPri(me, me->tc_Node.ln_Pri);
        return;
    }

    // Calculate Delay ticks (should be 50 ticks per second)
    ticks = (ULONG)((double)TICKS_PER_SECOND * time + 0.5);
    if (ticks == 0)
    {
        ticks = 1;
    }

    // Put process to sleep
    IDOS->Delay(ticks);
}
