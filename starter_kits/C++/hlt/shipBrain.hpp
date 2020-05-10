#pragma once

#include "entity.hpp"
#include "constants.hpp"
#include "command.hpp"
#include "game.hpp"
#include <time.h>       /* time */

#include <memory>
#include "BrainTree.h"


#define HALITE_STORAGE 400
#define DROPOFF_COST 6000
#define DIST_DROP_OFF 10
#define ENEMY_FLEE_DISTANCE 3
#define ENEMY_ATTACK_DISTANCE 2

namespace hlt 
{	
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

	#pragma region ForwardDeclaration



	class CheckStorageInf;
	class FindHalite;
	class GoTo;
	class CheckStorageSup;
	class DropHalite;
	class CountNbrTurnsLeft;
	class CheckDistFromDropPoint;
	class ConvertToDropPoint;
	class CheckStorageTotalSup;
	class FleeDecision;
	class FindCaseAwayFromEnemy;
	class SearchForEnemy;
	class IsGoingToDeposit;
	class AttackDecision;
	class AttackEnemy;
	class CountHaliteOnCase;
	class CountNbrTurnsLeftMin;


#pragma endregion

	class BrainAI
	{
		private:
			std::shared_ptr<BrainTree::Node> p_tree;

		public:			

			static Game * p_game;
			static std::shared_ptr<Ship> p_ship;
			static bool dropOffCreated;

			BrainAI()
			{
				buildTree();
			}

			void buildTree()
			{
				p_tree = BrainTree::Builder()
					.composite<BrainTree::Selector>()
						//FLEE SEQUENCE
						.composite<BrainTree::Sequence>()
							.leaf<SearchForEnemy>(ENEMY_FLEE_DISTANCE)
							.leaf<FleeDecision>()
							.leaf<FindCaseAwayFromEnemy>()
							.leaf<GoTo>()
						.end()
						//ATTACK SEQUENCE
						.composite<BrainTree::Sequence>()
							.leaf<SearchForEnemy>(ENEMY_ATTACK_DISTANCE)
							.leaf<AttackDecision>()
							.leaf<AttackEnemy>()
							.leaf<GoTo>()
						.end()
						//CONVERT TO DROP OFF SEQUENCE
						.composite<BrainTree::Sequence>()
							.leaf<CheckStorageTotalSup>(DROPOFF_COST)
							.leaf<CountNbrTurnsLeft>(150)
							.leaf<CheckDistFromDropPoint>(DIST_DROP_OFF)
							.leaf<ConvertToDropPoint>()
						.end()
						//DROP HALITE SEQUENCE
						.composite<BrainTree::Sequence>()
							.composite<BrainTree::Selector>()
								.leaf<CheckStorageSup>(HALITE_STORAGE)
								.leaf<IsGoingToDeposit>()
								.composite<BrainTree::Sequence>()
									.leaf<CountNbrTurnsLeftMin>(20)
									.leaf<CheckStorageSup>(HALITE_STORAGE * 0.25)
								.end()
							.end()
							.composite<BrainTree::Selector>()
								.leaf<CountHaliteOnCase>(false)
								.leaf<CheckStorageSup>(800)
							.end()
							.leaf<DropHalite>()
							.leaf<GoTo>()
						.end()
						//COLLECT HALITE SEQUENCE
						.composite<BrainTree::Sequence>()
							.composite<BrainTree::Selector>()
								.leaf<CheckStorageInf>(HALITE_STORAGE)
							.end()
							.leaf<CountHaliteOnCase>(false)
							.leaf<FindHalite>()
							.leaf<GoTo>()
						.end()
					.end()
					.build();
			}

