// engine_entity.cpp

internal u32
AddNextFreeEntity(game_state *GameState)
{
    u32 Result;

    if(GameState->FreeEntityCount == 0)
    {
        Result = GameState->EntityCount++;
        Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
    }
    else
    {
        Result = GameState->FirstFreeEntityIndex;
        GameState->FreeEntityCount--;
        if(GameState->FreeEntityCount)
        {
            b32 Found = false;
            for(u32 EntityIndex = GameState->FirstFreeEntityIndex + 1;
                EntityIndex < GameState->EntityCount;
                ++EntityIndex)
            {
                if(GameState->Entities[EntityIndex].Inactive)
                {
                    GameState->FirstFreeEntityIndex = EntityIndex;
                    Found = true;
                    break;
                }
            }
            Assert(Found);
        }
    }

    ZeroSize(sizeof(entity), &GameState->Entities[Result]);
    GameState->Entities[Result].StorageIndex = Result;
    return Result;
}

internal void
FreeCollisionVolume(game_state *GameState, collision_volume *CollisionVolume)
{
    while(CollisionVolume)
    {
        collision_volume *Free = CollisionVolume;
        CollisionVolume = CollisionVolume->Next;
        
        Free->Next = GameState->FirstFreeCollisionVolume;
        GameState->FirstFreeCollisionVolume = Free;
    }
}

internal void
FreeEntity(game_state *GameState, u32 EntityIndex)
{
    if(!GameState->Entities[EntityIndex].Inactive)
    {
        GameState->Entities[EntityIndex].Inactive = true;

        collision_volume *CollisionVolume = 
            GameState->Entities[EntityIndex].FirstCollisionVolume;
        
        FreeCollisionVolume(GameState, CollisionVolume);

        ClearCollisionRulesFor(GameState, EntityIndex);

        ++GameState->FreeEntityCount;
        if(GameState->FreeEntityCount == 1 || GameState->FirstFreeEntityIndex > EntityIndex)
        {
            GameState->FirstFreeEntityIndex = EntityIndex;
        }
    }
}

internal void
PolygonCollisionVolume(collision_volume *CollisionVolume, vec2 OffsetP, vec2 Dim,
                       real32 Angle = 0, bool StopOnCollision = true)
{
    collision_volume *Result = CollisionVolume;
    Result->Shape = CollisionShape_Polygon;
    Result->OffsetP = OffsetP;
    Result->Dim = Dim;
    Result->Angle = Angle;
}

internal void
CircleCollisionVolume(collision_volume *CollisionVolume, vec2 OffsetP, real32 Radius,
                      bool StopOnCollision = true)
{
    collision_volume *Result = CollisionVolume;
    Result->Shape = CollisionShape_Circle;
    Result->OffsetP = OffsetP;
    Result->Radius = Radius;
}

internal collision_volume *
GetNextFreeCollisionVolume(game_state *GameState, collision_volume *Next = 0)
{
    collision_volume *Result;
    if(GameState->FirstFreeCollisionVolume)
    {
        Result = GameState->FirstFreeCollisionVolume;
        GameState->FirstFreeCollisionVolume = GameState->FirstFreeCollisionVolume->Next;
    }
    else
    {
        Assert(GameState->CollisionVolumeCount < ArrayCount(GameState->CollisionVolumes));
        Result = GameState->CollisionVolumes + GameState->CollisionVolumeCount++;
        //GameState->FirstFreeCollisionVolume = GameState->CollisionVolumes + GameState->CollisionVolumeCount;
    }

    Result->Next = Next;
    return Result;
}

internal vec2
EntitySeekTarget(vec2 TargetP, vec2 EntityP, vec2 Velocity,
                 real32 DistanceFromTarget, real32 MaxSpeed,
                 real32 MaxForce)
{
#if 0
    vec2 Desired = TargetP - EntityP;
    Desired.Normalize();
    Desired.Scale(MaxSpeed);

    vec2 Steer = Limit(Desired - Velocity, MaxForce);
#else

    vec2 Desired = TargetP - EntityP;

    float d = Length(Desired) - DistanceFromTarget;
    Desired.Normalize();
    if(d < 3.0f)
    {
        float m = Map(d, 0, 3.0f, 0, MaxSpeed);
        Desired.Scale(m);
    }
    else
    {
        Desired.Scale(MaxSpeed);
    }

    vec2 Steer = Limit(Desired - Velocity, MaxForce);
#endif
    return Steer;
}

