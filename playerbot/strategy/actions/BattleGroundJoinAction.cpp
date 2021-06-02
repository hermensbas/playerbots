#include "ObjectGuid.h"
#include "botpch.h"
#include "../../playerbot.h"
#include "../../playerbotAI.h"
#include "LfgActions.h"
#include "../../AiFactory.h"
//#include "../../PlayerbotAIConfig.h"
//#include "../ItemVisitors.h"
#include "../../RandomPlayerbotMgr.h"
//#include "../../../../game/LFGMgr.h"
//#include "strategy/values/PositionValue.h"
//#include "ServerFacade.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "strategy/values/LastMovementValue.h"
#include "strategy/actions/LogLevelAction.h"
#include "strategy/values/LastSpellCastValue.h"
#include "../values/PositionValue.h"
#include "MovementActions.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
//#include "../values/PositionValue.h"
#include "MotionGenerators/TargetedMovementGenerator.h"
#include "BattleGround.h"
#include "BattleGroundMgr.h"
#include "BattlegroundJoinAction.h"

using namespace ai;


bool BGJoinAction::Execute(Event event)
{
   if (!sPlayerbotAIConfig.randomBotJoinBG)
      return false;

   if (!bot->CanJoinToBattleground())
       return false;

   if (bot->IsDead())
      return false;

   if (bot->getLevel() < 10)
      return false;

   if (bot->IsBeingTeleported())
      return false;

   Map* map = bot->GetMap();
   if (map && map->Instanceable())
      return false;

   uint32 queueType = AI_VALUE(uint32, "bg type");
   return JoinQueue(queueType);
}

bool BGJoinAction::JoinQueue(uint32 type)
{
    // ignore if player is already in BG
    if (bot->InBattleGround())
        return false;

    // get BG TypeId
    BattleGroundQueueTypeId queueTypeId = BattleGroundQueueTypeId(type);
    BattleGroundTypeId bgTypeId = sServerFacade.BgTemplateId(queueTypeId);

    BattleGround* bg = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);
    if (!bg)
        return false;

    // check Deserter debuff
    if (!bot->CanJoinToBattleground())
        return false;

    // check if already in queue
    if (bot->InBattleGroundQueueForBattleGroundQueueType(queueTypeId))
        return false;

   if (bot->getLevel() < bg->GetMinLevel())
      return false;

   // check if has free queue slots
   if (!bot->HasFreeBattleGroundQueueId())
   {
       return false;
   }

   // get BattleMaster unit
   Unit* unit = ai->GetUnit(AI_VALUE2(CreatureDataPair const*, "bg master", bgTypeId));
   if (!unit)
   {
       sLog.outError("Bot %d could not find Battlemaster to join", bot->GetGUIDLow());
       return false;
   }
   // get BG MapId
#ifdef MANGOSBOT_ZERO
   uint32 mapId = GetBattleGrounMapIdByTypeId(bgTypeId);
#else
   uint32 bgTypeId_ = bgTypeId;
#endif
   uint32 instanceId = 0; // 0 = First Available
   bool joinAsGroup = bot->GetGroup() && bot->GetGroup()->GetLeaderGuid() == bot->GetObjectGuid() ? true : false;
   bool isPremade = false;
   bool isArena = false;
   bool isRated = false;
   uint8 arenaslot = 0;
   uint8 asGroup = false;
   string _bgType;

   switch (bgTypeId)
   {
   case BATTLEGROUND_AV:
       _bgType = "AV";
       break;
   case BATTLEGROUND_WS:
       _bgType = "WSG";
       break;
   case BATTLEGROUND_AB:
       _bgType = "AB";
       break;
#ifndef MANGOSBOT_ZERO
   case BATTLEGROUND_EY:
       _bgType = "EotS";
       break;
#endif
   default:
       break;
}

#ifndef MANGOSBOT_ZERO
   ArenaType arenaType = sServerFacade.BgArenaType(queueTypeId);
   if (arenaType != ARENA_TYPE_NONE)
   {
       isArena = true;
       isRated = ai->GetAiObjectContext()->GetValue<uint32>("arena type")->Get();

       if (joinAsGroup)
           asGroup = true;

       switch (arenaType)
       {
       case ARENA_TYPE_2v2:
           arenaslot = 0;
           _bgType = "2v2";
           break;
       case ARENA_TYPE_3v3:
           arenaslot = 1;
           _bgType = "3v3";
           break;
       case ARENA_TYPE_5v5:
           arenaslot = 2;
           _bgType = "5v5";
           break;
       default:
           break;
       }
   }
