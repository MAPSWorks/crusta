///\todo GPL

#ifndef _ColorTextureSpecs_H_
#define _ColorTextureSpecs_H_

#include <Geometry/Vector.h>

#include <construo/QuadtreeFile.h>
#include <construo/QuadtreeFileHeaders.h>
#include <construo/Tree.h>

BEGIN_CRUSTA

/// data type for values of a color texture stored in the preproccessed database
typedef Geometry::Vector<uint8, 3> TextureColor;

inline
TextureColor operator*(const TextureColor& tc, double d) {
    return TextureColor(tc[0]*d, tc[1]*d, tc[2]*d);
}
inline
TextureColor operator*(double d, const TextureColor& tc) {
    return d * tc;
}

template<>
class TreeState<TextureColor>
{
public:
    typedef QuadtreeFile< TextureColor, QuadtreeFileHeader<TextureColor>,
                          QuadtreeTileHeader<TextureColor>               > File;

    File* file;
};

template <>
struct QuadtreeTileHeader<TextureColor> :
    public QuadtreeTileHeaderBase<TextureColor>
{
    static const TextureColor defaultPixelValue;
};


END_CRUSTA

#endif //_ColorTextureSpecs_H_