			void update()
			{
				p_tree->update();
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
			return BrainAI::p_ship->halite < amount ? Node::Status::Success : Node::Status::Failure;
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
			return BrainAI::p_ship->halite > amount ? Node::Status::Success : Node::Status::Failure;
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
			return BrainAI::p_game->me->halite > amount ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class DropHalite : public BrainTree::Node
	{
		Status update() override
		{
			Position p;
			MapCell *dropOffPosition;
			int dist(INT_MAX);
			log::log("I am gonna to depsit " + std::to_string(BrainAI::p_ship->goingToDeposit));

			for (int x = 0; x < BrainAI::p_game->game_map->width; ++x)
			{
				p.x = x;
				for (int y = 0; y < BrainAI::p_game->game_map->height; ++y)
				{
					p.y = y;

					if (BrainAI::p_game->game_map->at(p)->has_structure() &&
						BrainAI::p_game->game_map->at(p)->structure->owner == BrainAI::p_game->me->id &&
						BrainAI::p_game->game_map->calculate_distance(BrainAI::p_ship->position, p) < dist)
					{
						dist = BrainAI::p_game->game_map->calculate_distance(BrainAI::p_ship->position, p);
						BrainAI::p_ship->dropPoint = p;
						//log::log("Find drop point " + std::to_string(BrainAI::ship->halite));
						dropOffPosition = BrainAI::p_game->game_map->at(p);
					}		
				}
			}

			BrainAI::p_ship->goalPosition = dropOffPosition->position;
			BrainAI::p_ship->goingToDeposit = true;

			return Node::Status::Success;
		}
	};


	class CountHaliteOnCase : public BrainTree::Node
	{
		private:
			bool isReturningSucess;

		public:
			CountHaliteOnCase(bool val) : isReturningSucess(val) {}

		Status update() override
		{
			if (BrainAI::p_game->game_map->at(BrainAI::p_ship->position)->halite > HALITE_STORAGE * 0.5f)
			{
				return isReturningSucess ? Node::Status::Success : Node::Status::Failure;
			}
			else {
				return !isReturningSucess ? Node::Status::Success : Node::Status::Failure;
			}
		}
	};

	class FindHalite : public BrainTree::Node
	{
		Status update() override
		{
			MapCell *halitePosition;

			float intereset(0.0);
			float tempIntereset;

			Position p;			

			int cost(0);

			for (int x = 0; x < BrainAI::p_game->game_map->width; ++x)
			{
				p.x = x;
				for (int y = 0; y < BrainAI::p_game->game_map->height; ++y)
				{
					p.y = y;
					cost = 0;

					tempIntereset = calculateInterest(BrainAI::p_game->game_map->at(p)->halite, BrainAI::p_game->game_map->calculate_distance(p, BrainAI::p_ship->position), cost);
					if (tempIntereset > intereset)
					{
						//cost = BrainAI::game->game_map->get_cost(BrainAI::ship, p);
						//tempIntereset = calculateInterest(BrainAI::game->game_map->at(p)->halite, BrainAI::game->game_map->calculate_distance(p, BrainAI::ship->position), cost);
						/*if (tempIntereset > intereset)
						{*/
							intereset = tempIntereset;
							halitePosition = BrainAI::p_game->game_map->at(p);
						//}
					}
				}
			}

			BrainAI::p_ship->goalPosition = halitePosition->position;
			//log::log("FindHalite x:" + std::to_string(halitePosition->position.x) + " y : " + std::to_string(halitePosition->position.y) + " amount : " + std::to_string(halitePosition->halite));
			return intereset >= 0.0 ? Node::Status::Success : Node::Status::Failure;
		}

		/**
		* @brief : Calculate interest to reach this position
		* @param halite Numbers of halite on posiiton
		* @param distance Distance to each position
		* @param totalCost //Unused atm
		* @return Value between 0 and 1 depending the interest too reach the position
		*
		*/		
		float calculateInterest(float halite, float distance, int totalCost) 
		{
			//distance 0 --> 10
			//Halite 0 --> 1000
			//Max = Distance = 0 && halite = 1000
			//Min = Distance = 10 && halite = 0
			float haliteWeight = UtilsMath::invLerp(0, 1000, halite);
			float distanceWeight = 1 - UtilsMath::invLerp(0, 10, distance);
			float costWeight = 1 - UtilsMath::invLerp(0, 3000, totalCost);

			return haliteWeight * distanceWeight * costWeight;
		}
	};

	class GoTo : public BrainTree::Node
	{
		Status update() override
		{

			if (BrainAI::p_ship->goingToDeposit && BrainAI::p_ship->position == BrainAI::p_ship->dropPoint)
			{
				//log::log("Revert goingToDeposit my pos " + BrainAI::p_ship->position.to_string() + " drop point pos : " + BrainAI::p_ship->dropPoint.to_string());
				BrainAI::p_ship->goingToDeposit = false;
			}
			//log::log("Go to " + BrainAI::ship->goalPosition->to_string());
			
			BrainAI::p_ship->executeCommand = BrainAI::p_ship->move(BrainAI::p_game->game_map->astar_navigate(BrainAI::p_ship, BrainAI::p_ship->goalPosition));

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
			//log::log("Turn Left " + std::to_string(constants::MAX_TURNS - BrainAI::p_game->turn_number));
			return  constants::MAX_TURNS - BrainAI::p_game->turn_number > countTurns ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class CountNbrTurnsLeftMin : public BrainTree::Node
	{
	private:
		int countTurns;
	public:
		CountNbrTurnsLeftMin(int val) : countTurns(val) {}

		Status update() override
		{
			//log::log("Turn Left " + std::to_string(constants::MAX_TURNS - BrainAI::game->turn_number));
			return  constants::MAX_TURNS - BrainAI::p_game->turn_number < countTurns ? Node::Status::Success : Node::Status::Failure;
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

			for (int x = 0; x < BrainAI::p_game->game_map->width; ++x)
			{
				p.x = x;
				for (int y = 0; y < BrainAI::p_game->game_map->height; ++y)
				{
					p.y = y;
					if (BrainAI::p_game->game_map->at(p)->has_structure() &&
						BrainAI::p_game->game_map->at(p)->structure->owner == BrainAI::p_game->me->id)
					{
						BrainAI::dropOffCreated = BrainAI::p_game->game_map->calculate_distance(BrainAI::p_ship->position, p) > dist;
						if (!BrainAI::dropOffCreated)
							return Node::Status::Failure;
					}					
				}
			}

			return Node::Status::Success;
		}
	};

	class ConvertToDropPoint :  public BrainTree::Node
	{
		Status update() override
		{

			//log::log("Create Drop Off " +  std::to_string(BrainAI::ship->id));

			BrainAI::p_ship->executeCommand = BrainAI::p_ship->make_dropoff();
			return  Node::Status::Success;
		}
	};

	class SearchForEnemy : public BrainTree::Node
	{
		private :
			int dist;

		public:
			SearchForEnemy(int val) : dist(val) {}


		Status update() override
		{
			Position p;

			for (int x = 0; x < BrainAI::p_game->game_map->width; ++x)
			{
				p.x = x;
				for (int y = 0; y < BrainAI::p_game->game_map->height; ++y)
				{
					p.y = y;
					if (BrainAI::p_game->game_map->at(p)->is_occupied() &&
						BrainAI::p_game->game_map->at(p)->ship->owner != BrainAI::p_game->me->id &&
						BrainAI::p_game->game_map->calculate_distance(BrainAI::p_ship->position, p) < dist
						)
					{

						//log::log("Enemy near my position " + std::to_string(BrainAI::p_ship->id));
						BrainAI::p_ship->enemy = BrainAI::p_game->game_map->at(p)->ship;
						return Node::Status::Success;
					}
				}
			}
			return Node::Status::Failure;
		}
	};

	class FleeDecision : public BrainTree::Node
	{
		Status update() override
		{
			if (BrainAI::p_ship->enemy->halite < BrainAI::p_ship->halite || BrainAI::p_ship->halite > HALITE_STORAGE || BrainAI::p_game->me->ships.size() < 5)
			{
				//log::log("I flee " + std::to_string(BrainAI::ship->id));
				return Node::Status::Success;
			}
			return Node::Status::Failure;
		}
	};

	class IsGoingToDeposit : public BrainTree::Node
	{
		Status update() override
		{
			//log::log("I am going to depsit id : " + std::to_string(BrainAI::ship->id) + " result : " + std::to_string(BrainAI::ship->goingToDeposit));
			return BrainAI::p_ship->goingToDeposit ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class AttackDecision : public BrainTree::Node
	{
		Status update() override
		{			
			return (BrainAI::p_ship->enemy->halite > BrainAI::p_ship->halite && 
				BrainAI::p_ship->enemy->halite > 300 && 
				BrainAI::p_game->me->halite > 3000) ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class AttackEnemy : public BrainTree::Node
	{
		Status update() override
		{
			BrainAI::p_ship->goalPosition = BrainAI::p_ship->enemy->position;
			return Node::Status::Success;
		}
	};

	class FindCaseAwayFromEnemy : public BrainTree::Node
	{
		Status update() override
		{
			Position p = { BrainAI::p_ship->position.x - BrainAI::p_ship->enemy->position.x , BrainAI::p_ship->position.y - BrainAI::p_ship->enemy->position.y };

			BrainAI::p_ship->goalPosition = { BrainAI::p_ship->position.x + p.x, BrainAI::p_ship->position.y + p.y };

			return Node::Status::Success;
		}
	};

	Game * BrainAI::p_game = nullptr;
	std::shared_ptr<Ship> BrainAI::p_ship = nullptr;
	bool BrainAI::dropOffCreated = false;
}


