// engine_collision.cpp

struct collision
{
    entity *A;
    entity *B;
    
    real32 Penetration;
    vec2 Normal;
    vec2 Contacts[2];
    uint32 ContactCount;
    real32 Restitution;     // Mixed restitution
    real32 DynamicFriction; // Mixed dynamic friction
    real32 StaticFriction;  // Mixed static friction
};

struct polygon
{
    // Assume that this is rectangle for now!
    vec2 P;
    vec2 Normals[4];
    vec2 Vertices[4];
    uint32 VertexCount;
    mat2 u; // Orientation matrix from model to world
};

internal void
PositionalCorrection(collision Contact)
{
    Assert(Contact.A->Mass)
    Assert(Contact.B->Mass)

    real32 k_slop = 0.001f; // Penetration allowance
    real32 percent = 0.6f; // Penetration percentage to correct
    vec2 Correction = (Max(Contact.Penetration - k_slop, 0.0f) / (1.0f/Contact.A->Mass + 1.0f/Contact.B->Mass)) * Contact.Normal * percent;

    //entity *A = &Contact.A;
    //entity *B = &Contact.B;

    if(HasComponent(Contact.A, MOVEMENT_MASK) &&
       !Contact.A->Inactive)
        Contact.A->P -= Correction * (1.0f/Contact.A->Mass);
    if(HasComponent(Contact.B, MOVEMENT_MASK) && 
       !Contact.B->Inactive)
        Contact.B->P += Correction * (1.0f/Contact.B->Mass);
}

internal polygon
SetPolygon(vec2 Position, vec2 *Vertices, uint32 Count)
{
    TIMED_BLOCK();

    polygon Result = {};
/*
    Result.VertexCount = Count;

    for(uint32 i = 0; i < Result.VertexCount; ++i)
        Result.Vertices[i] = Vertices[i];

    Result.Normals[0] = v2(0.0f, -1.0f );
    Result.Normals[1] = v2(1.0f, 0.0f );
    Result.Normals[2] = v2(0.0f, 1.0f );
    Result.Normals[3] = v2(-1.0f, 0.0f );
*/
    Result.P = Position;
    
    // No hulls with less than 3 vertices (ensure actual polygon)
    const uint32 MaxPolyVertexCount = 4;
    Assert(Count > 2 && Count <= MaxPolyVertexCount);
    Count = Min((int32)Count, MaxPolyVertexCount);
    Result.VertexCount = Count;

    // Find the right most point on the hull
    int32 rightMost = 0;
    real32 highestXCoord = Vertices[0].x;
    for(uint32 i = 1; i < Count; ++i)
    {
        real32 x = Vertices[i].x;
        if(x > highestXCoord)
        {
            highestXCoord = x;
            rightMost = i;
        }

        // If matching x then take farthest negative y
        else if(x == highestXCoord)
            if(Vertices[i].y < Vertices[rightMost].y)
                rightMost = i;
    }

    int32 hull[MaxPolyVertexCount];
    int32 outCount = 0;
    int32 indexHull = rightMost;

    for(;;)
    {
        hull[outCount] = indexHull;

        // Search for next index that wraps around the hull
        // by computing cross products to find the most counter-clockwise
        // vertex in the set, given the previos hull index
        int32 nextHullIndex = 0;
        for(int32 i = 1; i < (int32)Count; ++i)
        {
            // Skip if same coordinate as we need three unique
            // points in the set to perform a cross product
            if(nextHullIndex == indexHull)
            {
                nextHullIndex = i;
                continue;
            }

            // Cross every set of three unique vertices
            // Record each counter clockwise third vertex and add
            // to the output hull
            // See : http://www.oocities.org/pcgpe/math2d.html
            vec2 e1 = Vertices[nextHullIndex] - Vertices[hull[outCount]];
            vec2 e2 = Vertices[i] - Vertices[hull[outCount]];
            real32 c = Cross(e1, e2);
            if(c < 0.0f) nextHullIndex = i;

            // Cross product is zero then e vectors are on same line
            // therefor want to record vertex farthest along that line
            if(c == 0.0f && LengthSq(e2) > LengthSq(e1))
                nextHullIndex = i;
        }
        
        ++outCount;
        indexHull = nextHullIndex;

        // Conclude algorithm upon wrap-around
        if(nextHullIndex == rightMost)
        {
            Result.VertexCount = outCount;
            break;
        }
    }

    // Copy vertices into shape's vertices
    for(uint32 i = 0; i < Result.VertexCount; ++i)
        Result.Vertices[i] = Vertices[hull[i]];

    // Compute face normals
    for(uint32 i1 = 0; i1 < Result.VertexCount; ++i1)
    {
        uint32 i2 = i1 + 1 < Result.VertexCount ? i1 + 1 : 0;
        vec2 face = Result.Vertices[i2] - Result.Vertices[i1];

        // Ensure no zero-length edges, because that's bad
        Assert(LengthSq(face) > EPSILON * EPSILON);

        // Calculate normal with 2D cross product between vector and scalar
        Result.Normals[i1] = v2(face.y, -face.x);
        Result.Normals[i1].Normalize();
    }

    return Result;
}

