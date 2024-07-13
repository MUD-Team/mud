//----------------------------------------------------------------------------
//  MDL Models
//----------------------------------------------------------------------------
//
//  Copyright (c) 2023-2024 The EDGE Team.
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
//
//  Based on "qfiles.h" and "anorms.h" from the GPL'd quake 2 source
//  release.  Copyright (C) 1997-2001 Id Software, Inc.
//
//  Based on MDL loading and rendering code (C) 2004 David Henry.
//
//----------------------------------------------------------------------------

#include "r_mdl.h"

#include <stddef.h>

#include <vector>

#include "ddf_types.h"
#include "dm_state.h" // EDGE_IMAGE_IS_SKY
#include "epi.h"
#include "epi_endian.h"
#include "epi_str_compare.h"
#include "g_game.h" //current_map
#include "i_defs_gl.h"
#include "im_data.h"
#include "p_blockmap.h"
#include "r_colormap.h"
#include "r_effects.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_mdcommon.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_shader.h"
#include "r_state.h"
#include "r_texgl.h"
#include "r_units.h"

extern float ApproximateDistance(float dx, float dy, float dz);

extern bool            need_to_draw_sky;

/*============== MDL FORMAT DEFINITIONS ====================*/

// format uses float pointing values, but to allow for endianness
// conversions they are represented here as unsigned integers.

// struct member naming deviates from the style guide to reflect
// MDL format documentation

static constexpr const char *kMDLIdentifier = "IDPO";
static constexpr uint8_t     kMDLVersion    = 6;

struct RawMDLHeader
{
    char ident[4];

    int32_t version;

    uint32_t scale_x;
    uint32_t scale_y;
    uint32_t scale_z;

    uint32_t trans_x;
    uint32_t trans_y;
    uint32_t trans_z;

    uint32_t boundingradius;

    uint32_t eyepos_x;
    uint32_t eyepos_y;
    uint32_t eyepos_z;

    int32_t num_skins;

    int32_t skin_width;
    int32_t skin_height;

    int32_t num_vertices; // per frame
    int32_t total_triangles_;
    int32_t total_frames_;

    int32_t  synctype;
    int32_t  flags;
    uint32_t size;
};

struct RawMDLTextureCoordinate
{
    int32_t onseam;
    int32_t s;
    int32_t t;
};

struct RawMDLTriangle
{
    int32_t facesfront;
    int32_t vertex[3];
};

struct RawMDLVertex
{
    uint8_t x, y, z;
    uint8_t light_normal;
};

struct RawMDLSimpleFrame
{
    RawMDLVertex  bboxmin;
    RawMDLVertex  bboxmax;
    char          name[16];
    RawMDLVertex *verts;
};

struct RawMDLFrame
{
    int32_t           type;
    RawMDLSimpleFrame frame;
};

/*============== EDGE REPRESENTATION ====================*/

struct MDLVertex
{
    float x, y, z;

    short normal_idx;
};

struct MDLFrame
{
    MDLVertex *vertices;

    const char *name;

    // list of normals which are used.  Terminated by -1.
    short *used_normals;
};

struct MDLPoint
{
    float skin_s, skin_t;

    // index into frame's vertex array (mdl_frame_c::verts)
    int vert_idx;
};

struct MDLTriangle
{
    // index to the first point (within MDLModel::points).
    // All points for the strip are contiguous in that array.
    int first;
};

class MDLModel
{
  public:
    int total_frames_;
    int total_points_;
    int total_triangles_;
    int skin_width_;
    int skin_height_;

    MDLFrame    *frames_;
    MDLPoint    *points_;
    MDLTriangle *triangles_;

    int vertices_per_frame_;

    std::vector<uint32_t> skin_id_list_;

    GLuint vertex_buffer_object_;

    RendererVertex *gl_vertices_;

  public:
    MDLModel(int nframe, int npoint, int ntris, int swidth, int sheight)
        : total_frames_(nframe), total_points_(npoint), total_triangles_(ntris), skin_width_(swidth),
          skin_height_(sheight), vertices_per_frame_(0), vertex_buffer_object_(0), gl_vertices_(nullptr)
    {
        frames_      = new MDLFrame[total_frames_];
        points_      = new MDLPoint[total_points_];
        triangles_   = new MDLTriangle[total_triangles_];
        gl_vertices_ = new RendererVertex[total_triangles_ * 3];
    }

