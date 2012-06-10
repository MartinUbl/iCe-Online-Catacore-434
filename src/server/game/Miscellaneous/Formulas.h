/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef TRINITY_FORMULAS_H
#define TRINITY_FORMULAS_H

#include "World.h"
#include "SharedDefines.h"
#include "ScriptMgr.h"

namespace Trinity
{
    namespace Honor
    {
        inline float hk_honor_at_level_f(uint8 level, float multiplier = 1.0f)
        {
            float honor = multiplier * level * 1.55f * 0.024f; // 155% = original multiplier, 2.4% = Cata modifier
            sScriptMgr->OnHonorCalculation(honor, level, multiplier);

            return honor;
        }

        inline uint32 hk_honor_at_level(uint8 level, float multiplier = 1.0f)
        {
            return uint32(ceil(hk_honor_at_level_f(level, multiplier)));
        }
    }
    namespace XP
    {
        inline uint8 GetGrayLevel(uint8 pl_level)
        {
            uint8 level;

            if (pl_level <= 5)
                level = 0;
            else if (pl_level <= 39)
                level = pl_level - 5 - pl_level / 10;
            else if (pl_level <= 59)
                level = pl_level - 1 - pl_level / 5;
            else
                level = pl_level - 9;

            sScriptMgr->OnGrayLevelCalculation(level, pl_level);
            return level;
        }

        inline XPColorChar GetColorCode(uint8 pl_level, uint8 mob_level)
        {
            XPColorChar color;

            if (mob_level >= pl_level + 5)
                color = XP_RED;
            else if (mob_level >= pl_level + 3)
                color = XP_ORANGE;
            else if (mob_level >= pl_level - 2)
                color = XP_YELLOW;
            else if (mob_level > GetGrayLevel(pl_level))
                color = XP_GREEN;
            else
                color = XP_GRAY;

            sScriptMgr->OnColorCodeCalculation(color, pl_level, mob_level);
            return color;
        }

        inline uint8 GetZeroDifference(uint8 pl_level)
        {
            uint8 diff;

            if (pl_level < 8)
                diff = 5;
            else if (pl_level < 10)
                diff = 6;
            else if (pl_level < 12)
                diff = 7;
            else if (pl_level < 16)
                diff = 8;
            else if (pl_level < 20)
                diff = 9;
            else if (pl_level < 30)
                diff = 11;
            else if (pl_level < 40)
                diff = 12;
            else if (pl_level < 45)
                diff = 13;
            else if (pl_level < 50)
                diff = 14;
            else if (pl_level < 55)
                diff = 15;
            else if (pl_level < 60)
                diff = 16;
            else
                diff = 17;

            sScriptMgr->OnZeroDifferenceCalculation(diff, pl_level);
            return diff;
        }

        inline uint32 BaseGain(uint8 pl_level, uint8 mob_level, ContentLevels content)
        {
            uint32 baseGain;
            uint32 nBaseExp;

            switch (content)
            {
                case CONTENT_1_60:
                    nBaseExp = 45;
                    break;
                case CONTENT_61_70:
                    nBaseExp = 235;
                    break;
                case CONTENT_71_80:
                    nBaseExp = 580;
                    break;
                case CONTENT_81_85:
                    nBaseExp = 1878;
                    break;
                default:
                    sLog->outError("BaseGain: Unsupported content level %u",content);
                    nBaseExp = 45;
                    break;
            }

            if (mob_level >= pl_level)
            {
                uint8 nLevelDiff = mob_level - pl_level;
                if (nLevelDiff > 4)
                    nLevelDiff = 4;

                baseGain = ((pl_level * 5 + nBaseExp) * (20 + nLevelDiff) / 10 + 1) / 2;
            }
            else
            {
                uint8 gray_level = GetGrayLevel(pl_level);
                if (mob_level > gray_level)
                {
                    uint8 ZD = GetZeroDifference(pl_level);
                    baseGain = (pl_level * 5 + nBaseExp) * (ZD + mob_level - pl_level) / ZD;
                }
                else
                    baseGain = 0;
            }

            sScriptMgr->OnBaseGainCalculation(baseGain, pl_level, mob_level, content);
            return baseGain;
        }

