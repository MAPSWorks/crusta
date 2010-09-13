#ifndef _Crusta_H_
#define _Crusta_H_

#include <string>
#include <vector>

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/GLShader.h>
#include <Threads/Mutex.h>

#include <crusta/basics.h>
#include <crusta/CrustaSettings.h>
#include <crusta/LightingShader.h>
#include <crusta/map/Shape.h>

#if CRUSTA_ENABLE_DEBUG
#include <crusta/DebugTool.h>
#include <crusta/Timer.h>
#endif //CRUSTA_ENABLE_DEBUG


class GLColorMap;
class GLContextData;

BEGIN_CRUSTA

template <typename NodeDataType>
class CacheBuffer;

class Cache;
class DataManager;
class GpuLineCache;
class MapManager;
class QuadNodeMainData;
class QuadTerrain;
class VideoCache;

struct CrustaGlData : public GLObject::DataItem
{
    CrustaGlData();
    ~CrustaGlData();

    /** store a handle to the video cache of this context for convenient
        access */
    VideoCache* videoCache;
    /** store a handle to the line cache of this context for convenient
        access */
    GpuLineCache* lineCache;

    /** basic data being passed to the GL to represent a vertex. The
        template provides simply texel-centered, normalized texture
        coordinates that are used to address the corresponding data in the
        geometry, elevation and color textures */
    GLuint vertexAttributeTemplate;
    /** defines a triangle-string triangulation of the vertices */
    GLuint indexTemplate;

    GLuint coverageFbo;

    GLuint symbolTex;

    GLuint colorMap;

    LightingShader terrainShader;

    GLint    lineCoverageTransformUniform;
    GLShader lineCoverageShader;
};

/** Main crusta class */
class Crusta : public GLObject
{
public:
    typedef std::vector<CacheBuffer<QuadNodeMainData>*> Actives;

    void init(const std::string& demFileBase, const std::string& colorFileBase);
    void shutdown();

///\todo potentially deprecate
    /** snap the given cartesian point to the surface of the terrain (at an
        optional offset) */
    Point3 snapToSurface(const Point3& pos, Scalar offset=Scalar(0));
    /** intersect a ray with the crusta globe */
    HitResult intersect(const Ray& ray) const;

    /** intersect a single segment with the global hierarchy */
    void intersect(Shape::ControlPointHandle start,
                   Shape::IntersectionFunctor& callback) const;

    const FrameNumber& getCurrentFrame()   const;
    const FrameNumber& getLastScaleFrame() const;

    /** configure the display of the terrain to use a texture or not */
    void setTexturingMode(int mode);
    /** set the vertical exaggeration. Make sure to set this value within a
        frame callback so that it doesn't change during a rendering phase */
    void setVerticalScale(double newVerticalScale);
    /** retrieve the vertical exaggeration factor */
    double getVerticalScale() const;

    /** query the color map for external update */
    GLColorMap* getColorMap();
    /** signal crusta that changes have been made to the color map */
    void touchColorMap();
    /** upload the color map to the GL */
    void uploadColorMap(GLuint colorTex);

    /** map a 3D cartesian point specified wrt an unscaled globe representation
        to the corresponding point in a scaled representation */
    Point3 mapToScaledGlobe(const Point3& pos);
    /** map a 3D cartesian point specified on a scaled globe representation to
        the corresponding point in an unscaled representation */
    Point3 mapToUnscaledGlobe(const Point3& pos);

    Cache*       getCache()       const;
    DataManager* getDataManager() const;
    MapManager*  getMapManager()  const;

    /** inform crusta of nodes that must be kept current */
    void submitActives(const Actives& touched);

    void frame();
    void display(GLContextData& contextData);

    /** query the crusta settings */
    const CrustaSettings& getSettings() const;

    /** toggle the decoration of the line mapping */
    void setDecoratedVectorArt(bool flag=true);
    /** change the specular color of the terrain surface */
    void setTerrainSpecularColor(const Color& color);
    /** change the shininess of the terrain surface */
    void setTerrainShininess(const float& shininess);


///\todo integrate me into the system properly (VIS 2010)
/** the size of the line data texture */
static const int   lineDataTexSize;
static const float lineDataCoordStep;
static const float lineDataStartCoord;
/** the pixel resolution of the coverage maps */
static const int lineCoverageTexSize;

#if CRUSTA_ENABLE_DEBUG
DebugTool* debugTool;
Timer      debugTimers[10];
#endif //CRUSTA_ENABLE_DEBUG

///\todo debug
void confirmLineCoverageRemoval(Shape* shape, Shape::ControlPointHandle cp);
void validateLineCoverage();

protected:
    typedef std::vector<QuadTerrain*> RenderPatches;

    /** make sure the bounding objects used for visibility and LOD checks are
        up-to-date wrt to the vertical scale */
    void confirmActives();

    /** keep track of the number of frames processed. Used, for example, by the
        cache to perform LRU that is aware of currently active nodes (the ones
        from the previous frame) */
    FrameNumber currentFrame;
    /** keep track of the last frame at which the vertical scale was modified.
        The vertical scale affects the bounding primitives for the nodes and
        these must be updated each time the scale changes. Validity of a node's
        semi-static data can be verified by comparison with this number */
    FrameNumber lastScaleFrame;

    /** texturing mode to use for terrain rendering */
    int texturingMode;
    /** the vertical scale to be applied to all surface elevations */
    Scalar verticalScale;
    /** the vertical scale that has been externally set. Buffers the scales
        changes up to the next frame call */
    Scalar newVerticalScale;

    /** the cache management component */
    Cache* cache;
    /** the data management component */
    DataManager* dataMan;
    /** the mapping management component */
    MapManager* mapMan;

    /** the spheroid base patches used for rendering */
    RenderPatches renderPatches;

    /** the nodes that have been touch during the traversals of the previous
        frame */
    Actives actives;
    /** guarantee serial manipulation of the set of active nodes */
    Threads::Mutex activesMutex;

    /** the global height range */
    Scalar globalElevationRange[2];

    /** flags if the color map has been changed and needs to be updated to the
        GL */
    bool colorMapDirty;
    /** the color map used to color the elevation of the terrain */
    GLColorMap* colorMap;

    /** user tweakable settings (both runtime and through configuration file */
    CrustaSettings settings;

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_Crusta_H_