    ~MDLModel()
    {
        delete[] frames_;
        delete[] points_;
        delete[] triangles_;
    }
};

/*============== LOADING CODE ====================*/

static const char *CopyFrameName(RawMDLSimpleFrame *frm)
{
    char *str = new char[20];

    memcpy(str, frm->name, 16);

    // ensure it is NUL terminated
    str[16] = 0;

    return str;
}

static short *CreateNormalList(uint8_t *which_normals)
{
    int count = 0;
    int i;

    for (i = 0; i < kTotalMDFormatNormals; i++)
        if (which_normals[i])
            count++;

    short *n_list = new short[count + 1];

    count = 0;

    for (i = 0; i < kTotalMDFormatNormals; i++)
        if (which_normals[i])
            n_list[count++] = i;

    n_list[count] = -1;

    return n_list;
}

MDLModel *MDLLoad(epi::File *f)
{
    RawMDLHeader header;

    /* read header */
    f->Read(&header, sizeof(RawMDLHeader));

    int version = AlignedLittleEndianS32(header.version);

    LogDebug("MODEL IDENT: [%c%c%c%c] VERSION: %d", header.ident[0], header.ident[1], header.ident[2], header.ident[3],
             version);

    if (epi::StringPrefixCompare(header.ident, kMDLIdentifier) != 0)
    {
        FatalError("MDL_LoadModel: lump is not an MDL model!");
        return nullptr; /* NOT REACHED */
    }

    if (version != kMDLVersion)
    {
        FatalError("MDL_LoadModel: strange version!");
        return nullptr; /* NOT REACHED */
    }

    int total_frames_    = AlignedLittleEndianS32(header.total_frames_);
    int total_triangles_ = AlignedLittleEndianS32(header.total_triangles_);
    int num_verts        = AlignedLittleEndianS32(header.num_vertices);
    int swidth           = AlignedLittleEndianS32(header.skin_width);
    int sheight          = AlignedLittleEndianS32(header.skin_height);
    int total_points_    = total_triangles_ * 3;

    MDLModel *md = new MDLModel(total_frames_, total_points_, total_triangles_, swidth, sheight);

    /* PARSE SKINS */

    for (int i = 0; i < AlignedLittleEndianS32(header.num_skins); i++)
    {
        int      group  = 0;
        uint8_t *pixels = new uint8_t[sheight * swidth];

        // Check for single vs. group skins; error if group skin found
        f->Read(&group, sizeof(int));
        if (AlignedLittleEndianS32(group))
        {
            FatalError("MDL_LoadModel: Group skins unsupported!\n");
            return nullptr; // Not reached
        }

        f->Read(pixels, sheight * swidth * sizeof(uint8_t));
        ImageData *tmp_img = new ImageData(swidth, sheight, 3);
        // Expand 8 bits paletted image to RGB
        for (int j = 0; j < swidth * sheight; ++j)
        {
            for (int k = 0; k < 3; ++k)
            {
                tmp_img->pixels_[(j * 3) + k] = md_colormap[pixels[j]][k];
            }
        }
        delete[] pixels;
        md->skin_id_list_.push_back(UploadTexture(tmp_img, kUploadMipMap | kUploadSmooth));
        delete tmp_img;
    }

    /* PARSE TEXCOORDS */
    RawMDLTextureCoordinate *texcoords = new RawMDLTextureCoordinate[num_verts];
    f->Read(texcoords, num_verts * sizeof(RawMDLTextureCoordinate));

    /* PARSE TRIANGLES */

    RawMDLTriangle *tris = new RawMDLTriangle[total_triangles_];
    f->Read(tris, total_triangles_ * sizeof(RawMDLTriangle));

    /* PARSE FRAMES */

    RawMDLFrame *frames = new RawMDLFrame[total_frames_];

    for (int fr = 0; fr < total_frames_; fr++)
    {
        frames[fr].frame.verts = new RawMDLVertex[num_verts];
        f->Read(&frames[fr].type, sizeof(int));
        f->Read(&frames[fr].frame.bboxmin, sizeof(RawMDLVertex));
        f->Read(&frames[fr].frame.bboxmax, sizeof(RawMDLVertex));
        f->Read(frames[fr].frame.name, 16 * sizeof(char));
        f->Read(frames[fr].frame.verts, num_verts * sizeof(RawMDLVertex));
    }

    LogDebug("  frames:%d  points:%d  tris: %d\n", total_frames_, total_triangles_ * 3, total_triangles_);

    md->vertices_per_frame_ = num_verts;

    LogDebug("  vertices_per_frame_:%d\n", md->vertices_per_frame_);

    // convert glcmds into tris and points
    MDLTriangle *tri   = md->triangles_;
    MDLPoint    *point = md->points_;

    for (int i = 0; i < total_triangles_; i++)
    {
        EPI_ASSERT(tri < md->triangles_ + md->total_triangles_);
        EPI_ASSERT(point < md->points_ + md->total_points_);

        tri->first = point - md->points_;

        tri++;

        for (int j = 0; j < 3; j++, point++)
        {
            RawMDLTriangle raw_tri = tris[i];
            point->vert_idx        = AlignedLittleEndianS32(raw_tri.vertex[j]);
            float s                = (float)AlignedLittleEndianS16(texcoords[point->vert_idx].s);
            float t                = (float)AlignedLittleEndianS16(texcoords[point->vert_idx].t);
            if (!AlignedLittleEndianS32(raw_tri.facesfront) &&
                AlignedLittleEndianS32(texcoords[point->vert_idx].onseam))
                s += (float)swidth * 0.5f;
            point->skin_s = (s + 0.5f) / (float)swidth;
            point->skin_t = (t + 0.5f) / (float)sheight;
            EPI_ASSERT(point->vert_idx >= 0);
            EPI_ASSERT(point->vert_idx < md->vertices_per_frame_);
        }
    }

    EPI_ASSERT(tri == md->triangles_ + md->total_triangles_);
    EPI_ASSERT(point == md->points_ + md->total_points_);

    /* PARSE FRAMES */

    uint8_t which_normals[kTotalMDFormatNormals];

    uint32_t raw_scale[3];
    uint32_t raw_translate[3];

    raw_scale[0]     = AlignedLittleEndianU32(header.scale_x);
    raw_scale[1]     = AlignedLittleEndianU32(header.scale_y);
    raw_scale[2]     = AlignedLittleEndianU32(header.scale_z);
    raw_translate[0] = AlignedLittleEndianU32(header.trans_x);
    raw_translate[1] = AlignedLittleEndianU32(header.trans_y);
    raw_translate[2] = AlignedLittleEndianU32(header.trans_z);

    float *f_ptr = (float *)raw_scale;
    float  scale[3];
    float  translate[3];

    scale[0] = f_ptr[0];
    scale[1] = f_ptr[1];
    scale[2] = f_ptr[2];

    f_ptr        = (float *)raw_translate;
    translate[0] = f_ptr[0];
    translate[1] = f_ptr[1];
    translate[2] = f_ptr[2];

    for (int i = 0; i < total_frames_; i++)
    {
        RawMDLFrame raw_frame = frames[i];

        md->frames_[i].name = CopyFrameName(&raw_frame.frame);

        RawMDLVertex *raw_verts = frames[i].frame.verts;

        md->frames_[i].vertices = new MDLVertex[md->vertices_per_frame_];

        memset(which_normals, 0, sizeof(which_normals));

        for (int v = 0; v < md->vertices_per_frame_; v++)
        {
            RawMDLVertex *raw_V  = raw_verts + v;
            MDLVertex    *good_V = md->frames_[i].vertices + v;

            good_V->x = (int)raw_V->x * scale[0] + translate[0];
            good_V->y = (int)raw_V->y * scale[1] + translate[1];
            good_V->z = (int)raw_V->z * scale[2] + translate[2];

            good_V->normal_idx = raw_V->light_normal;

            EPI_ASSERT(good_V->normal_idx >= 0);
            // EPI_ASSERT(good_V->normal_idx < kTotalMDFormatNormals);
            //  Dasho: Maybe try to salvage bad MDL models?
            if (good_V->normal_idx >= kTotalMDFormatNormals)
            {
                LogDebug("Vert %d of Frame %d has an invalid normal index: %d\n", v, i, good_V->normal_idx);
                good_V->normal_idx = (good_V->normal_idx % kTotalMDFormatNormals);
            }

            which_normals[good_V->normal_idx] = 1;
        }

        md->frames_[i].used_normals = CreateNormalList(which_normals);
    }

    delete[] texcoords;
    delete[] tris;
    delete[] frames;
    glGenBuffers(1, &md->vertex_buffer_object_);
    if (md->vertex_buffer_object_ == 0)
        FatalError("MDL_LoadModel: Failed to bind VBO!\n");
    glBindBuffer(GL_ARRAY_BUFFER, md->vertex_buffer_object_);
    glBufferData(GL_ARRAY_BUFFER, md->total_triangles_ * 3 * sizeof(RendererVertex), nullptr, GL_STREAM_DRAW);
    return md;
}

