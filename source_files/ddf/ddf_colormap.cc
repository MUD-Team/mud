//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Colourmaps)
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#include "ddf_colormap.h"

#include "ddf_local.h"
#include "epi_str_compare.h"

static Colormap *dynamic_colmap;

ColormapContainer colormaps;

void DDFColmapGetSpecial(const char *info, void *storage);

static Colormap dummy_colmap;

static const DDFCommandList colmap_commands[] = {DDF_FIELD("SPECIAL", dummy_colmap, special_, DDFColmapGetSpecial),
                                                 DDF_FIELD("GL_COLOUR", dummy_colmap, gl_color_, DDFMainGetRGB),
                                                 {nullptr, nullptr, 0, nullptr}};

//
//  DDF PARSE ROUTINES
//

static void ColmapStartEntry(const char *name, bool extend)
{
    if (!name || name[0] == 0)
    {
        DDFWarnError("New colormap entry is missing a name!");
        name = "COLORMAP_WITH_NO_NAME";
    }

    dynamic_colmap = colormaps.Lookup(name);

    if (extend)
    {
        if (!dynamic_colmap)
            DDFError("Unknown colormap to extend: %s\n", name);
        return;
    }

    // replaces the existing entry
    if (dynamic_colmap)
    {
        dynamic_colmap->Default();
        return;
    }

    // not found, create a new one
    dynamic_colmap = new Colormap;
    dynamic_colmap->name_ = name;
    colormaps.push_back(dynamic_colmap);
}

static void ColmapParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DDF_DEBUG)
    LogDebug("COLMAP_PARSE: %s = %s;\n", field, contents);
#endif
    if (DDFMainParseField(colmap_commands, field, contents, (uint8_t *)dynamic_colmap))
        return; // OK

    DDFError("Unknown colmap.ddf command: %s\n", field);
}

static void ColmapFinishEntry(void)
{
    if (dynamic_colmap->gl_color_ == kRGBANoValue)
    {
        DDFWarnError("Colourmap entry missing GL_COLOUR.\n");
        // We are now assuming that the intent is to remove all
        // colmaps with this name (i.e., "null" it), as the only way to get here
        // is to create an empty entry or use gl_color_= NONE;
        std::string doomed_name = dynamic_colmap->name_;
        for (std::vector<Colormap *>::iterator iter = colormaps.begin(); iter != colormaps.end(); iter++)
        {
            Colormap *cmap = *iter;
            if (DDFCompareName(doomed_name.c_str(), cmap->name_.c_str()) == 0)
            {
                delete cmap;
                cmap = nullptr;
                iter = colormaps.erase(iter);
            }
            else
                ++iter;
        }
    }
}

static void ColmapClearAll(void)
{
    LogWarning("Ignoring #CLEARALL in colormap.ddf\n");
}

void DDFReadColourMaps(const std::string &data)
{
    DDFReadInfo colm_r;

    colm_r.tag        = "COLOURMAPS";
    colm_r.short_name = "DDFCOLM";

    colm_r.start_entry  = ColmapStartEntry;
    colm_r.parse_field  = ColmapParseField;
    colm_r.finish_entry = ColmapFinishEntry;
    colm_r.clear_all    = ColmapClearAll;

    DDFMainReadFile(&colm_r, data);
}

void DDFColmapInit(void)
{
    for (Colormap *cmap : colormaps)
    {
        delete cmap;
        cmap = nullptr;
    }
    colormaps.clear();
}

void DDFColmapCleanUp(void)
{
    colormaps.shrink_to_fit();
}

DDFSpecialFlags colmap_specials[] = {{"FLASH", kColorSpecialNoFlash, true},
                                     {"WHITEN", kColorSpecialWhiten, false},
                                     {nullptr, 0, 0}};

//
// DDFColmapGetSpecial
//
// Gets the colormap specials.
//
void DDFColmapGetSpecial(const char *info, void *storage)
{
    ColorSpecial *spec = (ColorSpecial *)storage;

    int flag_value;

    switch (DDFMainCheckSpecialFlag(info, colmap_specials, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        *spec = (ColorSpecial)(*spec | flag_value);
        break;

    case kDDFCheckFlagNegative:
        *spec = (ColorSpecial)(*spec & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("DDFColmapGetSpecial: Unknown Special: %s", info);
        break;
    }
}

// --> Colourmap Class

//
// Colormap Constructor
//
Colormap::Colormap() : name_()
{
    Default();
}

//
// Colormap Deconstructor
//
Colormap::~Colormap()
{
}

//
// Colormap::CopyDetail()
//
void Colormap::CopyDetail(Colormap &src)
{
    special_ = src.special_;
    gl_color_    = src.gl_color_;
    analysis_   = nullptr;
}

//
// Colormap::Default()
//
void Colormap::Default()
{
    special_ = kColorSpecialNone;
    gl_color_    = kRGBANoValue;
    analysis_   = nullptr;
}

// --> ColormapContainer class

//
// ColormapContainer::ColormapContainer()
//
ColormapContainer::ColormapContainer()
{
}

//
// ~ColormapContainer::ColormapContainer()
//
ColormapContainer::~ColormapContainer()
{
    for (std::vector<Colormap *>::iterator iter = begin(), iter_end = end(); iter != iter_end; iter++)
    {
        Colormap *cmap = *iter;
        delete cmap;
        cmap = nullptr;
    }
}

//
// Colormap* ColormapContainer::Lookup()
//
Colormap *ColormapContainer::Lookup(const char *refname)
{
    if (!refname || !refname[0])
        return nullptr;

    for (std::vector<Colormap *>::iterator iter = begin(), iter_end = end(); iter != iter_end; iter++)
    {
        Colormap *cmap = *iter;
        if (DDFCompareName(cmap->name_.c_str(), refname) == 0)
            return cmap;
    }

    return nullptr;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
