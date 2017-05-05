#pragma once

#include "gui/Settings.h"
#include "objects/containers/Array.h"
#include "objects/wrappers/SharedPtr.h"
#include <wx/frame.h>

class wxComboBox;
class wxGauge;

NAMESPACE_SPH_BEGIN

namespace Abstract {
    class Renderer;
    class Element;
}
class WindowCommandEvent;
class MainWindow;
class Controller;
class Storage;
class OrthoPane;

/// Main frame of the application. Run is coupled with the window; currently there can only be one window and
/// one run at the same time. Run is ended when user closes the window.
class MainWindow : public wxFrame {
private:
    /// Parent control object
    Controller* controller;

    /// Drawing pane
    /// \todo used some virtual base class instead of directly orthopane
    OrthoPane* pane;

    /// Additional wx controls
    wxComboBox* quantityBox;
    wxGauge* gauge;

    Array<SharedPtr<Abstract::Element>> elementList;

public:
    MainWindow(Controller* controller, const GuiSettings& guiSettings);

    void setProgress(const float progress);

    void setElementList(Array<SharedPtr<Abstract::Element>>&& elements);

private:
    /// wx event handlers

    void onComboBox(wxCommandEvent& evt);

    void onClose(wxCloseEvent& evt);
};


NAMESPACE_SPH_END