inline void
IntegrateForces(entity *A, float dt)
{
    Assert(A->Mass)
    Assert(A->Inertia)
    if(A->Mass) A->Velocity += A->Force / A->Mass * (0.5f*dt);
    if(A->Inertia) A->aVelocity += A->Torque / A->Inertia * (0.5f*dt);
}

inline void
IntegrateVelocity(entity *A, float dt)
{
    A->P += A->Velocity*dt;
    A->Angle += A->aVelocity*dt;
}

internal void
ApplyImpulse(collision Collision, entity *A, entity *B)
{
    // Early out and positional correct if both objects have infinite mass
    /*if(Equal(1.0f/A->Mass + 1.0f/B->Mass, 0))
    {
    InfiniteMassCorrection();
    return;
    }*/

    for(u32 i = 0; i < Collision.ContactCount; ++i)
    {
        // Calculate radii from COM to contact
        vec2 ra = Collision.Contacts[i] - A->P;
        vec2 rb = Collision.Contacts[i] - B->P;
        vec2 Normal = Collision.Normal;

        // Relative velocity
        vec2 rv = B->Velocity + Cross(B->aVelocity, rb) -
				  A->Velocity - Cross(A->aVelocity, ra);

        // Relative velocity along the normal
        real32 ContactVel = Dot(rv, Normal);

        // Do not resolve if velocities are separating
        if(ContactVel > 0.0f) return;

        real32 raCrossN = Cross(ra, Normal);
        real32 rbCrossN = Cross(rb, Normal);
        // NOTE: Mass and Inertia can't be zero!!!
        real32 InvMassSum = 0;

        // Calculate impulse scalar
        real32 j = -(1 + Collision.Restitution) * ContactVel;

        if(HasComponent(B, EntityComponent_Velocity))
        {
            if(B->Mass && B->Inertia)
            {
                InvMassSum = 1.0f/B->Mass + Square(rbCrossN)*(1.0f/B->Inertia);
                j /= InvMassSum;
                j /= (real32)Collision.ContactCount;
            }
        }
        
        if(HasComponent(A, EntityComponent_Velocity))
        {
            if(A->Mass && A->Inertia)
            {
                InvMassSum = 1.0f/A->Mass + Square(raCrossN)*(1.0f/A->Inertia);
                j /= InvMassSum;
                j /= (real32)Collision.ContactCount;
            }
        }

        if(HasComponent(A, EntityComponent_Velocity) &&
           HasComponent(B, EntityComponent_Velocity))
        {
            Assert(A->Mass && A->Inertia && B->Mass && B->Inertia);
            InvMassSum = 1.0f/A->Mass + 1.0f/B->Mass + Square(raCrossN) * (1.0f/A->Inertia) + Square(rbCrossN) * (1.0f/B->Inertia);
            j /= InvMassSum;
            j /= (real32)Collision.ContactCount;
        }

        // Apply impulse
        vec2 Impulse = Normal*j;

        if(HasComponent(A, EntityComponent_Velocity))
        {
            if(A->Mass && A->Inertia)
            {
                A->Velocity += (1.0f/A->Mass) * -Impulse;
                A->aVelocity += (1.0f/A->Inertia) * Cross(ra, -Impulse);
            }
        }

        if(HasComponent(B, EntityComponent_Velocity))
        {
            if(B->Mass && B->Inertia)
            {
                B->Velocity += (1.0f/B->Mass) * Impulse;
                B->aVelocity += (1.0f/B->Inertia) * Cross(rb, Impulse);
            }
        }

        // Friction impulse
        rv = B->Velocity + Cross( B->aVelocity, rb) -
			 A->Velocity - Cross( A->aVelocity, ra);

        vec2 t = rv - (Normal * Dot(rv, Normal));
        t.Normalize();

        // j tangent magnitude
        real32 jt = -Dot(rv, t);
        jt /= InvMassSum;
        jt /= (real32)Collision.ContactCount;

        // Don't apply tiny friction impulses
        if(Equal(jt, 0.0f)) return;

        // Coulumb's law
        vec2 TangentImpulse;
        if(Abs(jt) < j*Collision.StaticFriction)
        TangentImpulse = t * jt;
        else
        TangentImpulse = t * -j*Collision.DynamicFriction;

        // Apply friction impulse
        if(HasComponent(A, EntityComponent_Velocity))
        {
            A->Velocity += (1.0f/A->Mass) * -TangentImpulse;
            A->aVelocity += (1.0f/A->Inertia) * Cross(ra, -TangentImpulse);
        }

        if(HasComponent(B, EntityComponent_Velocity))
        {
            B->Velocity += (1.0f/B->Mass) * TangentImpulse;
            B->aVelocity += (1.0f/B->Inertia) * Cross(rb, TangentImpulse);
        }
    }    
}

