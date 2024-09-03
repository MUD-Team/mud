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

// Special thanks to SDL2 for portions of DirectInput and XInput code used in this implementation

#define _WIN32_WINNT 0x0501
#define INITGUID
#define DIRECTINPUT_VERSION 0x0800
#ifdef _MSC_VER
#define strdup _strdup
#undef UNICODE
#else
#define __in
#define __out
#define __reserved
#endif
  
#include "gamepad/Gamepad.h"
#include "gamepad/Gamepad_private.h"
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <regstr.h>
#include <dinput.h>
#include <xinput.h>
#include <dbt.h>

// The following code is from SDL2
// detects gamepad device remove/attach events without having to poll multiple times per second
static gamepad_bool s_bWindowsDeviceChanged = gamepad_false;

static LRESULT CALLBACK PrivateJoystickDetectProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DEVICECHANGE:
		switch (wParam) {
		case DBT_DEVICEARRIVAL:
			if (((DEV_BROADCAST_HDR*)lParam)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
				s_bWindowsDeviceChanged = gamepad_true;
			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			if (((DEV_BROADCAST_HDR*)lParam)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
				s_bWindowsDeviceChanged = gamepad_true;
			}
			break;
		}
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

typedef struct {
	HRESULT coinitialized;
	WNDCLASSEX wincl;
	HWND messageWindow;
	HDEVNOTIFY hNotify;
} DeviceNotificationData;

static void CleanupDeviceNotification(DeviceNotificationData *data)
{
	if (data->hNotify)
		UnregisterDeviceNotification(data->hNotify);

	if (data->messageWindow)
		DestroyWindow(data->messageWindow);

	UnregisterClass(data->wincl.lpszClassName, data->wincl.hInstance);

	if (data->coinitialized == S_OK) {
		CoUninitialize();
	}
}

static int CreateDeviceNotification(DeviceNotificationData *data)
{
	DEV_BROADCAST_DEVICEINTERFACE dbh;
	GUID GUID_DEVINTERFACE_HID = { 0x4D1E55B2L, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

	memset(data, 0, sizeof(*data));

	data->coinitialized = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (data->coinitialized == RPC_E_CHANGED_MODE) {
		data->coinitialized = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  }

	data->wincl.hInstance = GetModuleHandle(NULL);
	data->wincl.lpszClassName = "Message";
	data->wincl.lpfnWndProc = PrivateJoystickDetectProc;      /* This function is called by windows */
	data->wincl.cbSize = sizeof (WNDCLASSEX);

	if (!RegisterClassEx(&data->wincl)) {
		Gamepad_logCallback(gamepad_log_warning, "Failed to create register class for joystick autodetect");
		CleanupDeviceNotification(data);
		return -1;
	}

	data->messageWindow = (HWND)CreateWindowEx(0, "Message", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	if (!data->messageWindow) {
		Gamepad_logCallback(gamepad_log_warning, "Failed to create message window for joystick autodetect");
		CleanupDeviceNotification(data);
		return -1;
	}

	memset(&dbh, 0, sizeof(dbh));
	dbh.dbcc_size = sizeof(dbh);
	dbh.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	dbh.dbcc_classguid = GUID_DEVINTERFACE_HID;

	data->hNotify = RegisterDeviceNotification(data->messageWindow, &dbh, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (!data->hNotify) {
		Gamepad_logCallback(gamepad_log_warning, "Failed to create notify device for joystick autodetect");
		CleanupDeviceNotification(data);
		return -1;
	}
	return 0;
}


static void CheckDeviceNotification(DeviceNotificationData *data)
{
	MSG msg;

	if (!data->messageWindow) {
		return;
	}

	while (PeekMessage(&msg, data->messageWindow, 0, 0, PM_NOREMOVE)) {
		if (GetMessage(&msg, data->messageWindow, 0, 0) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////

// Copy from MinGW-w64 to MinGW, along with wbemcli.h, wbemprov.h, wbemdisp.h, and wbemtran.h
#ifndef __MINGW_EXTENSION
#define __MINGW_EXTENSION
#endif
#define COBJMACROS 1
#include <wbemidl.h>
#include <oleauto.h>
// Super helpful info: http://www.wreckedgames.com/forum/index.php?topic=2584.0

#define INPUT_QUEUE_SIZE 32
#define XINPUT_GAMEPAD_GUIDE 0x400

typedef struct {
	XINPUT_CAPABILITIES Capabilities;
	WORD VendorId;
	WORD ProductId;
	WORD VersionNumber;
	WORD unk1;
	DWORD unk2;
} LIBSTEM_XINPUT_CAPABILITIES_EX;

struct Gamepad_devicePrivate {
	gamepad_bool isXInput;
	
	// DInput only
	GUID guidInstance;
	IDirectInputDevice8 * deviceInterface;
	gamepad_bool buffered;
	unsigned int sliderCount;
	DWORD * axisOffsets;
	DWORD * buttonOffsets;
	DWORD * hatOffsets;
	
	// XInput only
	unsigned int playerIndex;
};

static DeviceNotificationData notificationData;
static struct Gamepad_device ** devices = NULL;
static unsigned int numDevices = 0;
static unsigned int nextDeviceID = 0;
static struct Gamepad_device * registeredXInputDevices[4] = { NULL, NULL, NULL, NULL };
static const char * xInputDeviceNames[4] = {
	"Generic XInput Controller 1",
	"Generic XInput Controller 2",
	"Generic XInput Controller 3",
	"Generic XInput Controller 4"
};

static DWORD (WINAPI * XInputGetState_proc)(DWORD dwUserIndex, XINPUT_STATE * pState) = NULL;
static DWORD (WINAPI * XInputGetCapabilitiesEx_proc)(DWORD unk1, DWORD dwUserIndex, DWORD dwFlags, LIBSTEM_XINPUT_CAPABILITIES_EX * pCapabilities) = NULL;

static LPDIRECTINPUT directInputInterface;
static gamepad_bool inited = gamepad_false;
static gamepad_bool xInputAvailable = gamepad_false;

void Gamepad_init() {
	if (!inited) {
		HRESULT result;
		HMODULE module;
		HRESULT (WINAPI * DirectInput8Create_proc)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
		
		module = LoadLibrary("XInput1_4.dll");
		if (module == NULL) {
			Gamepad_logCallback(gamepad_log_warning, "Gamepad_init couldn't load XInput1_4.dll; proceeding with DInput only\n");
			xInputAvailable = gamepad_false;
		} else {
			xInputAvailable = gamepad_true;
			XInputGetState_proc = (DWORD (WINAPI *)(DWORD, XINPUT_STATE *)) GetProcAddress(module, "XInputGetState");
			XInputGetCapabilitiesEx_proc = (DWORD (WINAPI *)(DWORD, DWORD, DWORD, LIBSTEM_XINPUT_CAPABILITIES_EX *)) GetProcAddress(module, (LPCSTR) 108);
		}
		
		module = LoadLibrary("DINPUT8.dll");
		if (module == NULL) {
			Gamepad_logCallback(gamepad_log_error, "Gamepad_init fatal error: Couldn't load DINPUT8.dll\n");
			return; // Shouldn't reach here if program treats gamepad_log_error as a fatal program error
		}
		DirectInput8Create_proc = (HRESULT (WINAPI *)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN)) GetProcAddress(module, "DirectInput8Create");
		result = DirectInput8Create_proc(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8, (void **) &directInputInterface, NULL);
		
		if (result != DI_OK) {
			Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Warning: DirectInput8Create returned 0x%X\n", (unsigned int) result));
		}
		
		inited = gamepad_true;
		s_bWindowsDeviceChanged = gamepad_true;
		Gamepad_detectDevices();
		CreateDeviceNotification(&notificationData);
	}
}

static void disposeDevice(struct Gamepad_device * deviceRecord) {
	struct Gamepad_devicePrivate * deviceRecordPrivate = deviceRecord->privateData;
	
	if (!deviceRecordPrivate->isXInput) {
		IDirectInputDevice8_Release(deviceRecordPrivate->deviceInterface);
		free(deviceRecordPrivate->axisOffsets);
		free(deviceRecordPrivate->buttonOffsets);
		free(deviceRecordPrivate->hatOffsets);
		free((void *) deviceRecord->description);
	}
	free(deviceRecordPrivate);
	
	free(deviceRecord->axisStates);
	free(deviceRecord->buttonStates);
	free(deviceRecord->hatStates);

	free(deviceRecord->axisBindings);
	free(deviceRecord->buttonBindings);
	free(deviceRecord->hatBindings);
	
	free(deviceRecord);
}

void Gamepad_shutdown() {
	unsigned int deviceIndex;
	
	if (inited) {
		for (deviceIndex = 0; deviceIndex < numDevices; deviceIndex++) {
			if (Gamepad_deviceRemoveCallback != NULL) {
				Gamepad_deviceRemoveCallback(devices[deviceIndex], Gamepad_deviceRemoveContext);
			}
			disposeDevice(devices[deviceIndex]);
		}
		free(devices);
		devices = NULL;
		numDevices = 0;
		for (unsigned int playerIndex = 0; playerIndex < 4; playerIndex++) {
      registeredXInputDevices[playerIndex] = NULL;
		}
		inited = gamepad_false;
		CleanupDeviceNotification(&notificationData);

		for (deviceIndex = 0; deviceIndex < Gamepad_mappingsCount; deviceIndex++) {
			free(Gamepad_mappings[deviceIndex].name);
		}
		free(Gamepad_mappings);
	}
}

unsigned int Gamepad_numDevices() {
	return numDevices;
}

struct Gamepad_device * Gamepad_deviceAtIndex(unsigned int deviceIndex) {
	if (deviceIndex >= numDevices) {
		return NULL;
	}
	return devices[deviceIndex];
}

static double currentTime() {
	static LARGE_INTEGER frequency;
	LARGE_INTEGER currentTime;
	
	if (frequency.QuadPart == 0) {
		QueryPerformanceFrequency(&frequency);
	}
	QueryPerformanceCounter(&currentTime);
	
	return (double) currentTime.QuadPart / frequency.QuadPart;
}

DEFINE_GUID(IID_ValveStreamingGamepad, MAKELONG(0x28DE, 0x11FF),0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
DEFINE_GUID(IID_X360WiredGamepad, MAKELONG(0x045E, 0x02A1),0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
DEFINE_GUID(IID_X360WirelessGamepad, MAKELONG(0x045E, 0x028E),0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);

static PRAWINPUTDEVICELIST rawDevList = NULL;
static UINT rawDevListCount = 0;

static void clearRawDevList()
{
	if (rawDevList)
	{
		free(rawDevList);
		rawDevList = NULL;
		rawDevListCount = 0;
	}
}

static gamepad_bool isXInputDevice(const GUID * pGuidProductFromDirectInput) {
	static const GUID * s_XInputProductGUID[] = {
		&IID_ValveStreamingGamepad,
		&IID_X360WiredGamepad,   // Microsoft's wired X360 controller for Windows
		&IID_X360WirelessGamepad // Microsoft's wireless X360 controller for Windows
	};
	
	size_t iDevice;
	UINT i;
	
	// Check for well known XInput device GUIDs
	// This lets us skip RAWINPUT for popular devices. Also, we need to do this for the Valve Streaming Gamepad because it's virtualized and doesn't show up in the device list.
	for (iDevice = 0; iDevice < sizeof(s_XInputProductGUID) / sizeof(s_XInputProductGUID[0]); ++iDevice) {
		if (!memcmp(pGuidProductFromDirectInput, s_XInputProductGUID[iDevice], sizeof(GUID))) {
			return gamepad_true;
		}
	}
	
	// Go through RAWINPUT (WinXP and later) to find HID devices.
	// Cache this if we end up using it.
	if (rawDevList == NULL) {
		if ((GetRawInputDeviceList(NULL, &rawDevListCount, sizeof(RAWINPUTDEVICELIST)) == (UINT) -1) || rawDevListCount == 0) {
			return gamepad_false;
		}
		
		rawDevList = malloc(sizeof(RAWINPUTDEVICELIST) * rawDevListCount);
		
		if (GetRawInputDeviceList(rawDevList, &rawDevListCount, sizeof(RAWINPUTDEVICELIST)) == (UINT) -1) {
			free(rawDevList);
			rawDevList = NULL;
			return gamepad_false;
		}
	}
	
	for (i = 0; i < rawDevListCount; i++) {
		RID_DEVICE_INFO rdi;
		char devName[128];
		UINT rdiSize = sizeof(rdi);
		UINT nameSize = sizeof(devName);
		
		rdi.cbSize = sizeof(rdi);
		if (rawDevList[i].dwType == RIM_TYPEHID &&
		    GetRawInputDeviceInfoA(rawDevList[i].hDevice, RIDI_DEVICEINFO, &rdi, &rdiSize) != (UINT) -1 &&
		    MAKELONG(rdi.hid.dwVendorId, rdi.hid.dwProductId) == (LONG) pGuidProductFromDirectInput->Data1 &&
		    GetRawInputDeviceInfoA(rawDevList[i].hDevice, RIDI_DEVICENAME, devName, &nameSize) != (UINT) -1 &&
		    strstr(devName, "IG_") != NULL) {
			return gamepad_true;
		}
	}
	
	return gamepad_false;
}

static BOOL CALLBACK countAxesCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID context) {
	struct Gamepad_device * deviceRecord = context;
	
	deviceRecord->numAxes++;
	return DIENUM_CONTINUE;
}

static BOOL CALLBACK countButtonsCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID context) {
	struct Gamepad_device * deviceRecord = context;
	
	deviceRecord->numButtons++;
	return DIENUM_CONTINUE;
}

static BOOL CALLBACK countHatsCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID context) {
	struct Gamepad_device* deviceRecord = context;

	deviceRecord->numHats++;
	return DIENUM_CONTINUE;
}

#define AXIS_MIN -32768
#define AXIS_MAX 32767

static BOOL CALLBACK enumAxesCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID context) {
	DWORD offset;
	DIPROPRANGE range;
	DIPROPDWORD deadZone;
	HRESULT result;

	struct Gamepad_device * deviceRecord = context;
	struct Gamepad_devicePrivate * deviceRecordPrivate = deviceRecord->privateData;
	
	if (!memcmp(&instance->guidType, &GUID_XAxis, sizeof(instance->guidType))) {
		offset = DIJOFS_X;
	} else if (!memcmp(&instance->guidType, &GUID_YAxis, sizeof(instance->guidType))) {
		offset = DIJOFS_Y;
	} else if (!memcmp(&instance->guidType, &GUID_ZAxis, sizeof(instance->guidType))) {
		offset = DIJOFS_Z;
	} else if (!memcmp(&instance->guidType, &GUID_RxAxis, sizeof(instance->guidType))) {
		offset = DIJOFS_RX;
	} else if (!memcmp(&instance->guidType, &GUID_RyAxis, sizeof(instance->guidType))) {
		offset = DIJOFS_RY;
	} else if (!memcmp(&instance->guidType, &GUID_RzAxis, sizeof(instance->guidType))) {
		offset = DIJOFS_RZ;
	} else if (!memcmp(&instance->guidType, &GUID_Slider, sizeof(instance->guidType))) {
		offset = DIJOFS_SLIDER(deviceRecordPrivate->sliderCount++);
	} else {
		offset = -1;
	}
	deviceRecordPrivate->axisOffsets[deviceRecord->numAxes] = offset;
	deviceRecord->numAxes++;

	range.diph.dwSize = sizeof(range);
	range.diph.dwHeaderSize = sizeof(range.diph);
	range.diph.dwObj = instance->dwType;
	range.diph.dwHow = DIPH_BYID;
	range.lMin = AXIS_MIN;
	range.lMax = AXIS_MAX;
		
	result = IDirectInputDevice8_SetProperty(deviceRecordPrivate->deviceInterface, DIPROP_RANGE, &range.diph);
	if (result != DI_OK) {
		Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Warning: IDIrectInputDevice8_SetProperty returned 0x%X\n", (unsigned int) result));
	}
		
	deadZone.diph.dwSize = sizeof(deadZone);
	deadZone.diph.dwHeaderSize = sizeof(deadZone.diph);
	deadZone.diph.dwObj = instance->dwType;
	deadZone.diph.dwHow = DIPH_BYID;
	deadZone.dwData = 0;
	result = IDirectInputDevice8_SetProperty(deviceRecordPrivate->deviceInterface, DIPROP_DEADZONE, &deadZone.diph);
	if (result != DI_OK) {
		Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Warning: IDIrectInputDevice8_SetProperty returned 0x%X\n", (unsigned int) result));
	}
	return DIENUM_CONTINUE;
}

static BOOL CALLBACK enumButtonsCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID context) {
	struct Gamepad_device * deviceRecord = context;
	struct Gamepad_devicePrivate * deviceRecordPrivate = deviceRecord->privateData;
	
	deviceRecordPrivate->buttonOffsets[deviceRecord->numButtons] = DIJOFS_BUTTON(deviceRecord->numButtons);
	deviceRecord->numButtons++;
	return DIENUM_CONTINUE;
}

static BOOL CALLBACK enumHatsCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID context) {
	struct Gamepad_device* deviceRecord = context;
	struct Gamepad_devicePrivate* deviceRecordPrivate = deviceRecord->privateData;

	deviceRecordPrivate->hatOffsets[deviceRecord->numHats] = DIJOFS_POV(deviceRecord->numHats);
	deviceRecord->numHats++;
	return DIENUM_CONTINUE;
}

#if defined _MSC_VER || defined __WINE_XINPUT_H
#ifndef DIDFT_OPTIONAL
#define DIDFT_OPTIONAL      0x80000000
#endif

/* Taken from Wine - Thanks! */
DIOBJECTDATAFORMAT dfDIJoystick2[] = {
	{ &GUID_XAxis, DIJOFS_X, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_YAxis, DIJOFS_Y, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_ZAxis, DIJOFS_Z, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RxAxis, DIJOFS_RX, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RyAxis, DIJOFS_RY, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RzAxis, DIJOFS_RZ, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_Slider, DIJOFS_SLIDER(0), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_Slider, DIJOFS_SLIDER(1), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_POV, DIJOFS_POV(0), DIDFT_OPTIONAL | DIDFT_POV | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_POV, DIJOFS_POV(1), DIDFT_OPTIONAL | DIDFT_POV | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_POV, DIJOFS_POV(2), DIDFT_OPTIONAL | DIDFT_POV | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_POV, DIJOFS_POV(3), DIDFT_OPTIONAL | DIDFT_POV | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(0), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(1), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(2), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(3), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(4), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(5), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(6), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(7), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(8), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(9), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(10), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(11), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(12), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(13), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(14), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(15), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(16), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(17), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(18), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(19), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(20), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(21), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(22), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(23), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(24), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(25), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(26), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(27), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(28), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(29), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(30), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(31), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(32), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(33), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(34), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(35), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(36), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(37), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(38), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(39), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(40), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(41), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(42), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(43), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(44), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(45), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(46), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(47), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(48), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(49), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(50), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(51), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(52), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(53), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(54), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(55), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(56), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(57), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(58), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(59), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(60), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(61), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(62), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(63), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(64), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(65), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(66), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(67), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(68), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(69), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(70), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(71), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(72), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(73), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(74), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(75), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(76), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(77), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(78), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(79), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(80), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(81), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(82), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(83), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(84), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(85), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(86), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(87), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(88), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(89), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(90), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(91), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(92), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(93), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(94), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(95), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(96), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(97), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(98), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(99), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(100), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(101), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(102), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(103), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(104), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(105), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(106), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(107), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(108), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(109), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(110), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(111), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(112), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(113), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(114), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(115), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(116), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(117), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(118), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(119), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(120), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(121), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(122), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(123), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(124), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(125), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(126), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ NULL, DIJOFS_BUTTON(127), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_XAxis, FIELD_OFFSET(DIJOYSTATE2, lVX), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_YAxis, FIELD_OFFSET(DIJOYSTATE2, lVY), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_ZAxis, FIELD_OFFSET(DIJOYSTATE2, lVZ), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RxAxis, FIELD_OFFSET(DIJOYSTATE2, lVRx), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RyAxis, FIELD_OFFSET(DIJOYSTATE2, lVRy), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RzAxis, FIELD_OFFSET(DIJOYSTATE2, lVRz), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_Slider, FIELD_OFFSET(DIJOYSTATE2, rglVSlider[0]), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_Slider, FIELD_OFFSET(DIJOYSTATE2, rglVSlider[1]), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_XAxis, FIELD_OFFSET(DIJOYSTATE2, lAX), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_YAxis, FIELD_OFFSET(DIJOYSTATE2, lAY), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_ZAxis, FIELD_OFFSET(DIJOYSTATE2, lAZ), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RxAxis, FIELD_OFFSET(DIJOYSTATE2, lARx), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RyAxis, FIELD_OFFSET(DIJOYSTATE2, lARy), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RzAxis, FIELD_OFFSET(DIJOYSTATE2, lARz), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_Slider, FIELD_OFFSET(DIJOYSTATE2, rglASlider[0]), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_Slider, FIELD_OFFSET(DIJOYSTATE2, rglASlider[1]), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_XAxis, FIELD_OFFSET(DIJOYSTATE2, lFX), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_YAxis, FIELD_OFFSET(DIJOYSTATE2, lFY), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_ZAxis, FIELD_OFFSET(DIJOYSTATE2, lFZ), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RxAxis, FIELD_OFFSET(DIJOYSTATE2, lFRx), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RyAxis, FIELD_OFFSET(DIJOYSTATE2, lFRy), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_RzAxis, FIELD_OFFSET(DIJOYSTATE2, lFRz), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_Slider, FIELD_OFFSET(DIJOYSTATE2, rglFSlider[0]), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
	{ &GUID_Slider, FIELD_OFFSET(DIJOYSTATE2, rglFSlider[1]), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
};

const DIDATAFORMAT c_dfDIJoystick2 = {
	sizeof(DIDATAFORMAT),
	sizeof(DIOBJECTDATAFORMAT),
	DIDF_ABSAXIS,
	sizeof(DIJOYSTATE2),
	sizeof(dfDIJoystick2) / sizeof(dfDIJoystick2[0]),
	dfDIJoystick2
};
#endif

static int compareOffset(const void * e1, const void * e2) {
	const DWORD * o1 = e1;
	const DWORD * o2 = e2;

	if (*o1 < *o2) {
		return -1;
	} else if (*o1 > *o2) {
		return 1;
	} else {
		return 0;
	}
}

static uint16_t crc8(uint8_t r) {
	uint16_t crc = 0;

	for (int i = 0; i < 8; i++, r >>= 1) {
		crc = ((crc ^ r) & 1 ? 0xa001 : 0) ^ crc >> 1;
	}

	return crc;
}

static void buildGuid(struct Gamepad_device * deviceRecord, gamepad_bool isXInput) {
	unsigned int i;
	uint16_t crc;

	if (isXInput) {
		memset(&deviceRecord->guid, 0, sizeof(deviceRecord->guid));
		memcpy(&deviceRecord->guid, "xinput", strlen("xinput"));
	}
	else {
		crc = 0;

		if (deviceRecord->description) {
			for (i = 0; deviceRecord->description[i] != 0; i++) {
				crc = crc8((uint8_t)crc ^ (uint8_t)deviceRecord->description[i]) ^ crc >> 8;
			}
		}

		deviceRecord->guid.standard.bus = 0x03; // usb
		deviceRecord->guid.standard.crc = crc;
		deviceRecord->guid.standard.vendor = deviceRecord->vendorID;
		deviceRecord->guid.standard.zero1 = 0;
		deviceRecord->guid.standard.product = deviceRecord->productID;
		deviceRecord->guid.standard.zero2 = 0;
		deviceRecord->guid.standard.version = 1;
		deviceRecord->guid.standard.driver = 'h';
		deviceRecord->guid.standard.info = 0;
	}
}

static BOOL CALLBACK enumDevicesCallback(const DIDEVICEINSTANCE * instance, LPVOID context) {
	struct Gamepad_device * deviceRecord;
	struct Gamepad_devicePrivate * deviceRecordPrivate;
	unsigned int deviceIndex;
	IDirectInputDevice * diDevice;
	IDirectInputDevice8 * di8Device;
	HRESULT result;
	DIPROPDWORD bufferSizeProp;
	gamepad_bool buffered = gamepad_true;
	
	if (xInputAvailable && isXInputDevice(&instance->guidProduct)) {
		return DIENUM_CONTINUE;
	}
	
	for (deviceIndex = 0; deviceIndex < numDevices; deviceIndex++) {
		if (!memcmp(&((struct Gamepad_devicePrivate *) devices[deviceIndex]->privateData)->guidInstance, &instance->guidInstance, sizeof(GUID))) {
			return DIENUM_CONTINUE;
		}
	}
	
	result = IDirectInput8_CreateDevice(directInputInterface, &instance->guidInstance, &diDevice, NULL);
	if (result != DI_OK) {
		Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Warning: IDirectInput8_CreateDevice returned 0x%X\n", (unsigned int) result));
	}
	result = IDirectInputDevice8_QueryInterface(diDevice, &IID_IDirectInputDevice8, (LPVOID *) &di8Device);
	if (result != DI_OK) {
		Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Warning: IDirectInputDevice8_QueryInterface returned 0x%X\n", (unsigned int) result));
	}
	IDirectInputDevice8_Release(diDevice);
	
	result = IDirectInputDevice8_SetCooperativeLevel(di8Device, GetActiveWindow(), DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	if (result != DI_OK) {
		Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Warning: IDirectInputDevice8_SetCooperativeLevel returned 0x%X\n", (unsigned int) result));
	}
	
	result = IDirectInputDevice8_SetDataFormat(di8Device, &c_dfDIJoystick2);
	if (result != DI_OK) {
		Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Warning: IDirectInputDevice8_SetDataFormat returned 0x%X\n", (unsigned int) result));
	}
	
	bufferSizeProp.diph.dwSize = sizeof(DIPROPDWORD);
	bufferSizeProp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	bufferSizeProp.diph.dwObj = 0;
	bufferSizeProp.diph.dwHow = DIPH_DEVICE;
	bufferSizeProp.dwData = INPUT_QUEUE_SIZE;
	result = IDirectInputDevice8_SetProperty(di8Device, DIPROP_BUFFERSIZE, &bufferSizeProp.diph);
	if (result == DI_POLLEDDEVICE) {
		buffered = gamepad_false;
	} else if (result != DI_OK) {
		Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Warning: IDirectInputDevice8_SetProperty returned 0x%X\n", (unsigned int) result));
	}
	
	deviceRecord = malloc(sizeof(struct Gamepad_device));
	deviceRecordPrivate = malloc(sizeof(struct Gamepad_devicePrivate));
	deviceRecordPrivate->guidInstance = instance->guidInstance;
	deviceRecordPrivate->isXInput = gamepad_false;
	deviceRecordPrivate->deviceInterface = di8Device;
	deviceRecordPrivate->buffered = buffered;
	deviceRecordPrivate->sliderCount = 0;
	deviceRecord->privateData = deviceRecordPrivate;
	deviceRecord->deviceID = nextDeviceID++;
	deviceRecord->description = strdup(instance->tszProductName);
	deviceRecord->vendorID = instance->guidProduct.Data1 & 0xFFFF;
	deviceRecord->productID = instance->guidProduct.Data1 >> 16 & 0xFFFF;
	buildGuid(deviceRecord, gamepad_false);
	// if no valid mapping, skip
	const struct Gamepad_mapping * deviceMap = Gamepad_findMapping(deviceRecord);
	if (!deviceMap) {
		free(deviceRecordPrivate);
		free(deviceRecord);
		return DIENUM_CONTINUE;
	}
	deviceRecord->numAxes = 0;
	IDirectInputDevice_EnumObjects(di8Device, countAxesCallback, deviceRecord, DIDFT_AXIS);
	deviceRecord->numButtons = 0;
	IDirectInputDevice_EnumObjects(di8Device, countButtonsCallback, deviceRecord, DIDFT_BUTTON);
	deviceRecord->numHats = 0;
	IDirectInputDevice_EnumObjects(di8Device, countHatsCallback, deviceRecord, DIDFT_POV);
	deviceRecord->axisStates = calloc(sizeof(float), deviceRecord->numAxes);
	deviceRecord->buttonStates = calloc(sizeof(gamepad_bool), deviceRecord->numButtons);
	deviceRecord->hatStates = calloc(sizeof(char), deviceRecord->numHats);
	deviceRecordPrivate->axisOffsets = calloc(sizeof(DWORD), deviceRecord->numAxes);
	deviceRecordPrivate->buttonOffsets = calloc(sizeof(DWORD), deviceRecord->numButtons);
	deviceRecordPrivate->hatOffsets = calloc(sizeof(DWORD), deviceRecord->numHats);
	deviceRecord->numAxes = 0;
	IDirectInputDevice_EnumObjects(di8Device, enumAxesCallback, deviceRecord, DIDFT_AXIS);
	qsort(deviceRecordPrivate->axisOffsets, deviceRecord->numAxes, sizeof(DWORD), compareOffset);
	deviceRecord->numButtons = 0;
	IDirectInputDevice_EnumObjects(di8Device, enumButtonsCallback, deviceRecord, DIDFT_BUTTON);
	qsort(deviceRecordPrivate->buttonOffsets, deviceRecord->numButtons, sizeof(DWORD), compareOffset);
	deviceRecord->numHats = 0;
	IDirectInputDevice_EnumObjects(di8Device, enumHatsCallback, deviceRecord, DIDFT_POV);
	qsort(deviceRecordPrivate->hatOffsets, deviceRecord->numHats, sizeof(DWORD), compareOffset);
	devices = realloc(devices, sizeof(struct Gamepad_device *) * (numDevices + 1));
	devices[numDevices++] = deviceRecord;
	// populate bindings from discovered mapping after qsorts
	assignDeviceBindings(deviceRecord, deviceMap);
	if (Gamepad_deviceAttachCallback != NULL) {
		Gamepad_deviceAttachCallback(deviceRecord, Gamepad_deviceAttachContext);
	}
	
	return DIENUM_CONTINUE;
}

static void removeDevice(unsigned int deviceIndex) {
	if (Gamepad_deviceRemoveCallback != NULL) {
		Gamepad_deviceRemoveCallback(devices[deviceIndex], Gamepad_deviceRemoveContext);
	}
	
	disposeDevice(devices[deviceIndex]);
	numDevices--;
	for (; deviceIndex < numDevices; deviceIndex++) {
		devices[deviceIndex] = devices[deviceIndex + 1];
	}

	clearRawDevList();
}

void Gamepad_detectDevices() {
	HRESULT result;
	DWORD xResult;
	LIBSTEM_XINPUT_CAPABILITIES_EX capabilities_ex;
	unsigned int playerIndex, deviceIndex;
	
	if (!inited) {
		return;
	}
	
	CheckDeviceNotification(&notificationData);
	if (s_bWindowsDeviceChanged) {
		result = IDirectInput_EnumDevices(directInputInterface, DI8DEVCLASS_GAMECTRL, enumDevicesCallback, NULL, DIEDFL_ALLDEVICES);
		if (result != DI_OK) {
			Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Warning: IDirectInput_EnumDevices returned 0x%X\n", (unsigned int) result));
		}
		s_bWindowsDeviceChanged = gamepad_false;
	}
	
	if (xInputAvailable) {
		for (playerIndex = 0; playerIndex < 4; playerIndex++) {
			xResult = XInputGetCapabilitiesEx_proc(1, playerIndex, 0, &capabilities_ex);
			if (xResult == ERROR_SUCCESS && registeredXInputDevices[playerIndex] == NULL) {
				struct Gamepad_device * deviceRecord;
				struct Gamepad_devicePrivate * deviceRecordPrivate;
				
				deviceRecord = malloc(sizeof(struct Gamepad_device));
				deviceRecordPrivate = malloc(sizeof(struct Gamepad_devicePrivate));
				deviceRecordPrivate->isXInput = gamepad_true;
				deviceRecordPrivate->playerIndex = playerIndex;
				deviceRecord->privateData = deviceRecordPrivate;
				deviceRecord->deviceID = nextDeviceID++;
				deviceRecord->vendorID = capabilities_ex.VendorId;
				deviceRecord->productID = capabilities_ex.ProductId;
				deviceRecord->description = NULL;
				// Need to build a non-XInput GUID to find the real name, if present
				buildGuid(deviceRecord, gamepad_false);
				deviceRecord->description = Gamepad_findXInputDeviceName(deviceRecord);
				if (!deviceRecord->description) {
					deviceRecord->description = xInputDeviceNames[playerIndex];
				}
				// Create the XInput GUID to actually find the mapping
				buildGuid(deviceRecord, gamepad_true);
				// check for valid mapping, but this really shouldn't happen with xinput devices
				const struct Gamepad_mapping * deviceMap = Gamepad_findMapping(deviceRecord);
				if (!deviceMap) {
					free(deviceRecordPrivate);
					free(deviceRecord);
					registeredXInputDevices[playerIndex] = NULL;
					continue;
				}
				deviceRecord->numAxes = 6;
				deviceRecord->numButtons = 11;
				deviceRecord->numHats = 1;
				deviceRecord->axisStates = calloc(sizeof(float), deviceRecord->numAxes);
				deviceRecord->buttonStates = calloc(sizeof(gamepad_bool), deviceRecord->numButtons);
				deviceRecord->hatStates = calloc(sizeof(char), deviceRecord->numHats);
				devices = realloc(devices, sizeof(struct Gamepad_device *) * (numDevices + 1));
				devices[numDevices++] = deviceRecord;
				registeredXInputDevices[playerIndex] = deviceRecord;
				// populate bindings from discovered mapping
				assignDeviceBindings(deviceRecord, deviceMap);
				if (Gamepad_deviceAttachCallback != NULL) {
					Gamepad_deviceAttachCallback(deviceRecord, Gamepad_deviceAttachContext);
				}
			} else if (xResult != ERROR_SUCCESS && registeredXInputDevices[playerIndex] != NULL) {
				for (deviceIndex = 0; deviceIndex < numDevices; deviceIndex++) {
					if (devices[deviceIndex] == registeredXInputDevices[playerIndex]) {
						removeDevice(deviceIndex);
						break;
					}
				}
				registeredXInputDevices[playerIndex] = NULL;
			}
		}
	}
}

static void updateButtonValue(struct Gamepad_device * device, unsigned int buttonIndex, gamepad_bool down, double timestamp) {
	if (down != device->buttonStates[buttonIndex]) {
		device->buttonStates[buttonIndex] = down;
		const struct Gamepad_binding * bind = device->buttonBindings[buttonIndex];
		if (bind) {
			if (down) {
					if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && Gamepad_axisMoveCallback != NULL) {
						Gamepad_axisMoveCallback(device, bind->output.axis.axis, 1.0f, -1.0f, timestamp, Gamepad_axisMoveContext);
						if (Gamepad_debugEvents) {
							Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: 1.0\n", device->description, bind->output.axis.axis));
						}
					} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON && Gamepad_buttonDownCallback != NULL) {
						Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
						if (Gamepad_debugEvents) {
							Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
						}
					}
			} else {
					if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && Gamepad_axisMoveCallback != NULL) {
						Gamepad_axisMoveCallback(device, bind->output.axis.axis, -1.0f, 1.0f, timestamp, Gamepad_axisMoveContext);
						if (Gamepad_debugEvents) {
							Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: -1.0\n", device->description, bind->output.axis.axis));
						}
					} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON && Gamepad_buttonUpCallback != NULL) {
						Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
						if (Gamepad_debugEvents) {
							Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
						}
					}
			}
		}
	}
}

static void updateAxisValueFloat(struct Gamepad_device * device, unsigned int axisIndex, float value, double timestamp) {
	float lastValue;
	lastValue = device->axisStates[axisIndex];
	device->axisStates[axisIndex] = value;
	const struct Gamepad_binding * bind = device->axisBindings[axisIndex];
	if (bind && value != lastValue) {
		if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && Gamepad_axisMoveCallback != NULL) {
			Gamepad_axisMoveCallback(device, bind->output.axis.axis, value, lastValue, timestamp, Gamepad_axisMoveContext);
			if (Gamepad_debugEvents) {
				Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: %f\n", device->description, bind->output.axis.axis, value));
			}
		}
		// A little guesswork here, but I'm not sure in which situations a non-hat axis would have a button output in SDL
		else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON) {
			if (lastValue < 0 && value > 0 && Gamepad_buttonDownCallback != NULL) {
				Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
				}
			} else if (lastValue > 0 && value < 0 && Gamepad_buttonUpCallback != NULL) {
				Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
				}
			}
		}
	}
}