        inline uint32 Gain(Player *pl, Unit *u)
        {
            uint32 gain;

            if (u->GetTypeId() == TYPEID_UNIT &&
                (((Creature*)u)->isTotem() || ((Creature*)u)->isPet() ||
                (((Creature*)u)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_XP_AT_KILL) ||
                ((Creature*)u)->GetCreatureInfo()->type == CREATURE_TYPE_CRITTER))
                gain = 0;
            else
            {
                gain = BaseGain(pl->getLevel(), u->getLevel(), GetContentLevelsForMapAndZone(u->GetMapId(), u->GetZoneId()));

                if (gain != 0 && u->GetTypeId() == TYPEID_UNIT && ((Creature*)u)->isElite())
                {
                    // Elites in instances have a 2.75x XP bonus instead of the regular 2x world bonus.
                    if (u->GetMap() && u->GetMap()->IsDungeon())
                       gain = uint32(gain * 2.75);
                    else
                        gain *= 2;
                }

                gain = uint32(gain * sWorld->getRate(RATE_XP_KILL));
            }

            // Reduce XP gain for level higher or equal to max in lower expansions
            if (u->ToCreature() && u->ToCreature()->GetCreatureInfo())
            {
                if (u->ToCreature()->GetCreatureInfo()->expansion < 1 && pl->getLevel() >= 60)
                    gain *= 0.1;
                else if (u->ToCreature()->GetCreatureInfo()->expansion < 2 && pl->getLevel() >= 70)
                    gain *= 0.1;
                else if (u->ToCreature()->GetCreatureInfo()->expansion < 3 && pl->getLevel() >= 80)
                    gain *= 0.1;
            }

            sScriptMgr->OnGainCalculation(gain, pl, u);
            return gain;
        }

        inline uint32 BattlegroundVictoryXP(uint8 level)
        {
            uint32 gain = 0;
            if (level <= 14)
                gain = 1960;
            else if (level <= 19)
                gain = 3280;
            else if (level <= 24)
                gain = 4800;
            else if (level <= 29)
                gain = 6440;
            else if (level <= 34)
                gain = 8920;
            else if (level <= 39)
                gain = 10350;
            else if (level <= 44)
                gain = 12480;
            else if (level <= 49)
                gain = 13225;
            else if (level <= 54)
                gain = 16437;
            else if (level <= 59)
                gain = 19987;
            else if (level <= 64)
                gain = 43625;
            else if (level <= 69)
                gain = 73125;
            else if (level <= 74)
                gain = 155570;
            else if (level <= 79)
                gain = 163740;
            else if (level <= 85)
                gain = 200000;

            return gain;
        }
        inline uint32 BattlegroundLossXP(uint8 level)
        {
            uint32 gain = BattlegroundVictoryXP(level)/2;

            return gain;
        }
        inline uint32 BattlegroundKillXP(uint8 level)
        {
            uint32 gain = 0;
            if (level <= 14)
                gain = 17;
            else if (level <= 19)
                gain = 21;
            else if (level <= 24)
                gain = 25;
            else if (level <= 29)
                gain = 27;
            else if (level <= 34)
                gain = 33;
            else if (level <= 39)
                gain = 35;
            else if (level <= 44)
                gain = 40;
            else if (level <= 49)
                gain = 44;
            else if (level <= 54)
                gain = 48;
            else if (level <= 59)
                gain = 52;
            else if (level <= 64)
                gain = 54;
            else if (level <= 69)
                gain = 57;
            else if (level <= 74)
                gain = 61;
            else if (level <= 79)
                gain = 68;
            else if (level <= 85)
                gain = 75;

            return gain;
        }

        inline float xp_in_group_rate(uint32 count, bool isRaid)
        {
            float rate;

            if (isRaid)
            {
                // FIXME: Must apply decrease modifiers depending on raid size.
                rate = 1.0f;
            }
            else
            {
                switch (count)
                {
                    case 0:
                    case 1:
                    case 2:
                        rate = 1.0f;
                        break;
                    case 3:
                        rate = 1.166f;
                        break;
                    case 4:
                        rate = 1.3f;
                        break;
                    case 5:
                    default:
                        rate = 1.4f;
                }
            }

            sScriptMgr->OnGroupRateCalculation(rate, count, isRaid);
            return rate;
        }
    }

