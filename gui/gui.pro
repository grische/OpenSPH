TEMPLATE = lib
CONFIG += c++14 staticlib thread silent
CONFIG -= app_bundle
CONFIG -= qt

DEPENDPATH += ../core ../test
PRE_TARGETDEPS += ../core/libcore.a
LIBS += ../core/libcore.a

include(../core/sharedCore.pro)
include(sharedGui.pro)

INCLUDEPATH += $$PREFIX/include/wx-3.0 ../core/ ..

linux-g++ {
    # disabling maybe-uninitialized because of Factory::getCamera, either gcc bug or some weird behavior
    QMAKE_CXXFLAGS += -Wno-maybe-uninitialized
}

SOURCES += \
    Controller.cpp \
    Factory.cpp \
    ImageTransform.cpp \
    MainLoop.cpp \
    Project.cpp \
    Settings.cpp \
    Utils.cpp \
    jobs/Presets.cpp \
    jobs/CameraJobs.cpp \
    jobs/RenderJobs.cpp \
    objects/Bitmap.cpp \
    objects/Camera.cpp \
    objects/Colorizer.cpp \
    objects/Filmic.cpp \
    objects/Movie.cpp \
    objects/Palette.cpp \
    objects/PaletteEntry.cpp \
    objects/RenderContext.cpp \
    renderers/ContourRenderer.cpp \
    renderers/IRenderer.cpp \
    renderers/MeshRenderer.cpp \
    renderers/ParticleRenderer.cpp \
    renderers/RayMarcher.cpp \
    renderers/VolumeRenderer.cpp \
    renderers/Spectrum.cpp \
    windows/BatchDialog.cpp \
    windows/PaletteEditor.cpp \
    windows/PaletteProperty.cpp \
    windows/PaletteWidget.cpp \
    windows/PreviewPane.cpp \
    windows/ProgressPanel.cpp \
    windows/RenderPage.cpp \
    windows/RenderSetup.cpp \
    windows/RunSelectDialog.cpp \
    windows/SessionDialog.cpp \
    windows/CurveDialog.cpp \
    windows/GridPage.cpp \
    windows/OrthoPane.cpp \
    windows/ParticleProbe.cpp \
    windows/PlotView.cpp \
    windows/RunPage.cpp \
    windows/NodePage.cpp \
    windows/MainWindow.cpp \
    windows/TimeLine.cpp \
    windows/Tooltip.cpp \
    windows/Widgets.cpp \
    windows/GuiSettingsDialog.cpp \
    objects/Plots.cpp

HEADERS += \
    ArcBall.h \
    Controller.h \
    Factory.h \
    ImageTransform.h \
    MainLoop.h \
    Project.h \
    Renderer.h \
    Settings.h \
    Utils.h \
    jobs/Presets.h \
    jobs/CameraJobs.h \
    jobs/RenderJobs.h \
    objects/Bitmap.h \
    objects/Camera.h \
    objects/Color.h \
    objects/Colorizer.h \
    objects/DelayedCallback.h \
    objects/Filmic.h \
    objects/GraphicsContext.h \
    objects/Movie.h \
    objects/Palette.h \
    objects/PaletteEntry.h \
    objects/Point.h \
    objects/RenderContext.h \
    objects/SvgContext.h \
    renderers/Brdf.h \
    renderers/ContourRenderer.h \
    renderers/FrameBuffer.h \
    renderers/IRenderer.h \
    renderers/Lensing.h \
    renderers/MeshRenderer.h \
    renderers/ParticleRenderer.h \
    renderers/RayMarcher.h \
    renderers/VolumeRenderer.h \
    renderers/Spectrum.h \
    windows/BatchDialog.h \
    windows/PaletteEditor.h \
    windows/PaletteProperty.h \
    windows/PaletteWidget.h \
    windows/PreviewPane.h \
    windows/RenderPage.h \
    windows/RenderSetup.h \
    windows/RunSelectDialog.h \
    windows/SessionDialog.h \
    windows/CurveDialog.h \
    windows/GridPage.h \
    windows/IGraphicsPane.h \
    windows/Icons.data.h \
    windows/OrthoPane.h \
    windows/ParticleProbe.h \
    windows/PlotView.h \
    objects/Texture.h \
    windows/RunPage.h \
    windows/NodePage.h \
    windows/MainWindow.h \
    windows/TimeLine.h \
    windows/ProgressPanel.h \
    windows/Tooltip.h \
    windows/WeakRef.h \
    windows/Widgets.h \
    windows/GuiSettingsDialog.h \
    objects/Plots.h
