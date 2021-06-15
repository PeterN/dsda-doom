//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

//===========================================================================
// Korax Variables
//      special1        last teleport destination
//      special2        set if "below half" script not yet run
//
// Korax Scripts (reserved)
//      249             Tell scripts that we are below half health
//      250-254 Control scripts
//      255             Death script
//
// Korax TIDs (reserved)
//      245             Reserved for Korax himself
//  248         Initial teleport destination
//      249             Teleport destination
//      250-254 For use in respective control scripts
//      255             For use in death script (spawn spots)
//===========================================================================
#define KORAX_SPIRIT_LIFETIME	(5*(35/5))      // 5 seconds
#define KORAX_COMMAND_HEIGHT	(120*FRACUNIT)
#define KORAX_COMMAND_OFFSET	(27*FRACUNIT)

void KoraxFire1(mobj_t * actor, int type);
void KoraxFire2(mobj_t * actor, int type);
void KoraxFire3(mobj_t * actor, int type);
void KoraxFire4(mobj_t * actor, int type);
void KoraxFire5(mobj_t * actor, int type);
void KoraxFire6(mobj_t * actor, int type);
void KSpiritInit(mobj_t * spirit, mobj_t * korax);

#define KORAX_TID					(245)
#define KORAX_FIRST_TELEPORT_TID	(248)
#define KORAX_TELEPORT_TID			(249)

void A_KoraxChase(mobj_t * actor)
{
    mobj_t *spot;
    int lastfound;
    byte args[3] = {0, 0, 0};

    if ((!actor->special2.i) &&
        (actor->health <= (actor->info->spawnhealth / 2)))
    {
        lastfound = 0;
        spot = P_FindMobjFromTID(KORAX_FIRST_TELEPORT_TID, &lastfound);
        if (spot)
        {
            P_Teleport(actor, spot->x, spot->y, spot->angle, true);
        }

        CheckACSPresent(249);
        P_StartACS(249, 0, args, actor, NULL, 0);
        actor->special2.i = 1;    // Don't run again

        return;
    }

    if (!actor->target)
        return;
    if (P_Random(pr_hexen) < 30)
    {
        P_SetMobjState(actor, actor->info->missilestate);
    }
    else if (P_Random(pr_hexen) < 30)
    {
        S_StartSound(NULL, hexen_sfx_korax_active);
    }

    // Teleport away
    if (actor->health < (actor->info->spawnhealth >> 1))
    {
        if (P_Random(pr_hexen) < 10)
        {
            lastfound = actor->special1.i;
            spot = P_FindMobjFromTID(KORAX_TELEPORT_TID, &lastfound);
            actor->special1.i = lastfound;
            if (spot)
            {
                P_Teleport(actor, spot->x, spot->y, spot->angle, true);
            }
        }
    }
}

void A_KoraxStep(mobj_t * actor)
{
    A_Chase(actor);
}

void A_KoraxStep2(mobj_t * actor)
{
    S_StartSound(NULL, hexen_sfx_korax_step);
    A_Chase(actor);
}

void A_KoraxBonePop(mobj_t * actor)
{
    mobj_t *mo;
    byte args[5];

    args[0] = args[1] = args[2] = args[3] = args[4] = 0;

    // Spawn 6 spirits equalangularly
    mo = P_SpawnMissileAngle(actor, HEXEN_MT_KORAX_SPIRIT1, ANG60 * 0,
                             5 * FRACUNIT);
    if (mo)
        KSpiritInit(mo, actor);
    mo = P_SpawnMissileAngle(actor, HEXEN_MT_KORAX_SPIRIT2, ANG60 * 1,
                             5 * FRACUNIT);
    if (mo)
        KSpiritInit(mo, actor);
    mo = P_SpawnMissileAngle(actor, HEXEN_MT_KORAX_SPIRIT3, ANG60 * 2,
                             5 * FRACUNIT);
    if (mo)
        KSpiritInit(mo, actor);
    mo = P_SpawnMissileAngle(actor, HEXEN_MT_KORAX_SPIRIT4, ANG60 * 3,
                             5 * FRACUNIT);
    if (mo)
        KSpiritInit(mo, actor);
    mo = P_SpawnMissileAngle(actor, HEXEN_MT_KORAX_SPIRIT5, ANG60 * 4,
                             5 * FRACUNIT);
    if (mo)
        KSpiritInit(mo, actor);
    mo = P_SpawnMissileAngle(actor, HEXEN_MT_KORAX_SPIRIT6, ANG60 * 5,
                             5 * FRACUNIT);
    if (mo)
        KSpiritInit(mo, actor);

    CheckACSPresent(255);
    P_StartACS(255, 0, args, actor, NULL, 0);   // Death script
}