#endif


   WorldPacket packet(CMSG_BATTLEMASTER_JOIN, 20);
#ifdef MANGOSBOT_ZERO
   packet << unit->GetObjectGuid() << mapId << instanceId << joinAsGroup;
   sLog.outBasic("Bot #%d %s:%d <%s> queued %s", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->getLevel(), bot->GetName(), _bgType);
#else
   sLog.outBasic("Bot #%d %s:%d <%s> queued %s %s", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->getLevel(), bot->GetName(), _bgType, isRated ? "Rated Arena" : isArena ? "Arena" : "");
   if (!isArena)
   {
       packet << guid << bgTypeId_ << instanceId << joinAsGroup;
   }
   else
   {
       WorldPacket arena_packet(CMSG_BATTLEMASTER_JOIN_ARENA, 20);
       arena_packet << guid << arenaslot << asGroup << isRated;
       bot->GetSession()->HandleBattlemasterJoinArena(arena_packet);
       return true;
   }
#endif
   bot->GetSession()->HandleBattlemasterJoinOpcode(packet);
   return true;
}

bool BGLeaveAction::Execute(Event event)
{
    if (!bot->InBattleGroundQueue())
        return false;

    uint32 queueType = AI_VALUE(uint32, "bg type");
    if (!queueType)
        return false;

    ai->ChangeStrategy("-bg", BOT_STATE_NON_COMBAT);

    BattleGroundQueueTypeId queueTypeId = bot->GetBattleGroundQueueTypeId(0);
    BattleGroundTypeId _bgTypeId = sServerFacade.BgTemplateId(queueTypeId);
    uint8 type = false;
    uint16 unk = 0x1F90;
    uint8 unk2 = 0x0;
    bool isArena = false;
    bool IsRandomBot = sRandomPlayerbotMgr.IsRandomBot(bot->GetGUIDLow());

#ifndef MANGOSBOT_ZERO
    ArenaType arenaType = sServerFacade.BgArenaType(queueTypeId);
    if (arenaType != ARENA_TYPE_NONE)
    {
        isArena = true;
        type = arenaType;
    }
#endif
    sLog.outDetail("Bot #%d %s:%d <%s> leaves %s queue", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->getLevel(), bot->GetName(), isArena ? "Arena" : "BG");

    WorldPacket packet(CMSG_BATTLEFIELD_PORT, 20);
#ifdef MANGOSBOT_ZERO
    uint32 mapId = GetBattleGrounMapIdByTypeId(_bgTypeId);
    packet << mapId << uint8(0);
#else
    packet << type << unk2 << (uint32)_bgTypeId << unk << uint8(0);
#endif
#ifdef MANGOS
    bot->GetSession()->HandleBattleFieldPortOpcode(packet);
#endif
#ifdef CMANGOS
    bot->GetSession()->HandleBattlefieldPortOpcode(packet);
#endif
    if (IsRandomBot)
        ai->SetMaster(NULL);

    ai->ResetStrategies(!IsRandomBot);
    ai->GetAiObjectContext()->GetValue<uint32>("bg type")->Set(NULL);
    ai->GetAiObjectContext()->GetValue<uint32>("bg role")->Set(NULL);
    ai->GetAiObjectContext()->GetValue<uint32>("arena type")->Set(NULL);
    return true;
}

