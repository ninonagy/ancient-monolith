#pragma once

#define Pi 3.14159265358f
#define Tau ((float)(2.0f * Pi))
#define Square(x) (x*x)
#define Min(a, b) ((a) > (b) ? (b) : (a))
#define Max(a, b) ((a) < (b) ? (b) : (a))
#define Mod(a, m) (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))
#define EPSILON 0.0001f
//#define Perp(A) (v2(-A.y, A.x))

inline float
Cos(float x)
{
    return cosf(x);
}

inline float
Sin(float x)
{
    return sinf(x);
}

inline float
Clamp(float Min, float Max, float x)
{    
    if (x < Min) x = Min;
    else if (x > Max) x = Max;
    
    return x;
}

inline 
float Smoothstep(float t) 
{ 
    return (3 - 2*t) * Square(t);
}

inline
float Smootherstep(float t)
{
    return t*t*t*(t*(t*6 - 15) + 10);
}

inline float
Lerp(float A, float B, float t)
{
    return (A*(1-t) + B*t);
}

internal float
Map(float v, float min1, float max1, float min2, float max2)
{
    return (min2 + (max2 - min2) * ((v - min1) / (max1 - min1)));
}

inline float
Abs(float Value)
{
    return (Value < 0) ? -Value : Value;
}

// Comparison with tolerance of EPSILON
inline bool
Equal(r32 a, r32 b)
{
    // <= instead of < for NaN comparison safety
    return Abs(a - b) <= EPSILON;
}

inline float
Exp(float x, int e)
{
    float Result = 1;
    
    if(e < 0)
    {
        for(int i = 1; i <= -e; ++i) Result *= x;
        Result = 1.0f/Result;
    }
    else
    {
        for(int i = 1; i <= e; ++i) Result *= x;
    }
    
    return Result;
}


union vec2
{
    struct
    {
        float x, y;
    };
    float Elements[2];

    void Add(vec2);
    void Sub(vec2);
    void Scale(float);
    void Div(float);
    void Rotate(vec2, float);
    void Normalize();
};

inline vec2
v2(float x, float y)
{
    vec2 Result;
    
    Result.x = x;
    Result.y = y;
    
    return Result;
}

void vec2::Add(vec2 v)
{
    x += v.x;
    y += v.y;
}

void vec2::Sub(vec2 v)
{
    x -= v.x;
    y -= v.y;
}

void vec2::Scale(float a)
{
    x *= a;
    y *= a;
}

void vec2::Div(float a)
{
    x /= a;
    y /= a;
}

void vec2::Rotate(vec2 v, float Angle)
{
    vec2 T = v2((x - v.x)*Cos(Angle) - (y - v.y)*Sin(Angle),
                (y - v.y)*Cos(Angle) + (x - v.x)*Sin(Angle));
    
    x = T.x + v.x;
    y = T.y + v.y;
}

inline vec2
operator*(float A, vec2 B)
{
    vec2 Result;
    
    Result.x = A * B.x;
    Result.y = A * B.y;
    
    return Result;
}

inline vec2
operator*(vec2 B, float A)
{
    vec2 Result = A * B;
    
    return Result;
}

inline vec2 &
operator*=(vec2 &B, float A)
{
    B = A * B;
    
    return B;
}

inline vec2
operator/(vec2 B, float A)
{
    vec2 Result;
    
    Result.x = B.x / A;
    Result.y = B.y / A;
    
    return Result;
}

inline vec2 &
operator/=(vec2 &B, float A)
{
    B = B / A;
    
    return B;
}

inline vec2
operator-(vec2 A)
{
    vec2 Result;
    
    Result.x = -A.x;
    Result.y = -A.y;
    
    return Result;
}

inline vec2
operator-(vec2 A, vec2 B)
{
    vec2 Result;
    
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    
    return Result;
}

inline vec2
operator-(vec2 B, float A)
{
    vec2 Result;
    
    Result.x = B.x - A;
    Result.y = B.y - A;
    
    return Result;
}

inline vec2 &
operator-=(vec2 &B, float A)
{
    B = B - A;
       
    return B;
}

