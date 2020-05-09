#pragma once

#include "types.hpp"
#include "map_cell.hpp"
#include <vector>
#include <deque>

class AStar;


namespace hlt {
    class Node;
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

        Position normalize(const Position& position) {
            const int x = ((position.x % width) + width) % width;
            const int y = ((position.y % height) + height) % height;
            return { x, y };
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

        Direction naive_navigate(std::shared_ptr<Ship> ship, const Position& destination) {
            // get_unsafe_moves normalizes for us
            for (auto direction : get_unsafe_moves(ship->position, destination)) {
                Position target_pos = ship->position.directional_offset(direction);
                if (!at(target_pos)->is_occupied()) {
                    at(target_pos)->mark_unsafe(ship);
                    return direction;
                }
            }

            return Direction::STILL;
        }

        std::deque<Node *> aStarPath(std::shared_ptr<Ship> ship, Position& destination);
        Direction aStar_navigate(std::shared_ptr<Ship> ship, std::deque<hlt::Node *> &path);

        void _update();
        static std::unique_ptr<GameMap> _generate();

    };


    class Node {
    public:
        Node *parent;
        const Position &position;
        double g, h;

        Node(Node *parent, const hlt::Position &position, double g, double h)
                : parent(parent), position(position), g(g), h(h) {}

        bool operator<(const Node &o) const { return g + h < o.g + o.h; }

        bool operator==(const Node &o) const { return g + h == o.g + o.h; }
    };


    class AStar {
    private:
        std::unique_ptr<std::deque<Node *>> open = std::make_unique<std::deque<Node *>>();
        std::unique_ptr<std::deque<Node *>> closed = std::make_unique<std::deque<Node *>>();
        std::unique_ptr<std::deque<Node *>> path = std::make_unique<std::deque<Node *>>();
        GameMap *map;
        Node *current;
        Position *start;
        Position *end;

    public:
        AStar(GameMap *map) : map(map) {}

        std::deque<Node *> findPathTo(hlt::Position *from, hlt::Position *whereTo);

    private:

        /**
         * Calculate the Manhattan distance drom the current node to the destination
         * @return Manhattan distance in int
         */
        double distance() {
            return map->calculate_distance(current->position, *end);
        }

        /**
         * A method that computes the Manhattan distance between two locations, and accounts for the toroidal wraparound.
         * @param from
         * @param to
         * @return Manhattan distance in int
         */
        double distance(hlt::Position from, hlt::Position to) {
            return map->calculate_distance(from, to);
        }


        static bool findNeighborInList(const std::deque<Node *> &array, const Node &node) {
            for (auto n : array) {
                if (n->position == node.position)
                    return true;
            }
            return false;
        }

        static bool sorting(Node *a, Node *b) { return *a < *b; }

        void addNeighborsToOpenList();

    };


}