bool BGStatusAction::Execute(Event event)
{
    uint32 QueueSlot;
    uint32 instanceId;
    uint32 mapId;
    uint32 statusid;
    uint32 Time1;
    uint32 Time2;
    uint8 unk1;
    string _bgType;

#ifndef MANGOSBOT_ZERO
    uint64 arenatype;
    uint64 arenaByte;
    uint8 arenaTeam;
    uint8 isRated;
    uint64 unk0;
    uint64 x1f90;
    uint8 minlevel;
    uint8 maxlevel;
    uint64 bgTypeId;
    uint32 battleId;
#endif

    WorldPacket p(event.getPacket());
    statusid = 0;
#ifndef MANGOSBOT_ZERO
    p >> QueueSlot; // queue id (0...2) - player can be in 3 queues in time
    p >> arenaByte;
    if (arenaByte == 0)
        return false;
#ifdef MANGOSBOT_TWO
    p >> minlevel;
    p >> maxlevel;
#endif
    p >> instanceId;
    p >> isRated;
    p >> statusid;

    // check status
    switch (statusid)
    {
    case STATUS_WAIT_QUEUE:                  // status_in_queue
        p >> Time1;                         // average wait time, milliseconds
        p >> Time2;                        // time in queue, updated every minute!, milliseconds
        break;
    case STATUS_WAIT_JOIN:                   // status_invite
        p >> mapId;    //sLog.outBasic("mapId %d!", mapId);                    // map id
#ifdef MANGOSBOT_TWO
        p >> unk0;
#endif
        p >> Time1;   //sLog.outBasic("Time1 %d!", Time1);                      // time to remove from queue, milliseconds
        break;
    case STATUS_IN_PROGRESS:                  // status_in_progress
        p >> mapId;                          // map id
#ifdef MANGOSBOT_TWO
        p >> unk0;
#endif
        p >> Time1;                         // time to bg auto leave, 0 at bg start, 120000 after bg end, milliseconds
        p >> Time2;                        // time from bg start, milliseconds
        p >> arenaTeam;
        break;
    default:
        sLog.outError("Unknown BG status!");
        break;
    }
#else
    p >> QueueSlot; // queue id (0...2) - player can be in 3 queues in time
    p >> mapId;     // MapID
    if (mapId == 0)
        return false;
    p >> unk1;      // Unknown
    p >> instanceId;
    p >> statusid;  // status

    // check status
    switch (statusid)
    {
    case STATUS_WAIT_QUEUE:                  // status_in_queue
        p >> Time1;                         // average wait time, milliseconds
        p >> Time2;                        // time in queue, updated every minute!, milliseconds
        break;
    case STATUS_WAIT_JOIN:                   // status_invite
        p >> Time1;                         // time to remove from queue, milliseconds
        break;
    case STATUS_IN_PROGRESS:                 // status_in_progress
        p >> Time1;                         // time to bg auto leave, 0 at bg start, 120000 after bg end, milliseconds
        p >> Time2;                        // time from bg start, milliseconds
        break;
    default:
        sLog.outError("Unknown BG status!");
        break;
    }
#endif

    bool IsRandomBot = sRandomPlayerbotMgr.IsRandomBot(bot->GetGUIDLow());
    BattleGroundQueueTypeId queueTypeId = bot->GetBattleGroundQueueTypeId(QueueSlot);
    BattleGroundTypeId _bgTypeId = sServerFacade.BgTemplateId(queueTypeId);
    bool isArena = false;

    uint8 type = false;                                             // arenatype if arena
    //uint32 bgTypeId_ = _bgTypeId;                                       // type id from dbc
    uint16 unk = 0x1F90;
    uint8 unk2 = 0x0;
    uint8 action = 0x1;

#ifndef MANGOSBOT_ZERO
    ArenaType arenaType = sServerFacade.BgArenaType(queueTypeId);
    if (arenaType != ARENA_TYPE_NONE)
    {
        isArena = true;
        type = arenaType;
    }
#endif

    switch (_bgTypeId)
    {
    case BATTLEGROUND_AV:
        _bgType = "AV";
        break;
    case BATTLEGROUND_WS:
        _bgType = "WSG";
        break;
    case BATTLEGROUND_AB:
        _bgType = "AB";
        break;
#ifndef MANGOSBOT_ZERO
    case BATTLEGROUND_EY:
        _bgType = "EotS";
        break;
#endif
    default:
        break;
    }

#ifndef MANGOSBOT_ZERO
    switch (arenaType)
    {
    case ARENA_TYPE_2v2:
        _bgType = "2v2";
        break;
    case ARENA_TYPE_3v3:
        _bgType = "3v3";
        break;
    case ARENA_TYPE_5v5:
        _bgType = "5v5";
        break;
    default:
        break;
    }
#endif

    if (Time1 == TIME_TO_AUTOREMOVE) //battleground is over, bot needs to leave
    {
        BattleGround* bg = bot->GetBattleGround();
        if (bg)
        {
            BattleGroundBracketId bracketId = bot->GetBattleGround()->GetBracketId();
            uint32 TeamId = bot->GetTeam() == ALLIANCE ? 0 : 1;
#ifndef MANGOSBOT_ZERO
            if (isArena)
            {
                sRandomPlayerbotMgr.ArenaBots[queueTypeId][bracketId][isRated][TeamId]--;
                TeamId = isRated ? 1 : 0;
            }
#endif
            sRandomPlayerbotMgr.BgBots[queueTypeId][bracketId][TeamId]--;
            sRandomPlayerbotMgr.BgPlayers[queueTypeId][bracketId][TeamId] = 0;
        }

        // remove warsong strategy
        if (IsRandomBot)
            ai->SetMaster(NULL);
        ai->ChangeStrategy("-warsong", BOT_STATE_COMBAT);
        ai->ChangeStrategy("-warsong", BOT_STATE_NON_COMBAT);
        ai->ChangeStrategy("-arathi", BOT_STATE_COMBAT);
        ai->ChangeStrategy("-arathi", BOT_STATE_NON_COMBAT);
        ai->ChangeStrategy("-battleground", BOT_STATE_COMBAT);
        ai->ChangeStrategy("-battleground", BOT_STATE_NON_COMBAT);
        ai->ChangeStrategy("-arena", BOT_STATE_COMBAT);
        ai->ChangeStrategy("-arena", BOT_STATE_NON_COMBAT);
        sLog.outBasic("Bot #%d <%s> leaves %s (%s).", bot->GetGUIDLow(), bot->GetName(), isArena ? "Arena" : "BG", _bgType);

        WorldPacket packet(CMSG_LEAVE_BATTLEFIELD);
        packet << uint8(0);
        packet << uint8(0);                           // BattleGroundTypeId-1 ?
#ifdef MANGOSBOT_ZERO
        packet << uint16(0);
#else
        packet << uint32(0);
        packet << uint16(0);
#endif
        bot->GetSession()->HandleLeaveBattlefieldOpcode(packet);
        //bot->GetSession()->QueuePacket(packet);
        ai->ResetStrategies(!IsRandomBot);
        ai->GetAiObjectContext()->GetValue<uint32>("bg type")->Set(NULL);
        ai->GetAiObjectContext()->GetValue<uint32>("bg role")->Set(NULL);
        ai->GetAiObjectContext()->GetValue<uint32>("arena type")->Set(NULL);
        ai::PositionMap& posMap = context->GetValue<ai::PositionMap&>("position")->Get();
        ai::PositionEntry pos = context->GetValue<ai::PositionMap&>("position")->Get()["bg objective"];
        pos.Reset();
        posMap["bg objective"] = pos;
    }
    if (statusid == STATUS_WAIT_QUEUE) //bot is in queue
    {
        BattleGround* bg = sBattleGroundMgr.GetBattleGroundTemplate(_bgTypeId);
        if (!bg)
            return false;

        bool leaveQ = false;
        uint32 timer;
        if (_bgTypeId > 4 && _bgTypeId != 7)
            timer = TIME_TO_AUTOREMOVE + 90 * 1000;
        else
            timer = TIME_TO_AUTOREMOVE + 1000 * (bg->GetMaxPlayers() * 4);

        //if (Time2 > timer) Disabled to test 
        //    leaveQ = true;

        if (leaveQ && !(bot->GetGroup() || ai->GetMaster()))
        {
            uint32 TeamId = bot->GetTeam() == ALLIANCE ? 0 : 1;
            BattleGroundBracketId bracketId = BG_BRACKET_ID_TEMPLATE;
#ifdef CMANGOS
#ifdef MANGOSBOT_TWO
            BattleGround* bg = sBattleGroundMgr.GetBattleGroundTemplate(_bgTypeId);
            uint32 mapId = bg->GetMapId();
            PvPDifficultyEntry const* pvpDiff = GetBattlegroundBracketByLevel(mapId, bot->getLevel());
            if (pvpDiff)
                bracketId = pvpDiff->GetBracketId();

#else
            bracketId = bot->GetBattleGroundBracketIdFromLevel(_bgTypeId);
#endif
#endif
            bool realPlayers = sRandomPlayerbotMgr.BgPlayers[queueTypeId][bracketId][TeamId];
            if (realPlayers)
                return false;
            ai->ChangeStrategy("-bg", BOT_STATE_NON_COMBAT);
            sLog.outBasic("Bot #%u <%s> (%u %s) waited too long and leaves queue (%s %s).", bot->GetGUIDLow(), bot->GetName(), bot->getLevel(), bot->GetTeam() == ALLIANCE ? "A" : "H", isArena ? "Arena" : "BG", _bgType);
            WorldPacket packet(CMSG_BATTLEFIELD_PORT, 20);
            action = 0;
#ifdef MANGOSBOT_ZERO
            packet << mapId << action;
#else
            packet << type << unk2 << (uint32)_bgTypeId << unk << action;
#endif
#ifdef MANGOS
            bot->GetSession()->HandleBattleFieldPortOpcode(packet);
#endif
#ifdef CMANGOS
            bot->GetSession()->HandleBattlefieldPortOpcode(packet);
#endif
            ai->ResetStrategies(!IsRandomBot);
            ai->GetAiObjectContext()->GetValue<uint32>("bg type")->Set(NULL);
            ai->GetAiObjectContext()->GetValue<uint32>("bg role")->Set(NULL);
            ai->GetAiObjectContext()->GetValue<uint32>("arena type")->Set(NULL);
            sRandomPlayerbotMgr.BgBots[queueTypeId][bracketId][TeamId]--;
            return true;
        }
    }
    if (statusid == STATUS_WAIT_JOIN) //bot may join
    {
#ifndef MANGOSBOT_ZERO
        if (isArena)
        {
            isArena = true;
            BattleGroundQueue& bgQueue = sServerFacade.bgQueue(queueTypeId);
            GroupQueueInfo ginfo;
            if (!bgQueue.GetPlayerGroupInfoData(bot->GetObjectGuid(), &ginfo))
            {
                return false;
            }
#ifdef MANGOS
            if (ginfo.IsInvitedToBGInstanceGUID)
            {
                BattleGround* bg = sBattleGroundMgr.GetBattleGround(ginfo.IsInvitedToBGInstanceGUID, BATTLEGROUND_TYPE_NONE);
                if (!bg)
                {
                    return false;
                }

                _bgTypeId = bg->GetTypeID();
            }
#endif
#ifdef CMANGOS
            if (ginfo.isInvitedToBgInstanceGuid)
            {
                BattleGround* bg = sBattleGroundMgr.GetBattleGround(ginfo.isInvitedToBgInstanceGuid, BATTLEGROUND_TYPE_NONE);
                if (!bg)
                {
                    return false;
                }

                _bgTypeId = bg->GetTypeId();
            }
        }
#endif
#endif

#ifdef MANGOSBOT_ZERO
        sLog.outBasic("Bot #%d <%s> (%u %s) joined BG (%s)", bot->GetGUIDLow(), bot->GetName(), bot->getLevel(), bot->GetTeam() == ALLIANCE ? "A" : "H", _bgType);
#else
        sLog.outBasic("Bot #%d <%s> (%u %s) joined %s (%s)", bot->GetGUIDLow(), bot->GetName(), bot->getLevel(), bot->GetTeam() == ALLIANCE ? "A" : "H", isArena ? "Arena" : "BG", _bgType);
#endif
        bot->Unmount();

        WorldPacket packet(CMSG_BATTLEFIELD_PORT, 20);
#ifdef MANGOSBOT_ZERO
        packet << mapId << action;
#else
        packet << type << unk2 << (uint32)_bgTypeId << unk << action;
#endif
#ifdef MANGOS
        bot->GetSession()->HandleBattleFieldPortOpcode(packet);
#endif
#ifdef CMANGOS
        bot->GetSession()->HandleBattlefieldPortOpcode(packet);
#endif

        ai->ResetStrategies(false);
        //ai->ChangeStrategy("-bg,-rpg,-travel,-grind", BOT_STATE_NON_COMBAT);
        ai::PositionMap& posMap = context->GetValue<ai::PositionMap&>("position")->Get();
        ai::PositionEntry pos = context->GetValue<ai::PositionMap&>("position")->Get()["bg objective"];
        pos.Reset();
        posMap["bg objective"] = pos;

        return true;
    }

    if (statusid == STATUS_IN_PROGRESS) // placeholder for Leave BG if it takes too long
    {
        return true;
    }
    return true;
}