inline vec2 &
operator-=(vec2 &B, vec2 A)
{
    B = B - A;
       
    return B;
}

inline vec2
operator+(vec2 A, vec2 B)
{
    vec2 Result;
    
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    
    return Result;
}

inline vec2
operator+(vec2 B, float A)
{
    vec2 Result;
    
    Result.x = B.x + A;
    Result.y = B.y + A;
    
    return Result;
}

inline vec2 &
operator+=(vec2 &B, float A)
{
    B = B + A;
    
    return B;
}

inline vec2 &
operator+=(vec2 &A, vec2 B)
{
    A = A + B;
    
    return A;
}

inline vec2
Perp(vec2 A)
{
    return v2(-A.y, A.x);
}

inline float
Dot(vec2 A, vec2 B)
{
    return (A.x*B.x + A.y*B.y);
}

inline float
Cross(vec2 A, vec2 B)
{
    return (A.x*B.y - A.y*B.x);
}

inline vec2
Cross(vec2 A, float B)
{
    return v2(B*A.y, -B*A.x);
}

inline vec2
Cross(float B, vec2 A)
{
    return v2(-B*A.y, B*A.x);
}

inline float
Length(vec2 A)
{
    return sqrtf(Dot(A, A));
}

inline float
LengthSq(vec2 A)
{
    return Dot(A, A);
}

inline vec2
Limit(vec2 v, float Value)
{
    vec2 Result = v;
    float LengthSquared = LengthSq(v);

    if((LengthSquared > Value*Value) && (LengthSquared > 0))
    {
        float Ratio = Value/sqrtf(LengthSquared);
        Result.x *= Ratio;
        Result.y *= Ratio;
    }

    return Result;
}

void vec2::Normalize()
{
    float l = Length(v2(x, y));
    if(l > EPSILON)
    {
        float InvLength = 1.0f / l;
        x *= InvLength;
        y *= InvLength;
    }
}

inline vec2
Normalize(vec2 A)
{
    vec2 Result;

    float l = Length(A);
    if(l > EPSILON)
    {
        float InvLength = 1.0f / l;
        Result = v2(A.x, A.y)*InvLength;
    }
    
    return Result;
};

inline bool
NotZeroVec2(vec2 v)
{
    bool Result = (v.x != 0.0f || v.y != 0.0f);
    return Result;
}

inline vec2
AngleToHeading(float A) // in radians
{
    // Need normalize?
    return v2(Cos(A), Sin(A));
}

inline float
HeadingToAngle(vec2 v)
{
    return atan2f(v.y, v.x);
}

inline float
ToDegrees(float Radians)
{
    return (Radians * (180.0f/Pi));
}

inline float
ToRadians(float Degrees)
{
    return (Degrees * (Pi/180.0f));
}

union vec3
{
    struct
    {
        float x, y, z;
    };
    struct
    {
        vec2 xy;
        float Ignored0;
    };
    struct
    {
        float Ignored1;
        vec2 yz;
    };
    float E[3];
};
union uvec3
{
    struct
    {
        uint32 x, y, z;
    };
    float E[3];
};

inline vec3
operator*(vec3 B, float A)
{
    vec3 Result;
    
    Result.x = B.x * A;
    Result.y = B.y * A;
    Result.z = B.z * A;
    
    return Result;
}

inline vec3
operator-(vec3 A)
{
    vec3 Result;
    
    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    
    return Result;
}

inline vec3
operator-(vec3 A, vec3 B)
{
    vec3 Result;
    
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    
    return Result;
}

inline vec3
operator-(vec3 B, float A)
{
    vec3 Result;
    
    Result.x = B.x - A;
    Result.y = B.y - A;
    Result.z = B.z - A;
    
    return Result;
}

inline vec3 &
operator-=(vec3 &B, float A)
{
    B = B - A;
       
    return B;
}

inline vec3 &
operator-=(vec3 &B, vec3 A)
{
    B = B - A;
       
    return B;
}

inline vec3
operator+(vec3 A, vec3 B)
{
    vec3 Result;
    
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    
    return Result;
}