    namespace GatherXP
    {
        inline uint8 GetContent (uint32 item_id, uint32 skill)
        {
            if (skill <= 275)
                return CONTENT_1_60;
            if (skill <= 305)       // 275 - 305
            {
                if(item_id != 181270 && item_id != 181555)
                    return CONTENT_1_60;        // Azeroth object with req. skill > 275
                else
                    return CONTENT_61_70;       // Felweed, Fel Iron Deposit
            }
            if (skill < 350)
                return CONTENT_61_70;
            if (skill <= 375)       // 350 - 375
            {
                if (item_id != 189973 && item_id != 191303 && item_id != 190169 && item_id != 189978 && item_id != 189979)
                    return CONTENT_61_70;
                else
                    return CONTENT_71_80;       // Goldclover, Firethorn, Tiger Lily, Cobalt Deposit, Rich Cobalt Deposit
            }
            if (skill < 425)
                return CONTENT_71_80;
            if (skill <= 450)       // 425 - 450
            {
                if (item_id != 202749 && item_id != 202748 && item_id != 202736 && item_id != 202747)
                    return CONTENT_71_80;
                else
                    return CONTENT_81_85;       // Azshara's Veil, Stormvine, Obsidium Deposit
            }

            return CONTENT_81_85;
        }

        inline uint8 GetItemLevel (uint32 item_id, uint32 skill, uint8 content)
        {
            switch (content)
            {
                case CONTENT_1_60:
                {
                    if (skill < 50)
                        skill = 50;
                    if (skill > 300)
                        skill = 300;
                    return skill / 5;
                }
                case CONTENT_61_70:
                {
                    if (skill < 300)
                        skill = 300;
                    if (skill > 350)
                        skill = 350;
                    return skill / 5;
                }
                case CONTENT_71_80:
                {
                    if (skill < 350)
                        skill = 350;
                    if (skill > 400)
                        skill = 400;
                    return skill / 5;
                }
                case CONTENT_81_85:
                {
                    return (skill + 1175) / 20;
                }
                default:
                {
                    return 0;
                }
            }
        }

        inline double GetBaseXp (uint32 item_id, uint32 skill, uint8 item_level)
        {
            uint8 content = GetContent (item_id, skill);

            switch (content)
            {
                case CONTENT_1_60:
                {
                    if (item_level < 50)
                        return 12.76 * item_level;
                    else
                        return 25 * item_level - 550;
                }
                case CONTENT_61_70:
                    return 20 * item_level - 200;
                case CONTENT_71_80:
                    return 100 * item_level - 6600;
                case CONTENT_81_85:
                    return 750 * item_level - 58250;
                default:
                    return 0;
            }
        }

        inline double GetLevelPenaltyCoef (uint8 pl_level, uint8 item_level)
        {
            if (pl_level <= item_level - 2)
                return 0.948;
            if (pl_level == item_level - 1)
                return 0.974;
            if (pl_level >= item_level && pl_level <= item_level + 5)
                return 1.000;
            if (pl_level == item_level + 6)
                return 0.800;
            if (pl_level == item_level + 7)
                return 0.600;
            if (pl_level == item_level + 8)
                return 0.400;
            if (pl_level == item_level + 9)
                return 0.200;
            if (pl_level >= item_level + 10)
                return 0.100;

            return 0;
        }

        inline uint32 Gain (uint8 pl_level, uint32 skill, uint32 item_id)
        {
            uint8 content = GetContent (item_id, skill);
            uint8 ilvl = GetItemLevel (item_id, skill, content);
            double xp = GetBaseXp (item_id, skill, ilvl);
            xp *= GetLevelPenaltyCoef (pl_level, ilvl);
            xp *= sWorld->getRate (RATE_XP_GATHER);

            return (uint32) xp;
        }
    }
}

#endif
