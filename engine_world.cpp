// engine_world.cpp

inline real32
MassFromDim(vec2 Dim)
{
    //gust = m / V
    //m = gust * V
    return (1.0f * Dim.x * Dim.y);
}

// TODO: For different shapes?
inline real32
GetInertia(real32 Mass, vec2 Dim)
{
    return ((Mass * (Square(Dim.x) + Square(Dim.y))) / 12.0f);
}

// TODO: Add Entity and init values later?
internal u32
AddEntity(game_state *GameState, vec2 P, vec2 Dim,
          int32 Mask, u32 ParentIndex = 0)
{
    u32 EntityIndex = AddNextFreeEntity(GameState);
    entity *Entity = GetEntity(EntityIndex);

    Entity->Mask = Mask;
    
    Entity->P = P;
    Entity->Dim = Dim;
    Entity->Mass = 0.5f;
    Entity->Inertia = GetInertia(Entity->Mass, Dim);

    // TODO: Decide how to expand this?
    Entity->FirstCollisionVolume = GetNextFreeCollisionVolume(GameState);
    PolygonCollisionVolume(Entity->FirstCollisionVolume, v2(0, 0), Dim);

    return EntityIndex;
}

#define AddComponent(Components) (Entity)->Mask |= Components;
// AddTransformComponent()
// AddCollisionComponent()

internal u32
AddPlayer(game_state *GameState, vec2 P, vec2 Dim)
{
    u32 EntityIndex = AddNextFreeEntity(GameState);
    GameState->ControllingEntityIndex = EntityIndex;
    GameState->PlayerIndex = EntityIndex;

    entity *Entity = GetEntity(EntityIndex);
    
    // TODO: Calling on function should add component flag. 
    AddComponent(EntityComponent_Player | EntityComponent_Position |
                 EntityComponent_Velocity | EntityComponent_Render |
                 EntityComponent_Collide);
    
    Entity->P = P;
    Entity->Dim = Dim;
    Entity->Mass = 1.0f;
    Entity->Inertia = (Entity->Mass*(Square(Dim.x) + 
                      Square(Dim.y))) / 12.0f;

    Entity->StaticFriction = 0.05f;
    Entity->DynamicFriction = 0.1f;

    Entity->FirstCollisionVolume = GetNextFreeCollisionVolume(GameState);
    PolygonCollisionVolume(Entity->FirstCollisionVolume, v2(0, 0), Dim);

    return EntityIndex;
}

internal u32
AddBall(game_state *GameState, vec2 P, vec2 Dim)
{
    u32 EntityIndex = AddNextFreeEntity(GameState);
    entity *Entity = GetEntity(EntityIndex);

    //Entity->Type = EntityType_Ball;
    // TODO: Rule on adding parent entity to the ball
    AddComponent(EntityComponent_Ball | EntityComponent_Position | 
                 EntityComponent_Velocity | EntityComponent_Render | 
                 EntityComponent_Collide);
    //Entity->Parent = GetEntity(GameState->ControllingEntityIndex);
    
    Entity->P = P;
    Entity->Dim = Dim;
    Entity->Mass = 0.3f;
    Entity->Inertia = (Entity->Mass*(Square(Dim.x) + 
                      Square(Dim.y))) / 12.0f;

    Entity->StaticFriction = 0.05f;
    Entity->DynamicFriction = 0.0f;

    Entity->FirstCollisionVolume = GetNextFreeCollisionVolume(GameState);
    PolygonCollisionVolume(Entity->FirstCollisionVolume, v2(0, 0), Dim);

    return EntityIndex;
}

internal u32
AddEnemy(game_state *GameState, vec2 P, vec2 Dim)
{
    u32 EntityIndex = AddNextFreeEntity(GameState);
    entity *Entity = GetEntity(EntityIndex);

    AddComponent(EntityComponent_Enemy | EntityComponent_Position | 
                 EntityComponent_Velocity | EntityComponent_Render |
                 EntityComponent_Collide);
    
    Entity->P = P;
    Entity->Dim = Dim;
    Entity->Mass = 0.2f;
    Entity->Inertia = (Entity->Mass*(Square(Dim.x) + 
                      Square(Dim.y))) / 12.0f;

    Entity->FirstCollisionVolume = GetNextFreeCollisionVolume(GameState);
    PolygonCollisionVolume(Entity->FirstCollisionVolume, v2(0, 0), Dim);

    return EntityIndex;
}