inline vec3
operator+(vec3 B, float A)
{
    vec3 Result;
    
    Result.x = B.x + A;
    Result.y = B.y + A;
    Result.z = B.z + A;
    
    return Result;
}

inline vec3 &
operator+=(vec3 &B, float A)
{
    B = B + A;
       
    return B;
}

inline vec3 &
operator+=(vec3 &B, vec3 A)
{
    B = B + A;
       
    return B;
}

inline vec3
v3(float x, float y, float z)
{
    vec3 Result;
    
    Result.x = x;
    Result.y = y;
    Result.z = z;
    
    return Result;
}

inline vec3
v3(vec2 xy, float z)
{
    vec3 Result;
    
    Result.x = xy.x;
    Result.y = xy.y;
    Result.z = z;
    
    return Result;
}
inline uvec3
v3(uint32 x, uint32 y, uint32 z)
{
    uvec3 Result;
    
    Result.x = x;
    Result.y = y;
    Result.z = z;
    
    return Result;
}



union vec4
{
    struct
    {
        float x, y, z, w;
    };
    struct
    {
        float r, g, b, a;
    };
    struct
    {
        vec3 xyz;
        float Ignored0;
    };
    float Elements[4];
};

inline vec4
v4(float x, float y,
   float z, float w)
{
    vec4 Result;
    
    Result.x = x;
    Result.y = y;
    Result.z = z;
    Result.w = w;
    
    return Result;
}

inline vec4
v4(vec3 xyz, float w)
{
    vec4 Result;
    
    Result.x = xyz.x;
    Result.y = xyz.y;
    Result.z = xyz.z;
    Result.w = w;
    
    return Result;
}

inline vec4
operator*(vec4 B, float A)
{
    vec4 Result;
    
    Result.x = A * B.x;
    Result.y = A * B.y;
    Result.z = A * B.z;
    Result.w = A * B.w;
    
    return Result;
}

struct mat2
{
    union
    {
        struct
        {
            real32 m00, m01;
            real32 m10, m11;
        };

        real32 m[2][2];
        real32 v[4];
    };

    mat2() {}

    mat2(real32 Radians)
    {
        real32 c = Cos(Radians);
        real32 s = Sin(Radians);

        m00 = c; m01 = -s;
        m10 = s; m11 =  c;
    }

    mat2(real32 a, real32 b, real32 c, real32 d)
        : m00(a), m01(b)
        , m10(c), m11(d)
    {}

    void SetOrient(real32 Radians)
    {
        real32 c = Cos(Radians);
        real32 s = Sin(Radians);

        m00 = c; m01 = -s;
        m10 = s; m11 =  c;
    }

    vec2 AxisX(void)
    {
        return v2(m00, m10);
    }

    vec2 AxisY(void)
    {
        return v2(m01, m11);
    }

    mat2 Transpose(void)
    {
        return mat2(m00, m10, m01, m11);
    }

    vec2 operator*(vec2& rhs)
    {
        return v2(m00 * rhs.x + m01 * rhs.y, m10 * rhs.x + m11 * rhs.y);
    }

    mat2 operator*(mat2& rhs)
    {
        // [00 01]  [00 01]
        // [10 11]  [10 11]

        return mat2(
            m[0][0] * rhs.m[0][0] + m[0][1] * rhs.m[1][0],
            m[0][0] * rhs.m[0][1] + m[0][1] * rhs.m[1][1],
            m[1][0] * rhs.m[0][0] + m[1][1] * rhs.m[1][0],
            m[1][0] * rhs.m[0][1] + m[1][1] * rhs.m[1][1]
        );
    }
};


union mat4
{
    struct
    {
        float m00, m01, m02, m03;
        float m10, m11, m12, m13;
        float m20, m21, m22, m23;
        float m30, m31, m32, m33;
    };
    float Elements[4][4];
};

inline mat4
m4(float E[])
{
    mat4 Result = {0};
    return Result;
}

inline vec4
MultiplyMat4ByVec4(mat4 Matrix, vec4 Vector)
{
    vec4 Result = {0};
    
    u32 Columns, Rows;
    for(Rows = 0; Rows < 4; ++Rows)
    {
        float Sum = 0;
        for(Columns = 0; Columns < 4; ++Columns)
        {
            Sum += Matrix.Elements[Columns][Rows] * 
                   Vector.Elements[Columns];
        }
        
        Result.Elements[Rows] = Sum;
    }
    
    return (Result);
}

