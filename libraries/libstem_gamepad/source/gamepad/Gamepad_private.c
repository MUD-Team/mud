/*
  Copyright (c) 2014 Alex Diener
  
  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
  
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  
  Alex Diener alex@ludobloom.com
*/

#include "gamepad/Gamepad.h"
#include "gamepad/Gamepad_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

gamepad_bool Gamepad_debugEvents = gamepad_false;

void Gamepad_setDebug(gamepad_bool enabled)
{
	Gamepad_debugEvents = enabled;
}

const char * Gamepad_formatLogMessage(const char * format, ...) {
		int buf_size = 128;

    for (;;)
		{
			char *buf = calloc(1, buf_size);

			va_list args;

			va_start(args, format);
			int out_len = vsnprintf(buf, buf_size, format, args);
			va_end(args);

			if (out_len >= 0 && out_len < buf_size)
				return buf;

			free(buf);

			buf_size *= 2;
		}
}

static void Gamepad_logFuncDefault(int priority, const char * message) {
    switch (priority)
    {
        case gamepad_log_warning:
        case gamepad_log_error:
            fprintf(stderr, "%s", message);
            break;
        default:
            fprintf(stdout, "%s", message);
						break;
    }
}

void (* Gamepad_deviceAttachCallback)(struct Gamepad_device * device, void * context) = NULL;
void (* Gamepad_deviceRemoveCallback)(struct Gamepad_device * device, void * context) = NULL;
void (* Gamepad_buttonDownCallback)(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context) = NULL;
void (* Gamepad_buttonUpCallback)(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context) = NULL;
void (* Gamepad_axisMoveCallback)(struct Gamepad_device * device, unsigned int buttonID, float value, float lastValue, double timestamp, void * context) = NULL;
void (* Gamepad_logCallback)(int priority, const char * message) = Gamepad_logFuncDefault;
void * Gamepad_deviceAttachContext = NULL;
void * Gamepad_deviceRemoveContext = NULL;
void * Gamepad_buttonDownContext = NULL;
void * Gamepad_buttonUpContext = NULL;
void * Gamepad_axisMoveContext = NULL;

void Gamepad_deviceAttachFunc(void (* callback)(struct Gamepad_device * device, void * context), void * context) {
	Gamepad_deviceAttachCallback = callback;
	Gamepad_deviceAttachContext = context;
}

void Gamepad_deviceRemoveFunc(void (* callback)(struct Gamepad_device * device, void * context), void * context) {
	Gamepad_deviceRemoveCallback = callback;
	Gamepad_deviceRemoveContext = context;
}

void Gamepad_buttonDownFunc(void (* callback)(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context), void * context) {
	Gamepad_buttonDownCallback = callback;
	Gamepad_buttonDownContext = context;
}

void Gamepad_buttonUpFunc(void (* callback)(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context), void * context) {
	Gamepad_buttonUpCallback = callback;
	Gamepad_buttonUpContext = context;
}

void Gamepad_axisMoveFunc(void (* callback)(struct Gamepad_device * device, unsigned int axisID, float value, float lastValue, double timestamp, void * context), void * context) {
	Gamepad_axisMoveCallback = callback;
	Gamepad_axisMoveContext = context;
}

void Gamepad_logFunc(void (* callback)(int priority, const char * message)) {
    Gamepad_logCallback = callback;
}

#define GAMEPAD_TOTAL_BINDINGS 27

size_t mappingOffsets[GAMEPAD_TOTAL_BINDINGS] = {
	offsetof(struct Gamepad_mapping, south),
	offsetof(struct Gamepad_mapping, east),
	offsetof(struct Gamepad_mapping, west),
	offsetof(struct Gamepad_mapping, north),
	offsetof(struct Gamepad_mapping, back),
	offsetof(struct Gamepad_mapping, guide),
	offsetof(struct Gamepad_mapping, start),
	offsetof(struct Gamepad_mapping, leftStick),
	offsetof(struct Gamepad_mapping, rightStick),
	offsetof(struct Gamepad_mapping, leftShoulder),
	offsetof(struct Gamepad_mapping, rightShoulder),
	offsetof(struct Gamepad_mapping, dpadUp),
	offsetof(struct Gamepad_mapping, dpadDown),
	offsetof(struct Gamepad_mapping, dpadLeft),
	offsetof(struct Gamepad_mapping, dpadRight),
	offsetof(struct Gamepad_mapping, misc1),
	offsetof(struct Gamepad_mapping, rightPaddle1),
	offsetof(struct Gamepad_mapping, leftPaddle1),
	offsetof(struct Gamepad_mapping, rightPaddle2),
	offsetof(struct Gamepad_mapping, leftPaddle2),
	offsetof(struct Gamepad_mapping, leftX),
	offsetof(struct Gamepad_mapping, leftY),
	offsetof(struct Gamepad_mapping, rightX),
	offsetof(struct Gamepad_mapping, rightY),
	offsetof(struct Gamepad_mapping, leftTrigger),
	offsetof(struct Gamepad_mapping, rightTrigger),
	offsetof(struct Gamepad_mapping, touchpad)
};

const struct Gamepad_binding * findBinding(const struct Gamepad_mapping * deviceMap, enum Gamepad_bindingType type, unsigned int index, unsigned int mask) {
	for (int i = 0; i < GAMEPAD_TOTAL_BINDINGS; i++) {
		const struct Gamepad_binding * bind = (const struct Gamepad_binding *)((char *)deviceMap + mappingOffsets[i]);
		if (bind->inputType == type)
		{
			switch (bind->inputType)
			{
				case GAMEPAD_BINDINGTYPE_AXIS:
				{
					if (bind->input.axis.axis == index)
						return bind;
					break;
				}
				case GAMEPAD_BINDINGTYPE_BUTTON:
				{
					if (bind->input.button == index)
						return bind;
					break;
				}
				// Might need to take the hat input masks into consideration
				case GAMEPAD_BINDINGTYPE_HAT:
				{
					if (bind->input.hat.hat == index) {
						if (bind->input.hat.mask == (1 << mask)) {
							return bind;
						}
					}
					break;
				}
			}
		}
	}
	return NULL;
}

void assignDeviceBindings(struct Gamepad_device * deviceRecord, const struct Gamepad_mapping * deviceMap) {
	unsigned int axes = deviceRecord->numAxes;
	unsigned int buttons = deviceRecord->numButtons;
	unsigned int hats = deviceRecord->numHats;
	unsigned int index;
	deviceRecord->axisBindings = calloc(axes, sizeof(struct Gamepad_mapping *));
	deviceRecord->buttonBindings = calloc(buttons, sizeof(struct Gamepad_mapping *));
	deviceRecord->hatBindings = calloc(hats * 4, sizeof(struct Gamepad_mapping *));
	for (index = 0; index < axes; index++) {
			deviceRecord->axisBindings[index] = findBinding(deviceMap, GAMEPAD_BINDINGTYPE_AXIS, index, 0);
	}
	for (index = 0; index < buttons; index++) {
			deviceRecord->buttonBindings[index] = findBinding(deviceMap, GAMEPAD_BINDINGTYPE_BUTTON, index, 0);
	}
	for (index = 0; index < hats; index++) {
			for (unsigned int mask = 0; mask < 4; mask++) {
				deviceRecord->hatBindings[index + mask] = findBinding(deviceMap, GAMEPAD_BINDINGTYPE_HAT, index, mask);
			}
	}
}