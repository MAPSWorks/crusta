#include <crusta/map/Shape.h>

#include <cassert>
#include <stdexcept>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>

#include <crusta/vrui.h>


namespace crusta {

      Shape::Symbol    Shape::DEFAULT_SYMBOL;
const Shape::ControlId Shape::BAD_ID(CONTROL_INVALID, ControlPointHandle());


/* the default stamp for a control point is one that will never prompt an
   update */
Shape::ControlPoint::
ControlPoint() :
    stamp(Math::Constants<FrameStamp>::max), pos(0), coord(0)
{}

Shape::ControlPoint::
ControlPoint(const ControlPoint& other) :
    stamp(other.stamp), pos(other.pos), coord(other.coord)
{}

Shape::ControlPoint::
ControlPoint(const Geometry::Point<double,3>& iPos) :
    stamp(Math::Constants<FrameStamp>::max), pos(iPos), coord(0)
{}

Shape::ControlPoint::
ControlPoint(const FrameStamp& iStamp, const Geometry::Point<double,3>& iPos) :
    stamp(iStamp), pos(iPos), coord(0)
{}


std::ostream&
operator<<(std::ostream& os, const Shape::ControlPointHandle& cph)
{
    os << "cph(" << cph->stamp << "." << &(*cph) << ")";
    return os;
}

Shape::Symbol::
Symbol() :
    id(0), color(0.8f, 0.7f, 0.5f, 1.0f), originSize(0.0f, 0.5f, 1.0f, 0.5f)
{}

Shape::Symbol::
Symbol(int iId, const Color& iColor) :
    id(iId), color(iColor)
{}

Shape::Symbol::
Symbol(int iId, float iRed, float iGreen, float iBlue) :
    id(iId), color(iRed, iGreen, iBlue)
{}



Shape::ControlId::
ControlId():
    type(BAD_ID.type), handle(BAD_ID.handle)
{}

Shape::ControlId::
ControlId(const ControlId& other) :
    type(other.type), handle(other.handle)
{}

Shape::ControlId::
ControlId(int iType, const ControlPointHandle& iHandle) :
    type(iType), handle(iHandle)
{}


bool Shape::ControlId::
isValid() const
{
    return type!=BAD_ID.type;
}

bool Shape::ControlId::
isPoint() const
{
    return type==CONTROL_POINT;
}

bool Shape::ControlId::
isSegment() const
{
    return type==CONTROL_SEGMENT;
}


Shape::ControlId& Shape::ControlId::
operator=(const ControlId& other)
{
    type   = other.type;
    handle = other.handle;
    return *this;
}

bool Shape::ControlId::
operator ==(const ControlId& other) const
{
    return type==other.type && handle==other.handle;
}

bool Shape::ControlId::
operator !=(const ControlId& other) const
{
    return type!=other.type || handle!=other.handle;
}

std::ostream&
operator<<(std::ostream& os, const Shape::ControlId& cid)
{
    os << "ci(" << cid.type << "." << &(*cid.handle) << ")";
    return os;
}


Shape::
Shape(Crusta* iCrusta) :
    CrustaComponent(iCrusta), id(~0), symbol(DEFAULT_SYMBOL)
{
}

Shape::
~Shape()
{
    //make sure to remove the shape's coverage and data from the hierarchy
    crusta->getMapManager()->removeShapeCoverage(this, controlPoints.begin(),
        controlPoints.end());
}


Shape::ControlId Shape::
select(const Geometry::Point<double,3>& pos, double& dist, double pointBias)
{
    double pointDist;
    ControlId pointId = selectPoint(pos, pointDist);
    double segmentDist;
    ControlId segmentId = selectSegment(pos, segmentDist);

    if (pointDist*pointBias <= segmentDist)
    {
        dist = pointDist;
        return pointId;
    }
    else
    {
        dist = segmentDist;
        return segmentId;
    }
}

Shape::ControlId Shape::
selectPoint(const Geometry::Point<double,3>& pos, double& dist)
{
    ControlId retId = BAD_ID;
    dist            = Math::Constants<double>::max;

    for (ControlPointHandle it=controlPoints.begin(); it!=controlPoints.end();
         ++it)
    {
        double newDist = Geometry::dist(it->pos, pos);
        if (newDist < dist)
        {
            retId.type   = CONTROL_POINT;
            retId.handle = it;
            dist         = newDist;
        }
    }

    return retId;
}

Shape::ControlId Shape::
selectSegment(const Geometry::Point<double,3>& pos, double& dist)
{
    ControlId retId = BAD_ID;
    dist            = Math::Constants<double>::max;

    ControlPointHandle end = --controlPoints.end();
    for (ControlPointHandle it=controlPoints.begin(); it!=end; ++it)
    {
        ControlPointHandle nit = it;
        ++nit;

        const Geometry::Point<double,3>& start = it->pos;
        const Geometry::Point<double,3>& end   = nit->pos;

        Geometry::Vector<double,3> seg   = end - start;
        Geometry::Vector<double,3> toPos = pos - start;

        double segSqrLen = seg*seg;
        double u = (toPos * seg) / segSqrLen;
        if (u<0 || u>1.0)
            continue;

        //assumes the center of the sphere is at the origin
        Geometry::Vector<double,3> toStart(start);
        toStart.normalize();
        seg           /= Math::sqrt(segSqrLen);
        Geometry::Vector<double,3> normal = Geometry::cross(toStart, seg);

        double newDist = Math::abs(Geometry::Vector<double,3>(pos) * normal);
        if (newDist < dist)
        {
            retId.type   = CONTROL_SEGMENT;
            retId.handle = it;
            dist         = newDist;
        }
    }

    return retId;
}

Shape::ControlId Shape::
selectExtremity(const Geometry::Point<double,3>& pos, double& dist, End& end)
{
    assert(controlPoints.size()>0);
    double frontDist = Geometry::dist(pos, controlPoints.front().pos);
    double backDist  = Geometry::dist(pos, controlPoints.back().pos);

    if (frontDist < backDist)
    {
        dist = frontDist;
        end  = END_FRONT;
        return ControlId(CONTROL_POINT, controlPoints.begin());
    }
    else
    {
        dist = backDist;
        end  = END_BACK;
        return ControlId(CONTROL_POINT, --controlPoints.end());
    }
}


void Shape::
setControlPoints(const std::vector<Geometry::Point<double,3> >& newControlPoints)
{
    MapManager* mapMan  = crusta->getMapManager();

    //delete the old control points to the coverage hierarchy
    mapMan->removeShapeCoverage(this,controlPoints.begin(),controlPoints.end());
    controlPoints.clear();

    //assign the new control point positions
    for (std::vector<Geometry::Point<double,3> >::const_iterator it=newControlPoints.begin();
         it!=newControlPoints.end(); ++it)
    {
        controlPoints.push_back(ControlPoint(*it));
    }

    //add the new control points to the coverage hierarchy
    mapMan->addShapeCoverage(this, controlPoints.begin(), controlPoints.end());
}

Shape::ControlId Shape::
addControlPoint(const Geometry::Point<double,3>& pos, End end)
{
    MapManager* mapMan  = crusta->getMapManager();

    if (end == END_FRONT)
    {
CRUSTA_DEBUG(40, std::cerr << "++++AddCP @ FRONT:\n";)
        controlPoints.push_front(ControlPoint(pos));
        ControlPointHandle end = ++controlPoints.begin();
        if (end != controlPoints.end())
            ++end;
        mapMan->addShapeCoverage(this, controlPoints.begin(), end);

CRUSTA_DEBUG(40, std::cerr << "----added\n" <<
             ControlId(CONTROL_POINT, controlPoints.begin()) << std::endl;)
        return ControlId(CONTROL_POINT, controlPoints.begin());
    }
    else
    {
CRUSTA_DEBUG(40, std::cerr << "++++AddCP @ END\n";)
        controlPoints.push_back(ControlPoint(pos));
        ControlPointHandle start = --controlPoints.end();
        if (controlPoints.size()>1)
            --start;
        mapMan->addShapeCoverage(this, start, controlPoints.end());

CRUSTA_DEBUG(40, std::cerr << "----added " <<
             ControlId(CONTROL_POINT, --controlPoints.end()) << ".\n\n\n";)
        return ControlId(CONTROL_POINT, --controlPoints.end());
    }
}

void Shape::
moveControlPoint(const ControlId& id, const Geometry::Point<double,3>& pos)
{
///\todo only works for single map tool: ids can't be invalidated outside tool
    assert(isValid(id));

    MapManager* mapMan  = crusta->getMapManager();

CRUSTA_DEBUG(40, std::cerr << "++++MoveCP " << id << " \n";)
    //remove the old affected segments
    ControlPointHandle start = id.handle;
    if (start != controlPoints.begin())
        --start;
    ControlPointHandle end = id.handle; ++end;
    if (end != controlPoints.end())
        ++end;

    mapMan->removeShapeCoverage(this, start, end);

    //move the control point and update its age
    id.handle->pos   = pos;

    //add the new affected segments
    mapMan->addShapeCoverage(this, start, end);
CRUSTA_DEBUG(40, std::cerr << "----moved.\n\n\n";)
}

void Shape::
removeControlPoint(const ControlId& id)
{
///\todo only works for single map tool: ids can't be invalidated outside tool
    assert(isValid(id));

    MapManager* mapMan  = crusta->getMapManager();

CRUSTA_DEBUG(40, std::cerr << "++++DelCP " << id << " \n";)
    //remove affected segments
    ControlPointHandle start = id.handle;
    if (start != controlPoints.begin())
        --start;
    ControlPointHandle end = id.handle; ++end;
    if (end != controlPoints.end())
        ++end;

    mapMan->removeShapeCoverage(this, start, end);

    //delete the control point
    start = controlPoints.erase(id.handle);

    //add new shortcut segment
    end = start;
    if (start!=controlPoints.begin())
        --start;
    if (end != controlPoints.end())
        ++end;

    mapMan->addShapeCoverage(this, start, end);

CRUSTA_DEBUG(40, std::cerr << "----deleted.\n\n\n";)
}

Shape::ControlId Shape::
refine(const ControlId& id, const Geometry::Point<double,3>& pos)
{
///\todo only works for single map tool: ids can't be invalidated outside tool
    assert(isValid(id));

    MapManager* mapMan  = crusta->getMapManager();

CRUSTA_DEBUG(40, std::cerr << "++++RefineSeg " << id << " \n";)
    //determine insertion point
    ControlId retControl = nextControl(id);

    //remove affected segment
    ControlPointHandle start = previousControl(id).handle;
    ControlPointHandle end   = retControl.handle;
    assert(end != controlPoints.end());
    ++end;

    mapMan->removeShapeCoverage(this, start, end);

    //insert new control point
    retControl.handle = controlPoints.insert(retControl.handle,
                                             ControlPoint(pos));

    //add affected segments
    mapMan->addShapeCoverage(this, start, end);

CRUSTA_DEBUG(40, std::cerr << "refined " << id << ".\n\n\n";)
    return retControl;
}


Shape::ControlId Shape::
previousControl(const ControlId& id) const
{
///\todo only works for single map tool: ids can't be invalidated outside tool
    assert(isValid(id));

    switch (id.type)
    {
        case CONTROL_POINT:
            if (id.handle != controlPoints.begin())
            {
                return ControlId(CONTROL_SEGMENT,
                                 --ControlPointHandle(id.handle));
            }
            else
                return BAD_ID;

        case CONTROL_SEGMENT:
            return ControlId(CONTROL_POINT, id.handle);

        default:
            return BAD_ID;
    }
}

Shape::ControlId Shape::
nextControl(const ControlId& id) const
{
///\todo only works for single map tool: ids can't be invalidated outside tool
    assert(isValid(id));

    switch (id.type)
    {
        case CONTROL_POINT:
            if (id.handle != --controlPoints.end())
                return BAD_ID;
            else
                return ControlId(CONTROL_SEGMENT, id.handle);

        case CONTROL_SEGMENT:
            return ControlId(CONTROL_POINT, ++ControlPointHandle(id.handle));

        default:
            return BAD_ID;
    }
}



void Shape::
setId(const IdGenerator32::Id& nId)
{
    id = nId;
}

const IdGenerator32::Id& Shape::
getId() const
{
    return id;
}

void Shape::
setSymbol(const Symbol& nSymbol)
{
    symbol = nSymbol;
    //dirty the whole shape to prompt an update of the display representation
    FrameStamp curStamp = CURRENT_FRAME;
    for (ControlPointList::iterator it=controlPoints.begin();
         it!=controlPoints.end(); ++it)
    {
        it->stamp = curStamp;
    }
}

const Shape::Symbol& Shape::
getSymbol() const
{
    return symbol;
}

Shape::ControlPointList& Shape::
getControlPoints()
{
    return controlPoints;
}

const Shape::ControlPointList& Shape::
getControlPoints() const
{
    return controlPoints;
}


bool Shape::
isValid(const ControlId& control) const
{
    ControlPointList::const_iterator handle;
    for (handle=controlPoints.begin();
         handle!=controlPoints.end() && handle!=control.handle; ++handle);
    if (handle!=controlPoints.end())
    {
        if (control.isSegment() && handle==--controlPoints.end())
            return false;
        else
            return true;
    }
    else
        return false;
}


} //namespace crusta
