/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEF_FOURWINDS_H
#define DEF_FOURWINDS_H


enum Data
{
    TYPE_CONCLAVE,
    TYPE_ALAKIR,
    MAX_ENCOUNTER,
    TYPE_CONCL_HELPER,
    TYPE_CONCL_TARGETS,
};

enum eCreatures
{
    NPC_BRIDGE              = 80000,
    SUMMON_ICEPATCH         = 46186,
    SUMMON_BREEZE           = 46246,
    SUMMON_TORNADO          = 46207,
    SUMMON_CREEPER_TRIGGER  = 45813,
    SUMMON_RAVENOUS_CREEPER = 45812,

    NPC_ANSHAL              = 45870,
    NPC_NEZIR               = 45871,
    NPC_ROHASH              = 45872,

    NPC_ALAKIR              = 46753,
};

enum eGOs
{
    GO_CENTER       = 4510369,
    GO_BRIDGE       = 4510370,
};

const Position BridgePos[]=
{
    {-238.1f, 871.9f, 193.9f, 0.0f}, // South -> West
    {-238.2f, 766.1f, 193.9f, 0.0f}, // South -> East
    {-107.2f, 631.1f, 193.9f, 0.0f}, // East  -> South
    {-2.2f, 631.4f, 193.9f, 0.0f},   // East  -> North
    {132.5f, 762.1f, 193.9f, 0.0f},  // North -> East
    {132.4f, 867.8f, 193.9f, 0.0f},  // North -> West
    {2.1f, 1001.8f, 193.9f, 0.0f},   // West  -> North
    {-103.6f, 1002.4f, 193.9f, 0.0f},// West  -> South
};

const Position BridgeTarget[]=
{
    {-83.5f, 1022.5f, 196.5f, 0.0f}, // South -> West
    {-87.6f, 611.6f, 196.5f, 0.0f},  // South -> East
    {-258.1f, 785.8f, 196.5f, 0.0f}, // East  -> South
    {152.4f, 781.8f, 196.5f, 0.0f},  // East  -> North
    {-22.1f, 611.3f, 196.5f, 0.0f},  // North -> East
    {-17.7f, 1021.9f, 196.5f, 0.0f}, // North -> West
    {152.8f, 847.8f, 196.5f, 0.0f},  // West  -> North
    {-257.8f, 852.2f, 196.5f, 0.0f}, // West  -> South
};

#endif