internal u32
AddBullet(game_state *GameState, vec2 P, vec2 Dim, real32 Angle,
          int32 ParentComponent = 0)
{
    u32 EntityIndex = AddNextFreeEntity(GameState);
    entity *Entity = GetEntity(EntityIndex);

    //Entity->Parent = GetEntity(ParentIndex);
    AddComponent(EntityComponent_Bullet | EntityComponent_Position | 
                 EntityComponent_Velocity | EntityComponent_Render |
                 EntityComponent_Collide);

    // TODO: Make this different?
    // AddParentComponent()
    Entity->ParentComponent = ParentComponent;
    
    Entity->P = P;
    Entity->Dim = Dim;
    Entity->Angle = Angle;
    Entity->Mass = 0.8f;
    Entity->Inertia = (Entity->Mass*(Square(Dim.x) + 
                      Square(Dim.y))) / 12.0f;

    Entity->FirstCollisionVolume = GetNextFreeCollisionVolume(GameState);
    PolygonCollisionVolume(Entity->FirstCollisionVolume, v2(0, 0), Dim);

    return EntityIndex;
}

internal u32
AddBlock(game_state *GameState, vec2 P, vec2 Dim)
{
    u32 EntityIndex = AddNextFreeEntity(GameState);
    entity *Entity = GetEntity(EntityIndex);

    AddComponent(EntityComponent_Block | EntityComponent_Position | 
                 EntityComponent_Render | EntityComponent_Collide);
    
    Entity->P = P;
    Entity->Dim = Dim;
    Entity->Mass = 1.0f;
    Entity->Inertia = (Entity->Mass*(Square(Dim.x) + 
                      Square(Dim.y))) / 12.0f;

    Entity->FirstCollisionVolume = GetNextFreeCollisionVolume(GameState);
    PolygonCollisionVolume(Entity->FirstCollisionVolume, v2(0, 0), Dim);

    return EntityIndex;
}

internal u32
AddLand(game_state *GameState, vec2 P, collision_volume *FirstCollisionVolume = 0)
{
    u32 EntityIndex = AddNextFreeEntity(GameState);
    entity *Entity = GetEntity(EntityIndex);

    AddComponent(EntityComponent_Land | EntityComponent_Position | 
                 EntityComponent_Render | EntityComponent_Collide);
    
    Entity->P = P;
    Entity->Mass = 1.0f;

    Entity->StaticFriction = 0.1f;
    Entity->DynamicFriction = 0.5f;

    if(!FirstCollisionVolume)
    {
        Entity->FirstCollisionVolume = GetNextFreeCollisionVolume(GameState);
        PolygonCollisionVolume(Entity->FirstCollisionVolume, v2(0, 0), v2(1, 1), 0);
    }
    else
    {
        Entity->FirstCollisionVolume = FirstCollisionVolume;
    }

    return EntityIndex;
}

internal void
AddCamera(game_state *GameState, vec2 P)
{
    u32 EntityIndex = AddNextFreeEntity(GameState);
    GameState->CameraIndex = EntityIndex;

    entity *Entity = GetEntity(EntityIndex);

    AddComponent(EntityComponent_Camera | EntityComponent_Position |
                 EntityComponent_Velocity);
    
    Entity->P = P;
    Entity->Dim = v2(1, 1);
    Entity->Mass = 1.0f;
    Entity->Inertia = (Entity->Mass*(Square(Entity->Dim.x) + 
                       Square(Entity->Dim.y))) / 12.0f;
}