internal b32
CollisionBetween(entity *EntityA, entity *EntityB, int32 ComponentA, int32 ComponentB)
{
    b32 Result = HasComponent(EntityA, ComponentA) && 
                 HasComponent(EntityB, ComponentB);
    if(!Result)
    {
        Result = HasComponent(EntityB, ComponentA) && 
                 HasComponent(EntityA, ComponentB);
    }

    return Result;
}

internal entity *
EntityWithComponents(entity *A, entity *B, entity **Other, int32 Components)
{
    entity *Result = 0;

    if(HasComponent(A, Components))
    {
        Result = A;
        *Other = B;
    }
    else if(HasComponent(B, Components))
    {
        Result = B;
        *Other = A;
    }

    return Result;
}

#define EntityWithComponent(Component)                                      \
    (HasComponent(A, Component)) ? (A)->StorageIndex : (B)->StorageIndex    \

internal b32
HandleCollision(game_state *GameState, entity *A, entity *B)
{
    b32 StopOnCollision = false;

    if(A->Mask > B->Mask)
    {
        entity *Temp = A;
        A = B;
        B = Temp;
    }

    if(!A->Inactive && !B->Inactive)
    {
        if(HasComponent(A, EntityComponent_Player) &&
           (HasComponent(B, EntityComponent_Block) ||
           HasComponent(B, EntityComponent_Land)))
        {
            StopOnCollision = true;
        }

        if(HasComponent(A, EntityComponent_Enemy) &&
           HasComponent(B, EntityComponent_Bullet))
        {
            if((B->ParentComponent == EntityComponent_Player) ||
               (B->ParentComponent == EntityComponent_Ball))
            {
                if(!GameState->GameOver) ++GameState->EnemiesDestroyed;

                FreeEntity(GameState, A->StorageIndex);

                StopOnCollision = false;
            }
        }

        if(HasComponent(A, EntityComponent_Monolith) &&
           HasComponent(B, EntityComponent_Bullet))
        {
            if(B->ParentComponent == EntityComponent_Enemy)
            {
                if(HasComponent(A, EntityComponent_Health))
                {
                    GameState->ShakeScreen = true;
                    GameState->MonolithHealth -= GameState->MonolithHealthDamage;
                }

                StopOnCollision = false;
            }
        }

        if(HasComponent(A, EntityComponent_Bullet))
        {
            Assert(0);
        }

        if(HasComponent(B, EntityComponent_Bullet))
        {
            FreeEntity(GameState, B->StorageIndex);
            StopOnCollision = true;
        }
    }

    return StopOnCollision;
}

internal b32
CanCollide(game_state *GameState, entity *A, entity *B)
{
    b32 Result = false;

    if(A != B)
    {
        if(A->StorageIndex > B->StorageIndex)
        {
            Swap(A, B);
        }

        // Continue if both entities are active
        if(!A->Inactive && !B->Inactive)
        {
            if (HasComponent(A, EntityComponent_Collide) &&
                HasComponent(B, EntityComponent_Collide))
            {
                Result = true;
            }
        }

        if (HasSameComponent(A, B, EntityComponent_Enemy) ||
            HasSameComponent(A, B, EntityComponent_Land) ||
            HasSameComponent(A, B, EntityComponent_Bullet))
        {
            AddCollisionRule(GameState, A->StorageIndex, B->StorageIndex, false);
            Result = false;
        }

        u32 HashSlot = A->StorageIndex & (ArrayCount(GameState->CollisionRuleHash) - 1);
        for(collision_rule *Rule = GameState->CollisionRuleHash[HashSlot];
            Rule;
            Rule = Rule->NextInHash)
        {
            if ((Rule->StorageIndexA == A->StorageIndex) &&
                (Rule->StorageIndexB == B->StorageIndex))
            {
                Result = Rule->CanCollide;
                break;
            }
        }
        
        if(A->Mask > B->Mask)
        {
            entity *Temp = A;
            A = B;
            B = Temp;
        }

        if(HasComponent(A, EntityComponent_Monolith) &&
           HasComponent(B, EntityComponent_Bullet))
        {
            if(B->ParentComponent == EntityComponent_Ball ||
               B->ParentComponent == EntityComponent_Player)
            {
                AddCollisionRule(GameState, A->StorageIndex, B->StorageIndex, false);
                Result = false;
            }
        }

        if(HasComponent(A, EntityComponent_Enemy) &&
           HasComponent(B, EntityComponent_Bullet))
        {
            if(B->ParentComponent == EntityComponent_Enemy)
            {
                AddCollisionRule(GameState, A->StorageIndex, B->StorageIndex, false);
                Result = false;
            }
        }

        if(HasComponent(A, EntityComponent_Ball) &&
           HasComponent(B, EntityComponent_Monolith))
        {
            AddCollisionRule(GameState, A->StorageIndex, B->StorageIndex, false);
            Result = false;
        }
    }

    return Result;
}

