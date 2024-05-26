// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: f0a60cf8f34b68e47e9ac15c0d20e0b3a0f56097 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2005 by Simon Howard
// Copyright (C) 2006-2020 by The Odamex Team.
// Copyright (C) 2024 by The MUD Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	[Russell] - Added some functions and cleaned up a few areas
//
//-----------------------------------------------------------------------------

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct _MEMFILE MEMFILE;

typedef enum
{
    MEM_SEEK_SET,
    MEM_SEEK_CUR,
    MEM_SEEK_END
} mem_rel_t;

MEMFILE *mem_fopen_read(void *buf, size_t buflen);
size_t   mem_fread(void *buf, size_t size, size_t nmemb, MEMFILE *stream);
MEMFILE *mem_fopen_write(void);
size_t   mem_fwrite(const void *ptr, size_t size, size_t nmemb, MEMFILE *stream);
void     mem_get_buf(MEMFILE *stream, void **buf, size_t *buflen);
void     mem_fclose(MEMFILE *stream);
uint8_t *mem_get_buf_and_close(MEMFILE *stream);
long     mem_ftell(MEMFILE *stream);
int32_t  mem_fseek(MEMFILE *stream, int32_t offset, mem_rel_t whence);
size_t   mem_fsize(MEMFILE *stream);   // [Russell] - get size of stream
char    *mem_fgetbuf(MEMFILE *stream); // [Russell] - return stream buffer
