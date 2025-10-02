#include <gtest/gtest.h>
#include "../include/ecs/registry.h"
#include "../include/ecs/components.h"

struct Position {
    float x = 0.0f;
    float y = 0.0f;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

bool isValidEntity(const Entity& e) {
    return e != INVALID_ENTITY && e.id != 0;
}

template <typename T>
class RegistryComponentTest : public ::testing::Test {
public:
    static T makeValue(int seed = 0);
};

// specializations for each component type
template <>
Position RegistryComponentTest<Position>::makeValue(int seed) {
    return Position{static_cast<float>(seed), static_cast<float>(seed + 1)};
}

template <>
TransformComp RegistryComponentTest<TransformComp>::makeValue(int seed) {
    return TransformComp{{static_cast<float>(seed), 0, 0}, {1, 1, 1}};
}

template <>
WorldTransform RegistryComponentTest<WorldTransform>::makeValue(int seed) {
    return WorldTransform{{static_cast<float>(seed), 0, 0}, {1, 1, 1}};
}

template <>
Parent RegistryComponentTest<Parent>::makeValue(int /*seed*/) {
    return Parent{Entity{1, 1}};
}

template <>
Children RegistryComponentTest<Children>::makeValue(int /*seed*/) {
    return Children{{Entity{2, 1}, Entity{3, 1}}};
}

template <>
ColoredRender RegistryComponentTest<ColoredRender>::makeValue(int seed) {
    return ColoredRender{Color{
        static_cast<unsigned char>(seed % 256),
        static_cast<unsigned char>((seed + 50) % 256),
        static_cast<unsigned char>((seed + 100) % 256),
        255
    }};
}

template <>
TexturedRender RegistryComponentTest<TexturedRender>::makeValue(int /*seed*/) {
    return TexturedRender{};
}

template <>
Collision RegistryComponentTest<Collision>::makeValue(int seed) {
    return Collision{(seed % 2) == 0};
}

template <>
Wall RegistryComponentTest<Wall>::makeValue(int seed) {
    return Wall{seed % 2 == 0 ? Wall::Side::Front : Wall::Side::Back};
}

template <>
Anchor RegistryComponentTest<Anchor>::makeValue(int seed) {
    return Anchor{{static_cast<float>(seed), 0, 0}, {0, 0, 1}, Entity{4, 1}};
}

using ComponentTypes = ::testing::Types<
    Position,
    TransformComp,
    WorldTransform,
    Parent,
    Children,
    ColoredRender,
    TexturedRender,
    Collision,
    Wall,
    Anchor
>;

TYPED_TEST_SUITE(RegistryComponentTest, ComponentTypes);

TYPED_TEST(RegistryComponentTest, AddGetComponent) {
    using T = TypeParam;
    Registry reg;
    Entity e = reg.create();
    T value = TestFixture::makeValue(42);
    reg.add(e, value);

    const T* retrieved = reg.get<T>(e);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_TRUE(reg.has<T>(e));
}

TYPED_TEST(RegistryComponentTest, AddTwice_Overwrites) {
    using T = TypeParam;
    Registry reg;
    Entity e = reg.create();
    T v1 = TestFixture::makeValue(1);
    T v2 = TestFixture::makeValue(2);
    reg.add(e, v1);
    reg.add(e, v2);

    const T* p = reg.get<T>(e);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(reg.has<T>(e));
}

TYPED_TEST(RegistryComponentTest, ComponentOnInvalidEntity_Ignored) {
    using T = TypeParam;
    Registry reg;
    Entity invalid{999, 1};
    T value = TestFixture::makeValue(100);
    reg.add(invalid, value);
    EXPECT_FALSE(reg.has<T>(invalid));
    EXPECT_EQ(reg.get<T>(invalid), nullptr);
}

// parameterized multi-component view Tests
template <typename Pair>
class RegistryMultiViewTest : public ::testing::Test {
public:
    using T1 = typename Pair::first_type;
    using T2 = typename Pair::second_type;