void KSpiritInit(mobj_t * spirit, mobj_t * korax)
{
    int i;
    mobj_t *tail, *next;

    spirit->health = KORAX_SPIRIT_LIFETIME;

    spirit->special1.m = korax;     // Swarm around korax
    spirit->special2.i = 32 + (P_Random(pr_hexen) & 7);   // Float bob index
    spirit->args[0] = 10;       // initial turn value
    spirit->args[1] = 0;        // initial look angle

    // Spawn a tail for spirit
    tail = P_SpawnMobj(spirit->x, spirit->y, spirit->z, HEXEN_MT_HOLY_TAIL);
    tail->special2.m = spirit;      // parent
    for (i = 1; i < 3; i++)
    {
        next = P_SpawnMobj(spirit->x, spirit->y, spirit->z, HEXEN_MT_HOLY_TAIL);
        P_SetMobjState(next, next->info->spawnstate + 1);
        tail->special1.m = next;
        tail = next;
    }
    tail->special1.m = NULL;         // last tail bit
}

void A_KoraxDecide(mobj_t * actor)
{
    if (P_Random(pr_hexen) < 220)
    {
        P_SetMobjState(actor, HEXEN_S_KORAX_MISSILE1);
    }
    else
    {
        P_SetMobjState(actor, HEXEN_S_KORAX_COMMAND1);
    }
}

void A_KoraxMissile(mobj_t * actor)
{
    int type = P_Random(pr_hexen) % 6;
    int sound = 0;

    S_StartSound(actor, hexen_sfx_korax_attack);

    switch (type)
    {
        case 0:
            type = HEXEN_MT_WRAITHFX1;
            sound = hexen_sfx_wraith_missile_fire;
            break;
        case 1:
            type = HEXEN_MT_DEMONFX1;
            sound = hexen_sfx_demon_missile_fire;
            break;
        case 2:
            type = HEXEN_MT_DEMON2FX1;
            sound = hexen_sfx_demon_missile_fire;
            break;
        case 3:
            type = HEXEN_MT_FIREDEMON_FX6;
            sound = hexen_sfx_fired_attack;
            break;
        case 4:
            type = HEXEN_MT_CENTAUR_FX;
            sound = hexen_sfx_centaurleader_attack;
            break;
        case 5:
            type = HEXEN_MT_SERPENTFX;
            sound = hexen_sfx_centaurleader_attack;
            break;
    }

    // Fire all 6 missiles at once
    S_StartSound(NULL, sound);
    KoraxFire1(actor, type);
    KoraxFire2(actor, type);
    KoraxFire3(actor, type);
    KoraxFire4(actor, type);
    KoraxFire5(actor, type);
    KoraxFire6(actor, type);
}


// Call action code scripts (250-254)
void A_KoraxCommand(mobj_t * actor)
{
    byte args[5];
    fixed_t x, y, z;
    angle_t ang;
    int numcommands;

    S_StartSound(actor, hexen_sfx_korax_command);

    // Shoot stream of lightning to ceiling
    ang = (actor->angle - ANG90) >> ANGLETOFINESHIFT;
    x = actor->x + FixedMul(KORAX_COMMAND_OFFSET, finecosine[ang]);
    y = actor->y + FixedMul(KORAX_COMMAND_OFFSET, finesine[ang]);
    z = actor->z + KORAX_COMMAND_HEIGHT;
    P_SpawnMobj(x, y, z, HEXEN_MT_KORAX_BOLT);

    args[0] = args[1] = args[2] = args[3] = args[4] = 0;

    if (actor->health <= (actor->info->spawnhealth >> 1))
    {
        numcommands = 5;
    }
    else
    {
        numcommands = 4;
    }

    switch (P_Random(pr_hexen) % numcommands)
    {
        case 0:
            CheckACSPresent(250);
            P_StartACS(250, 0, args, actor, NULL, 0);
            break;
        case 1:
            CheckACSPresent(251);
            P_StartACS(251, 0, args, actor, NULL, 0);
            break;
        case 2:
            CheckACSPresent(252);
            P_StartACS(252, 0, args, actor, NULL, 0);
            break;
        case 3:
            CheckACSPresent(253);
            P_StartACS(253, 0, args, actor, NULL, 0);
            break;
        case 4:
            CheckACSPresent(254);
            P_StartACS(254, 0, args, actor, NULL, 0);
            break;
    }
}


