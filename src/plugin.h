/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-input-sdl - plugin.h                                      *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2008-2009 Richard Goedeken                              *
 *   Copyright (C) 2008 Tillin9                                            *
 *   Copyright (C) 2002 Blight                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#if defined(__GNUC__)
#define ATTR_FMT(fmtpos, attrpos) __attribute__ ((format (printf, fmtpos, attrpos)))
#else
#define ATTR_FMT(fmtpos, attrpos)
#endif

#include <SDL.h>
#include <string.h>
#if ! SDL_VERSION_ATLEAST(1,3,0)

#define SDL_GetKeyboardState SDL_GetKeyState
#define SDL_SCANCODE_UNKNOWN SDLK_UNKNOWN
#define SDL_NUM_SCANCODES SDLK_LAST
#define SDL_SCANCODE_RCTRL SDLK_RCTRL
#define SDL_SCANCODE_RSHIFT SDLK_RSHIFT
#define SDL_SCANCODE_LCTRL SDLK_LCTRL
#define SDL_SCANCODE_LALT SDLK_LALT
#define SDL_SCANCODE_LGUI SDLK_LSUPER
#define SDL_Scancode SDLKey

#endif

#if SDL_VERSION_ATLEAST(2,0,0)

static inline const char* _SDL_JoystickName(int device_index)
{
    SDL_Joystick *joystick;
    const char *name;
    static char JoyName[256];

    joystick = SDL_JoystickOpen(device_index);
    if (!joystick)
        return NULL;

    name = SDL_JoystickName(joystick);
    if (name)
    {
        strncpy(JoyName, name, 255);
        JoyName[255] = 0;
    }
    SDL_JoystickClose(joystick);

    return JoyName;
}

#define SDL_JoystickName(device_index) _SDL_JoystickName(device_index)

#endif

#define M64P_PLUGIN_PROTOTYPES 1
#include "m64p_config.h"
#include "m64p_plugin.h"

#define DEVICE_NO_JOYSTICK  (-1)

// Some stuff from n-rage plugin
#define RD_GETSTATUS        0x00        // get status
#define RD_READKEYS         0x01        // read button values
#define RD_READPAK          0x02        // read from controllerpack
#define RD_WRITEPAK         0x03        // write to controllerpack
#define RD_RESETCONTROLLER  0xff        // reset controller
#define RD_READEEPROM       0x04        // read eeprom
#define RD_WRITEEPROM       0x05        // write eeprom

#define PAK_IO_RUMBLE       0xC000      // the address where rumble-commands are sent to

enum EButton
{
    R_DPAD          = 0,
    L_DPAD,
    D_DPAD,
    U_DPAD,
    START_BUTTON,
    Z_TRIG,
    B_BUTTON,
    A_BUTTON,
    R_CBUTTON,
    L_CBUTTON,
    D_CBUTTON,
    U_CBUTTON,
    R_TRIG,
    L_TRIG,
    MEMPAK,
    RUMBLEPAK,
    X_AXIS,
    Y_AXIS,
    NUM_BUTTONS
};

typedef struct
{
    int button;         // button index; -1 if notassigned
    SDL_Scancode key;   // sdl keysym; SDL_SCANCODE_UNKNOWN if not assigned
    int axis, axis_dir; // aixs + direction (i.e. 0, 1 = X Axis +; 0, -1 = X Axis -); -1 if notassigned
    int axis_deadzone;  // -1 for default, or >= 0 for custom value
    int hat, hat_pos;   // hat + hat position; -1 if not assigned
    int mouse;          // mouse button
} SButtonMap;

typedef struct
{
    int button_a, button_b;         // up/down or left/right; -1 if not assigned
    SDL_Scancode key_a, key_b;      // up/down or left/right; SDL_SCANCODE_UNKNOWN if not assigned
    int axis_a, axis_b;             // axis index; -1 if not assigned
    int axis_dir_a, axis_dir_b;     // direction (1 = X+, 0, -1 = X-)
    int hat, hat_pos_a, hat_pos_b;  // hat + hat position up/down and left/right; -1 if not assigned
} SAxisMap;

typedef struct
{
    CONTROL *control;               // pointer to CONTROL struct in Core library
    BUTTONS buttons;

    // mappings
    SButtonMap    button[16];       // 14 buttons; in the order of EButton + mempak/rumblepak switches
    SAxisMap      axis[2];          // 2 axis
    int           device;           // joystick device; -1 = keyboard; -2 = none
    int           mouse;            // mouse enabled: 0 = no; 1 = yes
    SDL_Joystick *joystick;         // SDL joystick device
#if SDL_VERSION_ATLEAST(2,0,0)
    SDL_Haptic   *event_joystick;   // the sdl device for force feeback
#else
    int           event_joystick;   // the /dev/input/eventX device for force feeback
#endif
    int           axis_deadzone[2]; // minimum absolute value before analog movement is recognized
    int           axis_peak[2];     // highest analog value returned by SDL, used for scaling
    float         mouse_sens[2];    // mouse sensitivity
} SController;

/* global data definitions */
extern SController controller[4];   // 4 controllers

/* global function definitions */




#if M64P_STATIC_PLUGINS

#define DebugMessage DebugMessageInput

#else


/* declarations of pointers to Core config functions */
extern ptr_ConfigListSections     ConfigListSections;
extern ptr_ConfigOpenSection      ConfigOpenSection;
extern ptr_ConfigDeleteSection    ConfigDeleteSection;
extern ptr_ConfigListParameters   ConfigListParameters;
extern ptr_ConfigSetParameter     ConfigSetParameter;
extern ptr_ConfigGetParameter     ConfigGetParameter;
extern ptr_ConfigGetParameterHelp ConfigGetParameterHelp;
extern ptr_ConfigSetDefaultInt    ConfigSetDefaultInt;
extern ptr_ConfigSetDefaultFloat  ConfigSetDefaultFloat;
extern ptr_ConfigSetDefaultBool   ConfigSetDefaultBool;
extern ptr_ConfigSetDefaultString ConfigSetDefaultString;
extern ptr_ConfigGetParamInt      ConfigGetParamInt;
extern ptr_ConfigGetParamFloat    ConfigGetParamFloat;
extern ptr_ConfigGetParamBool     ConfigGetParamBool;
extern ptr_ConfigGetParamString   ConfigGetParamString;

extern ptr_ConfigGetSharedDataFilepath ConfigGetSharedDataFilepath;
extern ptr_ConfigGetUserConfigPath     ConfigGetUserConfigPath;
extern ptr_ConfigGetUserDataPath       ConfigGetUserDataPath;
extern ptr_ConfigGetUserCachePath      ConfigGetUserCachePath;

#endif
extern void DebugMessage(int level, const char *message, ...) ATTR_FMT(2,3);

#endif // __PLUGIN_H__