internal void
GenerateTerrainVertices(vec2 *PointArray, real32 PosX, u32 PointCount)
{
    // NOTE: Generate height from position and store point location
    float OffsetX = PosX;
    float OffsetY = 6.0f;
    float StepSize = 1.0f/(float)TERRAIN_RESOLUTION;

    float FreqMlt = 1.8f;
    float AmplMlt = 0.5f;
    float BaseFrequency = 0.1f;
    float BaseAmplitude = 1.0f;
    float Frequency = BaseFrequency;
    float Amplitude = BaseAmplitude;

    uint8 LayerCount = 0;
    for(uint32 PointIndex = 0;
        PointIndex < PointCount;
        ++PointIndex)
    {
        // TODO: Random generator can't handle negative numbers yet!
        vec2 *Point = PointArray + PointIndex;

        Point->x = (PointIndex * StepSize) + OffsetX;
        Point->y = OffsetY;

        for(uint32 Layer = 0;
            Layer < LayerCount;
            ++Layer)
        {
            Point->y += SmoothNoise(Abs(Point->x * Frequency), TERRAIN_SIZE) * Amplitude;

            Frequency *= FreqMlt;
            Amplitude *= AmplMlt;
        }

        Frequency = BaseFrequency;
        Amplitude = BaseAmplitude;
    }
    
#if 0
    //PointArray[PointCount] = PointArray[0];

    // TODO: Density function for circle

    // NOTE: Transform points to the circle
    vec2 CircleOrigin = v2(0, 0);
    bool ProjectOnCircle = 0;
    float r = (float)TERRAIN_SIZE/(2*Pi);
    float Angle = 0.0f;
    float AngleMlt = (2*Pi)/(float)PointCount;
    for(uint32 PointIndex = 0;
        PointIndex < PointCount;
        ++PointIndex)
    {
        vec2 *Point = PointArray + PointIndex;
        if(ProjectOnCircle)
        {
            Point->x = (Point->y+r)*(cosf(Angle)) + CircleOrigin.x;
            Point->y = (Point->y+r)*(sinf(Angle)) + CircleOrigin.y;
            Angle += AngleMlt;
        }
    }
#endif
}

inline void
MeshFromPoints(triangle_mesh *Mesh, vec2 *Points, u32 PointCount)
{
    // TODO: Add indices
    vertex *MeshVertex = Mesh->Vertices + Mesh->NumberOfVertices;
    for(u32 i = 0; i < PointCount; ++i)
    {
        vec2 UV = v2(1, 0);
        *(MeshVertex + i) = {Points[i], UV};
    }

    Mesh->NumberOfVertices += PointCount;
}

// gY/O - pY/O <= 0  
    // gY - pY <= 0

    // -gY + pY >= 0
    // -6 + 5.3 -> -0.7   -4 + 3.3 -> -0.7  -0 + 5.3
    // -5 + 5.3 -> 0.3	  -3 + 3.3 -> 0.3   -0 + 5.3
    // -6 + 4.2 -> -1.8   -4 + 4.9 -> 0.9   -0 + 5.3
    // -5 + 4.2 -> -0.8   -3 + 4.9 -> 1.9   -0 + 5.3
