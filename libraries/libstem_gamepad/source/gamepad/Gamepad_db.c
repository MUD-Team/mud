#include "Gamepad.h"
#include "Gamepad_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Some Linux GUIDs seem to work with Windows DInput device identification
// Not sure how to account for this yet
#define SDL_JOYSTICK_DINPUT
#define SDL_JOYSTICK_XINPUT
#define SDL_JOYSTICK_LINUX

#include "SDL_gamepad_db.h"

struct Gamepad_mapping * Gamepad_mappings = NULL;
unsigned int Gamepad_mappingsCount = 0;
static unsigned int Gamepad_mappingsCapacity = 0;

static int getDecimal(int k) {
	switch (k) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return k - '0';

	default:
		return -1;
	}
};

static int getHexadecimal(int k) {
	switch (k) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return k - '0';

	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		return k - 'a' + 10;

	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		return k - 'A' + 10;

	default:
		return -1;
	}
};

static gamepad_bool isAlnum(int k) {
	switch (k) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	case 'a': case 'b': case 'c': case 'd': case 'e':
	case 'f': case 'g': case 'h': case 'i': case 'j':
	case 'k': case 'l': case 'm': case 'n': case 'o':
	case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E':
	case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O':
	case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
		return gamepad_true;

	default:
		return gamepad_false;
	}
}

static gamepad_bool parseControllerAxis(char const** line, enum Gamepad_controllerAxis* axis) {
	uint32_t hash = 5381;
	char const* rewind = *line;

	while (isAlnum(**line)) {
		hash = hash * 33 + (uint8_t)**line;
		(*line)++;
	}

	switch (hash) {
	case 0x0fda7a28U: /* leftx         */ *axis = GAMEPAD_CONTROLLERAXIS_LEFTX;        break;
	case 0x0fda7a29U: /* lefty         */ *axis = GAMEPAD_CONTROLLERAXIS_LEFTY;        break;
	case 0x19716e3bU: /* rightx        */ *axis = GAMEPAD_CONTROLLERAXIS_RIGHTX;       break;
	case 0x19716e3cU: /* righty        */ *axis = GAMEPAD_CONTROLLERAXIS_RIGHTY;       break;
	case 0x3110e864U: /* lefttrigger   */ *axis = GAMEPAD_CONTROLLERAXIS_LEFT_TRIGGER;  break;
	case 0xcba15eb7U: /* righttrigger  */ *axis = GAMEPAD_CONTROLLERAXIS_RIGHT_TRIGGER; break;

	default:
		*line = rewind;
		return gamepad_false;
	}

	return gamepad_true;
}