inline mat4
operator*(mat4 A, mat4 B)
{
    mat4 Result;
    
    Result.m00 = A.m00 * B.m00 + A.m01 * B.m10 + A.m02 * B.m20 + A.m03 * B.m30;
    Result.m01 = A.m00 * B.m01 + A.m01 * B.m11 + A.m02 * B.m21 + A.m03 * B.m31;
    Result.m02 = A.m00 * B.m02 + A.m01 * B.m12 + A.m02 * B.m22 + A.m03 * B.m32;
    Result.m03 = A.m00 * B.m03 + A.m01 * B.m13 + A.m02 * B.m23 + A.m03 * B.m33;
    Result.m10 = A.m10 * B.m00 + A.m11 * B.m10 + A.m12 * B.m20 + A.m13 * B.m30;
    Result.m11 = A.m10 * B.m01 + A.m11 * B.m11 + A.m12 * B.m21 + A.m13 * B.m31;
    Result.m12 = A.m10 * B.m02 + A.m11 * B.m12 + A.m12 * B.m22 + A.m13 * B.m32;
    Result.m13 = A.m10 * B.m03 + A.m11 * B.m13 + A.m12 * B.m23 + A.m13 * B.m33;
    Result.m20 = A.m20 * B.m00 + A.m21 * B.m10 + A.m22 * B.m20 + A.m23 * B.m30;
    Result.m21 = A.m20 * B.m01 + A.m21 * B.m11 + A.m22 * B.m21 + A.m23 * B.m31;
    Result.m22 = A.m20 * B.m02 + A.m21 * B.m12 + A.m22 * B.m22 + A.m23 * B.m32;
    Result.m23 = A.m20 * B.m03 + A.m21 * B.m13 + A.m22 * B.m23 + A.m23 * B.m33;
    Result.m30 = A.m30 * B.m00 + A.m31 * B.m10 + A.m32 * B.m20 + A.m33 * B.m30;
    Result.m31 = A.m30 * B.m01 + A.m31 * B.m11 + A.m32 * B.m21 + A.m33 * B.m31;
    Result.m32 = A.m30 * B.m02 + A.m31 * B.m12 + A.m32 * B.m22 + A.m33 * B.m32;
    Result.m33 = A.m30 * B.m03 + A.m31 * B.m13 + A.m32 * B.m23 + A.m33 * B.m33;
    
    return Result;
}

inline vec2
operator*(mat4 A, vec2 v)
{
    vec2 Result;
    
    float vz = 0.0f;
    float vw = 1.0f;
    
    Result.x = A.m00 * v.x + A.m01 * v.y + A.m02 * vz + A.m03 * vw;
    Result.y = A.m10 * v.x + A.m11 * v.y + A.m12 * vz + A.m13 * vw;
//    Result.z = A.m20 * v.x + A.m21 * v.y + A.m22 * vz + A.m23 * vw;
//    Result.w = A.m30 * v.x + A.m31 * v.y + A.m32 * vz + A.m33 * vw;
    
    return Result;
}

inline vec2 &
operator*=(vec2 &v, mat4 A)
{
    v = A * v;

    return v;
}

