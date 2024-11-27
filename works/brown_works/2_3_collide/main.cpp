#include <profile.h>
#include <test_runner.h>

#include "game_object.h"
#include "geo2d.h"

#include <cassert>
#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <forward_list>
#include <iterator>
#include <deque>
#include <tuple>
#include <random>
#include <functional>
#include <fstream>
#include <random>
#include <tuple>

using namespace std;

void TestAll();
void Profile();

// Определите классы Unit, Building, Tower и Fence так, чтобы они наследовались от
// GameObject и реализовывали его интерфейс.

template <typename T, typename Geometry>
class Collider : public GameObject
{
public:
    Collider(Geometry geometry)
        : _geometry(geometry)
    {
    }

    bool Collide(const GameObject &that) const final override
    {
        return that.CollideWith(static_cast<const T &>(*this));
    }

    Geometry geometry() const
    {
        return _geometry;
    }

protected:
    Geometry _geometry;
};

class Unit final : public Collider<Unit, geo2d::Point>
{
public:
    using Collider::Collider;

    bool CollideWith(const Unit &that) const override;
    bool CollideWith(const Building &that) const override;
    bool CollideWith(const Tower &that) const override;
    bool CollideWith(const Fence &that) const override;
};

class Building final : public Collider<Building, geo2d::Rectangle>
{
public:
    using Collider::Collider;

    bool CollideWith(const Unit &that) const override;
    bool CollideWith(const Building &that) const override;
    bool CollideWith(const Tower &that) const override;
    bool CollideWith(const Fence &that) const override;
};

class Tower final : public Collider<Tower, geo2d::Circle>
{
public:
    using Collider::Collider;

    bool CollideWith(const Unit &that) const override;
    bool CollideWith(const Building &that) const override;
    bool CollideWith(const Tower &that) const override;
    bool CollideWith(const Fence &that) const override;
};

class Fence final : public Collider<Fence, geo2d::Segment>
{
public:
    using Collider::Collider;

    bool CollideWith(const Unit &that) const override;
    bool CollideWith(const Building &that) const override;
    bool CollideWith(const Tower &that) const override;
    bool CollideWith(const Fence &that) const override;
};

#define COLLIDE_WITH(class, other_class) \
bool class::CollideWith(const other_class &that) const { return geo2d::Collide(_geometry, that.geometry()); }

COLLIDE_WITH(Unit, Unit)
COLLIDE_WITH(Unit, Building)
COLLIDE_WITH(Unit, Tower)
COLLIDE_WITH(Unit, Fence)
COLLIDE_WITH(Building, Unit)
COLLIDE_WITH(Building, Building)
COLLIDE_WITH(Building, Tower)
COLLIDE_WITH(Building, Fence)
COLLIDE_WITH(Tower, Unit)
COLLIDE_WITH(Tower, Building)
COLLIDE_WITH(Tower, Tower)
COLLIDE_WITH(Tower, Fence)
COLLIDE_WITH(Fence, Unit)
COLLIDE_WITH(Fence, Building)
COLLIDE_WITH(Fence, Tower)
COLLIDE_WITH(Fence, Fence)

bool Collide(const GameObject &first, const GameObject &second)
{
    return first.Collide(second);
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestAddingNewObjectOnMap()
{
    // Юнит-тест моделирует ситуацию, когда на игровой карте уже есть какие-то объекты,
    // и мы хотим добавить на неё новый, например, построить новое здание или башню.
    // Мы можем его добавить, только если он не пересекается ни с одним из существующих.
    using namespace geo2d;

    const vector<shared_ptr<GameObject>> game_map = {
        make_shared<Unit>(Point{3, 3}),
        make_shared<Unit>(Point{5, 5}),
        make_shared<Unit>(Point{3, 7}),
        make_shared<Fence>(Segment{{7, 3}, {9, 8}}),
        make_shared<Tower>(Circle{Point{9, 4}, 1}),
        make_shared<Tower>(Circle{Point{10, 7}, 1}),
        make_shared<Building>(Rectangle{{11, 4}, {14, 6}})};

    for (size_t i = 0; i < game_map.size(); ++i)
    {
        Assert(
            Collide(*game_map[i], *game_map[i]),
            "An object doesn't collide with itself: " + to_string(i));

        for (size_t j = 0; j < i; ++j)
        {
            Assert(
                !Collide(*game_map[i], *game_map[j]),
                "Unexpected collision found " + to_string(i) + ' ' + to_string(j));
        }
    }

    auto new_warehouse = make_shared<Building>(Rectangle{{4, 3}, {9, 6}});
    ASSERT(!Collide(*new_warehouse, *game_map[0]));
    ASSERT(Collide(*new_warehouse, *game_map[1]));
    ASSERT(!Collide(*new_warehouse, *game_map[2]));
    ASSERT(Collide(*new_warehouse, *game_map[3]));
    ASSERT(Collide(*new_warehouse, *game_map[4]));
    ASSERT(!Collide(*new_warehouse, *game_map[5]));
    ASSERT(!Collide(*new_warehouse, *game_map[6]));

    auto new_defense_tower = make_shared<Tower>(Circle{{8, 2}, 2});
    ASSERT(!Collide(*new_defense_tower, *game_map[0]));
    ASSERT(!Collide(*new_defense_tower, *game_map[1]));
    ASSERT(!Collide(*new_defense_tower, *game_map[2]));
    ASSERT(Collide(*new_defense_tower, *game_map[3]));
    ASSERT(Collide(*new_defense_tower, *game_map[4]));
    ASSERT(!Collide(*new_defense_tower, *game_map[5]));
    ASSERT(!Collide(*new_defense_tower, *game_map[6]));
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestAddingNewObjectOnMap);
}

void Profile()
{
}
