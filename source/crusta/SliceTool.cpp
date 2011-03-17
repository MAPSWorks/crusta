#include <crusta/SliceTool.h>


#include <Comm/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/StyleSheet.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <limits>

#include <crusta/Crusta.h>


BEGIN_CRUSTA


SliceTool::Factory* SliceTool::factory = NULL;
const Scalar SliceTool::markerSize            = 0.2;
const Scalar SliceTool::selectDistance        = 0.5;
SliceTool::SliceParameters SliceTool::_sliceParameters;
std::vector<Point3> SliceTool::markers;

SliceTool::CallbackData::
CallbackData(SliceTool* probe_) :
    probe(probe_)
{
}

SliceTool::SampleCallbackData::
SampleCallbackData(SliceTool* probe_, int sampleId_, int numSamples_,
                   const SurfacePoint& surfacePoint_) :
    CallbackData(probe_), sampleId(sampleId_), numSamples(numSamples_),
    surfacePoint(surfacePoint_)
{
}

SliceTool::
SliceTool(const Vrui::ToolFactory* iFactory,
                 const Vrui::ToolInputAssignment& inputAssignment) :
    Tool(iFactory, inputAssignment), markersHover(0), markersSelected(0),
    dialog(NULL)
{
    const GLMotif::StyleSheet *style = Vrui::getWidgetManager()->getStyleSheet();


    dialog = new GLMotif::PopupWindow("SliceDialog",
        Vrui::getWidgetManager(), "Crusta Slice Tool");

    GLMotif::RowColumn* top = new GLMotif::RowColumn("SPTtop", dialog, false);
    top->setNumMinorWidgets(3);

    new GLMotif::Label("Angle", top, "Displacement angle");
    GLMotif::Slider *angleSlider = new GLMotif::Slider("DipSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    angleSlider->setValueRange(0, 360.0, 2.5);
    angleSlider->getValueChangedCallbacks().add(this, &SliceTool::angleSliderCallback);
    angleSlider->setValue(0.0);

    angleTextField = new GLMotif::TextField("angleTextField", top, 5);
    angleTextField->setFloatFormat(GLMotif::TextField::FIXED);
    angleTextField->setFieldWidth(2);
    angleTextField->setPrecision(0);

    new GLMotif::Label("DisplacementLabel", top, "Displacement magnitude");
    GLMotif::Slider *displacementSlider = new GLMotif::Slider("displacementSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    displacementSlider->setValueRange(0, 1e6, 1e3);
    displacementSlider->getValueChangedCallbacks().add(this, &SliceTool::displacementSliderCallback);
    displacementSlider->setValue(0.0);

    displacementTextField = new GLMotif::TextField("displacemenTextField", top, 5);
    displacementTextField->setFloatFormat(GLMotif::TextField::FIXED);
    displacementTextField->setFieldWidth(2);
    displacementTextField->setPrecision(0);



    new GLMotif::Label("SlopeAngleLabel", top, "Slope angle");
    GLMotif::Slider *slopeAngleSlider = new GLMotif::Slider("slopeAngleSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    slopeAngleSlider->setValueRange(0.0 + 15.0, 180.0 - 15.0, 1.0);
    slopeAngleSlider->getValueChangedCallbacks().add(this, &SliceTool::slopeAngleSliderCallback);
    slopeAngleSlider->setValue(90.0);

    slopeAngleTextField = new GLMotif::TextField("displacemenTextField", top, 10);
    slopeAngleTextField->setFloatFormat(GLMotif::TextField::FIXED);
    slopeAngleTextField->setFieldWidth(2);
    slopeAngleTextField->setPrecision(0);


    new GLMotif::Label("FalloffLabel", top, "Falloff");
    GLMotif::Slider *falloffSlider = new GLMotif::Slider("falloffSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    falloffSlider->setValueRange(0.0, 5.0, 0.025);
    falloffSlider->getValueChangedCallbacks().add(this, &SliceTool::falloffSliderCallback);
    falloffSlider->setValue(1.0);

    top->manageChild();

    updateTextFields();
    _sliceParameters.updatePlaneParameters(markers);
}

void SliceTool::updateTextFields() {
     //displacementTextField->setValue(_sliceParameters.getShiftVector().mag());
     angleTextField->setValue(_sliceParameters.angle);
     slopeAngleTextField->setValue(_sliceParameters.slopeAngleDegrees);
}

void SliceTool::angleSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.angle = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters(markers);
    updateTextFields();
    Vrui::requestUpdate();
}

void SliceTool::displacementSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.displacementAmount = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters(markers);
    updateTextFields();
    Vrui::requestUpdate();
}
void SliceTool::slopeAngleSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.slopeAngleDegrees = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters(markers);
    updateTextFields();
    Vrui::requestUpdate();
}

void SliceTool::falloffSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.falloffFactor = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters(markers);
    updateTextFields();
    Vrui::requestUpdate();
}

