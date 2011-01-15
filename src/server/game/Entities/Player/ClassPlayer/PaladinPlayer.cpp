#include "PaladinPlayer.h"

PaladinPlayer::PaladinPlayer(WorldSession* session): Player(session)
{
}

uint8 PaladinPlayer::GetHolyPower()
{
    uint8 power = GetPower(POWER_HOLY_POWER);
    return power > 3 ? 3 : power;
}

void PaladinPlayer::SetHolyPower(uint8 value)
{
    if(value > 3)
        SetPower(POWER_HOLY_POWER,3);
    else
        SetPower(POWER_HOLY_POWER,value);
}
