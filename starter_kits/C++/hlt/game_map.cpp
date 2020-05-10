#include "game_map.hpp"
#include "input.hpp"

void hlt::GameMap::_update() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            cells[y][x].ship.reset();
        }
    }

    int update_count;
    hlt::get_sstream() >> update_count;

    for (int i = 0; i < update_count; ++i) {
        int x;
        int y;
        int halite;
        hlt::get_sstream() >> x >> y >> halite;
        cells[y][x].halite = halite;
    }
}

std::unique_ptr<hlt::GameMap> hlt::GameMap::_generate() {
    std::unique_ptr<hlt::GameMap> map = std::make_unique<GameMap>();

    hlt::get_sstream() >> map->width >> map->height;

    map->cells.resize((size_t)map->height);
    for (int y = 0; y < map->height; ++y) {
        auto in = hlt::get_sstream();

        map->cells[y].reserve((size_t)map->width);
        for (int x = 0; x < map->width; ++x) {
            hlt::Halite halite;
            in >> halite;

            map->cells[y].emplace_back(x, y, halite);
        }
    }

    return map;
}

std::vector<hlt::Position *> hlt::AStarPathfind::astar(hlt::Position &start, const hlt::Position &end, hlt::GameMap *map, int & totalCost) {
    std::set<Node *> open;
    std::set<Node *> closed;
    open.emplace(new Node(start, map->calculate_distance(start, end), map->at(start)->halite * 10/100));
    Node *current = nullptr;
    while (!open.empty()) {
        current = *open.begin();
        open.erase(current);
        closed.insert(current);

        if (current->position == end) { // Path as been found !!!!
            std::vector<Position *> path;
            totalCost = current->cost;
            while (current->parent != nullptr) {
                path.push_back(&current->position);
                Node * old = current;
                current = current->parent;
                delete(old);
            }
            std::reverse(path.begin(),path.end());
            return path;
        }
        for (int x(-1); x < 2; ++x) { // Pour tous les voisins !
            for (int y(-1); y < 2; ++y) {
                if (abs(x) == abs(y)) continue; //Pas les diagonales ni nous même
                Position * pos = new Position(current->position.x + x, current->position.y + y);
                map->normalize(pos);
                bool isInClosed = isInSet(pos,closed);
                if ((map->at(*pos)->ship || isInClosed) && *pos != end){
                    continue; //Si ça bloque on passe au(x) voisin(s) suivant
                }
                Node *neighbour = getNodeInSet(open, pos);
                bool isInOpen = neighbour != nullptr;

                if (!isInOpen || neighbour->cost > current->cost) {
                    if (!isInOpen) {
                        neighbour = new Node(current, *pos, map->calculate_distance(*pos, end),
                                             current->cost + map->at(*pos)->halite * 10/100);
                        open.insert(neighbour);
                    } else {
                        neighbour->cost = current->cost + 1;
                        neighbour->parent = current;
                    }
                }
            }
        }
    }
    return std::vector<Position *>();
}

hlt::Direction hlt::GameMap::astar_navigate(std::shared_ptr<Ship> ship, const hlt::Position &destination) {
    if(ship->position == destination)
        return Direction::STILL;
    std::vector<Position *> pos = AStarPathfind::astar(ship->position, destination, this);
    return naive_navigate(ship, *pos[0]);
}