short MDLFindFrame(MDLModel *md, const char *name)
{
    EPI_ASSERT(strlen(name) > 0);

    for (int f = 0; f < md->total_frames_; f++)
    {
        MDLFrame *frame = &md->frames_[f];

        if (DDFCompareName(name, frame->name) == 0)
            return f;
    }

    return -1; // NOT FOUND
}

/*============== MODEL RENDERING ====================*/

class MDLCoordinateData
{
  public:
    MapObject *map_object_;

    MDLModel *model_;

    const MDLFrame    *frame1_;
    const MDLFrame    *frame2_;
    const MDLTriangle *strip_;

    float lerp_;
    float x_, y_, z_;

    bool is_weapon_;
    bool is_fuzzy_;

    // scaling
    float xy_scale_;
    float z_scale_;
    float bias_;

    // image size
    float image_right_;
    float image_top_;

    // fuzzy info
    float    fuzz_multiplier_;
    HMM_Vec2 fuzz_add_;

    // mlook vectors
    HMM_Vec2 mouselook_x_vector_;
    HMM_Vec2 mouselook_z_vector_;

    // rotation vectors
    HMM_Vec2 rotation_vector_x_;
    HMM_Vec2 rotation_vector_y_;

    ColorMixer normal_colors_[kTotalMDFormatNormals];