internal void
SimulateEntities(sim_region *SimRegion, game_state *GameState, memory_block *TempBlock,
                 vec2 Direction, real32 dt)
{
    TIMED_BLOCK();
    
    temp_memory ContactMemory = BeginTempMemory(TempBlock);
    uint32 CollisionCount = 0;

    // TODO: Multiple simulation regions
    for(uint32 EntityIndexA = 0;
        EntityIndexA < SimRegion->EntityCount;
        ++EntityIndexA)
    {
        for(uint32 EntityIndexB = 0;
            EntityIndexB < SimRegion->EntityCount;
            ++EntityIndexB)
        {
            entity *A = SimRegion->Entities[EntityIndexA];
            entity *B = SimRegion->Entities[EntityIndexB];

            if(CanCollide(GameState, A, B))
            {
                for(collision_volume *CollisionVolumeA = A->FirstCollisionVolume;
                    CollisionVolumeA;
                    CollisionVolumeA = CollisionVolumeA->Next)
                {
                    for(collision_volume *CollisionVolumeB = B->FirstCollisionVolume;
                        CollisionVolumeB;
                        CollisionVolumeB = CollisionVolumeB->Next)
                    {
                        vec2 HalfDim = CollisionVolumeA->Dim * 0.5f;
                        vec2 VerticesA[] = {
                            v2(-HalfDim.x, -HalfDim.y),
                            v2(HalfDim.x, -HalfDim.y),
                            v2(HalfDim.x, HalfDim.y),
                            v2(-HalfDim.x, HalfDim.y)
                        };
                    
                        vec2 TestHalfDim = CollisionVolumeB->Dim * 0.5f;
                        vec2 VerticesB[] = {
                            v2(-TestHalfDim.x, -TestHalfDim.y),
                            v2(TestHalfDim.x, -TestHalfDim.y),
                            v2(TestHalfDim.x, TestHalfDim.y),
                            v2(-TestHalfDim.x, TestHalfDim.y)
                        };
                    
                        // Only doing for rect!
                        polygon PolygonA = SetPolygon(A->P + CollisionVolumeA->OffsetP, VerticesA, 4);
                        polygon PolygonB = SetPolygon(B->P + CollisionVolumeB->OffsetP, VerticesB, 4);
                        PolygonA.u.SetOrient(A->Angle + CollisionVolumeA->Angle);
                        PolygonB.u.SetOrient(B->Angle + CollisionVolumeB->Angle);
                    
                        // Calculate static and dynamic friction
                        real32 sf = sqrtf(A->StaticFriction * B->StaticFriction);
                        real32 df = sqrtf(A->DynamicFriction * B->DynamicFriction);
                    
                        collision Collision = {};
                        Collision.Restitution = 0.5f;
                        Collision.StaticFriction = sf;//0.0f;
                        Collision.DynamicFriction = df;//0.0f;
                    
                        PolygonToPolygon(&Collision, &PolygonA, &PolygonB);
                    
                        if(Collision.ContactCount)
                        {
                            Collision.A = A;
                            Collision.B = B;
                        
                            collision *Contact = PushStruct(TempBlock, collision);
                            *Contact = Collision;
                            ++CollisionCount;
                        }
                    }
                }
            }
        }
    }

    // TODO: What about moving entities?
    // TODO: ControllingEntityIndex is different from SimRegionIndex
    for(uint32 Index = 0;
        Index < SimRegion->EntityCount;
        Index++)
    {
        u32 EntityIndex = SimRegion->IndexTable[Index];
        entity *Entity = GetEntity(EntityIndex);

        if(HasComponent(Entity, MOVEMENT_MASK) &&
           !Entity->Inactive)
        {
            float DirectionLength = LengthSq(Direction);
            if(DirectionLength > 1.0f)
            {
                Direction *= (1.0f / sqrt(DirectionLength));
            }
            
            // NOTE: Dont allow up/down
            //Direction.y = 0;

            float g = -2.8f;
            vec2 MoveForce = 15.0f*Direction;
            vec2 Gravity = v2(0, g*Entity->Mass);
            /*
            vec2 dNormal = {};
            {
                dNormal = Direction;
                dNormal.x = 0.0f;
            }
            Rotate(&dNormal, v2(0, 0), Entity->Angle);
            vec2 ThrusterForce = 30.0f*dNormal;
            */

            if(HasComponent(Entity, EntityComponent_Camera))
            {
                if(GameState->ControllingEntityIndex == EntityIndex)
                {
                    Entity->ApplyForce(MoveForce);
                    Entity->ApplyForce(-4.0f*Entity->Velocity); // movement drag
                }
            }

            else if(HasComponent(Entity, EntityComponent_Player))
            {
                Direction.y = 0;
                MoveForce = 15.0f*Direction;
                    
                Entity->ApplyForce(Gravity);

                if(GameState->ControllingEntityIndex == EntityIndex)
                {
                    Entity->ApplyForce(MoveForce);
                    Entity->ApplyForce(-6.0f*Entity->Velocity); // movement drag

                    // TODO: Merge Force and Torque?
                    //Entity->ApplyForce(-30*Entity->Velocity); // movement drag
                    //Entity->ApplyForce(ThrusterForce);
                        
                    //vec2 r = 0.5f*Entity->Dim;
                    //Entity->ApplyTorque(-0.5f*Direction.x);//Cross(r, 3.0f*Direction));
                    Entity->ApplyTorque(-0.6f*Entity->aVelocity); // torque drag
                }
            }

            else if(HasComponent(Entity, EntityComponent_Ball))
            {
                // TODO: Not good idea?
                Entity->Angle = 0;
                vec2 PlayerP = GetEntity(GameState->ControllingEntityIndex)->P;
                real32 DistanceFromTarget = 1.3f;
                vec2 SteeringForce = EntitySeekTarget(PlayerP + v2(0, 0.2f), Entity->P, Entity->Velocity,
                                                      DistanceFromTarget, 12.0f, 20.0f);
                Entity->ApplyForce(SteeringForce);
            }

            else if(HasComponent(Entity, EntityComponent_Enemy))
            {
                entity *Monolith = GetEntity(GameState->MonolithEntityIndex);

                real32 RandomValue = RandomSequence[EntityIndex];

                real32 DistanceFromTarget = 3*Entity->Dim.x;
                real32 SideValue = (Entity->P.x < Monolith->P.x) ? -1 + RandomValue : 1 + RandomValue;
                vec2 SteeringForce = EntitySeekTarget(Monolith->P + v2(SideValue, 3.0f), Entity->P, Entity->Velocity,
                                                      DistanceFromTarget, 4.0f, 10.0f);
                Entity->ApplyForce(SteeringForce);
            }

            else continue;

            IntegrateForces(Entity, dt);
        }
    }

    collision *Contacts = TempPointer(ContactMemory, collision);
    for(uint32 ContactIndex = 0;
        ContactIndex < CollisionCount;
        ++ContactIndex)
    {
        collision Contact = Contacts[ContactIndex];
        
        b32 StopOnCollision = HandleCollision(GameState, Contact.A, Contact.B);
        if(StopOnCollision)
        {
            //if(Contact.CollisionVolume->StopOnCollision)
            for(u32 Iterations = 0;
                Iterations < 16;
                ++Iterations)
            {
                ApplyImpulse(Contact, Contact.A, Contact.B);
            }
        }
        else
        {
            //AddCollisionRule(GameState, Contact.A->StorageIndex, Contact.B->StorageIndex, false);
        }
    }

    for(uint32 Index = 0;
        Index < SimRegion->EntityCount;
        Index++)
    {
        entity *Entity = SimRegion->Entities[Index];

        if(HasComponent(Entity, MOVEMENT_MASK) && 
           !Entity->Inactive)
        {
            IntegrateVelocity(Entity, dt);
            IntegrateForces(Entity, dt);

            Entity->ResetForce();
        }
    }

    for(uint32 ContactIndex = 0;
        ContactIndex < CollisionCount;
        ++ContactIndex)
    {
        collision Contact = Contacts[ContactIndex];
        PositionalCorrection(Contact);
    }

    EndTempMemory(ContactMemory);
}