static gamepad_bool parseControllerButton(char const** line, enum Gamepad_controllerButton* button) {
	uint32_t hash = 5381;
	char const* rewind = *line;

	while (isAlnum(**line)) {
		hash = hash * 33 + (uint8_t)**line;
		(*line)++;
	}

	switch (hash) {
	case 0x0002b606U: /* a             */ *button = GAMEPAD_CONTROLLERBUTTON_SOUTH;         break;
	case 0x0002b607U: /* b             */ *button = GAMEPAD_CONTROLLERBUTTON_EAST;          break;
	case 0x0002b61dU: /* x             */ *button = GAMEPAD_CONTROLLERBUTTON_WEST;          break;
	case 0x0002b61eU: /* y             */ *button = GAMEPAD_CONTROLLERBUTTON_NORTH;         break;
	case 0x7c947676U: /* back          */ *button = GAMEPAD_CONTROLLERBUTTON_BACK;          break;
	case 0x0f88d053U: /* guide         */ *button = GAMEPAD_CONTROLLERBUTTON_GUIDE;         break;
	case 0x106149d3U: /* start         */ *button = GAMEPAD_CONTROLLERBUTTON_START;         break;
	case 0x0f423b6eU: /* leftstick     */ *button = GAMEPAD_CONTROLLERBUTTON_LEFT_STICK;     break;
	case 0x348b0101U: /* rightstick    */ *button = GAMEPAD_CONTROLLERBUTTON_RIGHT_STICK;    break;
	case 0x7424c3b6U: /* leftshoulder  */ *button = GAMEPAD_CONTROLLERBUTTON_LEFT_SHOULDER;  break;
	case 0x60c40469U: /* rightshoulder */ *button = GAMEPAD_CONTROLLERBUTTON_RIGHT_SHOULDER; break;
	case 0x7c95d15eU: /* dpup          */ *button = GAMEPAD_CONTROLLERBUTTON_DPAD_UP;        break;
	case 0xf94659f1U: /* dpdown        */ *button = GAMEPAD_CONTROLLERBUTTON_DPAD_DOWN;      break;
	case 0xf94a9044U: /* dpleft        */ *button = GAMEPAD_CONTROLLERBUTTON_DPAD_LEFT;      break;
	case 0x230b6077U: /* dpright       */ *button = GAMEPAD_CONTROLLERBUTTON_DPAD_RIGHT;     break;
	case 0x0feef902U: /* misc1         */ *button = GAMEPAD_CONTROLLERBUTTON_MISC_1;         break;
	case 0x9ac8d7c0U: /* paddle1       */ *button = GAMEPAD_CONTROLLERBUTTON_RIGHT_PADDLE_1;  break;
	case 0x9ac8d7c1U: /* paddle2       */ *button = GAMEPAD_CONTROLLERBUTTON_RIGHT_PADDLE_2;   break;
	case 0x9ac8d7c2U: /* paddle3       */ *button = GAMEPAD_CONTROLLERBUTTON_RIGHT_PADDLE_2;  break;
	case 0x9ac8d7c3U: /* paddle4       */ *button = GAMEPAD_CONTROLLERBUTTON_LEFT_PADDLE_2;   break;
	case 0x022e13ddU: /* touchpad      */ *button = GAMEPAD_CONTROLLERBUTTON_TOUCHPAD;      break;

	default:
		*line = rewind;
		return gamepad_false;
	}

	return gamepad_true;
}

static gamepad_bool parseu8(char const** line, uint8_t* value) {
	int digit = 0;
	*value = 0;

	if ((digit = getDecimal(**line)) != -1) {
		do {
			*value = *value * 10 + digit;
			(*line)++;
		}
		while ((digit = getDecimal(**line)) != -1);
	} else {
		Gamepad_logCallback(gamepad_log_warning, "Invalid hexadecimal digit in CRC16");
		return gamepad_false;
	}

	return gamepad_true;
};

static gamepad_bool parseCrc(char const** line, uint16_t* value) {
	int digit = 0;
	*value = 0;

	for (size_t i = 0; i < 4; i++) {
		if ((digit = getHexadecimal(**line)) == -1) {
			Gamepad_logCallback(gamepad_log_warning, "Invalid hexadecimal digit in CRC16");
			return gamepad_false;
		}

		*value = *value * 16 + digit;
		(*line)++;
	}

	return gamepad_true;
};