    short *used_normals_;

    bool is_additive_;

  public:
    void CalculatePosition(HMM_Vec3 *pos, float x1, float y1, float z1) const
    {
        x1 *= xy_scale_;
        y1 *= xy_scale_;
        z1 *= z_scale_;

        float x2 = x1 * mouselook_x_vector_.X + z1 * mouselook_x_vector_.Y;
        float z2 = x1 * mouselook_z_vector_.X + z1 * mouselook_z_vector_.Y;
        float y2 = y1;

        pos->X = x_ + x2 * rotation_vector_x_.X + y2 * rotation_vector_x_.Y;
        pos->Y = y_ + x2 * rotation_vector_y_.X + y2 * rotation_vector_y_.Y;
        pos->Z = z_ + z2;
    }

    void CalculateNormal(HMM_Vec3 *normal, const MDLVertex *vert) const
    {
        short n = vert->normal_idx;

        float nx1 = md_normals[n].X;
        float ny1 = md_normals[n].Y;
        float nz1 = md_normals[n].Z;

        float nx2 = nx1 * mouselook_x_vector_.X + nz1 * mouselook_x_vector_.Y;
        float nz2 = nx1 * mouselook_z_vector_.X + nz1 * mouselook_z_vector_.Y;
        float ny2 = ny1;

        normal->X = nx2 * rotation_vector_x_.X + ny2 * rotation_vector_x_.Y;
        normal->Y = nx2 * rotation_vector_y_.X + ny2 * rotation_vector_y_.Y;
        normal->Z = nz2;
    }
};

static void InitializeNormalColors(MDLCoordinateData *data)
{
    short *n_list = data->used_normals_;

    for (; *n_list >= 0; n_list++)
    {
        data->normal_colors_[*n_list].Clear();
    }
}

