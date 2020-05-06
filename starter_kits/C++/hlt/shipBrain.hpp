#pragma once

#include "entity.hpp"
#include "constants.hpp"
#include "command.hpp"
#include "game.hpp"

#include <memory>
#include "BrainTree.h"



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



	
	class CheckStorage;
	class FindHalite;
	class GoTo;

	class BrainAI
	{
		private:
			std::shared_ptr<BrainTree::Node> tree;

		public:			

			static Game * game;
			static std::shared_ptr<Ship> ship;

			BrainAI()
			{
				buildTree();
			}

			void buildTree()
			{
				tree = BrainTree::Builder()
					.composite<BrainTree::Sequence>()
						.leaf<CheckStorageToDrop>()
						.leaf<DropHalite>()
						.leaf<GoTo>()
					.end()
					.composite<BrainTree::Sequence>()
						.leaf<CheckStorage>()
						.leaf<FindHalite>()
						.leaf<GoTo>()
					.end()
					.build();
			}

			void update()
			{
				tree->update();
			}
	};


	class CheckStorage : public BrainTree::Node
	{
		Status update() override
		{
			//log::log("Check Storage " + std::to_string(BrainAI::ship->halite));
			return BrainAI::ship->halite < 500 ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class CheckStorageToDrop : public BrainTree::Node
	{
		Status update() override
		{
			//log::log("Check Storage " + std::to_string(BrainAI::ship->halite));
			return BrainAI::ship->halite > 500 ? Node::Status::Success : Node::Status::Failure;
		}
	};

	class DropHalite : public BrainTree::Node
	{
		Status update() override
		{

			Position p;

			for (int x = 0; x < BrainAI::game->game_map->width; ++x)
			{
				for (int y = 0; y < BrainAI::game->game_map->height; ++y)
				{

					BrainAI::game->game_map->at(p);
					/*	
					p.x = x;
					p.y = y;
					*/
				}
			}
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
			//log::log("Go to " + BrainAI::ship->goalPosition->to_string());

			BrainAI::ship->executeCommand = BrainAI::ship->move(BrainAI::game->game_map->naive_navigate(BrainAI::ship, *BrainAI::ship->goalPosition));
			return Node::Status::Success;
		}
	};

	Game * BrainAI::game = nullptr;
	std::shared_ptr<Ship> BrainAI::ship = nullptr;
}


