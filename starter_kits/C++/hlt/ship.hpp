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
		Position goalPosition;
		std::shared_ptr<Ship> enemy;
		bool goingToDeposit;
		Position dropPoint;

		Halite halite;

        Ship(PlayerId player_id, EntityId ship_id, int x, int y, Halite halite) :
            Entity(player_id, ship_id, x, y),
            halite(halite)
        {
			executeCommand = stay_still();
			goingToDeposit = false;
		}

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
