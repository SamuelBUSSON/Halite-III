#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"
#include "hlt/shipBrain.hpp"
#include <random>
#include <ctime>

using namespace std;
using namespace hlt;


int main(int argc, char* argv[]) {
    unsigned int rng_seed;
    if (argc > 1) {
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
    } else {
        rng_seed = static_cast<unsigned int>(time(nullptr));
    }
    mt19937 rng(rng_seed);

    Game game;
    // At this point "game" variable is populated with initial map data.
    // This is a good place to do computationally expensive start-up pre-processing.
    // As soon as you call "ready" function below, the 2 second per turn timer will start.
    game.ready("GuiSamBot");

    log::log("Successfully created bot! My Player ID is " + to_string(game.my_id) + ". Bot rng seed is " + to_string(rng_seed) + ".");
	
	BrainAI shipAi;

	BrainAI::game = &game;

    for (;;) {
        game.update_frame();
        shared_ptr<Player> me = game.me;
        unique_ptr<hlt::GameMap>& game_map = game.game_map;

        vector<Command> command_queue;

		BrainAI::dropOffCreated = false;
        for (const auto& ship_iterator : me->ships) {

            shared_ptr<Ship> ship = ship_iterator.second;	

			BrainAI::ship = ship;
			shipAi.update();

			command_queue.push_back(ship->executeCommand);
        }

		//Spawn ships
        if (
            game.turn_number <= 350 &&
            me->halite >= constants::SHIP_COST &&
            !game_map->at(me->shipyard)->is_occupied() &&
			game.me->ships.size() < 7 &&
			me->halite >= 1500
			)
        {
            command_queue.push_back(me->shipyard->spawn());
        }

        if (!game.end_turn(command_queue)) {
            break;
        }
    }

    return 0;
}