static void ShadeNormals(AbstractShader *shader, MDLCoordinateData *data, bool skip_calc)
{
    short *n_list = data->used_normals_;

    for (; *n_list >= 0; n_list++)
    {
        short n  = *n_list;
        float nx = 0;
        float ny = 0;
        float nz = 0;

        if (!skip_calc)
        {
            float nx1 = md_normals[n].X;
            float ny1 = md_normals[n].Y;
            float nz1 = md_normals[n].Z;

            float nx2 = nx1 * data->mouselook_x_vector_.X + nz1 * data->mouselook_x_vector_.Y;
            float nz2 = nx1 * data->mouselook_z_vector_.X + nz1 * data->mouselook_z_vector_.Y;
            float ny2 = ny1;

            nx = nx2 * data->rotation_vector_x_.X + ny2 * data->rotation_vector_x_.Y;
            ny = nx2 * data->rotation_vector_y_.X + ny2 * data->rotation_vector_y_.Y;
            nz = nz2;
        }

        shader->Corner(data->normal_colors_ + n, nx, ny, nz, data->map_object_, data->is_weapon_);
    }
}

static void MDLDynamicLightCallback(MapObject *mo, void *dataptr)
{
    MDLCoordinateData *data = (MDLCoordinateData *)dataptr;

    // dynamic lights do not light themselves up!
    if (mo == data->map_object_)
        return;

    EPI_ASSERT(mo->dynamic_light_.shader);

    ShadeNormals(mo->dynamic_light_.shader, data, false);
}

static int MDLMulticolorMaximumRGB(MDLCoordinateData *data, bool additive)
{
    int result = 0;

    short *n_list = data->used_normals_;

    for (; *n_list >= 0; n_list++)
    {
        ColorMixer *col = &data->normal_colors_[*n_list];

        int mx = additive ? col->add_MAX() : col->mod_MAX();

        result = HMM_MAX(result, mx);
    }

    return result;
}

static void UpdateMulticols(MDLCoordinateData *data)
{
    short *n_list = data->used_normals_;

    for (; *n_list >= 0; n_list++)
    {
        ColorMixer *col = &data->normal_colors_[*n_list];

        col->modulate_red_ -= 256;
        col->modulate_green_ -= 256;
        col->modulate_blue_ -= 256;
    }
}

static inline float LerpIt(float v1, float v2, float lerp)
{
    return v1 * (1.0f - lerp) + v2 * lerp;
}

static inline void ModelCoordFunc(MDLCoordinateData *data, int v_idx, HMM_Vec3 *pos, float *rgb, HMM_Vec2 *texc,
                                  HMM_Vec3 *normal)
{
    const MDLModel *md = data->model_;

    const MDLFrame    *frame1 = data->frame1_;
    const MDLFrame    *frame2 = data->frame2_;
    const MDLTriangle *strip  = data->strip_;

    EPI_ASSERT(strip->first + v_idx >= 0);
    EPI_ASSERT(strip->first + v_idx < md->total_points_);

    const MDLPoint *point = &md->points_[strip->first + v_idx];

    const MDLVertex *vert1 = &frame1->vertices[point->vert_idx];
    const MDLVertex *vert2 = &frame2->vertices[point->vert_idx];

    float x1 = LerpIt(vert1->x, vert2->x, data->lerp_);
    float y1 = LerpIt(vert1->y, vert2->y, data->lerp_);
    float z1 = LerpIt(vert1->z, vert2->z, data->lerp_) + data->bias_;

    data->CalculatePosition(pos, x1, y1, z1);

    const MDLVertex *n_vert = (data->lerp_ < 0.5) ? vert1 : vert2;

    data->CalculateNormal(normal, n_vert);

    if (data->is_fuzzy_)
    {
        texc->X = point->skin_s * data->fuzz_multiplier_ + data->fuzz_add_.X;
        texc->Y = point->skin_t * data->fuzz_multiplier_ + data->fuzz_add_.Y;

        rgb[0] = rgb[1] = rgb[2] = 0;
        return;
    }

    *texc = {{point->skin_s, point->skin_t}};

    ColorMixer *col = &data->normal_colors_[n_vert->normal_idx];

    if (!data->is_additive_)
    {
        rgb[0] = col->modulate_red_ / 255.0;
        rgb[1] = col->modulate_green_ / 255.0;
        rgb[2] = col->modulate_blue_ / 255.0;
    }
    else
    {
        rgb[0] = col->add_red_ / 255.0;
        rgb[1] = col->add_green_ / 255.0;
        rgb[2] = col->add_blue_ / 255.0;
    }

    rgb[0] *= render_view_red_multiplier;
    rgb[1] *= render_view_green_multiplier;
    rgb[2] *= render_view_blue_multiplier;
}

