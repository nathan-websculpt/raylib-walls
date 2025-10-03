#pragma once
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <typeindex>
#include <tuple>
#include <memory>
#include <queue>
#include <optional>
#include <unordered_set>
#include <cstdint>
#include <limits>
#include <typeinfo> // for typeid
#include <ranges>   // c++23

// 'Entity' is now a versioned handle: 24-bit ID + 8-bit generation
// prevents bugs when entity IDs are reused
struct Entity {
    uint32_t id : 24;
    uint32_t version : 8;

    constexpr Entity() : id(0), version(0) {}
    constexpr Entity(uint32_t i, uint8_t v) : id(i), version(v) {}

    // ensure two handles refer to the exact same logical entity
    // without this, C++ would either: 
        // generate a default member-wise comparison, or
        // not allow Entity to be compared at all (if you had custom logic)
    // used in: ComponentPool::getDenseIndex()
    constexpr bool operator==(const Entity& other) const {
        return id == other.id && version == other.version;
    }

    constexpr bool operator!=(const Entity& other) const {
        return !(*this == other);
    }
};

// hash support for Entity (needed for unordered_map if used elsewhere)
// defines a specialization of the std::hash template for the Entity struct
// allows Entity objects to be used as keys in hash-based containers
namespace std { // placed inside the std namespace because std::hash is part of the C++ Standard Library
    // note: template<> struct hash<Entity> explicitly specializes the std::hash template for the Entity type
    template<>
    struct hash<Entity> {
        // computes the hash value for an Entity object... marked noexcept to indicate that it does not throw exceptions
        size_t operator()(const Entity& e) const noexcept {
            // the hash value is computed by combining the version and id fields of the Entity
            // e.version (8 bits) is shifted left by 24 bits to occupy the higher-order bits of the hash...
            // e.id (24 bits) occupies the lower-order bits.
            // the bitwise OR (|) combines these two values into a single size_t hash
            return (static_cast<size_t>(e.version) << 24) | e.id;
        }
    };
    // note: with this specialization, Entity can be used as a key in an std::unordered_map
}

constexpr Entity INVALID_ENTITY{0, 0};

class Registry;

struct Parent;
struct Children;

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void erase(Entity e) = 0;
    virtual bool has(Entity e) const = 0;
    virtual size_t size() const = 0;
    virtual const std::vector<Entity>& getEntities() const = 0;
};

// DEV:: NOW USING SPARSE SETS: O(1) access, cache-friendly iteration, immediate cleanup
// BRANCH: change-from-classes-to-ecs

template<typename T>
class ComponentPool : public IComponentPool {
private:
    // sparse[entity.id] = index in dense arrays, or UINT32_MAX if not present
    // dense_entities and dense_components are parallel vectors (no holes)
    static constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

    std::vector<uint32_t> sparse; // indexed by entity ID
    std::vector<Entity> dense_entities; // alive entities (with version)
    std::vector<T> dense_components; // contiguous components

    size_t validCount = 0;

    // get dense index if entity is alive (in sparse set) and matches version
    // ensures that operations on entities are safe and consistent
    std::optional<uint32_t> getDenseIndex(Entity e) const {
        if (e.id >= sparse.size()) return std::nullopt;
        uint32_t idx = sparse[e.id];
        if (idx == NULL_INDEX || idx >= dense_entities.size()) return std::nullopt;
        if (dense_entities[idx] != e) return std::nullopt; // version mismatch
        return idx;
    }

public:
    // ensure sparse array can hold entity ID
    void enforceSparseSize(uint32_t id) {
        if (id >= sparse.size()) {
            sparse.resize(id + 1, NULL_INDEX);
        }
    }

    T* add(Entity e, T comp) {
        enforceSparseSize(e.id);

        // if already exists, overwrite
        auto existing = getDenseIndex(e);
        if (existing) {
            dense_components[*existing] = std::move(comp);
            return &dense_components[*existing];
        }

        // add to end of dense arrays
        uint32_t idx = static_cast<uint32_t>(dense_components.size());
        sparse[e.id] = idx;
        dense_entities.push_back(e);
        dense_components.push_back(std::move(comp));
        validCount++;
        return &dense_components.back();
    }