SliceTool::
~SliceTool()
{
    //close the dialog
    if (dialog != NULL)
        Vrui::popdownPrimaryWidget(dialog);
}


Vrui::ToolFactory* SliceTool::
init()
{
    //markers.push_back(Point3(6371000,0,0));
    //markers.push_back(Point3(0,6371000,0));

    _sliceParameters.angle = 0.0;
    _sliceParameters.displacementAmount = 0.0;
    _sliceParameters.slopeAngleDegrees = 90.0;
    _sliceParameters.falloffFactor = 1.0;
    _sliceParameters.updatePlaneParameters(markers);


    Vrui::ToolFactory* crustaToolFactory = dynamic_cast<Vrui::ToolFactory*>(
            Vrui::getToolManager()->loadClass("CrustaTool"));

    Factory* surfaceFactory = new Factory(
        "CrustaSliceTool", "Slice Tool",
        crustaToolFactory, *Vrui::getToolManager());

    surfaceFactory->setNumDevices(1);
    surfaceFactory->setNumButtons(0, 1);

    Vrui::getToolManager()->addClass(surfaceFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    SliceTool::factory = surfaceFactory;

    return SliceTool::factory;
}

void SliceTool::
resetMarkers()
{
    markers.clear();
    markersHover       = 0;
    markersSelected    = 0;
    _sliceParameters.updatePlaneParameters(markers);
}

void SliceTool::
callback()
{
}

const Vrui::ToolFactory* SliceTool::
getFactory() const
{
    return factory;
}

void SliceTool::
frame()
{
    //project the device position
    surfacePoint = project(input.getDevice(0), false);

    //no updates if the projection failed
    if (projectionFailed)
        return;

    const Point3& pos = surfacePoint.position;

    if (Vrui::isMaster())
    {
        //compute selection distance
        double scaleFac   = Vrui::getNavigationTransformation().getScaling();
        double selectDist = selectDistance / scaleFac;

        double minDist = std::numeric_limits<double>::max();
        markersHover = 0;

        for (size_t i=0; i < markers.size(); ++i) {
            double dist = Geometry::dist(pos, markers[i]);
            if (dist < selectDist && dist < minDist) {
                minDist = dist;
                markersHover = i+1;
            }
        }

        if (Vrui::getMainPipe() != NULL)
            Vrui::getMainPipe()->write<int>(markersHover);
    }
    else
        Vrui::getMainPipe()->read<int>(markersHover);

    if (markersSelected != 0)
    {
        markers[markersSelected - 1] = pos;
        _sliceParameters.updatePlaneParameters(markers);
        callback();
    }
}

void SliceTool::
display(GLContextData& contextData) const
{
//- render the surface projector stuff
    //transform the position back to physical space
    const Vrui::NavTransform& navXform = Vrui::getNavigationTransformation();
    Point3 position = navXform.transform(surfacePoint.position);

    Vrui::NavTransform devXform = input.getDevice(0)->getTransformation();
    Vrui::NavTransform projXform(Vector3(position), devXform.getRotation(),
                                 devXform.getScaling());

    SurfaceProjector::display(contextData, projXform, devXform);

//- render own stuff
    CHECK_GLA
    //don't draw anything if we don't have any control points yet
    if (markers.empty())
        return;

    //save relevant GL state and set state for marker rendering
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);

    //go to navigational coordinates
    std::vector<Point3> markerPos;
    for (int i=0; i< markers.size(); ++i)
        markerPos.push_back(crusta->mapToScaledGlobe(markers[i]));

    Vector3 centroid(0.0, 0.0, 0.0);
    for (int i=0; i < markers.size(); ++i)
        centroid += Vector3(markerPos[i]);
    centroid /= markers.size();

    //load the centroid relative translated navigation transformation
    glPushMatrix();

    Vrui::NavTransform nav =
        Vrui::getDisplayState(contextData).modelviewNavigational;
    nav *= Vrui::NavTransform::translate(centroid);
    glLoadMatrix(nav);

    //compute marker size
    Scalar scaleFac = Vrui::getNavigationTransformation().getScaling();
    Scalar size     = markerSize/scaleFac;

    CHECK_GLA
    //draw the control points
    for (int i=0; i < markers.size(); ++i)
    {
        if (markersHover == i+1)
        {
            glColor3f(0.3f, 0.9f, 0.5f);
            glLineWidth(2.0f);
        }
        else
        {
            glColor3f(1.0f, 1.0f, 1.0f);
            glLineWidth(1.0f);
        }

        markerPos[i] = Point3(Vector3(markerPos[i])-centroid);
        glBegin(GL_LINES);
            glVertex3f(markerPos[i][0]-size, markerPos[i][1], markerPos[i][2]);
            glVertex3f(markerPos[i][0]+size, markerPos[i][1], markerPos[i][2]);
            glVertex3f(markerPos[i][0], markerPos[i][1]-size, markerPos[i][2]);
            glVertex3f(markerPos[i][0], markerPos[i][1]+size, markerPos[i][2]);
            glVertex3f(markerPos[i][0], markerPos[i][1], markerPos[i][2]-size);
            glVertex3f(markerPos[i][0], markerPos[i][1], markerPos[i][2]+size);
        glEnd();

        CHECK_GLA
    }

    //restore coordinate system
    glPopMatrix();
    //restore GL state
    glPopAttrib();
    CHECK_GLA
}


