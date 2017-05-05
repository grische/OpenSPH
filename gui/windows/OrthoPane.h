#pragma once

#include "gui/Renderer.h"
#include "gui/Settings.h"
#include "gui/objects/Bitmap.h"
#include "gui/objects/Palette.h"
#include "gui/objects/Point.h"
#include "objects/containers/ArrayView.h"
#include "objects/containers/BufferedArray.h"

#include <mutex>
#include <wx/panel.h>
#include <wx/wx.h>

class wxTimer;

NAMESPACE_SPH_BEGIN

class Storage;
class Controller;

namespace Abstract {
    class Camera;
    class Element;
}


class OrthoRenderer : public Abstract::Renderer {
public:
    /// Can only be called from main thread
    virtual Bitmap render(ArrayView<const Vector> positions,
        Abstract::Element& element,
        const RenderParams& params,
        Statistics& stats) const override;

private:
    void drawPalette(wxDC& dc, const Palette& palette) const;
};

class OrthoPane : public wxPanel {
private:
    Controller* controller;

    /// Cached mouse position when dragging the window
    struct {
        Point position;
    } dragging;

    /// Timer for refreshing window
    AutoPtr<wxTimer> refreshTimer;

    /// Current camera of the view. The object is shared with parent model.
    SharedPtr<Abstract::Camera> camera;

public:
    OrthoPane(wxWindow* parent, Controller* controller);

    ~OrthoPane();

    /// Changes displayed element. Must be executed from the main thread
    void setElement(AutoPtr<Abstract::Element>&& newElement);

private:
    void requestUpdate();

    /// wx event handlers
    void onPaint(wxPaintEvent& evt);

    void onMouseMotion(wxMouseEvent& evt);

    void onMouseWheel(wxMouseEvent& evt);

    void onTimer(wxTimerEvent& evt);
};

NAMESPACE_SPH_END
