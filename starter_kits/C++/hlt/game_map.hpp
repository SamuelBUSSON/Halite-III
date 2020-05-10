#pragma once

#include "types.hpp"
#include "map_cell.hpp"
#include <vector>
#include <list>
#include <set>

namespace hlt {
    class AStarPathfind;
    struct GameMap {
        int width;
        int height;
        std::vector<std::vector<MapCell>> cells;

        MapCell* at(const Position& position) {
            Position normalized = normalize(position);
            return &cells[normalized.y][normalized.x];
        }

        MapCell* at(const Entity& entity) {
            return at(entity.position);
        }

        MapCell* at(const Entity* entity) {
            return at(entity->position);
        }

        MapCell* at(const std::shared_ptr<Entity>& entity) {
            return at(entity->position);
        }

        int calculate_distance(const Position& source, const Position& target) {
            const auto& normalized_source = normalize(source);
            const auto& normalized_target = normalize(target);

            const int dx = std::abs(normalized_source.x - normalized_target.x);
            const int dy = std::abs(normalized_source.y - normalized_target.y);

            const int toroidal_dx = std::min(dx, width - dx);
            const int toroidal_dy = std::min(dy, height - dy);

            return toroidal_dx + toroidal_dy;
        }

        /**
         * A method that normalizes a position within the bounds of the toroidal map. Useful for handling the wraparound modulus arithmetic on x and y.
         * For example, if a ship at (x = 31, y = 4) moves to the east on a 32x32 map, the normalized position would be (x = 0, y = 4),
         * rather than the off-the-map position of (x = 32, y = 4).
         * @param position
         * @return
         */
        Position normalize(const Position& position) {
            const int x = ((position.x % width) + width) % width;
            const int y = ((position.y % height) + height) % height;
            return { x, y };
        }


        void normalize( Position * position) {
            position->x = ((position->x % width) + width) % width;
            position->y = ((position->y % height) + height) % height;
        }

        std::vector<Direction> get_unsafe_moves(const Position& source, const Position& destination) {
            const auto& normalized_source = normalize(source);
            const auto& normalized_destination = normalize(destination);

            const int dx = std::abs(normalized_source.x - normalized_destination.x);
            const int dy = std::abs(normalized_source.y - normalized_destination.y);
            const int wrapped_dx = width - dx;
            const int wrapped_dy = height - dy;

            std::vector<Direction> possible_moves;

            if (normalized_source.x < normalized_destination.x) {
                possible_moves.push_back(dx > wrapped_dx ? Direction::WEST : Direction::EAST);
            } else if (normalized_source.x > normalized_destination.x) {
                possible_moves.push_back(dx < wrapped_dx ? Direction::WEST : Direction::EAST);
            }

            if (normalized_source.y < normalized_destination.y) {
                possible_moves.push_back(dy > wrapped_dy ? Direction::NORTH : Direction::SOUTH);
            } else if (normalized_source.y > normalized_destination.y) {
                possible_moves.push_back(dy < wrapped_dy ? Direction::NORTH : Direction::SOUTH);
            }

            return possible_moves;
        }

        Direction naive_navigate(std::shared_ptr<Ship> ship, const Position& destination) 
		{
					   
			// get_unsafe_moves normalizes for us
            for (auto direction : get_unsafe_moves(ship->position, destination))
			{
                Position target_pos = ship->position.directional_offset(direction);
                if (!at(target_pos)->is_occupied()) {
                    at(target_pos)->mark_unsafe(ship);
                    return direction;
                }
            }

			if (ship->position != destination)
			{
				Direction d;
				Position target_pos;
				for (int i = 0; i < ALL_CARDINALS.size(); i++)
				{
					d = ALL_CARDINALS[i];
					target_pos = ship->position.directional_offset(d);
					if (!at(target_pos)->is_occupied()) {
						at(target_pos)->mark_unsafe(ship);
						return d;
					}
				}
			}
			else {
				log::log("STILL" + std::to_string(ship->id));
			}

            return Direction::STILL;
        }

        Direction astar_navigate(std::shared_ptr<Ship> ship, const Position& destination);

		int get_cost(std::shared_ptr<Ship> ship, const hlt::Position & destination);

        void _update();
        static std::unique_ptr<GameMap> _generate();

    };


    class Node {
    public:
        Node *parent = nullptr;
        Position & position;
        int dist, cost;

        // dist = h // cost = g
        inline int f() const {
            return dist + cost;
        }

        Node(hlt::Position &position, int dist, int cost = 1) : position(position), dist(dist), cost(cost) {}

        Node(Node *parent, hlt::Position &position, int dist, int cost) : parent(parent), position(position),
                                                                                dist(dist), cost(cost) {}

        bool operator<(const Node &o) const { return f() < o.f(); }

        bool operator<(const Node *o) const { return f() < o->f(); }

        bool operator==(const Node &o) { return position == o.position; }

        bool operator==(const hlt::Position &o) { return position == o; }
    };

    class AStarPathfind {
    private:
        struct comparef {
            bool operator()(Node *i, Node *j) const {
                return i->f() < j->f();
            }
        };

        static bool isInSet(Position * pos, const std::set<Node *> &set) {
            for (Node* n : set) {
                if (n->position.x == pos->x && n->position.y == pos->y )
                    return true;
            }
            return false;
        }

        static Node *getNodeInSet(std::set<Node *, std::less<Node *>> set, Position * pos) {
            for (auto n : set) {
                if (n->position == *pos)
                    return n;
            }
            return nullptr;
        }

    public:
        static std::vector<Position *> astar(hlt::Position &start, const hlt::Position &end, GameMap *map, int & totalCost);

        static std::vector<Position *> astar(hlt::Position &start, const hlt::Position &end, GameMap *map){
            int fool(0);
            return astar( start, end, map, fool);
        }
    };

}
