// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 039f1d6b8b299ee4c6c72901ab7028da4412a750 $
//
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
//	HTTP Downloading.
//
//-----------------------------------------------------------------------------

#pragma once

#include "m_resfile.h"
#include "otransfer.h"

/**
 * @brief Set if the client should reconnect to the last server upon completion
 *        of the download.
 */
#define DL_RECONNECT (1 << 0)

typedef std::vector<std::string> Websites;

void              CL_DownloadInit();
void              CL_DownloadShutdown();
bool              CL_IsDownloading();
bool              CL_StartDownload(const Websites &urls, const OWantFile &filename, uint32_t flags);
bool              CL_StopDownload();
void              CL_DownloadTick();
std::string       CL_DownloadFilename();
OTransferProgress CL_DownloadProgress();
