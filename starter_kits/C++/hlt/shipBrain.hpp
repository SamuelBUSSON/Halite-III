#pragma once

#include "entity.hpp"
#include "constants.hpp"
#include "command.hpp"
#include "game.hpp"

#include <memory>
#include "BrainTree.h"


#define HALITE_STORAGE 500
#define DROPOFF_COST 7000
#define DIST_DROP_OFF 7

namespace hlt {	


	class UtilsMath
	{
		public:
			static float lerp(float a, float b, float t)
			{
				return a + t * (b - a);
			}

			static float invLerp(float a, float b, float t)
			{
				return (t - a)/(b - a);
			}
	};



	
	class CheckStorageInf;
	class FindHalite;
	class GoTo;
	class CheckStorageSup;
	class DropHalite;
	class CountNbrTurnsLeft;
	class CheckDistFromDropPoint;
	class ConvertToDropPoint;
	class CheckStorageTotalSup;

	class BrainAI
	{
		private:
			std::shared_ptr<BrainTree::Node> tree;

		public:			

			static Game * game;
			static std::shared_ptr<Ship> ship;
			static bool dropOffCreated;

			BrainAI()
			{
				buildTree();
			}

			void buildTree()
			{
				tree = BrainTree::Builder()
					.composite<BrainTree::Selector>()
						.composite<BrainTree::Sequence>()
							.leaf<CheckStorageTotalSup>(DROPOFF_COST)
							.leaf<CountNbrTurnsLeft>(150)
							.leaf<CheckDistFromDropPoint>(DIST_DROP_OFF)
							.leaf<ConvertToDropPoint>()
						.end()
						.composite<BrainTree::Sequence>()
							.leaf<CheckStorageSup>(HALITE_STORAGE)
							.leaf<DropHalite>()
							.leaf<GoTo>()
						.end()
						.composite<BrainTree::Sequence>()
							.leaf<CheckStorageInf>(HALITE_STORAGE)
							.leaf<FindHalite>()
							.leaf<GoTo>()
						.end()
					.end()
					.build();
			}

			void update()
			{
				tree->update();
			}
	};


	class CheckStorageInf : public BrainTree::Node
	{
		private:
			float amount;
		public:
			CheckStorageInf(float val) : amount(val){}

