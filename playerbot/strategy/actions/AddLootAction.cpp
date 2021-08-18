#include "botpch.h"
#include "../../playerbot.h"
#include "AddLootAction.h"

#include "../../LootObjectStack.h"
#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

using namespace ai;
using namespace MaNGOS;

using namespace ai;

bool AddLootAction::Execute(Event event)
{
    ObjectGuid guid = event.getObject();
    if (!guid)
        return false;

    return AI_VALUE(LootObjectStack*, "available loot")->Add(guid);
}

bool AddAllLootAction::Execute(Event event)
{
    bool added = false;

    list<ObjectGuid> gos = context->GetValue<list<ObjectGuid> >("nearest game objects")->Get();
    for (list<ObjectGuid>::iterator i = gos.begin(); i != gos.end(); i++)
        added |= AddLoot(*i);

    list<ObjectGuid> corpses = context->GetValue<list<ObjectGuid> >("nearest corpses")->Get();
    for (list<ObjectGuid>::iterator i = corpses.begin(); i != corpses.end(); i++)
        added |= AddLoot(*i);

    return added;
}

bool AddLootAction::isUseful()
{
    return true;
}

bool AddAllLootAction::isUseful()
{
    return true;
}

bool AddAllLootAction::AddLoot(ObjectGuid guid)
{
    return AI_VALUE(LootObjectStack*, "available loot")->Add(guid);
}

bool AddGatheringLootAction::AddLoot(ObjectGuid guid)
{
    LootObject loot(bot, guid);

    WorldObject *wo = loot.GetWorldObject(bot);
    if (loot.IsEmpty() || !wo)
        return false;

    if (!sServerFacade.IsWithinLOSInMap(bot, wo))
        return false;

    if (loot.skillId == SKILL_NONE)
        return false;

    if (!loot.IsLootPossible(bot))
        return false;

    if (sServerFacade.IsDistanceGreaterThan(sServerFacade.GetDistance2d(bot, wo), INTERACTION_DISTANCE))
    {
        list<Unit*> targets;
        MaNGOS::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, sPlayerbotAIConfig.lootDistance);
        MaNGOS::UnitListSearcher<MaNGOS::AnyUnfriendlyUnitInObjectRangeCheck> searcher(targets, u_check);
        Cell::VisitAllObjects(wo, searcher, sPlayerbotAIConfig.spellDistance * 1.5);
        if (!targets.empty())
        {
            ostringstream out;
            out << "Kill that " << targets.front()->GetName() << " so I can loot freely";
            ai->TellError(out.str());
            return false;
        }
    }

    return AddAllLootAction::AddLoot(guid);
}
