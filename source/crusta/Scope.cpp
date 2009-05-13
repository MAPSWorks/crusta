#include <crusta/Scope.h>

BEGIN_CRUSTA

Scope::
Scope()
{
    corners[0] = corners[1] = corners[2] = corners[3] = Vertex(0);
}
Scope::
Scope(const Vertex& p1,const Vertex& p2,const Vertex& p3,const Vertex& p4)
{
    corners[0] = p1; corners[1] = p2; corners[2] = p3; corners[3] = p4;
}

Scalar Scope::
getRadius()
{
    //compute the radius of the sphere the scope lies on
    return sqrt(corners[0][0]*corners[0][0] +
                corners[0][1]*corners[0][1] +
                corners[0][2]*corners[0][2]);    
}

void Scope::
split(Scope scopes[4])
{
    //compute the radius of the sphere the scope lies on
    Scope::Scalar radius = getRadius();

    //compute the scope edge mid-points
    Vertex mids[4];
    static const uint cornerIndices[5] = {3, 2, 0, 1, 3};
    for (uint i=0; i<4; ++i)
    {
        Vertex& one = corners[cornerIndices[i]];
        Vertex& two = corners[cornerIndices[i+1]];
        Scope::Scalar len = Scope::Scalar(0);
        for (uint j=0; j<3; ++j)
        {
            mids[i][j] = (one[j] + two[j]) * Scope::Scalar(0.5);
            len       += mids[i][j] * mids[i][j];
        }
        len = radius / sqrt(len);
        for (uint j=0; j<3; ++j)
            mids[i][j] *= len;
    }

    //compute the centroid point
    Vertex centroid;
    Scope::Scalar len = Scope::Scalar(0);
    for (uint i=0; i<3; ++i)
    {
        centroid[i] = (mids[0][i] + mids[2][i]) * Scope::Scalar(0.5);
        len        += centroid[i] * centroid[i];
    }
    len = radius / sqrt(len);
    for (uint i=0; i<3; ++i)
        centroid[i] *= len;

    //assemble the split scopes
    scopes[0][0] = corners[0];
    scopes[0][1] = mids[2];
    scopes[0][2] = mids[1];
    scopes[0][3] = centroid;

    scopes[1][0] = mids[2];
    scopes[1][1] = corners[1];
    scopes[1][2] = centroid;
    scopes[1][3] = mids[3];

    scopes[2][0] = mids[1];
    scopes[2][1] = centroid;
    scopes[2][2] = corners[2];
    scopes[2][3] = mids[0];

    scopes[3][0] = centroid;
    scopes[3][1] = mids[3];
    scopes[3][2] = mids[0];
    scopes[3][3] = corners[3];
}


END_CRUSTA