// https://github.com/niswegmann/small-matrix-inverse
inline void
Inverse(real32 *src, real32 *dst)
{
    real32 det;

    /* Compute adjoint: */
    dst[0] =
        + src[ 5] * src[10] * src[15]
        - src[ 5] * src[11] * src[14]
        - src[ 9] * src[ 6] * src[15]
        + src[ 9] * src[ 7] * src[14]
        + src[13] * src[ 6] * src[11]
        - src[13] * src[ 7] * src[10];

    dst[1] =
        - src[ 1] * src[10] * src[15]
        + src[ 1] * src[11] * src[14]
        + src[ 9] * src[ 2] * src[15]
        - src[ 9] * src[ 3] * src[14]
        - src[13] * src[ 2] * src[11]
        + src[13] * src[ 3] * src[10];

    dst[2] =
        + src[ 1] * src[ 6] * src[15]
        - src[ 1] * src[ 7] * src[14]
        - src[ 5] * src[ 2] * src[15]
        + src[ 5] * src[ 3] * src[14]
        + src[13] * src[ 2] * src[ 7]
        - src[13] * src[ 3] * src[ 6];

    dst[3] =
        - src[ 1] * src[ 6] * src[11]
        + src[ 1] * src[ 7] * src[10]
        + src[ 5] * src[ 2] * src[11]
        - src[ 5] * src[ 3] * src[10]
        - src[ 9] * src[ 2] * src[ 7]
        + src[ 9] * src[ 3] * src[ 6];

    dst[4] =
        - src[ 4] * src[10] * src[15]
        + src[ 4] * src[11] * src[14]
        + src[ 8] * src[ 6] * src[15]
        - src[ 8] * src[ 7] * src[14]
        - src[12] * src[ 6] * src[11]
        + src[12] * src[ 7] * src[10];

    dst[5] =
        + src[ 0] * src[10] * src[15]
        - src[ 0] * src[11] * src[14]
        - src[ 8] * src[ 2] * src[15]
        + src[ 8] * src[ 3] * src[14]
        + src[12] * src[ 2] * src[11]
        - src[12] * src[ 3] * src[10];

    dst[6] =
        - src[ 0] * src[ 6] * src[15]
        + src[ 0] * src[ 7] * src[14]
        + src[ 4] * src[ 2] * src[15]
        - src[ 4] * src[ 3] * src[14]
        - src[12] * src[ 2] * src[ 7]
        + src[12] * src[ 3] * src[ 6];

    dst[7] =
        + src[ 0] * src[ 6] * src[11]
        - src[ 0] * src[ 7] * src[10]
        - src[ 4] * src[ 2] * src[11]
        + src[ 4] * src[ 3] * src[10]
        + src[ 8] * src[ 2] * src[ 7]
        - src[ 8] * src[ 3] * src[ 6];

    dst[8] =
        + src[ 4] * src[ 9] * src[15]
        - src[ 4] * src[11] * src[13]
        - src[ 8] * src[ 5] * src[15]
        + src[ 8] * src[ 7] * src[13]
        + src[12] * src[ 5] * src[11]
        - src[12] * src[ 7] * src[ 9];

    dst[9] =
        - src[ 0] * src[ 9] * src[15]
        + src[ 0] * src[11] * src[13]
        + src[ 8] * src[ 1] * src[15]
        - src[ 8] * src[ 3] * src[13]
        - src[12] * src[ 1] * src[11]
        + src[12] * src[ 3] * src[ 9];

    dst[10] =
        + src[ 0] * src[ 5] * src[15]
        - src[ 0] * src[ 7] * src[13]
        - src[ 4] * src[ 1] * src[15]
        + src[ 4] * src[ 3] * src[13]
        + src[12] * src[ 1] * src[ 7]
        - src[12] * src[ 3] * src[ 5];

    dst[11] =
        - src[ 0] * src[ 5] * src[11]
        + src[ 0] * src[ 7] * src[ 9]
        + src[ 4] * src[ 1] * src[11]
        - src[ 4] * src[ 3] * src[ 9]
        - src[ 8] * src[ 1] * src[ 7]
        + src[ 8] * src[ 3] * src[ 5];

    dst[12] =
        - src[ 4] * src[ 9] * src[14]
        + src[ 4] * src[10] * src[13]
        + src[ 8] * src[ 5] * src[14]
        - src[ 8] * src[ 6] * src[13]
        - src[12] * src[ 5] * src[10]
        + src[12] * src[ 6] * src[ 9];

    dst[13] =
        + src[ 0] * src[ 9] * src[14]
        - src[ 0] * src[10] * src[13]
        - src[ 8] * src[ 1] * src[14]
        + src[ 8] * src[ 2] * src[13]
        + src[12] * src[ 1] * src[10]
        - src[12] * src[ 2] * src[ 9];

    dst[14] =
        - src[ 0] * src[ 5] * src[14]
        + src[ 0] * src[ 6] * src[13]
        + src[ 4] * src[ 1] * src[14]
        - src[ 4] * src[ 2] * src[13]
        - src[12] * src[ 1] * src[ 6]
        + src[12] * src[ 2] * src[ 5];

    dst[15] =
        + src[ 0] * src[ 5] * src[10]
        - src[ 0] * src[ 6] * src[ 9]
        - src[ 4] * src[ 1] * src[10]
        + src[ 4] * src[ 2] * src[ 9]
        + src[ 8] * src[ 1] * src[ 6]
        - src[ 8] * src[ 2] * src[ 5];

    /* Compute determinant: */

    det = + src[0] * dst[0] + src[1] * dst[4] + src[2] * dst[8] + src[3] * dst[12];

    /* Multiply adjoint with reciprocal of determinant: */

    det = 1.0f / det;

    dst[ 0] *= det;
    dst[ 1] *= det;
    dst[ 2] *= det;
    dst[ 3] *= det;
    dst[ 4] *= det;
    dst[ 5] *= det;
    dst[ 6] *= det;
    dst[ 7] *= det;
    dst[ 8] *= det;
    dst[ 9] *= det;
    dst[10] *= det;
    dst[11] *= det;
    dst[12] *= det;
    dst[13] *= det;
    dst[14] *= det;
    dst[15] *= det;
}