		Status update() override
		{
			//log::log("Check Storage " + std::to_string(BrainAI::ship->halite));
			return BrainAI::ship->halite < amount ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class CheckStorageSup : public BrainTree::Node
	{
		private:
			float amount;
		public:
			CheckStorageSup(float val) : amount(val) {}

		Status update() override
		{
			//log::log("Check Storage " + std::to_string(BrainAI::ship->halite));
			return BrainAI::ship->halite > amount ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class CheckStorageTotalSup : public BrainTree::Node
	{
	private:
		float amount;
	public:
		CheckStorageTotalSup(float val) : amount(val) {}

		Status update() override
		{
			//log::log("Check Storage " + std::to_string(BrainAI::ship->halite));
			return BrainAI::game->me->halite > amount ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class DropHalite : public BrainTree::Node
	{
		Status update() override
		{
			Position p;
			MapCell *dropOffPosition;
			int dist(INT_MAX);

			for (int x = 0; x < BrainAI::game->game_map->width; ++x)
			{
				for (int y = 0; y < BrainAI::game->game_map->height; ++y)
				{
					p.x = x;
					p.y = y;

					if (BrainAI::game->game_map->at(p)->has_structure() && 
						BrainAI::game->game_map->at(p)->structure->owner == BrainAI::game->me->id &&
						BrainAI::game->game_map->calculate_distance(BrainAI::ship->position, p) < dist)
					{
						dist = BrainAI::game->game_map->calculate_distance(BrainAI::ship->position, p);

						//log::log("Find drop point " + std::to_string(BrainAI::ship->halite));
						dropOffPosition = BrainAI::game->game_map->at(p);
					}		
				}
			}

			BrainAI::ship->goalPosition = &dropOffPosition->position;

			return Node::Status::Success;
		}
	};

	class FindHalite : public BrainTree::Node
	{
		Status update() override
		{
			Position shipPos = BrainAI::ship->position;
			MapCell *halitePosition;

			float intereset(0.0);
			float tempIntereset;

			Position p;	
			
			for (int x = 0; x < BrainAI::game->game_map->width; ++x)
			{
				for (int y = 0; y < BrainAI::game->game_map->height; ++y)
				{
					p.x = x;
					p.y = y;
					tempIntereset = calculateInterest(BrainAI::game->game_map->at(p)->halite, BrainAI::game->game_map->calculate_distance(p, shipPos));
					if (tempIntereset > intereset)
					{
						intereset = tempIntereset;
						halitePosition = BrainAI::game->game_map->at(p);
					}
				}
			}

			BrainAI::ship->goalPosition = &halitePosition->position;
			//log::log("FindHalite x:" + std::to_string(halitePosition->position.x) + " y : " + std::to_string(halitePosition->position.y) + " amount : " + std::to_string(halitePosition->halite));
			return intereset >= 0.0 ? Node::Status::Success : Node::Status::Failure;
		}

		float calculateInterest(float halite, float distance) 
		{
			//distance 0 --> 10
			//Halite 0 --> 1000
			//Max = Distance = 0 && halite = 1000
			//Min = Distance = 10 && halite = 0
			float haliteWeight = UtilsMath::invLerp(0, 1000, halite);
			float distanceWeight = 1 - UtilsMath::invLerp(0, 10, distance);

			return haliteWeight * distanceWeight;

		}
	};

	class GoTo : public BrainTree::Node
	{
		Status update() override
		{
			AStar aStar;
			Position &s = BrainAI::ship->position;
			Position &e = *BrainAI::ship->goalPosition;
            if(aStar.search(s,e,*BrainAI::game->game_map)) {
                log::log("We found the STARRRRR");
                BrainAI::ship->executeCommand = BrainAI::ship->move(BrainAI::game->game_map->naive_navigate(BrainAI::ship, *BrainAI::ship->goalPosition));
            }else {
                log::log("I guess we did not find the starrrrr");
                //BrainAI::ship->executeCommand = BrainAI::ship->move(Direction::STILL);
            }



            return Node::Status::Success;
		}
	};

	class CountNbrTurnsLeft : public BrainTree::Node
	{
		private:
			int countTurns;
		public:
			CountNbrTurnsLeft(int val) : countTurns(val) {}

		Status update() override
		{
			log::log("Turn Left " + std::to_string(constants::MAX_TURNS - BrainAI::game->turn_number));

			return  constants::MAX_TURNS - BrainAI::game->turn_number > countTurns ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class CheckDistFromDropPoint : public BrainTree::Node
	{
		private:
			int dist;
		public:
			CheckDistFromDropPoint(int val) : dist(val) {}

		Status update() override
		{
			Position p;			

			if (BrainAI::dropOffCreated)
			{
				return Node::Status::Failure;
			}

			for (int x = 0; x < BrainAI::game->game_map->width; ++x)
			{
				for (int y = 0; y < BrainAI::game->game_map->height; ++y)
				{
					p.x = x;
					p.y = y;
					if (BrainAI::game->game_map->at(p)->has_structure() &&
						BrainAI::game->game_map->at(p)->structure->owner == BrainAI::game->me->id &&
						BrainAI::game->game_map->calculate_distance(BrainAI::ship->position, p) > dist)
					{
						BrainAI::dropOffCreated = true;

						return Node::Status::Success;
					}					
				}
			}

			return Node::Status::Failure;
		}
	};

	class ConvertToDropPoint :  public BrainTree::Node
	{
		Status update() override
		{

			log::log("Create Drop Off " +  std::to_string(BrainAI::ship->id));

			BrainAI::ship->executeCommand = BrainAI::ship->make_dropoff();
			return  Node::Status::Success;
		}
	};


	Game * BrainAI::game = nullptr;
	std::shared_ptr<Ship> BrainAI::ship = nullptr;
	bool BrainAI::dropOffCreated = false;
}