static gamepad_bool parseBinding(char const** line, union Gamepad_guid* guid, struct Gamepad_binding* bind) {
	char halfAxisInput = 0;
	char halfAxisOutput = 0;

	if ((*line)[0] == 'h' && (*line)[1] == 'i' && (*line)[2] == 'n' && (*line)[3] == 't' && (*line)[4] == ':') {
		(*line) += 5;

		while (**line != 0 && **line != ',') {
			(*line)++;
		}

		return gamepad_true;
	}

	if ((*line)[0] == 'c' && (*line)[1] == 'r' && (*line)[2] == 'c' && (*line)[3] == ':') {
		uint16_t crc = 0;
		(*line) += 4;

		if (!parseCrc(line, &crc)) {
			return gamepad_false;
		}

		if (guid->standard.crc == 0) {
			guid->standard.crc = crc;
		}

		if (**line != 0) {
			(*line)++;
		}
	}

	if (**line == '+' || **line == '-') {
		halfAxisOutput = **line;
		(*line)++;
	}

	if (parseControllerAxis(line, &bind->output.axis.axis)) {
		bind->outputType = GAMEPAD_BINDINGTYPE_AXIS;

		gamepad_bool const istrigger = bind->output.axis.axis == GAMEPAD_CONTROLLERAXIS_LEFT_TRIGGER
							|| bind->output.axis.axis == GAMEPAD_CONTROLLERAXIS_RIGHT_TRIGGER;

		if (istrigger) {
			bind->output.axis.min = 0.0f;
			bind->output.axis.max = 1.0f;
		}
		else {
			if (halfAxisOutput == '+') {
				bind->output.axis.min = 0.0f;
				bind->output.axis.max = 1.0f;
			}
			else if (halfAxisOutput == '-') {
				bind->output.axis.min =  0.0f;
				bind->output.axis.max = -1.0f;
			}
			else {
				bind->output.axis.min = -1.0f;
				bind->output.axis.max =  1.0f;
			}
		}
	} else if (parseControllerButton(line, &bind->output.button)) {
		bind->outputType = GAMEPAD_BINDINGTYPE_BUTTON;
	} else {
		Gamepad_logCallback(gamepad_log_warning, "Invalid controller button or axis");
		return gamepad_false;
	}

	if (**line != ':') {
		return gamepad_false;
	}

	(*line)++;

	if (**line == '+' || **line == '-') {
		halfAxisInput = **line;
		(*line)++;
	}

	if (**line == 'a') {
		bind->inputType = GAMEPAD_BINDINGTYPE_AXIS;
		(*line)++;

		if (!parseu8(line, &bind->input.axis.axis)) {
			Gamepad_logCallback(gamepad_log_warning, "Invalid joystick axis number");
			return gamepad_false;
		}

		if (halfAxisInput == '+') {
			bind->input.axis.min = 0.0f;
			bind->input.axis.max = 1.0f;
		} else if (halfAxisInput == '-') {
			bind->input.axis.min =  0.0f;
			bind->input.axis.max = -1.0f;
		} else {
			bind->input.axis.min = -1.0f;
			bind->input.axis.max =  1.0f;
		}

		if (**line == '~') {
			const float tmp = bind->input.axis.min;
			bind->input.axis.min = bind->input.axis.max;
			bind->input.axis.max = tmp;
			(*line)++;
		}
	} else if (**line == 'b') {
		bind->inputType = GAMEPAD_BINDINGTYPE_BUTTON;
		(*line)++;

		if (!parseu8(line, &bind->input.button)) {
			Gamepad_logCallback(gamepad_log_warning, "Invalid joystick button number");
			return gamepad_false;
		}
	} else if (**line == 'h') {
		bind->inputType = GAMEPAD_BINDINGTYPE_HAT;
		(*line)++;

		if (!parseu8(line, &bind->input.hat.hat)) {
			Gamepad_logCallback(gamepad_log_warning, "Invalid joystick button number");
			return gamepad_false;
		}

		if (**line != '.') {
			Gamepad_logCallback(gamepad_log_warning, "Invalid joystick hat specifier");
			return gamepad_false;
		}

		(*line)++;

		if (!parseu8(line, &bind->input.hat.mask)) {
			Gamepad_logCallback(gamepad_log_warning, "Invalid joystick button number");
			return gamepad_false;
		}
	} else if (**line == 0 || **line == ',') {
		bind->inputType = GAMEPAD_BINDINGTYPE_NONE;
	} else {
		Gamepad_logCallback(gamepad_log_warning, "Invalid joystick input");
		return gamepad_false;
	}

	return gamepad_true;
}