static void updateAxisValue(struct Gamepad_device * device, unsigned int axisIndex, LONG ivalue, double timestamp) {
	updateAxisValueFloat(device, axisIndex, (ivalue - AXIS_MIN) / (float) (AXIS_MAX - AXIS_MIN) * 2.0f - 1.0f, timestamp);
}

static void updateHatValue(struct Gamepad_device* device, unsigned int hatIndex, DWORD value, double timestamp) {
	static const char states[] = {
		GAMEPAD_HAT_UP,
		GAMEPAD_HAT_UP | GAMEPAD_HAT_RIGHT,
		GAMEPAD_HAT_RIGHT,
		GAMEPAD_HAT_RIGHT | GAMEPAD_HAT_DOWN,
		GAMEPAD_HAT_DOWN,
		GAMEPAD_HAT_DOWN | GAMEPAD_HAT_LEFT,
		GAMEPAD_HAT_LEFT,
		GAMEPAD_HAT_LEFT | GAMEPAD_HAT_UP
	};

	char lastValue;

	lastValue = device->hatStates[hatIndex];

	if (LOWORD(value) == 0xffff) {
		value = 0;
	} else {
		value += 4500 / 2;
		value %= 36000;
		value /= 4500;
		if (value >= 8) {
			value = 0; // shouldn't happen
		}
		value = states[value];
	}
	device->hatStates[hatIndex] = value;
	const struct Gamepad_binding * bind = NULL;

	unsigned int value_diff = lastValue ^ value;
	gamepad_bool button_up_callback = (Gamepad_buttonUpCallback != NULL);
	gamepad_bool button_down_callback = (Gamepad_buttonDownCallback != NULL);
	gamepad_bool axis_move_callback = (Gamepad_axisMoveCallback != NULL);

	if (value_diff & GAMEPAD_HAT_UP) {
		bind = device->hatBindings[hatIndex];
		if (bind) {
			if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && axis_move_callback) {
				if (lastValue < 0) {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, 1.0f, -1.0f, timestamp, Gamepad_axisMoveContext);
				} else {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, -1.0f, 1.0f, timestamp, Gamepad_axisMoveContext);
				}
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: %f\n", device->description, bind->output.axis.axis, value));
				}
			} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON) {
				if (button_up_callback && (lastValue & GAMEPAD_HAT_UP)) {
					Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
					}
				} else if (button_down_callback && (value & GAMEPAD_HAT_UP)) {
					Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
					}
				}
			}
		}
	}
	if (value_diff & GAMEPAD_HAT_RIGHT) {
		bind = device->hatBindings[hatIndex+1];
		if (bind) {
			if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && axis_move_callback) {
				if (lastValue < 0) {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, 1.0f, -1.0f, timestamp, Gamepad_axisMoveContext);
				} else {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, -1.0f, 1.0f, timestamp, Gamepad_axisMoveContext);
				}
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: %f\n", device->description, bind->output.axis.axis, value));
				}
			} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON) {
				if (button_up_callback && (lastValue & GAMEPAD_HAT_RIGHT)) {
					Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
					}
				} else if (button_down_callback && (value & GAMEPAD_HAT_RIGHT)) {
					Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
					}
				}
			}
		}
	}
	if (value_diff & GAMEPAD_HAT_DOWN) {
		bind = device->hatBindings[hatIndex+2];
		if (bind) {
			if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && axis_move_callback) {
				if (lastValue < 0) {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, 1.0f, -1.0f, timestamp, Gamepad_axisMoveContext);
				} else {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, -1.0f, 1.0f, timestamp, Gamepad_axisMoveContext);
				}
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: %f\n", device->description, bind->output.axis.axis, value));
				}
			} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON) {
				if (button_up_callback && (lastValue & GAMEPAD_HAT_DOWN)) {
					Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
					}
				} else if (button_down_callback && (value & GAMEPAD_HAT_DOWN)) {
					Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
					}
				}
			}
		}
	}
	if (value_diff & GAMEPAD_HAT_LEFT) {
		bind = device->hatBindings[hatIndex+3];
		if (bind) {
			if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && axis_move_callback) {
				if (lastValue < 0) {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, 1.0f, -1.0f, timestamp, Gamepad_axisMoveContext);
				} else {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, -1.0f, 1.0f, timestamp, Gamepad_axisMoveContext);
				}
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: %f\n", device->description, bind->output.axis.axis, value));
				}
			} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON) {
				if (button_up_callback && (lastValue & GAMEPAD_HAT_LEFT)) {
					Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
					}
				} else if (button_down_callback && (value & GAMEPAD_HAT_LEFT)) {
					Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
					}
				}
			}
		}
	}
}

