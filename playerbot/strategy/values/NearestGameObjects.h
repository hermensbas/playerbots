#pragma once
#include "../Value.h"
#include "../../PlayerbotAIConfig.h"

namespace ai
{
    class NearestGameObjects : public ObjectGuidListCalculatedValue
	{
	public:
        NearestGameObjects(PlayerbotAI* ai, float range = sPlayerbotAIConfig.sightDistance, bool ignoreLos = false) :
            ObjectGuidListCalculatedValue(ai), range(range) , ignoreLos(ignoreLos) {}

    protected:
        virtual list<ObjectGuid> Calculate();

    private:
        float range;
        bool ignoreLos;
	};
}
