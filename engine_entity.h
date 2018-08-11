#pragma once

#define GetEntity(Index) (GameState->Entities + Index)

// http://www.dylanleigh.net/notes/c-cpp-tricks.html#Using_"Bitflags"
#define MOVEMENT_MASK (EntityComponent_Position | EntityComponent_Velocity)

enum entity_components
{   
    EntityComponent_None = 0,

    EntityComponent_Position = 1 << 0,
    EntityComponent_Rotation = 1 << 1,
    EntityComponent_Velocity = 1 << 2,

    EntityComponent_Collide = 1 << 3,
    EntityComponent_Render = 1 << 4,
    EntityComponent_Health = 1 << 5,

    EntityComponent_Player = 1 << 6,
    EntityComponent_Ball = 1 << 7,
    EntityComponent_Enemy = 1 << 8,
    EntityComponent_Monolith = 1 << 9,

    EntityComponent_Block = 1 << 10,
    EntityComponent_Land = 1 << 11,
    EntityComponent_Bullet = 1 << 12,
  
    EntityComponent_Camera = 1 << 13,
    EntityComponent_Mesh = 1 << 14
};

enum collision_shape
{
    CollisionShape_Polygon,
    CollisionShape_Circle,

    CollisionShapeCount
};

struct collision_volume
{
    collision_shape Shape;
    vec2 OffsetP;

    union
    {
        struct
        {
            vec2 Dim;
            real32 Angle;
        };

        struct
        {
            real32 Radius;
        };
    };
    
    collision_volume *Next;
};

struct entity
{
    bool32 Inactive;
    
    int32 Mask;
    int32 ParentComponent;
    u32 StorageIndex;

    collision_volume *FirstCollisionVolume;
    texture_slot *TextureSlot;

    triangle_mesh Mesh;
    
    // NOTE: mass data, transformation (position, rotation),
    //       velocity, torque?
    vec2 P;
    vec2 Dim;
    real32 Angle;

    real32 Mass;
    real32 Inertia;

    vec2 Velocity;
    real32 aVelocity;
    vec2 Force;
    real32 Torque;

    real32 StaticFriction;
    real32 DynamicFriction;

    void ResetForce()
    {
        Force = v2(0.0f, 0.0f);
        Torque = 0.0f;
    };
    void ApplyForce(vec2 F)
    {
        Force.Add(F);
    };
    void ApplyTorque(real32 T)
    {
        Torque += T;
    };
};
/*
void entity::ResetForce()
{
    Force = v2(0.0f, 0.0f);
    Torque = 0.0f;
};
void entity::ApplyForce(vec2 F)
{
    Force.Add(F);
};
void entity::ApplyTorque(real32 T)
{
    Torque += T;
};
*/
/*
struct entity : data
{
    void ResetForce()
    {
        Force = v2(0.0f, 0.0f);
        Torque = 0.0f;
    };
    void ApplyForce(vec2 F)
    {
        Force.Add(F);
    };
    void ApplyTorque(real32 T)
    {
        Torque += T;
    };
};*/

inline bool32
HasComponent(entity *Entity, int32 Component)
{
    bool32 Result = false;
    if((Entity->Mask & Component) == Component)
    {
        Result = true;
    }
    return Result;
}

inline bool32
HasSameComponent(entity *A, entity *B, int32 Component)
{
    bool32 Result = false;
    if(HasComponent(A, Component) &&
       HasComponent(B, Component))
    {
        Result = true;
    }
    return Result;
}