/*    

    6   0.7----1.8
    |       |
    5.3 |       |
    |       |
    5  -0.3----0.8
*/
internal void
GridVertex(triangle_mesh *Mesh, float g0, float g1, vec2 p0, vec2 p1)
{
    bool IsTopLeft = 0;
    bool IsTopRight = 0;
    bool IsBottomRight = 0;
    bool IsBottomLeft = 0;

    float A = g1 - p0.y;
    float B = g1 - p1.y;
    float D = g0 - p1.y;
    float C = g0 - p0.y;

    if(A <= 0.0f) IsTopLeft = true;
    if(B <= 0.0f) IsTopRight = true;
    if(D <= 0.0f) IsBottomRight = true;
    if(C <= 0.0f) IsBottomLeft = true;

    uint8 Configuration = 0;
    if(IsTopLeft) Configuration += 8;
    if(IsTopRight) Configuration += 4;
    if(IsBottomRight) Configuration += 2;
    if(IsBottomLeft) Configuration += 1;

    float ct = p0.x + (p1.x - p0.x)*((0 - A)/(B - A)); //x
    float cr = g1 + (g0 - g1)*((0 - B)/(D - B));       //y
    float cb = p0.x + (p1.x - p0.x)*((0 - C)/(D - C)); //x
    float cl = g1 + (g0 - g1)*((0 - A)/(C - A));       //y

    vec2 TopLeft = v2(p0.x, g1);
    vec2 TopRight = v2(p1.x, g1);
    vec2 BottomRight = v2(p1.x, g0);
    vec2 BottomLeft = v2(p0.x, g0);
    vec2 CenterTop = v2(ct, g1);
    vec2 CenterRight = v2(p1.x, cr);
    vec2 CenterBottom = v2(cb, g0);
    vec2 CenterLeft = v2(p0.x, cl);

    switch(Configuration)
    {
        case 0: break;

        // 1 points:
        case 1:
        {
            vec2 Points[] = {CenterLeft, CenterBottom, BottomLeft};
            MeshFromPoints(Mesh, Points, 3);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterBottom, CenterLeft);
            break;
        }
        case 2:
        {
            vec2 Points[] = {CenterRight, BottomRight, CenterBottom};
            MeshFromPoints(Mesh, Points, 3);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterRight, CenterBottom);
            break;
        }
        case 4:
        {
            vec2 Points[] = {CenterTop, TopRight, CenterRight};
            MeshFromPoints(Mesh, Points, 3);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterTop, CenterRight);
            break;
        }
        case 8:
        {
            vec2 Points[] = {TopLeft, CenterTop, CenterLeft};
            MeshFromPoints(Mesh, Points, 3);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterTop, CenterLeft);
            break;
        }
        // 2 points:
        case 3:
        {
            vec2 Points[] = {CenterRight, BottomRight, BottomLeft, 
                            BottomLeft, CenterLeft, CenterRight};
            MeshFromPoints(Mesh, Points, 6);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterRight, CenterLeft);
            break;
        }
        case 6:
        {
            vec2 Points[] = {CenterTop, TopRight, BottomRight,
                            CenterBottom, CenterTop, BottomRight};
            MeshFromPoints(Mesh, Points, 6);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterTop, CenterBottom);
            break;
        }
        case 9:
        {
            vec2 Points[] = {TopLeft, CenterTop, CenterBottom,
                            CenterBottom, BottomLeft, TopLeft};
            MeshFromPoints(Mesh, Points, 6);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterTop, CenterBottom);
            break;
        }
        case 12:
        {
            vec2 Points[] = {TopLeft, TopRight, CenterRight,
                            CenterRight, CenterLeft, TopLeft};
            MeshFromPoints(Mesh, Points, 6);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterRight, CenterLeft);
            break;
        }
        case 5:
        {
            vec2 Points[] = {CenterTop, TopRight, CenterRight,
                            CenterTop, CenterRight, CenterBottom,
                            CenterTop, CenterBottom, BottomLeft,
                            CenterTop, BottomLeft, CenterLeft};
            MeshFromPoints(Mesh, Points, 12);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterBottom, CenterRight);
            break;
        }
        case 10:
        {
            vec2 Points[] = {TopLeft, CenterTop, CenterRight,
                            TopLeft, CenterRight, BottomRight,
                            TopLeft, BottomRight, CenterBottom,
                            TopLeft, CenterBottom, CenterLeft};
            MeshFromPoints(Mesh, Points, 12);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterBottom, CenterLeft);
            break;
        }

        // 3 point:
        case 7:
        {
            vec2 Points[] = {CenterTop, TopRight, BottomRight,
                            CenterTop, BottomRight, BottomLeft,
                            CenterTop, BottomLeft, CenterLeft};
            MeshFromPoints(Mesh, Points, 9);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterTop, CenterLeft);
            break;
        }
        case 11:
        {
            vec2 Points[] = {TopLeft, CenterTop, CenterRight,
                            TopLeft, CenterRight, BottomRight,
                            TopLeft, BottomRight, BottomLeft};
            MeshFromPoints(Mesh, Points, 9);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterTop, CenterRight);
            break;
        }
        case 13:
        {
            vec2 Points[] = {TopLeft, TopRight, CenterRight,
                            TopLeft, CenterRight, CenterBottom,
                            TopLeft, CenterBottom, BottomLeft};
            MeshFromPoints(Mesh, Points, 9);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterBottom, CenterRight);
            break;
        }
        case 14:
        {
            vec2 Points[] = {TopLeft, TopRight, BottomRight,
                            TopLeft, BottomRight, CenterBottom,
                            TopLeft, CenterBottom, CenterLeft};
            MeshFromPoints(Mesh, Points, 9);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            //PushDrawLine(RenderBuffer, CenterBottom, CenterLeft);
            break;
        }

        // 4 point:
        case 15:
        {
            vec2 Points[] = {TopLeft, TopRight, BottomRight,
                            TopLeft, BottomRight, BottomLeft};
            MeshFromPoints(Mesh, Points, 6);
            //PushDrawMesh(RenderBuffer, Mesh, Color);
            break;
        }
    }
}