bool BGStatusCheckAction::Execute(Event event)
{
    if (bot->IsBeingTeleported())
        return false;

    if (!bot->InBattleGroundQueue())
        return false;

    if (bot->InBattleGround())
        return false;

    WorldPacket packet(CMSG_BATTLEFIELD_STATUS);
    bot->GetSession()->HandleBattlefieldStatusOpcode(packet);
    return true;
}

BattleGroundTypeId QueueAtBmAction::getBgTypeId(uint32 bgType)
{
    BattleGroundTypeId bgTypeId;
    switch (bgType)
    {
    case 0:
        bgTypeId = BATTLEGROUND_TYPE_NONE;
        break;
    case 1:
        bgTypeId = BATTLEGROUND_AV;
        break;
    case 2:
        bgTypeId = BATTLEGROUND_WS;
        break;
    case 3:
        bgTypeId = BATTLEGROUND_AB;
        break;
    }

    return bgTypeId;
}


bool QueueAtBmAction::canJoinBg(uint32 bgType)
{
    BattleGround* bg = sBattleGroundMgr.GetBattleGroundTemplate(getBgTypeId(bgType));
    if (!bg)
        return false;

    if (bot->getLevel() < bg->GetMinLevel())
        return false;

    // get BG TypeId
    BattleGroundQueueTypeId queueTypeId = BattleGroundQueueTypeId(getBgTypeId(bgType));
    BattleGroundTypeId bgTypeId = sServerFacade.BgTemplateId(queueTypeId);

    // check if already in queue
    if (bot->InBattleGroundQueueForBattleGroundQueueType(queueTypeId))
        return false;

    return true;
}

