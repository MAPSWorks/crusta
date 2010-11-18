#ifndef _DemHeightSubsampleFilter_H_
#define _DemHeightSubsampleFilter_H_


#include <construo/SubsampleFilter.h>

#include <Math/Math.h>

#include <crusta/DemHeight.h>


BEGIN_CRUSTA


template <>
struct SubsampleFilter<DemHeight, SUBSAMPLEFILTER_POINT>
{
    static int width()
    {
        return 0;
    }

    static DemHeight sample(
        const DemHeight* img, const int origin[2],
        const double at[2], const int size[2],
        const DemHeight& imgNodata, const DemHeight& defaultValue,
        const DemHeight&)
    {
        int ip[2];
        for (uint i=0; i<2; ++i)
        {
            double pFloor = Math::floor(at[i] + double(0.5));
            ip[i]         = static_cast<int>(pFloor) - origin[i];
        }
        DemHeight imgValue = img[ip[1]*size[0] + ip[0]];
        return imgValue==imgNodata ? defaultValue : imgValue;
    }

    static DemHeight sample(DemHeight* at, int rowLen, const DemHeight&)
    {
        return *at;
    }
};

template <>
struct SubsampleFilter<DemHeight, SUBSAMPLEFILTER_PYRAMID>
{
    static int width()
    {
        return 1;
    }

    static DemHeight sample(
        const DemHeight* img, const int origin[2],
        const double at[2], const int size[2],
        const DemHeight& imgNodata, const DemHeight& defaultValue,
        const DemHeight& globeNodata)
    {
        int ip[2];
        double d[2];
        for(uint i=0; i<2; ++i)
        {
            double pFloor = Math::floor(at[i]);
            d[i]          = at[i] - pFloor;
            ip[i]         = static_cast<int>(pFloor) - origin[i];
            if (ip[i] == size[i]-1)
            {
                --ip[i];
                d[i] = 1.0;
            }
        }
        const DemHeight* base = img + (ip[1]*size[0] + ip[0]);
        DemHeight v[4];
        v[0] = base[0]         == imgNodata ? defaultValue : base[0];
        v[1] = base[1]         == imgNodata ? defaultValue : base[1];
        v[2] = base[size[0]]   == imgNodata ? defaultValue : base[size[0]];
        v[3] = base[size[0]+1] == imgNodata ? defaultValue : base[size[0]+1];

        double weights[4] = { (1-d[1])*(1-d[0]), (1-d[1])*d[0],
                              d[1]*(1-d[0]), d[1]*d[0] };
        
        double sum        = 0.0;
        double sumWeights = 0.0;
        for (int i=0; i<4; ++i)
        {
            if (v[i] != globeNodata)
            {
                sum        += v[i] * weights[i];
                sumWeights += weights[i];
            }
        }
        
        if (sumWeights == 0.0)
            return globeNodata;
        
        return DemHeight(sum / sumWeights);
    }

    static DemHeight sample(DemHeight* at, int rowLen,
                            const DemHeight& globeNodata)
    {
        static double weightStorage[3] = {0.25, 0.50, 0.25};
        double* weights = &weightStorage[1];

        double sum        = 0.0;
        double sumWeights = 0.0;

        for (int y=-1; y<=1; ++y)
        {
            DemHeight* atY = at + y*rowLen;
            for (int x=-1; x<=1; ++x)
            {
                DemHeight* atYX = atY + x;
                if (*atYX != globeNodata)
                {
                    double weight = weights[y]*weights[x];
                    sum          += double(*atYX) * weight;
                    sumWeights   += weight;
                }
            }
        }

        return DemHeight(sum / sumWeights);
    }
};


template <>
struct SubsampleFilter<DemHeight, SUBSAMPLEFILTER_LANCZOS5>
{
    static int width()
    {
        return 10;
    }

    //no dynamic sample for lanczos5

    static DemHeight sample(DemHeight* at, int rowLen,
                            const DemHeight& globeNodata)
    {
        static const double weightStorage[21] = {
             7.60213661720011e-34,  0.00386785330198227,  -4.5610817871754e-18,
              -0.0167391813072476, 9.83998047615722e-18,    0.0405539013275657,
            -1.47599707142358e-17,  -0.0911355426727928,  1.82443271487016e-17,
                0.313296117460564,    0.500313703779858,     0.313296117460564,
             1.82443271487016e-17,  -0.0911355426727928, -1.47599707142358e-17,
               0.0405539013275657, 9.83998047615722e-18,   -0.0167391813072476,
             -4.5610817871754e-18,  0.00386785330198227,  7.60213661720011e-34};
        const double* weights = &weightStorage[10];

        double sum        = 0.0;
        double sumWeights = 0.0;

        for (int y=-10; y<=10; ++y)
        {
            DemHeight* atY = at + y*rowLen;
            for (int x=-10; x<=10; ++x)
            {
                DemHeight* atYX = atY + x;
                if (*atYX != globeNodata)
                {
                    double weight = weights[y]*weights[x];
                    sum          += double(*atYX) * weight;
                    sumWeights   += weight;
                }
            }
        }

        return DemHeight(sum / sumWeights);
    }
};


END_CRUSTA


#endif //_DemHeightSubsampleFilter_H_