void MDLRenderModel(MDLModel *md, const Image *skin_img, bool is_weapon, int frame1, int frame2, float lerp, float x,
                    float y, float z, MapObject *mo, RegionProperties *props, float scale, float aspect, float bias,
                    int rotation)
{
    // check if frames are valid
    if (frame1 < 0 || frame1 >= md->total_frames_)
    {
        LogDebug("Render model: bad frame %d\n", frame1);
        return;
    }
    if (frame2 < 0 || frame2 >= md->total_frames_)
    {
        LogDebug("Render model: bad frame %d\n", frame1);
        return;
    }

    MDLCoordinateData data;

    data.is_fuzzy_ = (mo->flags_ & kMapObjectFlagFuzzy) ? true : false;

    float trans = mo->visibility_;

    if (trans <= 0)
        return;

    int blending = kBlendingNone;

    if (mo->hyper_flags_ & kHyperFlagNoZBufferUpdate)
        blending |= kBlendingNoZBuffer;

    blending |= kBlendingCullBack;

    data.map_object_ = mo;
    data.model_      = md;

    data.frame1_ = &md->frames_[frame1];
    data.frame2_ = &md->frames_[frame2];

    data.lerp_ = lerp;

    data.x_ = x;
    data.y_ = y;
    data.z_ = z;

    data.is_weapon_ = is_weapon;

    data.xy_scale_ = scale * aspect;
    data.z_scale_  = scale;
    data.bias_     = bias;

    bool tilt = is_weapon || (mo->flags_ & kMapObjectFlagMissile) || (mo->hyper_flags_ & kHyperFlagForceModelTilt);

    BAMAngleToMatrix(tilt ? ~mo->vertical_angle_ : 0, &data.mouselook_x_vector_, &data.mouselook_z_vector_);

    BAMAngle ang = mo->angle_ + rotation;

    BAMAngleToMatrix(~ang, &data.rotation_vector_x_, &data.rotation_vector_y_);

    data.used_normals_ = (lerp < 0.5) ? data.frame1_->used_normals : data.frame2_->used_normals;

    InitializeNormalColors(&data);

    GLuint skin_tex = 0;

    if (data.is_fuzzy_)
    {
        skin_tex = ImageCache(fuzz_image, false);

        data.fuzz_multiplier_ = 0.8;
        data.fuzz_add_        = {{0, 0}};

        data.image_right_ = 1.0;
        data.image_top_   = 1.0;

        if (!data.is_weapon_ && !view_is_zoomed)
        {
            float dist = ApproximateDistance(mo->x - view_x, mo->y - view_y, mo->z - view_z);

            data.fuzz_multiplier_ = 70.0 / HMM_Clamp(35, dist, 700);
        }

        FuzzAdjust(&data.fuzz_add_, mo);

        trans = 1.0f;

        blending |= kBlendingAlpha | kBlendingMasked;
        blending &= ~kBlendingLess;
    }
    else /* (! data.is_fuzzy) */
    {
        int mdlSkin = 0;

        if (is_weapon == true)
            mdlSkin = mo->player_->weapons_[mo->player_->ready_weapon_].model_skin;
        else
            mdlSkin = mo->model_skin_;

        mdlSkin--; // ddf MODEL_SKIN starts at 1 not 0

        if (mdlSkin > -1)
            skin_tex = md->skin_id_list_[mdlSkin];
        else
            skin_tex = md->skin_id_list_[0]; // Just use skin 0?

        if (skin_tex == 0)
            FatalError("MDL Frame %s missing skins?\n", md->frames_[frame1].name);

        data.image_right_ = (float)md->skin_width_ / (float)MakeValidTextureSize(md->skin_width_);
        data.image_top_   = (float)md->skin_height_ / (float)MakeValidTextureSize(md->skin_height_);

        AbstractShader *shader = GetColormapShader(props, mo->state_->bright);

        ShadeNormals(shader, &data, true);

        if (render_view_extra_light < 250)
        {
            float r = mo->radius_;

            DynamicLightIterator(mo->x - r, mo->y - r, mo->z, mo->x + r, mo->y + r, mo->z + mo->height_,
                                 MDLDynamicLightCallback, &data);

            SectorGlowIterator(mo->subsector_->sector, mo->x - r, mo->y - r, mo->z, mo->x + r, mo->y + r,
                               mo->z + mo->height_, MDLDynamicLightCallback, &data);
        }
    }

    /* draw the model */

    int num_pass = data.is_fuzzy_ ? 1 : 4;

    RGBAColor fc_to_use = mo->subsector_->sector->properties.fog_color;
    float     fd_to_use = mo->subsector_->sector->properties.fog_density;
    // check for DDFLEVL fog
    if (fc_to_use == kRGBANoValue)
    {
        if (EDGE_IMAGE_IS_SKY(mo->subsector_->sector->ceiling))
        {
            fc_to_use = current_map->outdoor_fog_color_;
            fd_to_use = 0.01f * current_map->outdoor_fog_density_;
        }
        else
        {
            fc_to_use = current_map->indoor_fog_color_;
            fd_to_use = 0.01f * current_map->indoor_fog_density_;
        }
    }
    if (fc_to_use != kRGBANoValue)
    {
        GLfloat fc[4];
        fc[0] = (float)epi::GetRGBARed(fc_to_use) / 255.0f;
        fc[1] = (float)epi::GetRGBAGreen(fc_to_use) / 255.0f;
        fc[2] = (float)epi::GetRGBABlue(fc_to_use) / 255.0f;
        fc[3] = 1.0f;
        glClearColor(fc[0], fc[1], fc[2], 1.0f);
        glFogfv(GL_FOG_COLOR, fc);
        glFogf(GL_FOG_DENSITY, std::log1p(fd_to_use));
        glEnable(GL_FOG);
    }
    else
        glDisable(GL_FOG);

    for (int pass = 0; pass < num_pass; pass++)
    {
        if (pass == 1)
        {
            blending &= ~kBlendingAlpha;
            blending |= kBlendingAdd;
            glDisable(GL_FOG);
        }

        data.is_additive_ = (pass > 0 && pass == num_pass - 1);

        if (pass > 0 && pass < num_pass - 1)
        {
            UpdateMulticols(&data);
            if (MDLMulticolorMaximumRGB(&data, false) <= 0)
                continue;
        }
        else if (data.is_additive_)
        {
            if (MDLMulticolorMaximumRGB(&data, true) <= 0)
                continue;
        }

        glPolygonOffset(0, -pass);

        if (blending & (kBlendingMasked | kBlendingLess))
        {
            if (blending & kBlendingLess)
            {
                glEnable(GL_ALPHA_TEST);
            }
            else if (blending & kBlendingMasked)
            {
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0);
            }
            else
                glDisable(GL_ALPHA_TEST);
        }

        if (blending & (kBlendingAlpha | kBlendingAdd))
        {
            if (blending & kBlendingAdd)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            }
            else if (blending & kBlendingAlpha)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            else
                glDisable(GL_BLEND);
        }

        if (blending & (kBlendingCullBack | kBlendingCullFront))
        {
            if (blending & (kBlendingCullBack | kBlendingCullFront))
            {
                glEnable(GL_CULL_FACE);
                glCullFace((blending & kBlendingCullFront) ? GL_FRONT : GL_BACK);
            }
            else
                glDisable(GL_CULL_FACE);
        }

        if (blending & kBlendingNoZBuffer)
        {
            glDepthMask((blending & kBlendingNoZBuffer) ? GL_FALSE : GL_TRUE);
        }

        if (blending & kBlendingLess)
        {
            // NOTE: assumes alpha is constant over whole model
            glAlphaFunc(GL_GREATER, trans * 0.66f);
        }

        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, skin_tex);

        if (data.is_additive_)
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
        }
        else
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
        }

        GLint old_clamp = 789;

        if (blending & kBlendingClampY)
        {
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &old_clamp);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, renderer_dumb_clamp.d_ ? GL_CLAMP : GL_CLAMP_TO_EDGE);
        }

        RendererVertex *start = md->gl_vertices_;

        for (int i = 0; i < md->total_triangles_; i++)
        {
            data.strip_ = &md->triangles_[i];

            for (int v_idx = 0; v_idx < 3; v_idx++)
            {
                RendererVertex *dest = start + (i * 3) + v_idx;

                ModelCoordFunc(&data, v_idx, &dest->position, dest->rgba_color, &dest->texture_coordinates[0],
                               &dest->normal);

                dest->rgba_color[3] = trans;
            }
        }

        // setup client state
        glBindBuffer(GL_ARRAY_BUFFER, md->vertex_buffer_object_);
        glBufferData(GL_ARRAY_BUFFER, md->total_triangles_ * 3 * sizeof(RendererVertex), md->gl_vertices_,
                     GL_STREAM_DRAW);
        glVertexPointer(3, GL_FLOAT, sizeof(RendererVertex), (void *)(offsetof(RendererVertex, position.X)));
        glColorPointer(4, GL_FLOAT, sizeof(RendererVertex), (void *)(offsetof(RendererVertex, rgba_color)));
        glNormalPointer(GL_FLOAT, sizeof(RendererVertex), (void *)(offsetof(RendererVertex, normal.X)));
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glClientActiveTexture(GL_TEXTURE0);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(RendererVertex),
                          (void *)(offsetof(RendererVertex, texture_coordinates[0])));

        glDrawArrays(GL_TRIANGLES, 0, md->total_triangles_ * 3);

        // restore the clamping mode
        if (old_clamp != 789)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, old_clamp);
    }

    RenderState *state = GetRenderState();
    state->SetDefaultStateFull();
}