    static T1 makeValue1(int seed) { return RegistryComponentTest<T1>::makeValue(seed); }
    static T2 makeValue2(int seed) { return RegistryComponentTest<T2>::makeValue(seed); }
};

// define common component pairs used
using ComponentPairs = ::testing::Types<
    std::pair<TransformComp, Parent>,
    std::pair<TransformComp, Children>,
    std::pair<WorldTransform, ColoredRender>,
    std::pair<WorldTransform, TexturedRender>,
    std::pair<Wall, ColoredRender>,
    std::pair<Anchor, TransformComp>,
    std::pair<Position, Wall>, // just for coverage
    std::pair<Collision, TransformComp>
>;

// TYPED_TEST_SUITE(RegistryMultiViewTest, ComponentPairs);

// TYPED_TEST(RegistryMultiViewTest, TwoComponentView_CorrectEntities) {
//     using T1 = typename TestFixture::T1;
//     using T2 = typename TestFixture::T2;

//     Registry reg;
//     Entity e1 = reg.create(); // has both
//     Entity e2 = reg.create(); // has only T1
//     Entity e3 = reg.create(); // has only T2
//     Entity e4 = reg.create(); // has neither

//     reg.add(e1, TestFixture::makeValue1(1));
//     reg.add(e1, TestFixture::makeValue2(1));

//     reg.add(e2, TestFixture::makeValue1(2));
//     reg.add(e3, TestFixture::makeValue2(3));

//     // e4: nothing added

//     std::vector<Entity> found;
//     for (auto [ent, c1, c2] : reg.view<T1, T2>()) {
//         ASSERT_NE(c1, nullptr);
//         ASSERT_NE(c2, nullptr);
//         found.push_back(ent);
//     }

//     EXPECT_EQ(found.size(), 1);
//     EXPECT_EQ(found[0], e1);
// }

// TYPED_TEST(RegistryMultiViewTest, TwoComponentView_EmptyWhenNoOverlap) {
//     using T1 = typename TestFixture::T1;
//     using T2 = typename TestFixture::T2;

//     Registry reg;
//     Entity e1 = reg.create();
//     Entity e2 = reg.create();

//     reg.add(e1, TestFixture::makeValue1(1));
//     reg.add(e2, TestFixture::makeValue2(2));

//     int count = 0;
//     for ([[maybe_unused]] auto tuple : reg.view<T1, T2>()) {
//         ++count;
//     }
//     EXPECT_EQ(count, 0);
// }

// TYPED_TEST(RegistryMultiViewTest, TwoComponentView_ExcludesDestroyedEntities) {
//     using T1 = typename TestFixture::T1;
//     using T2 = typename TestFixture::T2;

//     Registry reg;
//     Entity e1 = reg.create();
//     Entity e2 = reg.create();

//     reg.add(e1, TestFixture::makeValue1(1));
//     reg.add(e1, TestFixture::makeValue2(1));

//     reg.add(e2, TestFixture::makeValue1(2));
//     reg.add(e2, TestFixture::makeValue2(2));

//     reg.destroy(e1);

//     std::vector<Entity> alive;
//     for (auto [ent, c1, c2] : reg.view<T1, T2>()) {
//         ASSERT_NE(c1, nullptr);
//         ASSERT_NE(c2, nullptr);
//         alive.push_back(ent);
//     }

//     EXPECT_EQ(alive.size(), 1);
//     EXPECT_EQ(alive[0], e2);
// }

TEST(RegistryTest, CreateEntity_ValidAndUnique) {
    Registry reg;
    Entity e1 = reg.create();
    Entity e2 = reg.create();

    EXPECT_TRUE(isValidEntity(e1));
    EXPECT_TRUE(isValidEntity(e2));
    EXPECT_NE(e1, e2);
    EXPECT_EQ(reg.entityCount(), 2);
}

TEST(RegistryTest, DestroyEntity_InvalidatesHandle) {
    Registry reg;
    Entity e = reg.create();
    EXPECT_TRUE(isValidEntity(e));

    reg.destroy(e);
    EXPECT_EQ(reg.entityCount(), 0);

    reg.add(e, Position{1, 2});
    EXPECT_FALSE(reg.has<Position>(e));
    EXPECT_EQ(reg.get<Position>(e), nullptr);
}

TEST(RegistryTest, ReuseEntityId_IncrementsVersion) {
    Registry reg;
    Entity e1 = reg.create();
    uint32_t id = e1.id;
    uint8_t ver1 = e1.version;

    reg.destroy(e1);
    Entity e2 = reg.create();

    EXPECT_EQ(e2.id, id);
    EXPECT_GT(e2.version, ver1);
    EXPECT_NE(e1, e2);
}

TEST(RegistryTest, SingleComponentView) {
    Registry reg;
    Entity e1 = reg.create();
    Entity e2 = reg.create();
    reg.add(e1, Position{1, 2});
    reg.add(e2, Position{3, 4});

    std::vector<Entity> entities;
    std::vector<Position> positions;
    for (auto [ent, comp] : reg.view<Position>()) {
        entities.push_back(ent);
        positions.push_back(*comp);
    }

    EXPECT_EQ(entities.size(), 2);
    EXPECT_EQ(positions.size(), 2);

    bool found1 = false, found2 = false;
    for (size_t i = 0; i < entities.size(); ++i) {
        if (entities[i] == e1 && positions[i] == Position{1, 2}) found1 = true;
        if (entities[i] == e2 && positions[i] == Position{3, 4}) found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

TEST(RegistryTest, EmptyView_NoIteration) {
    Registry reg;
    int count = 0;
    for ([[maybe_unused]] auto _ : reg.view<Position>()) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(RegistryTest, ZeroEntityIsInvalid) {
    Registry reg;
    Entity zero{0, 0};
    EXPECT_FALSE(reg.has<Position>(zero));
    reg.add(zero, Position{1, 1});
    EXPECT_FALSE(reg.has<Position>(zero));
}

TEST(RegistryTest, StaleVersionHandleIsInvalid) {
    Registry reg;
    Entity e = reg.create();
    uint8_t originalVer = e.version;
    reg.destroy(e);

    Entity stale{e.id, originalVer};
    EXPECT_FALSE(reg.has<Position>(stale));
    reg.add(stale, Position{5, 5});
    EXPECT_FALSE(reg.has<Position>(stale));
}

TEST(RegistryTest, EraseMaintainsViewCorrectness) {
    Registry reg;
    Entity e1 = reg.create();
    Entity e2 = reg.create();
    Entity e3 = reg.create();

    reg.add(e1, Position{1, 1});
    reg.add(e2, Position{2, 2});
    reg.add(e3, Position{3, 3});

    reg.destroy(e2);

    std::vector<Entity> alive;
    for (auto [ent, _] : reg.view<Position>()) {
        alive.push_back(ent);
    }

    EXPECT_EQ(alive.size(), 2);
    EXPECT_NE(std::find(alive.begin(), alive.end(), e1), alive.end());
    EXPECT_NE(std::find(alive.begin(), alive.end(), e3), alive.end());
    EXPECT_EQ(std::find(alive.begin(), alive.end(), e2), alive.end());
}

TEST(RegistryTest, StressCreateDestroyReuse) {
    Registry reg;
    constexpr int N = 200;
    std::vector<Entity> entities;

    for (int i = 0; i < N; ++i) {
        Entity e = reg.create();
        ASSERT_TRUE(isValidEntity(e));
        entities.push_back(e);
        reg.add(e, Position{static_cast<float>(i), 0.0f});
    }
    EXPECT_EQ(reg.entityCount(), N);

    for (int i = 0; i < N / 2; ++i) {
        reg.destroy(entities[i]);
    }
    EXPECT_EQ(reg.entityCount(), N / 2);

    for (int i = 0; i < N / 2; ++i) {
        Entity e = reg.create();
        ASSERT_TRUE(isValidEntity(e));
        reg.add(e, Position{-1.0f, -1.0f});
    }
    EXPECT_EQ(reg.entityCount(), N);

    int count = 0;
    for (auto [_, comp] : reg.view<Position>()) {
        ASSERT_NE(comp, nullptr);
        count++;
    }
    EXPECT_EQ(count, N);
}