#define KORAX_DELTAANGLE			(85*ANG1)
#define KORAX_ARM_EXTENSION_SHORT	(40*FRACUNIT)
#define KORAX_ARM_EXTENSION_LONG	(55*FRACUNIT)

#define KORAX_ARM1_HEIGHT			(108*FRACUNIT)
#define KORAX_ARM2_HEIGHT			(82*FRACUNIT)
#define KORAX_ARM3_HEIGHT			(54*FRACUNIT)
#define KORAX_ARM4_HEIGHT			(104*FRACUNIT)
#define KORAX_ARM5_HEIGHT			(86*FRACUNIT)
#define KORAX_ARM6_HEIGHT			(53*FRACUNIT)


// Arm projectiles
//              arm positions numbered:
//                      1       top left
//                      2       middle left
//                      3       lower left
//                      4       top right
//                      5       middle right
//                      6       lower right


// Arm 1 projectile
void KoraxFire1(mobj_t * actor, int type)
{
    angle_t ang;
    fixed_t x, y, z;

    ang = (actor->angle - KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
    x = actor->x + FixedMul(KORAX_ARM_EXTENSION_SHORT, finecosine[ang]);
    y = actor->y + FixedMul(KORAX_ARM_EXTENSION_SHORT, finesine[ang]);
    z = actor->z - actor->floorclip + KORAX_ARM1_HEIGHT;
    P_SpawnKoraxMissile(x, y, z, actor, actor->target, type);
}


// Arm 2 projectile
void KoraxFire2(mobj_t * actor, int type)
{
    angle_t ang;
    fixed_t x, y, z;

    ang = (actor->angle - KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
    x = actor->x + FixedMul(KORAX_ARM_EXTENSION_LONG, finecosine[ang]);
    y = actor->y + FixedMul(KORAX_ARM_EXTENSION_LONG, finesine[ang]);
    z = actor->z - actor->floorclip + KORAX_ARM2_HEIGHT;
    P_SpawnKoraxMissile(x, y, z, actor, actor->target, type);
}

// Arm 3 projectile
void KoraxFire3(mobj_t * actor, int type)
{
    angle_t ang;
    fixed_t x, y, z;

    ang = (actor->angle - KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
    x = actor->x + FixedMul(KORAX_ARM_EXTENSION_LONG, finecosine[ang]);
    y = actor->y + FixedMul(KORAX_ARM_EXTENSION_LONG, finesine[ang]);
    z = actor->z - actor->floorclip + KORAX_ARM3_HEIGHT;
    P_SpawnKoraxMissile(x, y, z, actor, actor->target, type);
}

// Arm 4 projectile
void KoraxFire4(mobj_t * actor, int type)
{
    angle_t ang;
    fixed_t x, y, z;

    ang = (actor->angle + KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
    x = actor->x + FixedMul(KORAX_ARM_EXTENSION_SHORT, finecosine[ang]);
    y = actor->y + FixedMul(KORAX_ARM_EXTENSION_SHORT, finesine[ang]);
    z = actor->z - actor->floorclip + KORAX_ARM4_HEIGHT;
    P_SpawnKoraxMissile(x, y, z, actor, actor->target, type);
}

// Arm 5 projectile
void KoraxFire5(mobj_t * actor, int type)
{
    angle_t ang;
    fixed_t x, y, z;

    ang = (actor->angle + KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
    x = actor->x + FixedMul(KORAX_ARM_EXTENSION_LONG, finecosine[ang]);
    y = actor->y + FixedMul(KORAX_ARM_EXTENSION_LONG, finesine[ang]);
    z = actor->z - actor->floorclip + KORAX_ARM5_HEIGHT;
    P_SpawnKoraxMissile(x, y, z, actor, actor->target, type);
}

// Arm 6 projectile
void KoraxFire6(mobj_t * actor, int type)
{
    angle_t ang;
    fixed_t x, y, z;

    ang = (actor->angle + KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
    x = actor->x + FixedMul(KORAX_ARM_EXTENSION_LONG, finecosine[ang]);
    y = actor->y + FixedMul(KORAX_ARM_EXTENSION_LONG, finesine[ang]);
    z = actor->z - actor->floorclip + KORAX_ARM6_HEIGHT;
    P_SpawnKoraxMissile(x, y, z, actor, actor->target, type);
}


void A_KSpiritWeave(mobj_t * actor)
{
    fixed_t newX, newY;
    int weaveXY, weaveZ;
    int angle;

    weaveXY = actor->special2.i >> 16;
    weaveZ = actor->special2.i & 0xFFFF;
    angle = (actor->angle + ANG90) >> ANGLETOFINESHIFT;
    newX = actor->x - FixedMul(finecosine[angle],
                               FloatBobOffsets[weaveXY] << 2);
    newY = actor->y - FixedMul(finesine[angle],
                               FloatBobOffsets[weaveXY] << 2);
    weaveXY = (weaveXY + (P_Random(pr_hexen) % 5)) & 63;
    newX += FixedMul(finecosine[angle], FloatBobOffsets[weaveXY] << 2);
    newY += FixedMul(finesine[angle], FloatBobOffsets[weaveXY] << 2);
    P_TryMove(actor, newX, newY, false);
    actor->z -= FloatBobOffsets[weaveZ] << 1;
    weaveZ = (weaveZ + (P_Random(pr_hexen) % 5)) & 63;
    actor->z += FloatBobOffsets[weaveZ] << 1;
    actor->special2.i = weaveZ + (weaveXY << 16);
}

void A_KSpiritSeeker(mobj_t * actor, angle_t thresh, angle_t turnMax)
{
    int dir;
    int dist;
    angle_t delta;
    angle_t angle;
    mobj_t *target;
    fixed_t newZ;
    fixed_t deltaZ;

    target = actor->special1.m;
    if (target == NULL)
    {
        return;
    }
    dir = P_FaceMobj(actor, target, &delta);
    if (delta > thresh)
    {
        delta >>= 1;
        if (delta > turnMax)
        {
            delta = turnMax;
        }
    }
    if (dir)
    {                           // Turn clockwise
        actor->angle += delta;
    }
    else
    {                           // Turn counter clockwise
        actor->angle -= delta;
    }
    angle = actor->angle >> ANGLETOFINESHIFT;
    actor->momx = FixedMul(actor->info->speed, finecosine[angle]);
    actor->momy = FixedMul(actor->info->speed, finesine[angle]);

    if (!(leveltime & 15)
        || actor->z > target->z + (target->info->height)
        || actor->z + actor->height < target->z)
    {
        newZ = target->z + ((P_Random(pr_hexen) * target->info->height) >> 8);
        deltaZ = newZ - actor->z;
        if (abs(deltaZ) > 15 * FRACUNIT)
        {
            if (deltaZ > 0)
            {
                deltaZ = 15 * FRACUNIT;
            }
            else
            {
                deltaZ = -15 * FRACUNIT;
            }
        }
        dist = P_AproxDistance(target->x - actor->x, target->y - actor->y);
        dist = dist / actor->info->speed;
        if (dist < 1)
        {
            dist = 1;
        }
        actor->momz = deltaZ / dist;
    }
    return;
}


void A_KSpiritRoam(mobj_t * actor)
{
    if (actor->health-- <= 0)
    {
        S_StartSound(actor, hexen_sfx_spirit_die);
        P_SetMobjState(actor, HEXEN_S_KSPIRIT_DEATH1);
    }
    else
    {
        if (actor->special1.m)
        {
            A_KSpiritSeeker(actor, actor->args[0] * ANG1,
                            actor->args[0] * ANG1 * 2);
        }
        A_KSpiritWeave(actor);
        if (P_Random(pr_hexen) < 50)
        {
            S_StartSound(NULL, hexen_sfx_spirit_active);
        }
    }
}

void A_KBolt(mobj_t * actor)
{
    // Countdown lifetime
    if (actor->special1.i-- <= 0)
    {
        P_SetMobjState(actor, HEXEN_S_NULL);
    }
}


#define KORAX_BOLT_HEIGHT		48*FRACUNIT
#define KORAX_BOLT_LIFETIME		3

void A_KBoltRaise(mobj_t * actor)
{
    mobj_t *mo;
    fixed_t z;

    // Spawn a child upward
    z = actor->z + KORAX_BOLT_HEIGHT;

    if ((z + KORAX_BOLT_HEIGHT) < actor->ceilingz)
    {
        mo = P_SpawnMobj(actor->x, actor->y, z, HEXEN_MT_KORAX_BOLT);
        if (mo)
        {
            mo->special1.i = KORAX_BOLT_LIFETIME;
        }
    }
    else
    {
        // Maybe cap it off here
    }
}