void SliceTool::
buttonCallback(int, int, Vrui::InputDevice::ButtonCallbackData* cbData)
{
    //project the device position
    surfacePoint = project(input.getDevice(0), false);

    //disable any button callback if the projection has failed.
    if (projectionFailed)
        return;

    if (cbData->newButtonState)
    {
        if (markersHover > 0) {
            // cursor hovering above existing marker - drag it
            markers[markersHover - 1]      = surfacePoint.position;
            markersSelected = markersHover;
        } else if (markers.size() < 16) {
            // not all markers used - add new marker
            markers.push_back(surfacePoint.position);
            markersHover = markersSelected = markers.size();
        } else {
            // all markers in use - reset markers
            markers.clear();
            markers.push_back(surfacePoint.position);
            markersHover    = 1;
            markersSelected = 1;
        }

        _sliceParameters.updatePlaneParameters(markers);
        callback();
    }
    else
        markersSelected = 0;

}

void SliceTool::
setupComponent(Crusta* crusta)
{
    //base setup
    SurfaceProjector::setupComponent(crusta);

    //popup the dialog
    const Vrui::NavTransform& navXform = Vrui::getNavigationTransformation();
    Vrui::popupPrimaryWidget(dialog,
        navXform.transform(Vrui::getDisplayCenter()));
}

SliceTool::Plane::Plane(const Vector3 &a, const Vector3 &b, double slopeAngleDegrees) : startPoint(a), endPoint(b) {
    strikeDirection = endPoint - startPoint;
    strikeDirection.normalize();

    // midpoint of fault line / plane

    // radius vector (center of planet to midpoint of fault line) = normal (because planet center is at (0,0,0))
    normal = cross(strikeDirection, startPoint); // normal of plane containing point and planet center

    // rotate normal by 90 degrees around strike dir, now it's the tangential (horizon) plane
    // additionally, rotate by given slope angle
    normal = Vrui::Rotation::rotateAxis(strikeDirection, (90.0 + slopeAngleDegrees) / 360.0 * 2 * M_PI).transform(normal);
    normal.normalize();

    // dip vector
    dipDirection = cross(strikeDirection, normal);
    dipDirection.normalize(); // just to be sure :)

    distance = startPoint * normal;
}

double SliceTool::Plane::getPointDistance(const Vector3 &point) const {
    return normal * point - distance;
}

Vector3 SliceTool::Plane::getPlaneCenter() const {
    return getPointDistance(Vector3(0,0,0)) * normal;
}

SliceTool::SliceParameters::SliceParameters() {
}


const SliceTool::SliceParameters &SliceTool::getParameters() {
    return _sliceParameters;
}

void SliceTool::SliceParameters::updatePlaneParameters(const std::vector<Point3> &pts) {
    std::cout << "updatePlaneParameters: " << pts.size() << " markers" << std::endl;

    faultCenter = Vector3(0,0,0);
    if (!pts.empty()) {
        for (size_t i=0; i < pts.size(); ++i)
            faultCenter += Vector3(pts[i]);
        faultCenter *= 1.0 / pts.size();
    }

    faultPlanes.clear();
    separatingPlanes.clear();

    if (pts.size() < 2)
        return;

    size_t nPlanes = pts.size() - 1;

    for (size_t i=0; i < nPlanes; ++i) {
        faultPlanes.push_back(Plane(Vector3(pts[i]), Vector3(pts[i+1]), slopeAngleDegrees));
    }

    for (size_t i=0; i < pts.size(); ++i) {
        Vector3 n(0,0,0);
        if (i == 0)
            n = faultPlanes[i].normal;
        else if (i == pts.size() - 1)
            n = faultPlanes[i-1].normal;
        else
            n = 0.5 * (faultPlanes[i-1].normal + faultPlanes[i].normal);
        n.normalize();

        separatingPlanes.push_back(Plane(Vector3(pts[i]), Vector3(pts[i]) + 1e6 * n, 270.0));
    }
}


Vector3 SliceTool::SliceParameters::getShiftVector(const Plane &p) const {
    double strikeComponent = cos(angle / 360.0 * 2 * M_PI);
    double dipComponent = sin(angle / 360.0 * 2 * M_PI);

    return Vector3(displacementAmount * strikeComponent * p.strikeDirection +
                   displacementAmount * dipComponent * p.dipDirection);
}


END_CRUSTA