static gamepad_bool parseMapping(char const* line, struct Gamepad_mapping * mapping) {
	const char* const original = line;
	size_t len;
	char* name;

	memset(mapping, 0, sizeof(*mapping));

	// GUID
	{
#define TRY(s) if (strncmp(line, s, strlen(s)) == 0) { memcpy(&mapping->guid, s, strlen(s)); line += strlen(s); }

		TRY("xinput")
		else TRY("hidapi")
		else {
			size_t j  = 0;
			int    d1 = 0, d2 = 0;

			while (j < 16 && (d1 = getHexadecimal(line[0])) != -1 && (d2 = getHexadecimal(line[1])) != -1) {
				mapping->guid.data[j++] = d1 * 16 + d2;
				line += 2;
			}

			if (j != 16) {
				Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Invalid GUID in mapping database: %s", original));
				return gamepad_false;
			}
		}

#undef TRY

		if (*line != ',') {
			Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Invalid GUID in mapping database: %s", original));
			return gamepad_false;
		}

		line++;
	}

	// Name
	{
		const char* start = line;

		while (*line != 0 && *line != ',') {
			line++;
		}

		len = line - start;
		name = (char*)malloc(len + 1);

		if (name == NULL) {
			Gamepad_logCallback(gamepad_log_warning, "Could not allocate memory for controller mapping name");
			return gamepad_false;
		}

		memcpy(name, start, len);
		name[len] = 0;
		mapping->name = name;

		if (*line != ',') {
			Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Invalid name in mapping database: %s", original));
			return gamepad_false;
		}

		line++;
	}

	// Properties
	while (*line != 0) {
		struct Gamepad_binding bind;

		if (!parseBinding(&line, &mapping->guid, &bind)) {
			return gamepad_false;
		}

		switch (bind.outputType) {
		case GAMEPAD_BINDINGTYPE_BUTTON:
			switch (bind.output.button) {
			case GAMEPAD_CONTROLLERBUTTON_SOUTH:          mapping->south         = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_EAST:           mapping->east          = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_WEST:           mapping->west          = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_NORTH:          mapping->north         = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_BACK:           mapping->back          = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_GUIDE:          mapping->guide         = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_START:          mapping->start         = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_LEFT_STICK:     mapping->leftStick     = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_RIGHT_STICK:    mapping->rightStick    = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_LEFT_SHOULDER:  mapping->leftShoulder  = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_RIGHT_SHOULDER: mapping->rightShoulder = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_DPAD_UP:        mapping->dpadUp        = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_DPAD_DOWN:      mapping->dpadDown      = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_DPAD_LEFT:      mapping->dpadLeft      = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_DPAD_RIGHT:     mapping->dpadRight     = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_MISC_1:         mapping->misc1         = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_RIGHT_PADDLE_1: mapping->rightPaddle1  = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_LEFT_PADDLE_1:  mapping->leftPaddle1   = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_RIGHT_PADDLE_2: mapping->rightPaddle2  = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_LEFT_PADDLE_2:  mapping->leftPaddle2   = bind; break;
			case GAMEPAD_CONTROLLERBUTTON_TOUCHPAD:       mapping->touchpad      = bind; break;
			}

			break;

		case GAMEPAD_BINDINGTYPE_AXIS:
			switch (bind.output.axis.axis) {
			case GAMEPAD_CONTROLLERAXIS_LEFTX:         mapping->leftX        = bind; break;
			case GAMEPAD_CONTROLLERAXIS_LEFTY:         mapping->leftY        = bind; break;
			case GAMEPAD_CONTROLLERAXIS_RIGHTX:        mapping->rightX       = bind; break;
			case GAMEPAD_CONTROLLERAXIS_RIGHTY:        mapping->rightY       = bind; break;
			case GAMEPAD_CONTROLLERAXIS_LEFT_TRIGGER:  mapping->leftTrigger  = bind; break;
			case GAMEPAD_CONTROLLERAXIS_RIGHT_TRIGGER: mapping->rightTrigger = bind; break;
			}

			break;

		case GAMEPAD_BINDINGTYPE_HAT:
			Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Invalid output binding (Hat): %s", original));
			break;
		}

		if (*line != 0) {
			if (*line != ',') {
				Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Invalid character in binding: %c (%s)", *line, original));
				return gamepad_false;
			}

			line++;
		}
	}

	return gamepad_true;
}