internal triangle_mesh
GenerateTerrainMesh(stream_state *StreamState, vec2 *Vertices)
{
    uint32 TileCountX = TERRAIN_SIZE * TERRAIN_RESOLUTION;
    uint32 TileCountY = TERRAIN_SIZE * TERRAIN_RESOLUTION;
    float GridSize = 1.0f; // unit space
    float TileSize = 1.0f/(float)TERRAIN_RESOLUTION;
    uint32 TileCountInGrid = TERRAIN_RESOLUTION * GridSize;

    triangle_mesh Mesh = {0};
    Mesh.Vertices = MemoryPointer(StreamState->TranBlock, vertex);
    
    // TODO: Final optimization, quadtree?
#if 1
    // pack grids
    for(u32 TileX = 0;
        TileX < TileCountX;
        TileX += TileCountInGrid)
    {
        float HighestTerrainPointY = 0;
        float LowestTerrainPointY = TileCountY;
        for(u32 PointInGridX = 0;
            PointInGridX <= TileCountInGrid;
            ++PointInGridX)
        {
            vec2 Point = Vertices[TileX + PointInGridX];
            if(Point.y < LowestTerrainPointY)
                LowestTerrainPointY = Point.y;
            if(Point.y > HighestTerrainPointY)
                HighestTerrainPointY = Point.y;
        }

        u32 HighestTile = (u32)HighestTerrainPointY * TileCountInGrid;
        u32 LowestTile = (u32)LowestTerrainPointY * TileCountInGrid;
        for(u32 TileY = 0;
            TileY < TileCountY;
            TileY += TileCountInGrid)
        {
            if(TileY > HighestTile) break;

            if(TileY >= LowestTile && TileY <= HighestTile)
            {
                for(u32 X = 0;
                    X < TileCountInGrid;
                    ++X)
                {
                    vec2 Point0 = Vertices[TileX + X];
                    vec2 Point1 = Vertices[TileX + X + 1];

                    for(u32 Y = 0;
                        Y < TileCountInGrid;
                        ++Y)
                    {
                        float TileMinY = (TileY + Y) * TileSize;
                        float TileMaxY = TileMinY + TileSize;
                        GridVertex(&Mesh, TileMinY, TileMaxY, Point0, Point1);
                    }
                }
            }
            else
            {
                vec2 Point0 = Vertices[TileX];
                vec2 Point1 = Vertices[TileX + TileCountInGrid];
                float GridMinY = (TileY/TileCountInGrid) * GridSize;
                float GridMaxY = GridMinY + GridSize;
                GridVertex(&Mesh, GridMinY, GridMaxY, Point0, Point1);
            }
        }
    }
#else
    for(uint32 PointIndex = 0;
        PointIndex < POINT_COUNT - 1;
        ++PointIndex)
    {
        vec2 p0 = Chunk->Vertices[PointIndex];
        vec2 p1 = Chunk->Vertices[PointIndex + 1];

        for(uint8 TileY = 0;
            TileY < TileCountY;
            ++TileY)
        {
            float TileMinY = TileY * TileSize;
            float TileMaxY = TileMinY + TileSize;
            GridVertex(Mesh, TileMinY, TileMaxY, p0, p1);
        }
    }
#endif

    // TODO: This might be problem for multithreading!
    PushArray(&StreamState->TranBlock, Mesh.NumberOfVertices, vertex);

    return Mesh;
}

inline real32
GetLineAngle(vec2 A, vec2 B, real32 C)
{
    if(B.x < A.x) {Swap(B.x, A.x); Swap(B.y, A.y);}
    return (-asinf((A.y - B.y) / C));
}

internal void
StoreEntityInChunk(world_chunk *Chunk, entity *Entity)
{
    // TODO: Storing based on entity's position?
    // StoreEntityInClosestChunk
    Assert(Chunk->EntityCount < ArrayCount(Chunk->Entities))
    Chunk->Entities[Chunk->EntityCount++] = Entity;
}

