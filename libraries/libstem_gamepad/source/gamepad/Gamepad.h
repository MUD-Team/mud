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

#ifndef __GAMEPAD_H__
#define __GAMEPAD_H__
#ifdef __cplusplus
extern "C" {
#endif

#define gamepad_bool char
#define gamepad_true 1
#define gamepad_false 0

#define gamepad_log_default 0
#define gamepad_log_warning 1
#define gamepad_log_error 2

#if defined(_MSC_VER) && (_MSC_VER < 1600)
#if (_MSC_VER < 1300)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#else
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
#endif
#else
#include <stdint.h>
#endif

union Gamepad_guid {
	uint8_t data[16];

	struct {
		uint16_t bus;
		uint16_t crc;
		uint16_t vendor;
		uint16_t zero1;
		uint16_t product;
		uint16_t zero2;
		uint16_t version;
		uint8_t  driver;
		uint8_t  info;
	}
	standard;

	struct {
		uint16_t bus;
		uint16_t crc;
		uint8_t  name[12];
	}
	unknownVidPid;
};

enum Gamepad_hat {
	GAMEPAD_HAT_UP = 0x01,
	GAMEPAD_HAT_RIGHT = 0x02,
	GAMEPAD_HAT_DOWN = 0x04,
	GAMEPAD_HAT_LEFT = 0x08
};

enum Gamepad_bindingType {
	GAMEPAD_BINDINGTYPE_NONE,
	GAMEPAD_BINDINGTYPE_BUTTON,
	GAMEPAD_BINDINGTYPE_AXIS,
	GAMEPAD_BINDINGTYPE_HAT
};

enum Gamepad_controllerButton {
	GAMEPAD_CONTROLLERBUTTON_SOUTH,
	GAMEPAD_CONTROLLERBUTTON_EAST,
	GAMEPAD_CONTROLLERBUTTON_WEST,
	GAMEPAD_CONTROLLERBUTTON_NORTH,
	GAMEPAD_CONTROLLERBUTTON_BACK,
	GAMEPAD_CONTROLLERBUTTON_GUIDE,
	GAMEPAD_CONTROLLERBUTTON_START,
	GAMEPAD_CONTROLLERBUTTON_LEFT_STICK,
	GAMEPAD_CONTROLLERBUTTON_RIGHT_STICK,
	GAMEPAD_CONTROLLERBUTTON_LEFT_SHOULDER,
	GAMEPAD_CONTROLLERBUTTON_RIGHT_SHOULDER,
	GAMEPAD_CONTROLLERBUTTON_DPAD_UP,
	GAMEPAD_CONTROLLERBUTTON_DPAD_DOWN,
	GAMEPAD_CONTROLLERBUTTON_DPAD_LEFT,
	GAMEPAD_CONTROLLERBUTTON_DPAD_RIGHT,
	GAMEPAD_CONTROLLERBUTTON_MISC_1,
	GAMEPAD_CONTROLLERBUTTON_RIGHT_PADDLE_1,
	GAMEPAD_CONTROLLERBUTTON_LEFT_PADDLE_1,
	GAMEPAD_CONTROLLERBUTTON_RIGHT_PADDLE_2,
	GAMEPAD_CONTROLLERBUTTON_LEFT_PADDLE_2,
	GAMEPAD_CONTROLLERBUTTON_TOUCHPAD
};

enum Gamepad_controllerAxis {
	GAMEPAD_CONTROLLERAXIS_LEFTX,
	GAMEPAD_CONTROLLERAXIS_LEFTY,
	GAMEPAD_CONTROLLERAXIS_RIGHTX,
	GAMEPAD_CONTROLLERAXIS_RIGHTY,
	GAMEPAD_CONTROLLERAXIS_LEFT_TRIGGER,
	GAMEPAD_CONTROLLERAXIS_RIGHT_TRIGGER
};

struct Gamepad_binding {
	enum Gamepad_bindingType inputType;

	union {
		uint8_t button;

		struct {
			uint8_t axis;
			float min;
			float max;
		}
		axis;

		struct {
			uint8_t hat;
			uint8_t mask;
		} hat;

	}
	input;

	enum Gamepad_bindingType outputType;

	union {
		enum Gamepad_controllerButton button;

		struct {
			enum Gamepad_controllerAxis axis;
			float min;
			float max;
		} axis;

	}
	output;
};

struct Gamepad_mapping {
	const char * name;
	union Gamepad_guid guid;
	struct Gamepad_binding south;
	struct Gamepad_binding east;
	struct Gamepad_binding west;
	struct Gamepad_binding north;
	struct Gamepad_binding back;
	struct Gamepad_binding guide;
	struct Gamepad_binding start;
	struct Gamepad_binding leftStick;
	struct Gamepad_binding rightStick;
	struct Gamepad_binding leftShoulder;
	struct Gamepad_binding rightShoulder;
	struct Gamepad_binding dpadUp;
	struct Gamepad_binding dpadDown;
	struct Gamepad_binding dpadLeft;
	struct Gamepad_binding dpadRight;
	struct Gamepad_binding misc1;
	struct Gamepad_binding rightPaddle1;
	struct Gamepad_binding leftPaddle1;
	struct Gamepad_binding rightPaddle2;
	struct Gamepad_binding leftPaddle2;
	struct Gamepad_binding leftX;
	struct Gamepad_binding leftY;
	struct Gamepad_binding rightX;
	struct Gamepad_binding rightY;
	struct Gamepad_binding leftTrigger;
	struct Gamepad_binding rightTrigger;
	struct Gamepad_binding touchpad;
};
struct Gamepad_device {
	// Unique device identifier for application session, starting at 0 for the first device attached and
	// incrementing by 1 for each additional device. If a device is removed and subsequently reattached
	// during the same application session, it will have a new deviceID.
	unsigned int deviceID;
	
	// Human-readable device name
	const char * description;
	
	// USB vendor/product IDs as returned by the driver. Can be used to determine the particular model of device represented.
	int vendorID;
	int productID;
	
  // SDL-compatible joystick ID to use with the controller database
	union Gamepad_guid guid;

	// Number of axis elements belonging to the device
	unsigned int numAxes;
	
	// Number of button elements belonging to the device
	unsigned int numButtons;

   // Number of hat elements belonging to the device
	unsigned int numHats;
	
	// Array[numAxes] of values representing the current state of each axis, in the range [-1..1]
	float * axisStates;

	// Array[numAxes] of device to SDL axis bindings
	const struct Gamepad_binding ** axisBindings;
	
	// Array[numButtons] of values representing the current state of each button
	gamepad_bool * buttonStates;

	// Array[numButtons] of raw to SDL button bindings
	const struct Gamepad_binding ** buttonBindings;

   // Array[numHats] of values representing the current state of each hat
	char * hatStates;

	// Array[numHats * 4] of raw to SDL hat bindings
	// Each hat will have 4 masks to represent the cardinal directions
	const struct Gamepad_binding ** hatBindings;
	
	// Platform-specific device data storage. Don't touch unless you know what you're doing and don't
	// mind your code breaking in future versions of this library.
	void * privateData;
};

/* Initializes the controller mapping database. Call before Gamepad_init to make sure controllers
   are correctly identified when joysticks are detected. Controller mapping is disabled is this
   function isn't called. */
gamepad_bool Gamepad_initMappings(void);

/* Initializes gamepad library and detects initial devices. Call this before any other Gamepad_*()
   function, other than callback registration functions. If you want to receive deviceAttachFunc
   callbacks from devices detected in Gamepad_init(), you must call Gamepad_deviceAttachFunc()
   before calling Gamepad_init().
   
   This function must be called from the same thread that will be calling Gamepad_processEvents()
   and Gamepad_detectDevices(). */
void Gamepad_init();

/* Tears down all data structures created by the gamepad library and releases any memory that was
   allocated. It is not necessary to call this function at application termination, but it's
   provided in case you want to free memory associated with gamepads at some earlier time. */
void Gamepad_shutdown();

/* Returns the number of currently attached gamepad devices. */
unsigned int Gamepad_numDevices();

/* Returns the specified Gamepad_device struct, or NULL if deviceIndex is out of bounds. */
struct Gamepad_device * Gamepad_deviceAtIndex(unsigned int deviceIndex);

/* Polls for any devices that have been attached since the last call to Gamepad_detectDevices() or
   Gamepad_init(). If any new devices are found, the callback registered with
   Gamepad_deviceAttachFunc() (if any) will be called once per newly detected device.
   
   Note that depending on implementation, you may receive button and axis event callbacks for
   devices that have not yet been detected with Gamepad_detectDevices(). You can safely ignore
   these events, but be aware that your callbacks might receive a device ID that hasn't been seen
   by your deviceAttachFunc. */
void Gamepad_detectDevices();

/* Reads pending input from all attached devices and calls the appropriate input callbacks, if any
   have been registered. */
void Gamepad_processEvents();

/* Registers a function to be called whenever a device is attached. The specified function will be
   called only during calls to Gamepad_init() and Gamepad_detectDevices(), in the thread from
   which those functions were called. Calling this function with a NULL argument will stop any
   previously registered callback from being called subsequently. */
void Gamepad_deviceAttachFunc(void (* callback)(struct Gamepad_device * device, void * context), void * context);

/* Registers a function to be called whenever a device is detached. The specified function can be
   called at any time, and will not necessarily be called from the main thread. Calling this
   function with a NULL argument will stop any previously registered callback from being called
   subsequently. */
void Gamepad_deviceRemoveFunc(void (* callback)(struct Gamepad_device * device, void * context), void * context);

/* Registers a function to be called whenever a button on any attached device is pressed. The
   specified function will be called only during calls to Gamepad_processEvents(), in the
   thread from which Gamepad_processEvents() was called. Calling this function with a NULL
   argument will stop any previously registered callback from being called subsequently.  */
void Gamepad_buttonDownFunc(void (* callback)(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context), void * context);

/* Registers a function to be called whenever a button on any attached device is released. The
   specified function will be called only during calls to Gamepad_processEvents(), in the
   thread from which Gamepad_processEvents() was called. Calling this function with a NULL
   argument will stop any previously registered callback from being called subsequently.  */
void Gamepad_buttonUpFunc(void (* callback)(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context), void * context);

/* Registers a function to be called whenever an axis on any attached device is moved. The
   specified function will be called only during calls to Gamepad_processEvents(), in the
   thread from which Gamepad_processEvents() was called. Calling this function with a NULL
   argument will stop any previously registered callback from being called subsequently.  */
void Gamepad_axisMoveFunc(void (* callback)(struct Gamepad_device * device, unsigned int axisID, float value, float lastValue, double timestamp, void * context), void * context);

/* Registers a function to be called whenever libstem produces a log message */
void Gamepad_logFunc(void(*callback)(int priority, const char * message));

/* Toggles sending device events to Gamepad_logFunc for debugging purposes */
void Gamepad_setDebug(gamepad_bool enabled);

/* Adds a new mapping from a descriptive string. Useful to add mappings from 
   https://github.com/gabomdq/SDL_GameControllerDB */
gamepad_bool Gamepad_addMapping(const char* string);

#ifdef __cplusplus
}
#endif
#endif
