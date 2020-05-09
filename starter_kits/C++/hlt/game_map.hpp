#pragma once

#include "types.hpp"
#include "map_cell.hpp"
#include <vector>
#include <list>

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

        void _update();
        static std::unique_ptr<GameMap> _generate();

    };


    class Node {
    public:
        Node *parent = nullptr;
        const Position &position;
        int dist, cost;

        Node(const Position &position, int dist, int cost = 1) : position(position), dist(dist), cost(cost) {}

        Node(Node *parent, const Position &position, int dist, int cost) : parent(parent), position(position),
                                                                                 dist(dist), cost(cost) {}

        bool operator<(const Node &o) const { return dist + cost < o.dist + o.cost; }

        bool operator == (const Node& o ) { return position == o.position; }
        bool operator == (const Position& o ) { return position == o; }
    };


    class Point{
    public:
        int x,y;
        Point():x(0),y(0){}
        Point(int x, int y) : x(x), y(y) {}
        bool operator ==( const Point& o ) const { return o.x == x && o.y == y; }
        Point operator +( const Point& o ) const { return { o.x + x, o.y + y }; }
        Position add(const Position & to) const{
            return {to.x + x, to.y+y};
        }
    };


    class AStar {
    public:
        GameMap m; Position end, start;
        Point neighbours[4];
        std::list<Node> open;
        std::list<Node> closed;

        AStar() {
            neighbours[0] = Point(  0, -1 ); neighbours[1] = Point( -1,  0 );
            neighbours[2] = Point(  0,  1 ); neighbours[3] = Point(  1,  0 );
        }

        int calcDist( Position& p ){
            return m.calculate_distance(p, end);
        }

        bool isValid( Position& p ) {
            p = m.normalize(p);
            return true;
        }

        bool existPoint( Position& p, int cost ) {
            std::list<Node>::iterator i;
            i = std::find( closed.begin(), closed.end(), p );
            if( i != closed.end() ) {
                if( ( *i ).cost + ( *i ).dist < cost ) return true;
                else { closed.erase( i ); return false; }
            }
            i = std::find( open.begin(), open.end(), p );
            if( i != open.end() ) {
                if( ( *i ).cost + ( *i ).dist < cost ) return true;
                else { open.erase( i ); return false; }
            }
            return false;
        }

        bool fillOpen( Node& n ) {
            int stepCost, nc, dist;
            Position neighbour;

            for( int x = 0; x < 4; ++x) {

                stepCost = 1;
                neighbour = m.normalize(neighbours[x].add(n.position));
                if( neighbour == end ) return true;
                if( isValid( neighbour ) && !m.at(neighbour)->is_occupied() ) {
                    nc = stepCost + n.cost;
                    dist = calcDist( neighbour );
                    if( !existPoint( neighbour, nc + dist ) ) {
                        Node no = Node(&n,neighbour,dist,nc);

                        open.push_back( no );
                    }
                }
            }
            return false;
        }

        bool search( Position& s, Position& e, GameMap& mp ) {

            end = e; start = s; m = mp;
            Node n = Node(s,calcDist(s),0);
            open.push_back( n );
            while( !open.empty() ) {
                //open.sort();
                Node no = open.front();
                open.pop_front();
                closed.push_back( no );
                if( fillOpen( no ) ) return true;
            }
            return false;
        }

        int path( std::list<Position>& path ) {
            path.push_front( end );
            int cost = 1 + closed.back().cost;
            path.push_front( closed.back().position );
            Node* parent = closed.back().parent;

            for( auto i = closed.rbegin(); i != closed.rend(); i++ ) {
                if( ( *i ).position == parent->position && !( ( *i ).position == start ) ) {
                    path.push_front( ( *i ).position );
                    parent = ( *i ).parent;
                }
            }
            path.push_front( start );
            return cost;
        }

    };


}