    T* get(Entity e) {
        auto idx = getDenseIndex(e);
        return idx ? &dense_components[*idx] : nullptr;
    }

    const T* get(Entity e) const {
        auto idx = getDenseIndex(e);
        return idx ? &dense_components[*idx] : nullptr;
    }

    void erase(Entity e) override {
        auto idx = getDenseIndex(e);
        if (!idx) return;

        uint32_t last = static_cast<uint32_t>(dense_components.size() - 1);
        if (*idx != last) {
            // swap with last element
            dense_entities[*idx] = dense_entities[last];
            dense_components[*idx] = std::move(dense_components[last]);
            // update sparse for the swapped entity
            sparse[dense_entities[*idx].id] = *idx;
        }

        dense_entities.pop_back();
        dense_components.pop_back();
        sparse[e.id] = NULL_INDEX;
        validCount--;
    }

    bool has(Entity e) const override {
        return getDenseIndex(e).has_value();
    }

    size_t size() const override {
        return validCount;
    }

    // for iteration (const)
    const std::vector<Entity>& getEntities() const override { return dense_entities; }
    const std::vector<T>& getComponents() const { return dense_components; }

    // for iteration (non-const [used internally])
    // warning: do not change vector outside of ComponentPool
    std::vector<Entity>& getEntities() { return dense_entities; }
    std::vector<T>& getComponents() { return dense_components; }
};

class Registry {
private:
    static constexpr uint32_t MAX_ENTITIES = 1u << 24; // note: this is 16M entities max
    static constexpr uint8_t MAX_VERSION = std::numeric_limits<uint8_t>::max();
    static constexpr uint8_t INITIAL_VERSION = 1; // avoid version 0 for live entities

    size_t aliveEntityCount = 0;
    uint32_t nextId = 1;
    std::vector<uint8_t> entityVersions;   
    std::queue<uint32_t> freeIds; // list of reusable IDs

    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools;

    bool isValid(Entity e) const {
        if (e.id == 0) return false;
        if (e.id >= entityVersions.size()) return false;
        return e.version == entityVersions[e.id];
    }

    // ensure entityVersions can hold id
    void enforceEntityVersionSize(uint32_t id) {
        if (id >= entityVersions.size()) {
            entityVersions.resize(id + 1, 0);
        }
    }

    template<typename T>
    ComponentPool<T>* getPool() {
        auto type = std::type_index(typeid(T));
        auto it = pools.find(type);
        if (it == pools.end()) {
            auto pool = std::make_unique<ComponentPool<T>>();
            auto result = pool.get();
            pools[type] = std::move(pool);
            return result;
        }
        return static_cast<ComponentPool<T>*>(it->second.get());
    }

    template<typename T>
    const ComponentPool<T>* getPool() const {
        auto type = std::type_index(typeid(T));
        auto it = pools.find(type);
        return it == pools.end() ? nullptr : static_cast<ComponentPool<T>*>(it->second.get());
    }

    // helper for multi-component view: intersect entity lists efficiently
    template<typename T1, typename T2, typename... Rest>
    auto intersectEntities() const {
        auto pool1 = getPool<T1>();
        if (!pool1) return std::vector<Entity>{};

        // filter by T2
        auto pool2 = getPool<T2>();
        if (!pool2) return std::vector<Entity>{};
        const auto& entities2 = pool2->getEntities();
        std::unordered_set<Entity> set2(entities2.begin(), entities2.end());
        
        std::vector<Entity> result;
        for (const auto& e : pool1->getEntities()) {
            if (set2.contains(e) && (has<Rest>(e) && ...)) {
                result.push_back(e);
            }
        }
        return result;
    }

public:
    [[nodiscard]] Entity create() {
        uint32_t id;
        if (!freeIds.empty()) {
            id = freeIds.front();
            freeIds.pop();
        } else {
            if (nextId >= MAX_ENTITIES) {
                // TODO: handle error or resize
                return INVALID_ENTITY; // added return to avoid undefined behavior
            }
            id = nextId++;
        }

        enforceEntityVersionSize(id);
        uint8_t& ver = entityVersions[id];        
        if (ver == 0) ver = INITIAL_VERSION; // initialize version for new entities
        // note: version gets incremented in destroy()

        aliveEntityCount++;
        return Entity{id, ver};
    }

