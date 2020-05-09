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

std::deque<hlt::Node *> hlt::GameMap::aStarPath(std::shared_ptr<Ship> ship, hlt::Position &destination) {
    std::unique_ptr<AStar> astar = std::make_unique<AStar>(this);
    return astar->findPathTo(&ship->position, &destination);
}

hlt::Direction hlt::GameMap::aStar_navigate(std::shared_ptr<Ship> ship, std::deque<hlt::Node *> & path) {
    int iCur(-1);
    for (int i = 0; i < path.size(); ++i) {
        if(path[i]->position == ship->position){
            iCur = i;
            break;
        }
    }
    if(iCur == -1){
        hlt::log::log("THIS IS NOT SUPPOSED TO HAPPEN !!!!!!! LINE 56 gamemap.cpp, SHIP NOT ON PATH !");
        return naive_navigate(ship, path.back()->position); // This should not happen but there is a failsafe
    }
    else if(iCur+1 < path.size())
        return naive_navigate(ship,path[iCur+1]->position);
    else if (iCur+1 == path.size()){
        return Direction::STILL;
    }
    return Direction::STILL;
}

std::deque<hlt::Node *> hlt::AStar::findPathTo(hlt::Position *from, hlt::Position *whereTo) {
    path->clear(); //Clear previous result
    open->clear();
    closed->clear();
    start = from;
    current = new Node(nullptr, *start, 0, map->at(*start)->halite);
    end = whereTo;
    closed->push_back(current);
    addNeighborsToOpenList();
    while (current->position != *whereTo) {
        if (open->empty()) { // Nothing to examine
            return std::deque<Node *>(NULL);
        }
        current = open->front(); // get first node (lowest f score)
        open->pop_front();// remove it
        closed->push_back(current); // and add to the closed
        addNeighborsToOpenList();
    }
    //path.add(0, this.now);
    path->push_front(current);
    while (current->position != *start) {
        current = current->parent;
        path->push_front(current);
    }
    return *path;
}

void hlt::AStar::addNeighborsToOpenList() {
    Node *node;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            if (x != 0 && y != 0) {
                continue; // skip if diagonal movement
            }
            node = new Node(current, hlt::Position(current->position.x + x, current->position.y + y),
                            current->g, distance());
            if ((x != 0 || y != 0) // not the current done
                && !map->at(hlt::Position(current->position.x + x, current->position.y +
                                                                   y))->is_occupied() // check if there is not already a ship
                && !findNeighborInList(*open, *node) &&
                !findNeighborInList(*closed, *node)) { // if not already done
                node->g = node->parent->g + 1.; // Horizontal/vertical cost = 1.0
                node->g += map->at(hlt::Position(current->position.x,
                                                 current->position.y))->halite;// add movement cost for this square (corresponding to stock of halite)


            log::log("PUSHBACKOPEN: " + std::to_string(node->position.x) + ", " + std::to_string(node->position.y));

                open->push_back(node);
            }
        }
    }

    std::sort(open->begin(), open->end(), sorting);

}