//Select a random BG type.
void QueueAtBmAction::GetRandomBg()
{
    vector<uint32> bgTypes;

    for (uint32 i = 1; i < MAX_BATTLEGROUND_TYPE_ID; i++)
    {
        if (canJoinBg(getBgTypeId(i)))
            bgTypes.push_back(i);
    }

    uint32 bgType = 0;
    if (!bgTypes.empty())
        bgType = bgTypes[urand(0, bgTypes.size() - 1)];

    Qualify(bgType);
}

//Only go to BM's at level > 9
bool QueueAtBmAction::isUseful()
{
    if (!ChooseMoveDoAction::isUseful())
        return false;

    if (bot->IsBeingTeleported())
        return false;

    if (bot->InBattleGround())
        return false;   

    if (getQualifier().empty())
        return false;

    return canJoinBg(getBgTypeId(stoi(getQualifier())));
};

//Check if BM is not hostile and preferably not alive.
bool QueueAtBmAction::IsValidBm(CreatureDataPair const* bmPair, bool allowDead)
{
    if (!bmPair)
        return false;

    CreatureInfo const* bmTemplate = ObjectMgr::GetCreatureTemplate(bmPair->second.id);

    if (!bmTemplate)
        return false;

    FactionTemplateEntry const* bmFactionEntry = sFactionTemplateStore.LookupEntry(bmTemplate->Faction);

    //Is the unit hostile?
    if (ai->getReaction(bmFactionEntry) < REP_NEUTRAL)
        return false;

    Unit* unit = ai->GetUnit(bmPair);

    if (!unit)
        return false;

    WorldPosition bmPos(bmPair);

    AreaTableEntry const* area = bmPos.getArea();

    if (!area)
        return false;

    //Is the area hostile?
    if (area->team == 4 && bot->GetTeam() == ALLIANCE)
        return false;
    if (area->team == 2 && bot->GetTeam() == HORDE)
        return false;

    if (!allowDead)
    {
        //Is the unit dead?
        if (unit->GetDeathState() == DEAD)
            return false;
    }

    return true;
}

