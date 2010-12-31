#ifndef __CACTUSEMU_PALADINPLAYER_H
#define __CACTUSEMU_PALADINPLAYER_H

#include "Player.h"

class PaladinPlayer: public Player
{
public:
    explicit PaladinPlayer(WorldSession * session);
    virtual uint8 getClass() const { return CLASS_PALADIN; }

    uint8 GetHolyPower();
    void SetHolyPower(uint8 value);
};

#endif
