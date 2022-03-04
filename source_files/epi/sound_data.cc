//----------------------------------------------------------------------------
//  Sound Data
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#include "epi.h"
#include "sound_data.h"

#include <vector>

namespace epi
{

sound_data_c::sound_data_c() :
	length(0), freq(0), mode(0),
	data_L(NULL), data_R(NULL),
	fx_data_L(NULL), fx_data_R(NULL),
	priv_data(NULL), ref_count(0), is_sfx(false),
	current_mix(SFX_None), reverbed_room_size(RM_None)
{ }

sound_data_c::~sound_data_c()
{
	Free();
}

void sound_data_c::Free()
{
	length = 0;

	if (data_R && data_R != data_L)
		delete[] data_R;

	if (data_L)
		delete[] data_L;

	data_L = NULL;
	data_R = NULL;

	Free_FX();
}

void sound_data_c::Free_FX()
{
	if (fx_data_R && fx_data_R != fx_data_L)
		delete[] fx_data_R;

	if (fx_data_L)
		delete[] fx_data_L;

	fx_data_L = NULL;
	fx_data_R = NULL;
}

void sound_data_c::Allocate(int samples, int buf_mode)
{
	// early out when requirements are already met
	if (data_L && length >= samples && mode == buf_mode)
	{
		length = samples;  // FIXME: perhaps keep allocated count
		return;
	}

	if (data_L || data_R)
	{
		Free();
	}

	length = samples;
	mode   = buf_mode;

	switch (buf_mode)
	{
		case SBUF_Mono:
			data_L = new s16_t[samples];
			data_R = data_L;
			break;

		case SBUF_Stereo:
			data_L = new s16_t[samples];
			data_R = new s16_t[samples];
			break;

		case SBUF_Interleaved:
			data_L = new s16_t[samples * 2];
			data_R = data_L;
			break;

		default: break;
	}
}

void sound_data_c::Mix_Submerged()
{
	if (current_mix != SFX_Submerged)
	{
		// Setup lowpass + reverb parameters
		int out_L = 0;
		int accum_L = 0;
    	int out_R = 0;
    	int accum_R = 0;
    	int k = 3;
		s16_t *reverb_buffer_L;
		s16_t *reverb_buffer_R;
		int write_pos = 0;
		int read_pos = 0;
		int reverb_ratio = 20;
		int reverb_delay = 200;

		switch (mode)
		{
			case SBUF_Mono:
				if (!fx_data_L)
					fx_data_L = new s16_t[length];
				fx_data_R = fx_data_L;
				reverb_buffer_L = new s16_t[length];
				memset(reverb_buffer_L, 0, length * sizeof(s16_t));
				for (int i = 0; i < length; i++) 
				{
					fx_data_L[i] = out_L = accum_L >> k;
                	accum_L = accum_L - out_L + data_L[i];
					reverb_buffer_L[write_pos] = data_L[i];
					int reverbed = fx_data_L[i] + reverb_buffer_L[read_pos] * reverb_ratio / 100;
					fx_data_L[i] = CLAMP(INT16_MIN, reverbed, INT16_MAX);
					write_pos = (write_pos + 1) % (length);
					read_pos = (read_pos + 1) % (length);
				}
				current_mix = SFX_Submerged;
				delete[] reverb_buffer_L;
				reverb_buffer_L = NULL;
				break;

			case SBUF_Stereo:
				if (!fx_data_L)
					fx_data_L = new s16_t[length];
				if (!fx_data_R)
					fx_data_R = new s16_t[length];
				reverb_buffer_L = new s16_t[length];
				reverb_buffer_R = new s16_t[length];
				memset(reverb_buffer_L, 0, length * sizeof(s16_t));
				memset(reverb_buffer_R, 0, length * sizeof(s16_t));
				for (int i = 0; i < length; i++) 
				{
					fx_data_L[i] = out_L = accum_L >> k;
                	accum_L = accum_L - out_L + data_L[i];
					fx_data_R[i] = out_R = accum_R >> k;
                	accum_R = accum_R - out_R + data_R[i];
					reverb_buffer_L[write_pos] = data_L[i];
					reverb_buffer_R[write_pos] = data_R[i];
					int reverbed_L = fx_data_L[i] + reverb_buffer_L[read_pos] * reverb_ratio / 100;
					int reverbed_R = fx_data_R[i] + reverb_buffer_R[read_pos] * reverb_ratio / 100;
					fx_data_L[i] = CLAMP(INT16_MIN, reverbed_L, INT16_MAX);
					fx_data_R[i] = CLAMP(INT16_MIN, reverbed_R, INT16_MAX);
					write_pos = (write_pos + 1) % (length);
					read_pos = (read_pos + 1) % (length);
				}
				current_mix = SFX_Submerged;
				delete[] reverb_buffer_L;
				delete[] reverb_buffer_R;
				reverb_buffer_L = NULL;
				reverb_buffer_R = NULL;
				break;

			case SBUF_Interleaved:
				if (!fx_data_L)
					fx_data_L = new s16_t[length * 2];
				fx_data_R = fx_data_L;
				reverb_buffer_L = new s16_t[length * 2];
				memset(reverb_buffer_L, 0, length * sizeof(s16_t) * 2);
				for (int i = 0; i < length * 2; i++) 
				{
					fx_data_L[i] = out_L = accum_L >> k;
                	accum_L = accum_L - out_L + data_L[i];
					reverb_buffer_L[write_pos] = data_L[i];
					int reverbed = fx_data_L[i] + reverb_buffer_L[read_pos] * reverb_ratio / 100;
					fx_data_L[i] = CLAMP(INT16_MIN, reverbed, INT16_MAX);
					write_pos = (write_pos + 1) % (length * 2);
					read_pos = (read_pos + 1) % (length * 2);
				}
				current_mix = SFX_Submerged;
				delete[] reverb_buffer_L;
				reverb_buffer_L = NULL;
				break;
		}
	}
}

void sound_data_c::Mix_Vacuum()
{
	if (current_mix != SFX_Vacuum)
	{
		// Setup lowpass parameters
		int out_L = 0;
		int accum_L = 0;
    	int out_R = 0;
    	int accum_R = 0;
    	int k = 4;

		switch (mode)
		{
			case SBUF_Mono:
				if (!fx_data_L)
					fx_data_L = new s16_t[length];
				fx_data_R = fx_data_L;
				for (int i = 0; i < length; i++) 
				{
					fx_data_L[i] = out_L = accum_L >> k;
                	accum_L = accum_L - out_L + data_L[i];
				}
				current_mix = SFX_Vacuum;
				break;

			case SBUF_Stereo:
				if (!fx_data_L)
					fx_data_L = new s16_t[length];
				if (!fx_data_R)
					fx_data_R = new s16_t[length];
				for (int i = 0; i < length; i++) 
				{
					fx_data_L[i] = out_L = accum_L >> k;
                	accum_L = accum_L - out_L + data_L[i];
					fx_data_R[i] = out_R = accum_R >> k;
                	accum_R = accum_R - out_R + data_R[i];
				}
				current_mix = SFX_Vacuum;
				break;

			case SBUF_Interleaved:
				if (!fx_data_L)
					fx_data_L = new s16_t[length * 2];
				fx_data_R = fx_data_L;
				for (int i = 0; i < length * 2; i++) 
				{
					fx_data_L[i] = out_L = accum_L >> k;
                	accum_L = accum_L - out_L + data_L[i];
				}
				current_mix = SFX_Vacuum;
				break;
		}
	}
}

void sound_data_c::Mix_Reverb(float room_area)
{
	reverb_room_size_e current_room_size;
	if (room_area > 1000000)
		current_room_size = RM_Large;
	else if (room_area > 200000)
		current_room_size = RM_Medium;
	else
		current_room_size = RM_Small;

	if (current_mix != SFX_Reverb || reverbed_room_size != current_room_size)
	{
		// Setup reverb parameters
		s16_t *reverb_buffer_L;
		s16_t *reverb_buffer_R;
		int write_pos = 0;
		int read_pos = 0;
		int reverb_ratio, reverb_delay;
		switch (current_room_size)
		{
			case RM_Large:
				reverb_ratio = 20;
				reverb_delay = 200;
				break;
			case RM_Medium:
				reverb_ratio = 20;
				reverb_delay = 200;
				break;
			case RM_Small:
				reverb_ratio = 20;
				reverb_delay = 200;
				break;
		}
		switch (mode)
		{

			case SBUF_Mono:
				if (!fx_data_L)
					fx_data_L = new s16_t[length];
				fx_data_R = fx_data_L;
				memcpy(fx_data_L, data_L, length * sizeof(s16_t));
				current_mix = SFX_Reverb;
				reverbed_room_size = current_room_size;
				break;
			case SBUF_Stereo:
				if (!fx_data_L)
					fx_data_L = new s16_t[length];
				if (!fx_data_R)
					fx_data_R = new s16_t[length];
				memcpy(fx_data_L, data_L, length * sizeof(s16_t));
				memcpy(fx_data_R, data_R, length * sizeof(s16_t));
				current_mix = SFX_Reverb;
				reverbed_room_size = current_room_size;
				break;
			case SBUF_Interleaved:
				if (!fx_data_L)
					fx_data_L = new s16_t[length * 2];
				fx_data_R = fx_data_L;
				memcpy(fx_data_L, data_L, length * 2 * sizeof(s16_t));
				current_mix = SFX_Reverb;
				reverbed_room_size = current_room_size;
				break;

			/*case SBUF_Mono:
				if (!fx_data_L)
					fx_data_L = new s16_t[length];
				fx_data_R = fx_data_L;
				reverb_buffer_L = new s16_t[length];
				memset(reverb_buffer_L, 0, length * sizeof(s16_t));
				read_pos = ((write_pos - reverb_delay * freq / 1000) + length) % (length);
				for (int i = 0; i < length; i++) 
				{
					reverb_buffer_L[write_pos] = data_L[i];
					int reverbed = data_L[i] + reverb_buffer_L[read_pos] * reverb_ratio / 100;
					fx_data_L[i] = CLAMP(INT16_MIN, reverbed, INT16_MAX);
					write_pos = (write_pos + 1) % (length);
					read_pos = (read_pos + 1) % (length);
				}
				current_mix = SFX_Reverb;
				reverbed_room_size = current_room_size;
				delete[] reverb_buffer_L;
				reverb_buffer_L = NULL;
				break;

			case SBUF_Stereo:
				if (!fx_data_L)
					fx_data_L = new s16_t[length];
				if (!fx_data_R)
					fx_data_R = new s16_t[length];
				reverb_buffer_L = new s16_t[length];
				reverb_buffer_R = new s16_t[length];
				memset(reverb_buffer_L, 0, length * sizeof(s16_t));
				memset(reverb_buffer_R, 0, length * sizeof(s16_t));
				for (int i = 0; i < length; i++) 
				{
					reverb_buffer_L[write_pos] = data_L[i];
					reverb_buffer_R[write_pos] = data_R[i];
					int reverbed_L = data_L[i] + reverb_buffer_L[read_pos] * reverb_ratio / 100;
					int reverbed_R = data_R[i] + reverb_buffer_R[read_pos] * reverb_ratio / 100;
					fx_data_L[i] = CLAMP(INT16_MIN, reverbed_L, INT16_MAX);
					fx_data_R[i] = CLAMP(INT16_MIN, reverbed_R, INT16_MAX);
					write_pos = (write_pos + 1) % (length);
					read_pos = (read_pos + 1) % (length);					
				}
				current_mix = SFX_Reverb;
				reverbed_room_size = current_room_size;
				delete[] reverb_buffer_L;
				delete[] reverb_buffer_R;
				reverb_buffer_L = NULL;
				reverb_buffer_R = NULL;
				break;

			case SBUF_Interleaved:
				if (!fx_data_L)
					fx_data_L = new s16_t[length * 2];
				fx_data_R = fx_data_L;
				reverb_buffer_L = new s16_t[length * 2];
				memset(reverb_buffer_L, 0, length * sizeof(s16_t) * 2);
				read_pos = ((write_pos - reverb_delay * freq / 1000) + length * 2) % (length * 2);
				for (int i = 0; i < length * 2; i++) 
				{
					reverb_buffer_L[write_pos] = data_L[i];
					int reverbed = data_L[i] + reverb_buffer_L[read_pos] * reverb_ratio / 100;
					fx_data_L[i] = CLAMP(INT16_MIN, reverbed, INT16_MAX);
					write_pos = (write_pos + 1) % (length * 2);
					read_pos = (read_pos + 1) % (length * 2);
				}
				current_mix = SFX_Reverb;
				reverbed_room_size = current_room_size;
				delete[] reverb_buffer_L;
				reverb_buffer_L = NULL;
				break;*/
		}
	}
}

}  // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