inline mat4
Identity()
{
    mat4 Result = {};
    
    Result.m00 = 1.0f;
    Result.m11 = 1.0f;
    Result.m22 = 1.0f;
    Result.m33 = 1.0f;
    
    return Result;
}

inline mat4
Translate(vec2 Direction)
{
    mat4 Result = Identity();
    
    Result.m03 = Direction.x;
    Result.m13 = Direction.y;
    //Result.m23 = Direction.z;
    
    return Result;
}

inline mat4
Translate(vec3 Direction)
{
    mat4 Result = Identity();
    
    Result.m03 = Direction.x;
    Result.m13 = Direction.y;
    Result.m23 = Direction.z;
    
    return Result;
}


inline mat4
Scale(vec2 Size)
{
    mat4 Result = Identity();
    
    Result.m00 = Size.x;
    Result.m11 = Size.y;
    
    return Result;
}

inline mat4
Rotate(float Angle)
{
    mat4 Result = Identity();
    
    Result.m00 = cos(Angle);
    Result.m01 = -sin(Angle);
    Result.m10 = sin(Angle);
    Result.m11 = cos(Angle);
    
    return Result;
}

union rect2
{
    struct
    {
        vec2 Min;
        vec2 Max;
    };
};

inline rect2
RectMinDim(vec2 P, vec2 Dim)
{
    rect2 Result;
    
    Result.Min = P;
    Result.Max = P + Dim;
    
    return Result;
}

inline rect2
RectCenterDim(vec2 P, vec2 Dim)
{
    return RectMinDim(P - 0.5f*Dim, Dim);
}

inline bool
IsInRectangle(rect2 Rect, vec2 P)
{
    return (P.x >= Rect.Min.x &&
            P.x <= Rect.Max.x &&
            P.y >= Rect.Min.y &&
            P.y <= Rect.Max.y);
}

inline bool
DoRectanglesIntersect(rect2 A, rect2 B)
{
    return (A.Min.x <= B.Max.x &&
            A.Max.x >= B.Min.x &&
            A.Min.y <= B.Max.y &&
            A.Max.y >= B.Min.y);
}


struct line_segment
{
    vec2 A;
    vec2 B;
};

inline rect2
GetBoundingBox(line_segment Line)
{
    rect2 Result = {};
    Result.Min = {Min(Line.A.x, Line.B.x), Min(Line.A.y, Line.B.y)};
    Result.Max = {Max(Line.A.x, Line.B.x), Max(Line.A.y, Line.B.y)};
    
    return Result;
}

