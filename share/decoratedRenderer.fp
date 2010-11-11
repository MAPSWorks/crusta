#extension GL_EXT_gpu_shader4   : enable
#extension GL_EXT_texture_array : enable

uniform float lineStartCoord;
uniform float lineCoordStep;

uniform int lineNumSegments;
uniform float lineCoordScale;
uniform float lineWidth;

uniform vec3 center;

uniform vec2 lineDataTexOffset;
uniform vec2 lineDataTexScale;
uniform vec3 coverageTexOffset;
uniform vec2 coverageTexScale;

uniform sampler2D      symbolTex;
uniform sampler2D      lineDataTex;
uniform sampler2DArray lineCoverageTex;

varying vec3 position;
varying vec3 normal;
varying vec2 texCoord;

const float EPSILON = 0.00001;

#define NORMAL   1
#define TWIST    1
#define COVERAGE 0
#define COLORIZE_COVERAGE 0

///\todo move to uniform specification
#define U_SCALE 0.15
#define V_SCALE 3.0

struct Segment
{
    vec4 symbol;
    vec4 start;
    vec4 end;
    vec3 normal;
};

vec4 sampleLineData(in float tc)
{
    /* make sure the texture coordinate is texel centered. tc already is because
       it depends on lineStartCoord. We have a 1D strip so shift half a texel
       in s*/
    vec2 tc2 = vec2(tc, 0.5);
    tc2 = lineDataTexOffset + tc2*lineDataTexScale;
    return texture2D(lineDataTex, tc2);
}

vec2 sampleCoverage(in vec2 tc)
{
    vec3 tc2 = coverageTexOffset + vec3((tc * coverageTexScale), 0.0);
    return texture2DArray(lineCoverageTex, tc2).rg;
}

vec4 read(inout float coord)
{
    vec4 r = sampleLineData(coord);
    coord += lineCoordStep;
    return r;
}

Segment readSegment(inout float coord)
{
    Segment segment;
    segment.symbol = read(coord);
    segment.start  = read(coord);
    segment.end    = read(coord);
    segment.normal = read(coord).xyz;
    return segment;
}

void computeSegment(in Segment segment, inout vec4 color)
{
//- compute lateral coordinate v

    //compute v without taking distortion into account:
    float v = dot(position - segment.start.xyz, segment.normal);

#if TWIST
    // Correct for distortion by dividing by sin(alpha):
    float snn = dot(segment.normal, normal);
    v = v / sqrt(1.0 - snn * snn);
#endif //TWIST

    // Scale v to normalize to the segment width
    v /= V_SCALE*lineWidth;

    //- Reject if v<-1 or v>1:
    if(abs(v) <= 1.0)
    {
    //- Compute longitudinal coordinate u using simplified formula:

        // Compute the normal vector of the plane containing center and
        // position and being orthogonal to the wedge:
        vec3 b = cross(segment.normal, position + center);

        // Compute the plane's intersection ration with the line's skeleton:
        vec3 startToEnd = segment.end.xyz - segment.start.xyz;
        float u = (dot(position, b) - dot(segment.start.xyz, b)) /
                  dot(startToEnd, b);

        //- Reject if u<0 or u>1:
        if(u>=0.0 && u<=1.0)
        {
#if NORMAL
            //compute the u coordinate as normal projection
            vec3  startToPos = position  - segment.start.xyz;
            float sqrLen     = dot(startToEnd, startToEnd);
            u                = dot(startToPos, startToEnd);
            u               /= sqrLen;
#endif //NORMAL

            //compute the u coordinate wrt the length of the segment
            u  = mix(segment.start.w, segment.end.w, u);
            u *= U_SCALE*lineCoordScale;
            u  = fract(u / segment.symbol.b);

            //fetch the color contribution from the texture atlas
            vec2 symbolCoord = vec2(u, v);
            symbolCoord     *= segment.symbol.ba;
            symbolCoord     += segment.symbol.rg;
            vec4 symbolColor = texture2D(symbolTex, symbolCoord);

            //accumulate the contribution
            color = mix(color, symbolColor, symbolColor.w);
        }
    }
}

void main()
{
    //setup the default color to whatever the vertex program computed
    gl_FragColor = gl_Color;

    //does this node contain any lines?
    if (lineNumSegments == 0)
        return;

    vec2 coverage = sampleCoverage(texCoord);
#if COVERAGE
    if (coverage.g < 0.5)
    {
        if (coverage.g > 0.0)
            gl_FragColor.rgb = vec3(0.2, 0.8, 0.4);
        else
            gl_FragColor.rgb = vec3(0.0);
    }
    else
        gl_FragColor.rgb = vec3(0.8, 0.4, 0.2);
    gl_FragColor.a   = 1.0;
    return;
#endif

    //do lines overlap this fragment?
    if (coverage == vec2(0.0))
        return;

    vec4 color = vec4(0.0);

    //optimize for single coverage
    if (coverage.g < 0.5)
    {
        vec2 coordShift = vec2(255.0, 255.0*256.0);

        vec2 off         = coverage * coordShift;
        off.x           -= 64.0*256.0;
        float segmentOff = off.x + off.y;

        //read in the segment data
        segmentOff      = lineStartCoord + segmentOff*lineCoordStep;
        Segment segment = readSegment(segmentOff);

        //compute segment and done
#if COLORIZE_COVERAGE
        color = vec4(0.2, 0.8, 0.4, 0.2);
#endif //COLORIZE_COVERAGE
        computeSegment(segment, color);
    }
    else
    {
#if COLORIZE_COVERAGE
        color = vec4(0.8, 0.4, 0.2, 0.2);
#endif //COLORIZE_COVERAGE
        //walk all the line segments for the node of the fragment

        float coord = lineStartCoord;
        for (int i=0; i<lineNumSegments; ++i)
        {
            Segment segment = readSegment(coord);
            computeSegment(segment, color);
        }
    }

    gl_FragColor = mix(gl_FragColor, color, color.w);
}