static void updateHatValueXInput(struct Gamepad_device* device, char value, double timestamp) {
	char lastValue = device->hatStates[0];

	device->hatStates[0] = value;
	const struct Gamepad_binding * bind = NULL;

	unsigned int value_diff = lastValue ^ value;
	gamepad_bool button_up_callback = (Gamepad_buttonUpCallback != NULL);
	gamepad_bool button_down_callback = (Gamepad_buttonDownCallback != NULL);
	gamepad_bool axis_move_callback = (Gamepad_axisMoveCallback != NULL);

	if (value_diff & GAMEPAD_HAT_UP) {
		bind = device->hatBindings[0];
		if (bind) {
			if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && axis_move_callback) {
				if (lastValue < 0) {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, 1.0f, -1.0f, timestamp, Gamepad_axisMoveContext);
				} else {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, -1.0f, 1.0f, timestamp, Gamepad_axisMoveContext);
				}
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: %f\n", device->description, bind->output.axis.axis, value));
				}
			} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON) {
				if (button_up_callback && (lastValue & GAMEPAD_HAT_UP)) {
					Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
					}
				} else if (button_down_callback && (value & GAMEPAD_HAT_UP)) {
					Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
					}
				}
			}
		}
	}
	if (value_diff & GAMEPAD_HAT_RIGHT) {
		bind = device->hatBindings[1];
		if (bind) {
			if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && axis_move_callback) {
				if (lastValue < 0) {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, 1.0f, -1.0f, timestamp, Gamepad_axisMoveContext);
				} else {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, -1.0f, 1.0f, timestamp, Gamepad_axisMoveContext);
				}
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: %f\n", device->description, bind->output.axis.axis, value));
				}
			} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON) {
				if (button_up_callback && (lastValue & GAMEPAD_HAT_RIGHT)) {
					Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
					}
				} else if (button_down_callback && (value & GAMEPAD_HAT_RIGHT)) {
					Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
					}
				}
			}
		}
	}
	if (value_diff & GAMEPAD_HAT_DOWN) {
		bind = device->hatBindings[2];
		if (bind) {
			if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && axis_move_callback) {
				if (lastValue < 0) {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, 1.0f, -1.0f, timestamp, Gamepad_axisMoveContext);
				} else {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, -1.0f, 1.0f, timestamp, Gamepad_axisMoveContext);
				}
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: %f\n", device->description, bind->output.axis.axis, value));
				}
			} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON) {
				if (button_up_callback && (lastValue & GAMEPAD_HAT_DOWN)) {
					Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
					}
				} else if (button_down_callback && (value & GAMEPAD_HAT_DOWN)) {
					Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
					}
				}
			}
		}
	}
	if (value_diff & GAMEPAD_HAT_LEFT) {
		bind = device->hatBindings[3];
		if (bind) {
			if (bind->outputType == GAMEPAD_BINDINGTYPE_AXIS && axis_move_callback) {
				if (lastValue < 0) {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, 1.0f, -1.0f, timestamp, Gamepad_axisMoveContext);
				} else {
					Gamepad_axisMoveCallback(device, bind->output.axis.axis, -1.0f, 1.0f, timestamp, Gamepad_axisMoveContext);
				}
				if (Gamepad_debugEvents) {
					Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Axis %u: %f\n", device->description, bind->output.axis.axis, value));
				}
			} else if (bind->outputType == GAMEPAD_BINDINGTYPE_BUTTON) {
				if (button_up_callback && (lastValue & GAMEPAD_HAT_LEFT)) {
					Gamepad_buttonUpCallback(device, bind->output.button, timestamp, Gamepad_buttonUpContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Up\n", device->description, bind->output.button));
					}
				} else if (button_down_callback && (value & GAMEPAD_HAT_LEFT)) {
					Gamepad_buttonDownCallback(device, bind->output.button, timestamp, Gamepad_buttonDownContext);
					if (Gamepad_debugEvents) {
						Gamepad_logCallback(gamepad_log_default, Gamepad_formatLogMessage("%s Button %u: Down\n", device->description, bind->output.button));
					}
				}
			}
		}
	}
}