inline vec2
NormalAlongLine(line_segment Line, vec2 P)
{
    vec2 Result = {};

    line_segment TempLine = {v2(0, 0), Line.B - Line.A};
    vec2 TempP = P - Line.A;
    vec2 Direction = TempLine.B;
    vec2 Orthogonal[] = {v2(Direction.y, -Direction.x),
                         v2(-Direction.y, Direction.x)};
    
    if(Cross(TempLine.B, TempP) < 0.0f)
    {
        Result = Orthogonal[0];
    }
    else
    {
        Result = Orthogonal[1];
    }

    return Normalize(Result);
}

inline bool
IsPointOnLine(line_segment Line, vec2 B)
{
    bool Result = false;

    line_segment TempLine = {v2(0, 0), Line.B - Line.A};
    vec2 TempB = B - Line.A;
    
    float Epsilon = 0.0001f;
    float r = Cross(TempLine.B, TempB);
    Result = Abs(r) < Epsilon;

    return Result;
}

inline bool
IsPointRightOfLine(line_segment Line, vec2 B)
{
    bool Result = false;
    
    line_segment TempLine = {v2(0, 0), Line.B - Line.A};
    vec2 TempB = B - Line.A;
    
    Result = Cross(TempLine.B, TempB) > 0.0f;

    return Result;
}

inline bool
LineTouchesOrCrossesLine(line_segment LineA, line_segment LineB)
{
    return (IsPointOnLine(LineA, LineB.A) ||
            IsPointOnLine(LineA, LineB.B) ||
           (IsPointRightOfLine(LineA, LineB.A) ^ 
            IsPointRightOfLine(LineA, LineB.B)));
}

inline bool
DoLinesIntersect(line_segment LineA, line_segment LineB)
{
    rect2 RectA = GetBoundingBox(LineA);
    rect2 RectB = GetBoundingBox(LineB);

    return (DoRectanglesIntersect(RectA, RectB) &&
            LineTouchesOrCrossesLine(LineA, LineB) &&
            LineTouchesOrCrossesLine(LineB, LineA));
}

internal vec2
GetIntersection(line_segment LineA, line_segment LineB)
{
    /* the intersection [(x1,y1), (x2, y2)]
       it might be a line or a single point. If it is a line,
       then x1 = x2 and y1 = y2.  */
    float x1, y1, x2, y2;

    if(LineA.A.x == LineA.B.x)
    {
        // Case (A)
        // As a is a perfect vertical line, it cannot be represented
        // nicely in a mathematical way. But we directly know that
        //
        x1 = LineA.A.x;
        x2 = x1;
        if(LineB.A.x == LineB.B.x)
        {
            // Case (AA): all x are the same!
            // Normalize
            if(LineA.A.y > LineA.B.y)
            {
                LineA = {LineA.B, LineA.A};
            }
            if(LineB.A.y > LineB.B.y)
            {
                LineB = {LineB.B, LineB.A};
            }
            if(LineA.A.y > LineB.A.y)
            {
                line_segment Tmp = LineA;
                LineA = LineB;
                LineB = Tmp;
            }

            // Now we know that the y-value of LineA.A is the 
            // lowest of all 4 y values
            // this means, we are either in case (AAA):
            //   a: x--------------x
            //   b:    x---------------x
            // or in case (AAB)
            //   a: x--------------x
            //   b:    x-------x
            // in both cases:
            // get the relavant y intervall
            y1 = LineB.A.y;
            y2 = Min(LineA.B.y, LineB.B.y);
        }
        else
        {
            // Case (AB)
            // we can mathematically represent line b as
            //     y = m*x + t <=> t = y - m*x
            // m = (y1-y2)/(x1-x2)
            float m, t;
            m = (LineB.A.y - LineB.B.y) / (LineB.A.x - LineB.B.x);
            t = LineB.A.y - m*LineB.A.x;
            y1 = m*x1 + t;
            y2 = y1;
        }
    }
    else if(LineB.A.x == LineB.B.x)
    {
        // Case (B)
        // essentially the same as Case (AB), but with
        // a and b switched
        x1 = LineB.A.x;
        x2 = x1;

        line_segment Tmp = LineA;
        LineA = LineB;
        LineB = Tmp;

        float m, t;
        m = (LineB.A.y - LineB.B.y) / (LineB.A.x - LineB.B.x);
        t = LineB.A.y - m*LineB.A.x;
        y1 = m*x1 + t;
        y2 = y1;
    }
    else
    {
        // Case (C)
        // Both lines can be represented mathematically
        float ma, mb, ta, tb;
        ma = (LineA.A.y - LineA.B.y) / (LineA.A.x - LineA.B.x);
        mb = (LineB.A.y - LineB.B.y) / (LineB.A.x - LineB.B.x);
        ta = LineA.A.y - ma*LineA.A.x;
        tb = LineB.A.y - mb*LineB.A.x;
        if(ma == mb)
        {
            // Case (CA)
            // both lines are in parallel. As we know that they
            // intersect, the intersection could be a line
            // when we rotated this, it would be the same situation
            // as in case (AA)

            // Normalize
            if(LineA.A.x > LineA.B.x)
            {
                LineA = {LineA.B, LineA.A};
            }
            if(LineB.A.x > LineB.B.x)
            {
                LineB = {LineB.B, LineB.A};
            }
            if(LineA.A.x > LineB.A.x)
            {
                line_segment Tmp = LineA;
                LineA = LineB;
                LineB = Tmp;
            }
            
            // get the relavant x intervall
            x1 = LineB.A.x;
            x2 = Min(LineA.B.x, LineB.B.x);
            y1 = ma*x1+ta;
            y2 = ma*x2+ta;
        }
        else
        {
            // Case (CB): only a point as intersection:
            // y = ma*x+ta
            // y = mb*x+tb
            // ma*x + ta = mb*x + tb
            // (ma-mb)*x = tb - ta
            // x = (tb - ta)/(ma-mb)
            x1 = (tb-ta)/(ma-mb);
            y1 = ma*x1+ta;
            x2 = x1;
            y2 = y1;
        }
    }

    return v2(x2, y2);
}

