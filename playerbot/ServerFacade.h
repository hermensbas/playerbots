#ifndef _ServerFacade_H
#define _ServerFacade_H

#include "Common.h"
#include "Unit.h"
#include "Player.h"
#ifdef CMANGOS
#include "Entities/GameObject.h"
#endif
#ifdef MANGOS
#include "Object/GameObject.h"
#endif
#include "PlayerbotAIBase.h"
#include "PlayerbotAIConfig.h"

using namespace std;

class ServerFacade
{
    public:
        ServerFacade();
        virtual ~ServerFacade();
        static ServerFacade& instance()
        {
            static ServerFacade instance;
            return instance;
        }

	public:
        bool UnitIsDead(Unit *unit)
        {
#ifdef MANGOS
            return unit->IsDead();
#endif
#ifdef CMANGOS
            return unit->IsDead();
#endif
        }

        float GetDistance2d(Unit *unit, WorldObject* wo);

        float GetDistance2d(Unit *unit, float x, float y);

        DeathState GetDeathState(Unit *unit)
        {
#ifdef MANGOS
#ifndef MANGOSBOT_TWO
            return unit->GetDeathState();
#else
            return unit->getDeathState();
#endif
#endif
#ifdef CMANGOS
            return unit->GetDeathState();
#endif
        }

        bool isSpawned(GameObject *go)
        {
#ifdef MANGOS
            return go->isSpawned();
#endif
#ifdef CMANGOS
            return go->IsSpawned();
#endif
        }

        bool IsAlive(Unit *unit)
        {
#ifdef MANGOS
            return unit->IsAlive();
#endif
#ifdef CMANGOS
            return unit->IsAlive();
#endif
        }

        bool isMoving(Unit *unit);

        bool IsInCombat(Unit *unit)
        {
#ifdef MANGOS
            return unit->IsInCombat();
#endif
#ifdef CMANGOS
            return unit->IsInCombat();
#endif
        }

        bool IsFrozen(Unit *unit)
        {
#ifdef MANGOS
            return unit->IsFrozen();
#endif
#ifdef CMANGOS
            return unit->isFrozen();
#endif
        }

        bool IsInRoots(Unit *unit)
        {
#ifdef MANGOS
            return unit->IsInRoots();
#endif
#ifdef CMANGOS
            return unit->isInRoots();
#endif
        }

        bool IsCharmed(Unit *unit)
        {
#ifdef MANGOS
            return unit->GetCharmerGuid() && unit->GetCharmerGuid().IsPlayer();
#endif
#ifdef CMANGOS
            return unit->HasCharmer();
#endif
        }

        bool IsFeared(Unit *unit)
        {
#ifdef MANGOS
#ifndef MANGOSBOT_TWO
            return unit->IsFeared();
#else
            return unit->isFeared();
#endif
#endif
#ifdef CMANGOS
            return unit->isFeared();
#endif
        }

        bool IsInFront(Unit *unit, WorldObject const* target, float distance,  float arc /*= M_PI_F*/)
        {
#ifdef MANGOS
            return unit->IsInFront(target, distance, arc);
#endif
#ifdef CMANGOS
            return unit->isInFront(target, distance, arc);
#endif
        }

        HostileRefManager& GetHostileRefManager(Unit *unit)
        {
#ifdef MANGOS
            return unit->GetHostileRefManager();
#endif
#ifdef CMANGOS
            return unit->getHostileRefManager();
#endif
        }

        ThreatManager& GetThreatManager(Unit *unit)
        {
#ifdef MANGOS
            return unit->GetThreatManager();
#endif
#ifdef CMANGOS
            return unit->getThreatManager();
#endif
        }

        void SendPacket(Player *player, WorldPacket &packet)
        {
#ifdef MANGOS
            return player->GetSession()->SendPacket(&packet);
#endif
#ifdef CMANGOS
            return player->GetSession()->SendPacket(packet);
#endif
        }

        void SendMessageToSet(Player *player, WorldPacket &packet, bool self)
        {
#ifdef MANGOS
            return player->SendMessageToSet(&packet, self);
#endif
#ifdef CMANGOS
            return player->SendMessageToSet(packet, self);
#endif
        }

        SpellEntry const* LookupSpellInfo(uint32 spellId)
        {
#ifdef MANGOS
            return sSpellStore.LookupEntry(spellId);
#endif
#ifdef CMANGOS
            return sSpellTemplate.LookupEntry<SpellEntry>(spellId);
#endif
        }

        uint32 GetSpellInfoRows()
        {
#ifdef MANGOS
            return sSpellStore.GetNumRows();
#endif
#ifdef CMANGOS
            return sSpellTemplate.GetMaxEntry();
#endif
        }

        bool IsWithinLOSInMap(Player* bot, WorldObject *wo)
        {
#ifdef MANGOS
            return bot->IsWithinLOSInMap(wo);
#endif
#ifdef CMANGOS
            float x = wo->GetPositionX(), y = wo->GetPositionY(), z = wo->GetPositionZ();
            return //bot->GetMapId() == wo->GetMapId() &&
                (
                    bot->IsWithinLOS(x, y, z + 0.5f, true) ||
                    GetDistance2d(bot, wo) <= sPlayerbotAIConfig.tooCloseDistance
                );
#endif
        }

        bool IsDistanceLessThan(float dist1, float dist2);
        bool IsDistanceGreaterThan(float dist1, float dist2);
        bool IsDistanceGreaterOrEqualThan(float dist1, float dist2);
        bool IsDistanceLessOrEqualThan(float dist1, float dist2);

        void SetFacingTo(Player* bot, WorldObject* wo, bool force = false);

        bool IsFriendlyTo(Unit* bot, Unit* to);
        bool IsFriendlyTo(WorldObject* bot, Unit* to);
        bool IsHostileTo(Unit* bot, Unit* to);
        bool IsHostileTo(WorldObject* bot, Unit* to);

        bool IsSpellReady(Player* bot, uint32 spell);

        bool IsUnderwater(Unit *unit);
        FactionTemplateEntry const* GetFactionTemplateEntry(Unit *unit);
        Unit* GetChaseTarget(Unit* target);
};

#define sServerFacade ServerFacade::instance()

#endif
