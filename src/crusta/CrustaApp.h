#ifndef _CrustaApp_H_
#define _CrustaApp_H_


#include <crustavrui/GLMotif/ColorMapEditor.h>
#include <crustavrui/GLMotif/ColorPickerWindow.h>
#include <crustavrui/GLMotif/RangeEditor.h>

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/CrustaComponent.h>
#include <crusta/SurfaceProbeTool.h>

#include <crusta/vrui.h>


class GLContextData;

namespace GLMotif {
    class DropdownBox;
    class Label;
    class ListBox;
    class Menu;
    class PaletteEditor;
    class PopupMenu;
    class PopupWindow;
    class RadioBox;
    class RowColumn;
    class TextField;
}
namespace Vrui {
class Lightsource;
}


namespace crusta {


class Crusta;

class CrustaApp : public Vrui::Application
{
public:
    CrustaApp(int& argc, char**& argv, char**& appDefaults);
    ~CrustaApp();

private:
    void handleArg(const std::string& arg);

private:
    class Dialog
    {
    public:
        void createMenuEntry(GLMotif::Container* menu);

    protected:
        virtual void init();
        void showCallback(
            GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
        void closeCallback(Misc::CallbackData* cbData);

        std::string name;
        std::string label;

        GLMotif::PopupWindow*  dialog;
        GLMotif::Container*    parentMenu;
        GLMotif::ToggleButton* toggle;
    };

    class VerticalScaleDialog : public Dialog
    {
    public:
        VerticalScaleDialog();
        void setCrusta(Crusta* newCrusta);
    protected:
        void init();
        void changeScaleCallback(
            GLMotif::Slider::ValueChangedCallbackData* cbData);
    private:
        Crusta*         crusta;
        GLMotif::Label* scaleLabel;
    };

    class OpacityDialog : public Dialog
    {
    public:
        OpacityDialog();
        void setCrusta(Crusta* newCrusta);
    protected:
        void init();
        void changeOpacityCallback(
            GLMotif::Slider::ValueChangedCallbackData* cbData);
    private:
        Crusta*         crusta;
        GLMotif::Label* opacityLabel;
    };

    class LightSettingsDialog : public Dialog
    {
    public:
        LightSettingsDialog();
        ~LightSettingsDialog();
    protected:
        void init();
        void updateSun(void);
        void enableSunToggleCallback(
            GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
        void sunAzimuthSliderCallback(
            GLMotif::Slider::ValueChangedCallbackData* cbData);
        void sunElevationSliderCallback(
            GLMotif::Slider::ValueChangedCallbackData* cbData);
    private:
        /** Initial enable states of all viewers' headlights */
        std::vector<bool> viewerHeadlightStates;
        /** Flag to store the sun activation state */
        bool enableSun;
        /** Light source representing the sun */
        Vrui::Lightsource* sun;
        /** Azimuth of sunlight direction */
        Vrui::Scalar sunAzimuth;
        /** Elevation of sunlight direction */
        Vrui::Scalar sunElevation;

        GLMotif::TextField* sunAzimuthTextField;
        GLMotif::Slider* sunAzimuthSlider;
        GLMotif::TextField* sunElevationTextField;
        GLMotif::Slider* sunElevationSlider;
    };

    class TerrainColorSettingsDialog : public Dialog
    {
    public:
        TerrainColorSettingsDialog();
    protected:
        void init();
    private:
        void colorButtonCallback(
            GLMotif::Button::SelectCallbackData* cbData);
        void colorChangedCallback(
            GLMotif::ColorPicker::ColorChangedCallbackData* cbData);
        void shininessChangedCallback(
            GLMotif::Slider::ValueChangedCallbackData* cbData);

        GLMotif::Button*           currentButton;
        GLMotif::ColorPickerWindow colorPicker;
        GLMotif::TextField*        shininessField;
    };

    class LayerSettingsDialog : public Dialog
    {
    public:
        LayerSettingsDialog(GLMotif::PaletteEditor* editor);
        void updateLayerList();
    protected:
        void init();
    private:
        void layerChangedCallback(
            GLMotif::ListBox::ValueChangedCallbackData* cbData);
        void visibleCallback(
            GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
        void clampCallback(
            GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

        GLMotif::ListBox*       listBox;
        GLMotif::PaletteEditor* paletteEditor;
        GLMotif::RowColumn*     buttonRoot;
    };

    void produceMainMenu();

    void produceDataDialog();
    void showDataDialogCallback(GLMotif::Button::SelectCallbackData* cbData);
    void addDataCallback(GLMotif::Button::SelectCallbackData* cbData);
    void addDataFileOkCallback(
        GLMotif::FileSelectionDialog::OKCallbackData* cbData);
    void addDataFileCancelCallback(
        GLMotif::FileSelectionDialog::CancelCallbackData* cbData);
    void removeDataCallback(GLMotif::Button::SelectCallbackData* cbData);
    void clearDataCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadDataOkCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadDataCancelCallback(GLMotif::Button::SelectCallbackData* cbData);

    void alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData);

    void changeColorMapCallback(
        GLMotif::ColorMapEditor::ColorMapChangedCallbackData* cbData);
    void changeColorMapRangeCallback(
        GLMotif::RangeEditor::RangeChangedCallbackData* cbData);

    void showPaletteEditorCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

    void decorateLinesCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

    void debugGridCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void debugSpheresCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

    void surfaceSamplePickedCallback(
        SurfaceProbeTool::SampleCallbackData* cbData);

    void resetNavigationCallback(Misc::CallbackData* cbData);

    GLMotif::PopupMenu*   popMenu;
    GLMotif::RadioBox*    curTool;

    GLMotif::PopupWindow*    dataDialog;
    GLMotif::ListBox*        dataListBox;
    std::vector<std::string> dataPaths;

    GLMotif::PaletteEditor* paletteEditor;

    VerticalScaleDialog        verticalScaleSettings;
    OpacityDialog              opacitySettings;
    LightSettingsDialog        lightSettings;
    TerrainColorSettingsDialog terrainColorSettings;
    LayerSettingsDialog        layerSettings;

    /** the crusta instance */
    Crusta* crusta;
		
//- inherited from Vrui::Application
public:
    virtual void frame();
    virtual void display(GLContextData& contextData) const;
    virtual void toolCreationCallback(
        Vrui::ToolManager::ToolCreationCallbackData* cbData);
    virtual void toolDestructionCallback(
        Vrui::ToolManager::ToolDestructionCallbackData* cbData);
};

} //namespace crusta

#endif //_CrustaApp_H_
