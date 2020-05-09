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

std::list<hlt::Position *> hlt::AStarPathfind::astar(hlt::Position &start, const hlt::Position &end, hlt::GameMap *map) {
    std::set<Node *, comparef> open;
    std::set<Node *> closed;
    open.emplace(new Node(start, map->calculate_distance(start, end), 0));
    Node *current = nullptr;
    while (!open.empty()) {
        current = *open.begin();
        open.erase(current);
        closed.insert(current);

        if (current->position == end) { // Path as been found !!!!
            std::list<Position *> path;
            while (current->parent != nullptr) {
                path.push_front(&current->position);
                current = current->parent;
                log::log("J'ai trouvé, je dépile");
            }
            return path;
        }
        int test(0);
        for (int x(-1); x < 2; ++x) { // Pour tous les voisins !
            for (int y(-1); y < 2; ++y) {
                if (abs(x) == abs(y)) continue; //Pas les diagonales ni nous même
                Position pos = map->normalize(Position(current->position.x + x, current->position.y + y));
                if (map->at(pos)->ship || isInSet(pos, closed))
                    continue; //Si ça bloque on passe au(x) voisin(s) suivant
                Node *neighbour = getNodeInSet(open, pos); //  TODO: THE ISSUE SEEMS TO COME FROM getNodeInSet
                if (!isInSet(pos,open) || neighbour->cost > current->cost) { //  SINCE  IT PASSES THIS CONDITION
                    if (neighbour == nullptr) { // BUT NOT ENTER HERE (WELL ENTER JUST ONE TIME/4)
                        neighbour = new Node(current, pos, map->calculate_distance(pos, end),
                                             current->cost + 1);
                        open.insert(neighbour);
                    } else { // WTF  WHY HE FIND TH CORRESPONDING NEIGHBOUR
                        neighbour->cost = current->cost + 1;
                        neighbour->parent = current;
                    }
                }
            }
        }
    }
    return std::list<Position *>();
}

hlt::Direction hlt::GameMap::astar_navigate(std::shared_ptr<Ship> ship, const hlt::Position &destination) {
    std::list<Position*> pos = AStarPathfind::astar(ship->position, destination, this);
    return naive_navigate(ship,destination);
}

