#include "ui_playerview.h"

#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/MeshUtilities.h>
#include <RmlUi/Core/RenderManager.h>

#include "i_video.h"
#include "r_main.h"
#include "ui_render.h"

int32_t                  ElementPlayerView::mResolution = 1;
Rml::DataModelHandle ElementPlayerView::mPlayerViewHandle;

ElementPlayerView::ElementPlayerView(const Rml::String &tag) : Rml::Element(tag)
{
    AddEventListener(Rml::EventId::Click, this, true);
}

ElementPlayerView::~ElementPlayerView()
{
    if (mRenderSurface)
    {
        delete mRenderSurface;
    }
}

void ElementPlayerView::InitializeContext(Rml::Context *context)
{
    Rml::DataModelConstructor constructor = context->CreateDataModel("player_view");
    assert(constructor);

    constructor.BindFunc(
        "resolution", [](Rml::Variant &variant) { variant = mResolution; },
        [](const Rml::Variant &variant) { mResolution = variant.Get<int32_t>(); });

    mPlayerViewHandle = constructor.GetModelHandle();
}

bool ElementPlayerView::GetIntrinsicDimensions(Rml::Vector2f &dimensions, float &ratio)
{
    /*
    if (HasAttribute("width"))
    {
        dimensions.x = GetAttribute<float>("width", -1);
    }
    if (HasAttribute("height"))
    {
        dimensions.y = GetAttribute<float>("height", -1);
    }

    if (dimensions.y > 0)
        ratio = dimensions.x / dimensions.y;

    return true;
    */

    return false;
}

void ElementPlayerView::OnRender()
{
    const Rml::Vector2f render_dimensions_f = GetBox().GetSize(Rml::BoxArea::Content).Round();
    render_dimensions = Rml::Vector2i(render_dimensions_f.x / mResolution, render_dimensions_f.y / mResolution);

    if (render_dimensions.x < 1 || render_dimensions.y < 1)
    {
        return;
    }

    if (!mRenderSurface)
    {
        geometry_dirty = true;
    }
    else if (mRenderSurface && mRenderSurface->getWidth() != render_dimensions.x ||
             mRenderSurface->getHeight() != render_dimensions.y)
    {
        geometry_dirty = true;
        if (mRenderSurface)
        {
            delete mRenderSurface;
            mRenderSurface = nullptr;
        }
    }

    if (geometry_dirty)
        GenerateGeometry();

    IRenderSurface::setCurrentRenderSurface(mRenderSurface);
    R_RenderPlayerView(&displayplayer());
    IRenderSurface::setCurrentRenderSurface(nullptr);

    UpdateTexture();
    geometry.Render(GetAbsoluteOffset(Rml::BoxArea::Content), Rml::Texture(texture));

    texture_dirty = true;
}

void ElementPlayerView::ProcessEvent(Rml::Event &event)
{
    if (event == Rml::EventId::Click)    
    {
        if (event.GetPhase() == Rml::EventPhase::Target)
        {
            event.StopPropagation();
            Focus();
        }
    }
}

void ElementPlayerView::OnResize()
{
    Rml::Element::OnResize();

    geometry_dirty = true;
    texture_dirty  = true;
}

void ElementPlayerView::OnAttributeChange(const Rml::ElementAttributes &changed_attributes)
{
    Element::OnAttributeChange(changed_attributes);

    if (changed_attributes.find("width") != changed_attributes.end() ||
        changed_attributes.find("height") != changed_attributes.end())
    {
        DirtyLayout();
    }
}

void ElementPlayerView::OnPropertyChange(const Rml::PropertyIdSet &changed_properties)
{
    Element::OnPropertyChange(changed_properties);
}

void ElementPlayerView::GenerateGeometry()
{
    UIRenderInterface *interface = (UIRenderInterface *)Rml::GetRenderInterface();

    const Rml::ComputedValues &computed    = GetComputedValues();
    Rml::ColourbPremultiplied  quad_colour = computed.image_color().ToPremultiplied(computed.opacity());

    const Rml::Vector2f render_dimensions_f = GetBox().GetSize(Rml::BoxArea::Content).Round();

    if (!mRenderSurface)
    {
        render_dimensions = Rml::Vector2i(render_dimensions_f.x / mResolution, render_dimensions_f.y / mResolution);

        PixelFormat format = interface->GetPixelFormat();
        mRenderSurface     = new IRenderSurface(render_dimensions.x, render_dimensions.y, &format);
        interface->setRenderSurface(mRenderSurface);
    }

    Rml::Mesh mesh = geometry.Release(Rml::Geometry::ReleaseMode::ClearMesh);
    Rml::MeshUtilities::GenerateQuad(mesh, Rml::Vector2f(0), render_dimensions_f, quad_colour, Rml::Vector2f(0),
                                     Rml::Vector2f(float(mRenderSurface->getWidth()) / float(interface->getWidth()),
                                                   float(mRenderSurface->getHeight()) / float(interface->getHeight())));
    geometry = GetRenderManager()->MakeGeometry(std::move(mesh));

    geometry_dirty = false;
}

void ElementPlayerView::UpdateTexture()
{
    if (!texture_dirty)
    {
        return;
    }
    if (!texture)
    {
        Rml::RenderInterface *render_interface = Rml::GetRenderInterface();
        texture_handle                         = render_interface->LoadTexture(texture_dimensions, "*PLAYER_VIEW");
        Rml::RenderManager *render_manager     = GetRenderManager();
        if (!render_manager)
            return;

        texture = render_manager->LoadTexture("*PLAYER_VIEW");
    }

    if (!texture)
    {
        return;
    }

    SDL_Texture *sdl_texture = (SDL_Texture *)texture_handle;

    SDL_Rect rect;
    rect.x = rect.y = 0;
    rect.w          = mRenderSurface->getWidth();
    rect.h          = mRenderSurface->getHeight();

    SDL_UpdateTexture(sdl_texture, &rect, mRenderSurface->getBuffer(), mRenderSurface->getPitch());

    texture_dirty = false;
}