// The extreme point along a direction within a polygon
internal vec2
GetPolygonSupport(polygon *P, vec2 Dir)
{
    vec2 BestVertex;
    r32 BestProjection = -FLT_MAX;

    for(uint32 i = 0; i < P->VertexCount; ++i)
    {
        vec2 v = P->Vertices[i];
        r32 Projection = Dot(v, Dir);

        if(Projection > BestProjection)
        {
            BestVertex = v;
            BestProjection = Projection;
        }
    }

    return BestVertex;
}

internal float
FindAxisLeastPenetration(uint32 *FaceIndex, polygon *A, polygon *B)
{
    real32 BestDistance = -FLT_MAX;
    uint32 BestIndex;

    for(uint32 i = 0; i < A->VertexCount; ++i)
    {
        // Retrieve a face normal from A
        vec2 n = A->Normals[i];
        vec2 nw = A->u * n;

        // Transform face normal into B's model space
        mat2 buT = B->u.Transpose();
        n = buT * nw;

        // Retrieve support point from B along -n
        vec2 s = GetPolygonSupport(B, -n);

        // Retrieve vertex on face from A, transform into
        // B's model space
        vec2 v = A->Vertices[i];
        v = A->u * v + A->P;
        v -= B->P;
        v = buT * v;

        // Compute penetration distance (in B's model space)
        float d = Dot(n, s - v);

        // Store greatest distance
        if(d > BestDistance)
        {
            BestDistance = d;
            BestIndex = i;
        }
    }

    *FaceIndex = BestIndex;
    return BestDistance;
}

internal void
FindIncidentFace(vec2 *V, polygon *RefPoly, polygon *IncPoly, uint32 ReferenceIndex)
{
    vec2 ReferenceNormal = RefPoly->Normals[ReferenceIndex];

    // Calculate normal in incident's frame of reference
    ReferenceNormal = RefPoly->u * ReferenceNormal; // To world space
    ReferenceNormal = IncPoly->u.Transpose() * ReferenceNormal; // To incident's model space

    // Find most anti-normal face on incident polygon
    int32 IncidentFace = 0;
    real32 MinDot = FLT_MAX;
    for(uint32 i = 0; i < IncPoly->VertexCount; ++i)
    {
        real32 dot = Dot(ReferenceNormal, IncPoly->Normals[i]);
        if(dot < MinDot)
        {
            MinDot = dot;
            IncidentFace = i;
        }
    }

    // Assign face vertices for incidentFace
    V[0] = IncPoly->u * IncPoly->Vertices[IncidentFace] + IncPoly->P;
    IncidentFace = IncidentFace + 1 >= (int32)IncPoly->VertexCount ? 0 : IncidentFace + 1;
    V[1] = IncPoly->u * IncPoly->Vertices[IncidentFace] + IncPoly->P;
}

inline bool
BiasGreaterThan(real32 a, real32 b)
{
    real32 k_biasRelative = 0.95f;
    real32 k_biasAbsolute = 0.01f;
    return a >= b * k_biasRelative + a * k_biasAbsolute;
}