internal collision_volume *
GenerateTerrainCollisionVolumes(game_state *GameState, vec2 *Vertices, vec2 TerrainOrigin)
{
    collision_volume *FirstCollisionVolume = 0;

#if 0
    for(uint32 PointIndex = 0;
        PointIndex < POINT_COUNT - 1;
        ++PointIndex)
    {
        vec2 A = Vertices[PointIndex];
        vec2 B = Vertices[PointIndex + 1];

        vec2 LineCenter = v2(A.x + 0.5f*(B.x - A.x), A.y + 0.5f*(B.y - A.y));
        vec2 Dim = v2(Length(B - A), 0.6f);
        real32 Angle = GetLineAngle(A, B, Dim.x);
        vec2 OffsetP = (LineCenter - TerrainOrigin) - v2(0, 0.31f);

        if(FirstCollisionVolume)
        {
            FirstCollisionVolume = GetNextFreeCollisionVolume(GameState, FirstCollisionVolume);
        }
        else
        {
            FirstCollisionVolume = GetNextFreeCollisionVolume(GameState);
        }

        PolygonCollisionVolume(FirstCollisionVolume, OffsetP, Dim, Angle);
    }
#else
    // NOTE: Only for flat terrain!
    real32 HalfSize = 0.5f*(float)TERRAIN_SIZE;
    // NOTE: 6 is terrain OffsetY
    vec2 A = v2(TerrainOrigin.x - HalfSize, 6);
    vec2 B = v2(TerrainOrigin.x + HalfSize, 6);
    
    vec2 LineCenter = v2(A.x + 0.5f*(B.x - A.x), A.y + 0.5f*(B.y - A.y));
    vec2 Dim = v2(Length(B - A) + 0.1f, 0.6f);
    real32 Angle = 0;//GetLineAngle(A, B, Dim.x);
    vec2 OffsetP = (LineCenter - TerrainOrigin) - v2(0, 0.31f);

    FirstCollisionVolume = GetNextFreeCollisionVolume(GameState);
    PolygonCollisionVolume(FirstCollisionVolume, OffsetP, Dim, Angle);
#endif

    return FirstCollisionVolume;
}

internal world_chunk *
AllocateChunk(stream_state *StreamState, vec2 Pos, int32 Index)
{
    world_chunk *Result;

    Assert(StreamState->ChunkCount < ArrayCount(StreamState->Chunks))
    Result = &StreamState->Chunks[StreamState->ChunkCount++];

    Result->Index = Index;
    Result->Pos = Pos;

    u32 VerticesCount = POINT_COUNT;
    Result->Vertices = PushArray(&StreamState->TranBlock, VerticesCount, vec2);
    ZeroSize(VerticesCount*sizeof(vec2), (uint8 *)Result->Vertices);

    return Result;
}

internal int32
GetChunkIndex(float Pos, int32 *ChunkX)
{
    *ChunkX = (Pos > 0) ? (int32(Pos / TERRAIN_SIZE) * TERRAIN_SIZE) :
                         (int32(Pos / TERRAIN_SIZE - 1) * TERRAIN_SIZE);
    return (Pos > 0) ? (int32(Pos / TERRAIN_SIZE)) :
                       (int32(Pos / TERRAIN_SIZE - 1));
}

internal world_chunk *
GetChunkWithIndex(stream_state *StreamState, int32 Index)
{
    world_chunk *Result = 0;

    for(u32 ChunkIndex = 0;
        ChunkIndex < StreamState->ChunkCount;
        ChunkIndex++)
    {
        world_chunk *Chunk = &StreamState->Chunks[ChunkIndex];
        if(Chunk->Index == Index)
        {
            Result = Chunk;
            return Result;
        }
    }

    return Result;
}

internal void
InstantiateBullet(game_state *GameState, vec2 Pos, vec2 Target, real32 Speed,
                  u32 ParentIndex, int32 ParentComponent)
{
    vec2 Direction = Normalize(Target - Pos);
    real32 BulletAngle = HeadingToAngle(Direction) + ToRadians(90);

    u32 BulletIndex = AddBullet(GameState, Pos, v2(0.05f, 0.1f), BulletAngle, ParentComponent);
    AddCollisionRule(GameState, ParentIndex, BulletIndex, false);

    entity *Bullet = GetEntity(BulletIndex);
    Bullet->ApplyForce(Direction * Speed);
}

inline entity *
FindEntity(game_state *GameState, int32 Components)
{
    entity *Find = 0;
 
    for(u32 Index = 0;
        Index < GameState->EntityCount;
        ++Index)
    {
        Find = GetEntity(Index);
        if(HasComponent(Find, Components) && 
           !Find->Inactive)
        {
            return Find;
        }
        else
        {
            Find = 0;
        }
    }

    return Find;
}

internal enemy_stat *
FindEnemyStat(game_state *GameState, u32 EntityIndex)
{
    enemy_stat *Stat = 0;

    for(u32 i = 0;
        i < ArrayCount(GameState->Enemies);
        ++i)
    {
        Stat = &GameState->Enemies[i];
        if(Stat->Index == EntityIndex)
        {
            return Stat;
        }
    }

    return Stat;
}

internal void
RespawnPlayer(game_state *GameState, uint32 EntityIndex, vec2 Pos)
{
    entity *Player = &GameState->Entities[EntityIndex];

    Player->P = Pos;
    Player->Angle = 0;
    Player->Velocity = {0};
    Player->aVelocity = 0;
}
