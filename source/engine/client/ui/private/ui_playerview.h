#pragma once

#include <RmlUi/Core.h>
#include <RmlUi/Core/CallbackTexture.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/Header.h>

class IRenderSurface;

class ElementPlayerView : public Rml::Element, public Rml::EventListener
{
  public:
    RMLUI_RTTI_DefineWithParent(ElementPlayerView, Rml::Element)

        ElementPlayerView(const Rml::String &tag);
    virtual ~ElementPlayerView();

  protected:
    /// Renders the image.
    void OnRender() override;

    /// Regenerates the element's geometry.
    void OnResize() override;

    /// Checks for changes to the image's source or dimensions.
    /// @param[in] changed_attributes A list of attributes changed on the element.
    void OnAttributeChange(const Rml::ElementAttributes &changed_attributes) override;

    /// Called when properties on the element are changed.
    /// @param[in] changed_properties The properties changed on the element.
    void OnPropertyChange(const Rml::PropertyIdSet &changed_properties) override;

    bool GetIntrinsicDimensions(Rml::Vector2f &dimensions, float &ratio) override;

    void ProcessEvent(Rml::Event& event) override;

  private:

    friend class UIContextPlay;

    static void InitializeContext(Rml::Context *context);

    // Generates the element's geometry.
    void GenerateGeometry();
    // Loads the SVG document specified by the 'src' attribute.
    bool LoadSource();
    // Update the texture when necessary.
    void UpdateTexture();

    bool source_dirty   = false;
    bool geometry_dirty = false;
    bool texture_dirty  = false;

    // The texture this element is rendering from.
    Rml::Texture texture;

    Rml::TextureHandle texture_handle     = 0;
    Rml::Vector2i      texture_dimensions = {0, 0};

    // The element's size for rendering.
    Rml::Vector2i render_dimensions;

    // The geometry used to render this element.
    Rml::Geometry geometry;

    IRenderSurface* mRenderSurface = nullptr;

    // Data Model

    static int mResolution;
    static Rml::DataModelHandle mPlayerViewHandle;
};
