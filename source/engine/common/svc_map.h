// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: aa7fa21865cff46264f94869b46bf11a471c92b0 $
//
// Copyright (C) 2021 by Alex Mayfield.
// Copyright (C) 2024 by The MUD Team
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
//   Server message map.
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_net.h"

namespace google
{
namespace protobuf
{
class Descriptor;
}
} // namespace google

const google::protobuf::Descriptor *SVC_ResolveHeader(const uint8_t header);
svc_t                               SVC_ResolveDescriptor(const google::protobuf::Descriptor *desc);
