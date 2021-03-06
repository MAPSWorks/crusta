#ifndef _Converters_H_
#define _Converters_H_

#include <crustacore/basics.h>

#include <construo/vrui.h>

namespace crusta {

namespace Converter
{

template <typename ScalarParam>
inline Geometry::Point<ScalarParam, 2>
cartesianToSpherical(const Geometry::Point<ScalarParam, 3>& p)
{
    static const ScalarParam halfPi =
        Math::Constants<ScalarParam>::pi*ScalarParam(0.5);

    ScalarParam len   = sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
    ScalarParam theta = halfPi - acos(p[2]/len);
    ScalarParam phi   = atan2(p[1], p[0]);

    return Geometry::Point<ScalarParam, 2>(phi, theta);
}

template <typename ScalarParam>
inline Geometry::Point<ScalarParam, 3>
sphericalToCartesian(const Geometry::Point<ScalarParam, 2>& p,
                     ScalarParam radius=ScalarParam(1))
{
    static const ScalarParam halfPi =
        Math::Constants<ScalarParam>::pi*ScalarParam(0.5);

    Geometry::Point<ScalarParam, 3> r(
        radius * Math::cos(p[0]) * Math::sin(halfPi - p[1]),
        radius * Math::sin(p[0]) * Math::sin(halfPi - p[1]),
        radius * Math::cos(halfPi - p[1])
    );
    return r;
}

//Haversine distance between two points. Input assumed in radians.
template <typename ScalarParam>
inline ScalarParam
haversineDist(const Geometry::Point<ScalarParam, 2>& one,
              const Geometry::Point<ScalarParam, 2>& two,
              ScalarParam radius)
{
    Geometry::Point<ScalarParam, 2> delta(two[0]-one[0], two[1]-one[1]);

    ScalarParam a = Math::sin(delta[1]*ScalarParam(0.5)) *
                    Math::sin(delta[1]*ScalarParam(0.5))     +
                    Math::cos(one[1]) *Math::cos(two[1]) *
                    Math::sin(delta[0]*ScalarParam(0.5)) *
                    Math::sin(delta[0]*ScalarParam(0.5));
    ScalarParam c = ScalarParam(2) *
                    Math::atan2(Math::sqrt(a), Math::sqrt(ScalarParam(1)-a));

    return radius * c;
}


} //namespace Converter

} //namespace crusta

#endif //_Converters_H_
