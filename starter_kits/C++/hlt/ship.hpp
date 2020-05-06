#pragma once

#include "entity.hpp"
#include "constants.hpp"
#include "command.hpp"

#include <memory>


namespace hlt {
    struct Ship : Entity {

		//shared_ptr<BrainTree::BehaviorTree> tree;
		//static Game* game;

		Command executeCommand;
		Position * goalPosition;

		Halite halite;

        Ship(PlayerId player_id, EntityId ship_id, int x, int y, Halite halite) :
            Entity(player_id, ship_id, x, y),
            halite(halite)
        {
			executeCommand = stay_still();
			//CreatingBehaviorTreeUsingBuilders();
		}

		/*void CreatingBehaviorTreeUsingBuilders()
		{
		/*	tree = BrainTree::Builder()
				.composite<BrainTree::MemSequence>()
					.leaf<CheckStorage>(shared_ptr<Ship>(this))
					.leaf<FindHalite>(shared_ptr<Ship>(this))
					.leaf<GoTo>(shared_ptr<Ship>(this))
				.end()
				.build();			

			auto tree = BrainTree::Builder()
				.build();
		}*/

        bool is_full() const {
            return halite >= constants::MAX_HALITE;
        }

        Command make_dropoff() const {
            return hlt::command::transform_ship_into_dropoff_site(id);
        }

        Command move(Direction direction) const {
            return hlt::command::move(id, direction);
        }

        Command stay_still() const {
            return hlt::command::move(id, Direction::STILL);
        }

        static std::shared_ptr<Ship> _generate(PlayerId player_id);
    };

}