void Gamepad_processEvents() {
	static gamepad_bool inProcessEvents;
	unsigned int deviceIndex, buttonIndex, axisIndex, hatIndex;
	struct Gamepad_device * device;
	struct Gamepad_devicePrivate * devicePrivate;
	HRESULT result;
	
	if (!inited || inProcessEvents) {
		return;
	}
	
	inProcessEvents = gamepad_true;
	for (deviceIndex = 0; deviceIndex < numDevices; deviceIndex++) {
		device = devices[deviceIndex];
		devicePrivate = device->privateData;
		
		if (devicePrivate->isXInput) {
			XINPUT_STATE state;
			DWORD xResult = XInputGetState_proc(devicePrivate->playerIndex, &state);

			if (xResult == ERROR_SUCCESS) {
				const double now = currentTime();

				updateButtonValue(device, 0, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_A), now);
				updateButtonValue(device, 1, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_B), now);
				updateButtonValue(device, 2, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_X), now);
				updateButtonValue(device, 3, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_Y), now);
				updateButtonValue(device, 4, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER), now);
				updateButtonValue(device, 5, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER), now);
				updateButtonValue(device, 6, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK), now);
				updateButtonValue(device, 7, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_START), now);
				updateButtonValue(device, 8, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB), now);
				updateButtonValue(device, 9, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB), now);
				updateButtonValue(device, 10, !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE), now);
				updateAxisValue(device, 0, state.Gamepad.sThumbLX, now);
				updateAxisValue(device, 1, -state.Gamepad.sThumbLY, now);
				updateAxisValueFloat(device, 2, state.Gamepad.bLeftTrigger / 127.5f - 1.0f, now);
				updateAxisValue(device, 3, state.Gamepad.sThumbRX, now);
				updateAxisValue(device, 4, -state.Gamepad.sThumbRY, now);
				updateAxisValueFloat(device, 5, state.Gamepad.bRightTrigger / 127.5f - 1.0f, now);
				char hat = 0;
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) {
					hat |= GAMEPAD_HAT_UP;
				}
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
					hat |= GAMEPAD_HAT_RIGHT;
				}
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
					hat |= GAMEPAD_HAT_DOWN;
				}
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
					hat |= GAMEPAD_HAT_LEFT;
				}
				if (hat != device->hatStates[0]) {
					updateHatValueXInput(device, hat, now);
				}
			} else {
				registeredXInputDevices[devicePrivate->playerIndex] = NULL;
				removeDevice(deviceIndex);
				deviceIndex--;
				continue;
			}
			
		} else {
			result = IDirectInputDevice8_Poll(devicePrivate->deviceInterface);
			if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
				IDirectInputDevice8_Acquire(devicePrivate->deviceInterface);
				IDirectInputDevice8_Poll(devicePrivate->deviceInterface);
			}
			
			if (devicePrivate->buffered) {
				DWORD eventCount = INPUT_QUEUE_SIZE;
				DIDEVICEOBJECTDATA events[INPUT_QUEUE_SIZE];
				unsigned int eventIndex;
				
				result = IDirectInputDevice8_GetDeviceData(devicePrivate->deviceInterface, sizeof(DIDEVICEOBJECTDATA), events, &eventCount, 0);
				if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
					IDirectInputDevice8_Acquire(devicePrivate->deviceInterface);
					result = IDirectInputDevice8_GetDeviceData(devicePrivate->deviceInterface, sizeof(DIDEVICEOBJECTDATA), events, &eventCount, 0);
				}
				if (!SUCCEEDED(result)) {
					removeDevice(deviceIndex);
					deviceIndex--;
					continue;
				}
				
				for (eventIndex = 0; eventIndex < eventCount; eventIndex++) {
					for (buttonIndex = 0; buttonIndex < device->numButtons; buttonIndex++) {
						if (events[eventIndex].dwOfs == devicePrivate->buttonOffsets[buttonIndex]) {
							updateButtonValue(device, buttonIndex, !!events[eventIndex].dwData, events[eventIndex].dwTimeStamp / 1000.0);
						}
					}
					for (axisIndex = 0; axisIndex < device->numAxes; axisIndex++) {
						if (events[eventIndex].dwOfs == devicePrivate->axisOffsets[axisIndex]) {
							updateAxisValue(device, axisIndex, events[eventIndex].dwData, events[eventIndex].dwTimeStamp / 1000.0);
						}
					}
					for (hatIndex = 0; hatIndex < device->numHats; hatIndex++) {
						if (events[eventIndex].dwOfs == devicePrivate->hatOffsets[hatIndex]) {
							updateHatValue(device, hatIndex, events[eventIndex].dwData, events[eventIndex].dwTimeStamp / 1000.0);
						}
					}
				}
				
			} else {
				DIJOYSTATE2 state;
				const double now = currentTime();
				
				result = IDirectInputDevice8_GetDeviceState(devicePrivate->deviceInterface, sizeof(DIJOYSTATE2), &state);
				if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
					IDirectInputDevice8_Acquire(devicePrivate->deviceInterface);
					result = IDirectInputDevice8_GetDeviceState(devicePrivate->deviceInterface, sizeof(DIJOYSTATE2), &state);
				}
				
				if (!SUCCEEDED(result)) {
					removeDevice(deviceIndex);
					deviceIndex--;
					continue;
				}
				
				for (buttonIndex = 0; buttonIndex < device->numButtons; buttonIndex++) {
					updateButtonValue(device, buttonIndex, !!state.rgbButtons[buttonIndex], now);
				}
				
				for (axisIndex = 0; axisIndex < device->numAxes; axisIndex++) {
					switch (devicePrivate->axisOffsets[axisIndex]) {
						case DIJOFS_X:
							updateAxisValue(device, axisIndex, state.lX, now);
							break;
						case DIJOFS_Y:
							updateAxisValue(device, axisIndex, state.lY, now);
							break;
						case DIJOFS_Z:
							updateAxisValue(device, axisIndex, state.lZ, now);
							break;
						case DIJOFS_RX:
							updateAxisValue(device, axisIndex, state.lRx, now);
							break;
						case DIJOFS_RY:
							updateAxisValue(device, axisIndex, state.lRy, now);
							break;
						case DIJOFS_RZ:
							updateAxisValue(device, axisIndex, state.lRz, now);
							break;
						case DIJOFS_SLIDER(0):
							updateAxisValue(device, axisIndex, state.rglSlider[0], now);
							break;
						case DIJOFS_SLIDER(1):
							updateAxisValue(device, axisIndex, state.rglSlider[1], now);
							break;
						case DIJOFS_POV(0):
							updateHatValue(device, axisIndex, state.rgdwPOV[0], now);
							break;
						case DIJOFS_POV(1):
							updateHatValue(device, axisIndex, state.rgdwPOV[1], now);
							break;
						case DIJOFS_POV(2):
							updateHatValue(device, axisIndex, state.rgdwPOV[2], now);
							break;
						case DIJOFS_POV(3):
							updateHatValue(device, axisIndex, state.rgdwPOV[3], now);
							break;
					}
				}
			}
		}
	}
	inProcessEvents = gamepad_false;
}