union rect4
{
    struct
    {
        vec2 BottomLeft;
        vec2 BottomRight;
        vec2 TopRight;
        vec2 TopLeft;
    };
};

inline rect4
RectBottomDim(vec2 P, vec2 Dim)
{
    rect4 Result;
    
    Result.BottomLeft = P;
    Result.TopRight = P + Dim;
    Result.BottomRight = v2(P.x + Dim.x, P.y);
    Result.TopLeft = v2(P.x, P.y + Dim.y);
    
    return Result;
}

inline vec2
GetRectCenter(rect4 Rect)
{
    vec2 Result;

    vec2 Diagonal = Rect.BottomRight - Rect.TopLeft;
    Result = Rect.TopLeft + 0.5f*Diagonal;

    return Result;
}

inline vec2
GetRectDim(rect4 Rect)
{
    vec2 Result;

    Result.x = Rect.BottomRight.x - Rect.BottomLeft.x;
    Result.y = Rect.TopRight.y - Rect.BottomRight.y;

    return Result;
}

inline vec2
GetRectMin(rect4 Rect)
{
    vec2 *V = &Rect.BottomLeft;
    vec2 Min = *V;
    
    for(uint8 i = 0; i < 4; ++i)
    {
        if(V->y < Min.y) Min = *V;
        ++V;
    }

    return Min;
}

inline void
Rotate(vec2 *P, vec2 v, float Angle)
{
    vec2 T = v2((P->x-v.x)*cosf(Angle) - (P->y-v.y)*sinf(Angle),
                (P->y-v.y)*cosf(Angle) + (P->x-v.x)*sinf(Angle));
    *P = T + v;
}

inline vec2
RotateVec2(vec2 v, float Angle)
{
    vec2 Result = v2(v.x*cosf(Angle) - v.y*sinf(Angle),
                     v.y*cosf(Angle) + v.x*sinf(Angle));
    
    return Result;
}

inline void
RectRotate(rect4 *Rect, vec2 Origin, float Angle)
{
    Rotate(&Rect->BottomLeft, Origin, Angle);
    Rotate(&Rect->TopRight, Origin, Angle);
    Rotate(&Rect->BottomRight, Origin, Angle);
    Rotate(&Rect->TopLeft, Origin, Angle);
}