gamepad_bool Gamepad_initMappings(void) {
	unsigned int capacity, count, i;
	struct Gamepad_mapping * mappings;

	capacity = sizeof(s_GamepadMappings) / sizeof(s_GamepadMappings[0]);
	mappings = (struct Gamepad_mapping *)malloc(capacity * sizeof(struct Gamepad_mapping));

	if (mappings == NULL) {
		Gamepad_logCallback(gamepad_log_warning, Gamepad_formatLogMessage("Could not allocate memory for %u controller mapping(s)", capacity));
		return gamepad_false;
	}

	count = 0;

	for (i = 0; s_GamepadMappings[i] != NULL; i++) {
		struct Gamepad_mapping mapping;
		if (parseMapping(s_GamepadMappings[i], &mapping)) {
			mappings[count++] = mapping;
		}
	}

	if (count < capacity) {
		capacity = count;
		mappings = (struct Gamepad_mapping *)realloc(mappings, capacity * sizeof(struct Gamepad_mapping));
	}

	Gamepad_mappings = mappings;
	Gamepad_mappingsCount = count;
	Gamepad_mappingsCapacity = capacity;
	return gamepad_true;
}

gamepad_bool Gamepad_addMapping(const char* string) {
	struct Gamepad_mapping mapping;
	const gamepad_bool ok = parseMapping(string, &mapping);

	if (!ok) {
		return gamepad_false;
	}

	if (Gamepad_mappingsCount == Gamepad_mappingsCapacity) {
		const unsigned int newCapacity = Gamepad_mappingsCapacity * 2;
		const unsigned int newSize = newCapacity * sizeof(struct Gamepad_mapping);
		struct Gamepad_mapping * newMappings = (struct Gamepad_mapping *)realloc(Gamepad_mappings, newSize);

		if (newMappings == NULL) {
			Gamepad_logCallback(gamepad_log_warning, "Could not allocate memory for the mapping");
			return gamepad_false;
		}

		Gamepad_mappings = newMappings;
		Gamepad_mappingsCapacity = newCapacity;
	}

	Gamepad_mappings[Gamepad_mappingsCount++] = mapping;
	return gamepad_true;
}

const struct Gamepad_mapping * Gamepad_findMapping(struct Gamepad_device * device) {
	const gamepad_bool raw   = strcmp("xinput", (const char *)device->guid.data) == 0;
	unsigned int i;

	if (raw) {
		for (i = 0; i < Gamepad_mappingsCount; i++) {
			if (memcmp(&Gamepad_mappings[i].guid, &device->guid, sizeof(device->guid)) == 0) {
				return &Gamepad_mappings[i];
			}
		}
	}
	else {
		const uint16_t crc = device->guid.standard.crc;
		const uint16_t vendor = device->guid.standard.vendor;
		const uint16_t product = device->guid.standard.product;

		for (i = 0; i < Gamepad_mappingsCount; i++) {
			struct Gamepad_mapping * mapping = &Gamepad_mappings[i];

			gamepad_bool match = crc != 0 && mapping->guid.standard.crc != 0 ? crc == mapping->guid.standard.crc : gamepad_true;
			match      = match && mapping->guid.standard.vendor  == vendor;
			match      = match && mapping->guid.standard.product == product;

			if (match) {
				return mapping;
			}
		}
	}

	return NULL;
}

const char * Gamepad_findXInputDeviceName(struct Gamepad_device * device) {
	const uint16_t crc = device->guid.standard.crc;
	const uint16_t vendor = device->guid.standard.vendor;
	const uint16_t product = device->guid.standard.product;

	for (unsigned int i = 0; i < Gamepad_mappingsCount; i++) {
		struct Gamepad_mapping * mapping = &Gamepad_mappings[i];

		gamepad_bool match = crc != 0 && mapping->guid.standard.crc != 0 ? crc == mapping->guid.standard.crc : gamepad_true;
		match      = match && mapping->guid.standard.vendor  == vendor;
		match      = match && mapping->guid.standard.product == product;

		if (match) {
			return mapping->name;
		}
	}

	return NULL;
}