    void destroy(Entity e) {
        if (!isValid(e)) return; // handles id==0 and stale versions

        for (auto& [_, pool] : pools) {
            pool->erase(e);
        }

        // increment version (wrap to initial if maxed)
        enforceEntityVersionSize(e.id);
        uint8_t& ver = entityVersions[e.id];
        if (ver == MAX_VERSION) {
            ver = INITIAL_VERSION;
        } else {
            ver++;
        }

        freeIds.push(e.id);
        aliveEntityCount--;
    }

    template<typename T>
    void add(Entity e, T comp) {
        if (!isValid(e)) return;
        getPool<T>()->add(e, std::move(comp));
    }

    template<typename T>
    T* get(Entity e) {
        if (!isValid(e)) return nullptr;
        auto pool = getPool<T>();
        return pool ? pool->get(e) : nullptr;
    }

    template<typename T>
    const T* get(Entity e) const {
        if (!isValid(e)) return nullptr;
        auto pool = getPool<T>();
        return pool ? pool->get(e) : nullptr;
    }

    template<typename T>
    bool has(Entity e) const {
        if (!isValid(e)) return false;
        auto pool = getPool<T>();
        return pool ? pool->has(e) : false;
    }

    // single-component view using c++23 ranges
    template<typename T>
    auto view() const {
        // define transform lambda once to ensure consistent type
        auto transform_fn = [](const auto& pair) -> std::pair<Entity, const T*> {
            return {std::get<0>(pair), &std::get<1>(pair)};
        };
        // note: The -> is part of the trailing return type syntax... It specifies the return type of the lambda function, which takes an input pair and returns a std::pair<Entity, const T*>
        //       The trailing return type is useful when:
        //              The return type depends on the input parameters
        //              The return type cannot be easily deduced by the compiler

        auto pool = getPool<T>();
        if (!pool) {
            static const std::vector<Entity> empty_ents;
            static const std::vector<T> empty_comps;
            return std::views::zip(empty_ents, empty_comps) | std::views::transform(transform_fn);
            // note: 
            //      std::views::zip(empty_ents, empty_comps) creates a zipped view that combines the two input ranges
            //          each element of this zipped view is a tuple with one element from each of the two ranges at the corresponding position
            //          allows simultaneous iteration over empty_ents and empty_comps as tuples
            //      `| std::views::transform(transform_fn)` pipes the zipped view into a transform view, which applies the function transform_fn to each tuple element generated by the zipped view
            //
            // overall: the expression returns a new range/view where each element is the result of applying transform_fn() to the tuple of paired elements from empty_ents and empty_comps
        }
        const auto& entities = pool->getEntities();
        const auto& components = pool->getComponents();
        return std::views::zip(entities, components) | std::views::transform(transform_fn);
    }

    // this worked at some point ... 
    // TODO: pool-capture issue rewrite view<T1, T2> to use get<T>() for each component? 
    // multi-component view using sparse set intersection
    // template<typename T1, typename T2, typename... Rest>
    // auto view() const {
    //     auto entities = intersectEntities<T1, T2, Rest...>();
    //     auto pool1 = getPool<T1>();
    //     auto pool2 = getPool<T2>();
    //     // define transform lambda once
    //     auto transform_fn = [pool1, pool2, this](Entity e) -> std::tuple<Entity, const T1*, const T2*, const Rest*...> {
    //         return std::tuple_cat(
    //             std::make_tuple(e, pool1 ? pool1->get(e) : nullptr, pool2 ? pool2->get(e) : nullptr),
    //             std::make_tuple(get<Rest>(e)...)
    //         );
    //     };
    //     return entities | std::views::transform(transform_fn);
    // }

    size_t entityCount() const {
        return aliveEntityCount;
    }
};