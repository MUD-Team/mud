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

#ifndef __EPI_SOUNDDATA_H__
#define __EPI_SOUNDDATA_H__

namespace epi
{

typedef enum
{
	SBUF_Mono = 0,
	SBUF_Stereo = 1,
	SBUF_Interleaved = 2
}
sfx_buffer_mode_e;

typedef enum
{
	SFX_None = 0,
	SFX_Vacuum = 1,
	SFX_Submerged = 2,
	SFX_Reverb = 3
}
mixed_sfx_type_e;

typedef enum
{
	RM_None = 0,
	RM_Small = 1,
	RM_Medium = 2,
	RM_Large = 3
}
reverb_room_size_e;

class sound_data_c
{
public:
	int length; // number of samples
	int freq;   // frequency
	int mode;   // one of the SBUF_xxx values

	// signed 16-bit samples.
	// For SBUF_Mono, both pointers refer to the same memory.
	// For SBUF_Interleaved, only data_L is used and contains
	// both channels, left samples before right samples.
	s16_t *data_L;
	s16_t *data_R;

	// Floating-point versions of the above. Created and cached for FX mixing.
	float *float_data_L;
	float *float_data_R;

	// Temp buffer for mixed SFX. Will be overwritten as needed.
	s16_t *fx_data_L;
	s16_t *fx_data_R;

	// values for the engine to use
	void *priv_data;

	int ref_count;

	bool is_sfx;

	mixed_sfx_type_e current_mix;

	reverb_room_size_e reverbed_room_size;

public:
	sound_data_c();
	~sound_data_c();

	void Allocate(int samples, int buf_mode);
	void Allocate_Reverb();
	void Free();
	void Free_Float();
	void Free_FX();
	void Mix_Float();
	void Mix_Vacuum();
	void Mix_Submerged();
	void Mix_Reverb(float room_area);
};

} // namespace epi

#endif /* __EPI_SOUNDDATA_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