void MDLRenderModel2D(MDLModel *md, const Image *skin_img, int frame, float x, float y, float xscale, float yscale,
                      const MapObjectDefinition *info)
{
    // check if frame is valid
    if (frame < 0 || frame >= md->total_frames_)
        return;

    GLuint skin_tex = md->skin_id_list_[0]; // Just use skin 0?

    if (skin_tex == 0)
        FatalError("MDL Frame %s missing skins?\n", md->frames_[frame].name);

    xscale = yscale * info->model_scale_ * info->model_aspect_;
    yscale = yscale * info->model_scale_;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, skin_tex);

    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);

    if (info->flags_ & kMapObjectFlagFuzzy)
        glColor4f(0, 0, 0, 0.5f);
    else
        glColor4f(1, 1, 1, 1.0f);

    for (int i = 0; i < md->total_triangles_; i++)
    {
        const MDLTriangle *strip = &md->triangles_[i];

        glBegin(GL_TRIANGLES);

        for (int v_idx = 0; v_idx < 3; v_idx++)
        {
            const MDLFrame *frame_ptr = &md->frames_[frame];

            EPI_ASSERT(strip->first + v_idx >= 0);
            EPI_ASSERT(strip->first + v_idx < md->total_points_);

            const MDLPoint  *point = &md->points_[strip->first + v_idx];
            const MDLVertex *vert  = &frame_ptr->vertices[point->vert_idx];

            glTexCoord2f(point->skin_s, point->skin_t);

            short n = vert->normal_idx;

            float norm_x = md_normals[n].X;
            float norm_y = md_normals[n].Y;
            float norm_z = md_normals[n].Z;

            glNormal3f(norm_y, norm_z, norm_x);

            float dx = vert->x * xscale;
            float dy = vert->y * xscale;
            float dz = (vert->z + info->model_bias_) * yscale;

            glVertex3f(x + dy, y + dz, dx / 256.0f);
        }

        glEnd();
    }

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