//Try to find a friendly BM that is alive. Otherwise select a dead BM.
bool QueueAtBmAction::FilterPotentialTargets()
{
    list<CreatureDataPair const*> tmpList = potentialTargets;

    potentialTargets.remove_if([this](CreatureDataPair const* bmPair) {return !IsValidBm(bmPair, false); });

    if (potentialTargets.empty())
    {
        potentialTargets = tmpList;
        potentialTargets.remove_if([this](CreatureDataPair const* bmPair) {return !IsValidBm(bmPair, true); });
    }

    return !potentialTargets.empty();
}

bool QueueAtBmAction::ExecuteAction(Event event)
{
    if (!getObjectTarget())
        return false;

    CreatureDataPair const* bmPair = getObjectTarget()->Get();

    if (!bmPair)
        return false;

    Unit* bmUnit = ai->GetUnit(bmPair);

    if (!bmUnit)
        return false;

    bot->SetFacingTo(bot->GetAngle(bmUnit));

    //Work-around for getting BG-type from unit.
    for (uint32 i = 0; i < MAX_BATTLEGROUND_TYPE_ID; i++)
    {
        list<CreatureDataPair const*> bmPairs = AI_VALUE2(list<CreatureDataPair const*>, "bg masters", i);

        if (std::find(bmPairs.begin(), bmPairs.end(), bmPair) != bmPairs.end())
        {
            bot->GetPlayerbotAI()->GetAiObjectContext()->GetValue<uint32>("bg type")->Set(i);
            bot->GetPlayerbotAI()->ChangeStrategy("+bg", BOT_STATE_NON_COMBAT);
            return true;
        }
    }

    return false;
}