internal int32
Clip(vec2 n, real32 c, vec2 *Face)
{
    uint32 sp = 0;
    vec2 out[2] = {
        Face[0],
        Face[1]
    };

    // Retrieve distances from each endpoint to the line
    // d = ax + by - c
    real32 d1 = Dot(n, Face[0]) - c;
    real32 d2 = Dot(n, Face[1]) - c;

    // If negative (behind plane) clip
    if(d1 <= 0.0f) out[sp++] = Face[0];
    if(d2 <= 0.0f) out[sp++] = Face[1];
    
    // If the points are on different sides of the plane
    if(d1*d2 < 0.0f) // less than to ignore -0.0f
    {
        // Push interesection point
        real32 alpha = d1 / (d1 - d2);
        out[sp] = Face[0] + alpha * (Face[1] - Face[0]);
        ++sp;
    }

    // Assign our new converted values
    Face[0] = out[0];
    Face[1] = out[1];

    Assert(sp != 3);

    return sp;
}

internal void
PolygonToPolygon(collision *Collision, polygon *A, polygon *B)
{
    TIMED_BLOCK();

    // Check for a separating axis with A's face planes
    uint32 FaceA;
    float PenetrationA = FindAxisLeastPenetration(&FaceA, A, B);
    if(PenetrationA >= 0)
        return;

    // Check for a separating axis with B's face planes
    uint32 FaceB;
    real32 PenetrationB = FindAxisLeastPenetration(&FaceB, B, A);
    if(PenetrationB >= 0)
        return;

    uint32 ReferenceIndex;
    bool Flip; // Always point from A to B

    polygon *RefPoly; // Reference
    polygon *IncPoly; // Incident

    // Determine which shape contains reference face
    if(BiasGreaterThan(PenetrationA, PenetrationB))
    {
    	RefPoly = A;
    	IncPoly = B;
    	ReferenceIndex = FaceA;
    	Flip = false;
    }
    else
    {
        RefPoly = B;
        IncPoly = A;
        ReferenceIndex = FaceB;
        Flip = true;
    }

    // World space incident face
    vec2 IncidentFace[2];
    FindIncidentFace(IncidentFace, RefPoly, IncPoly, ReferenceIndex);

    //        y
    //        ^  ->n       ^
    //      +---c ------posPlane--
    //  x < | i |\
    //      +---+ c-----negPlane--
    //             \       v
    //              r
    //
    //  r : reference face
    //  i : incident poly
    //  c : clipped point
    //  n : incident normal

    // Setup reference face vertices
    vec2 v1 = RefPoly->Vertices[ReferenceIndex];
    ReferenceIndex = ReferenceIndex + 1 == RefPoly->VertexCount ? 0 : ReferenceIndex + 1;
    vec2 v2 = RefPoly->Vertices[ReferenceIndex];

    // Transform vertices to world space
    v1 = RefPoly->u * v1 + RefPoly->P;
    v2 = RefPoly->u * v2 + RefPoly->P;

    // Calculate reference face side normal in world space
    vec2 SidePlaneNormal = (v2 - v1);
    SidePlaneNormal.Normalize();

    // Orthogonalize
    vec2 RefFaceNormal = {SidePlaneNormal.y, -SidePlaneNormal.x};

    // ax + by = c
    // c is distance from origin
    real32 RefC = Dot(RefFaceNormal, v1);
    real32 NegSide = -Dot(SidePlaneNormal, v1);
    real32 PosSide =  Dot(SidePlaneNormal, v2);

    // Clip incident face to reference face side planes
    if(Clip(-SidePlaneNormal, NegSide, IncidentFace) < 2)
        return; // Due to floating point error, possible to not have required points

    if(Clip(SidePlaneNormal, PosSide, IncidentFace) < 2)
        return; // Due to floating point error, possible to not have required points

    // Flip
    Collision->Normal = Flip ? -RefFaceNormal : RefFaceNormal;

    // Keep points behind reference face
    uint32 cp = 0; // clipped points behind reference face
    real32 Separation = Dot(RefFaceNormal, IncidentFace[0]) - RefC;
    if(Separation <= 0.0f)
    {
        Collision->Contacts[cp] = IncidentFace[0];
        Collision->Penetration = -Separation;
        ++cp;
    }

    else Collision->Penetration = 0;

    Separation = Dot(RefFaceNormal, IncidentFace[1]) - RefC;
    if(Separation <= 0.0f)
    {
        Collision->Contacts[cp] = IncidentFace[1];

        Collision->Penetration += -Separation;
        ++cp;

        // Average penetration
        Collision->Penetration /= (real32)cp;
    }

    Collision->ContactCount = cp;
}