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

using Entity = int;
constexpr Entity INVALID_ENTITY = -1;

class Registry;

struct Parent;
struct Children;

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void erase(Entity e) = 0;
    virtual bool has(Entity e) const = 0;
    virtual size_t size() const = 0;
};

// BIGTODO::
// TODO: use sparse sets for component storage? ... use an "alive" bit in the entity data or defer destruction to batches?
// current approach tracks destroyed entities in a separate unordered_set and checks this set during get, has, AND view operations - too much overhead
// the current cleanup() does not reclaim component pool memory effectively (just entities)

template<typename T>
class ComponentPool : public IComponentPool {
private:
    std::vector<std::optional<T>> components; // storage for components
    std::unordered_map<Entity, size_t> entityToIndex; // maps an Entity (an integer ID) to its corresponding index in the components vector. Serves as lookup table to quickly find the index of an entity's component in the components vector
    std::vector<Entity> indexToEntity; // reverse mapping for the entityToIndex map. Maps an index in the components vector back to the Entity that owns the component at that index
    std::queue<size_t> freeIndices; //  tracks indices in the components vector that are no longer in use and can be reused for new components; in the erase function, the index of the removed component is added to the freeIndices queue
    size_t validCount = 0; // used to track the number of active (valid) components currently stored in the pool

public:
    T* add(Entity e, T comp) {
        size_t index;

        if (!freeIndices.empty()) {
            // reuse a free slot
            index = freeIndices.front();
            freeIndices.pop();
            
            // ensure components vector is large enough
            if (index >= components.size()) {
                components.resize(index + 1);
                indexToEntity.resize(index + 1, INVALID_ENTITY);
            }
            
            components[index] = std::move(comp);
            indexToEntity[index] = e;
        } else {
            // append a new slot
            index = components.size();
            components.emplace_back(std::move(comp));
            indexToEntity.push_back(e);
        }

        entityToIndex[e] = index;
        validCount++;
        return &components[index].value();
    }

    T* get(Entity e) {
        auto it = entityToIndex.find(e);
        if (it == entityToIndex.end() || it->second >= components.size()) 
            return nullptr;
        
        auto& opt = components[it->second];
        return opt.has_value() ? &opt.value() : nullptr;
    }

    const T* get(Entity e) const {
        auto it = entityToIndex.find(e);
        if (it == entityToIndex.end() || it->second >= components.size()) 
            return nullptr;
        
        const auto& opt = components[it->second];
        return opt.has_value() ? &opt.value() : nullptr;
    }

    void erase(Entity e) override {
        auto it = entityToIndex.find(e);
        if (it == entityToIndex.end()) return;
        
        size_t index = it->second;
        if (index < components.size() && components[index].has_value()) {
            components[index].reset();
            entityToIndex.erase(e);
            freeIndices.push(index);
            validCount--;
        }
    }

    bool has(Entity e) const override {
        auto it = entityToIndex.find(e);
        return it != entityToIndex.end() && it->second < components.size() && components[it->second].has_value();
    }

    size_t size() const override {
        return validCount;
    }

    // for iteration (non-const version)
    std::vector<std::optional<T>>& getComponents() { return components; }
    std::vector<Entity>& getEntities() { return indexToEntity; }
    
    // for iteration (const version)
    const std::vector<std::optional<T>>& getComponents() const { return components; }
    const std::vector<Entity>& getEntities() const { return indexToEntity; }
};

class Registry {
private:
    Entity nextId = 1;
    std::vector<Entity> entities;
    std::unordered_set<Entity> destroyedEntities; // track destroyed entities
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools;
    std::queue<Entity> freeList; // entity IDs that were destroyed and are now available for reuse (FIFO)

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
        // it->first will give you the key and it->second will give you the val from a std::unordered_map
        return static_cast<ComponentPool<T>*>(it->second.get());
    }

    template<typename T>
    const ComponentPool<T>* getPool() const {
        auto type = std::type_index(typeid(T));
        auto it = pools.find(type);
        return it == pools.end() ? nullptr : static_cast<ComponentPool<T>*>(it->second.get());
    }

public:
    Entity create() {
        Entity e;
        
        if (!freeList.empty()) {
            e = freeList.front();
            freeList.pop();
        } else {
            e = nextId++;
        }
        
        entities.push_back(e);
        destroyedEntities.erase(e); // ensure it's not marked as destroyed
        return e;
    }

    void destroy(Entity e) {
        // check if entity is already destroyed
        if (destroyedEntities.find(e) != destroyedEntities.end()) {
            return;
        }
        
        // mark as destroyed for lookup
        destroyedEntities.insert(e);
        
        // remove from all component pools
        for (auto& [_, pool] : pools) {
            pool->erase(e);
        }
        
        freeList.push(e);

        // TODO: shrink components vector as well
        // TODO: enforce child cleanup
    }

    template<typename T>
    void add(Entity e, T comp) {
        // check if entity is destroyed
        if (destroyedEntities.find(e) != destroyedEntities.end()) {
            return;
        }
        getPool<T>()->add(e, std::move(comp));
    }

    template<typename T>
    T* get(Entity e) {
        // check if entity is destroyed
        if (destroyedEntities.find(e) != destroyedEntities.end()) {
            return nullptr;
        }
        auto pool = getPool<T>();
        return pool ? pool->get(e) : nullptr;
    }

    template<typename T>
    const T* get(Entity e) const {
        // check if entity is destroyed
        if (destroyedEntities.find(e) != destroyedEntities.end()) {
            return nullptr;
        }
        auto pool = getPool<T>();
        return pool ? pool->get(e) : nullptr;
    }

    template<typename T>
    bool has(Entity e) const {
        // check if entity is destroyed
        if (destroyedEntities.find(e) != destroyedEntities.end()) {
            return false;
        }
        auto pool = getPool<T>();
        return pool ? pool->has(e) : false;
    }

    // single-component view ... filter out destroyed entities
    template<typename T>
    std::vector<std::pair<Entity, T*>> view() {
        std::vector<std::pair<Entity, T*>> result;
        auto pool = getPool<T>();
        if (!pool) return result;
        
        auto& components = pool->getComponents();
        auto& entities = pool->getEntities();
        
        for (size_t i = 0; i < components.size(); ++i) {
            if (components[i].has_value()) {
                Entity e = entities[i];
                // skip destroyed entities
                if (destroyedEntities.find(e) == destroyedEntities.end()) {
                    result.emplace_back(e, &components[i].value());
                }
            }
        }
        return result;
    }

    // const version of single-component view
    template<typename T>
    std::vector<std::pair<Entity, const T*>> view() const {
        std::vector<std::pair<Entity, const T*>> result;
        auto pool = getPool<T>();
        if (!pool) return result;
        
        auto& components = pool->getComponents();
        auto& entities = pool->getEntities();
        
        for (size_t i = 0; i < components.size(); ++i) {
            if (components[i].has_value()) {
                Entity e = entities[i];
                // skip destroyed entities
                if (destroyedEntities.find(e) == destroyedEntities.end()) {
                    result.emplace_back(e, &components[i].value());
                }
            }
        }
        return result;
    }

    // multi-component view - filter out destroyed entities
    template<typename T1, typename T2, typename... Rest>
    std::vector<std::tuple<Entity, T1*, T2*, Rest*...>> view() {
        std::vector<std::tuple<Entity, T1*, T2*, Rest*...>> result;
        
        // get all entities that have the first component
        auto pool1 = getPool<T1>();
        if (!pool1) return result;
        
        auto& entities1 = pool1->getEntities();
        auto& components1 = pool1->getComponents();
        
        for (size_t i = 0; i < components1.size(); ++i) {
            if (!components1[i].has_value()) continue;
            
            Entity e = entities1[i];
            
            // skip destroyed entities
            if (destroyedEntities.find(e) != destroyedEntities.end()) {
                continue;
            }
            
            // check if entity has all required components
            if (has<T2>(e) && (has<Rest>(e) && ...)) {
                result.emplace_back(e, &components1[i].value(), get<T2>(e), get<Rest>(e)...);
            }
        }        
        return result;
    }

    // const version of multi-component view
    template<typename T1, typename T2, typename... Rest>
    std::vector<std::tuple<Entity, const T1*, const T2*, const Rest*...>> view() const {
        std::vector<std::tuple<Entity, const T1*, const T2*, const Rest*...>> result;
        
        // get all entities that have the first component
        auto pool1 = getPool<T1>();
        if (!pool1) return result;
        
        auto& entities1 = pool1->getEntities();
        auto& components1 = pool1->getComponents();
        
        for (size_t i = 0; i < components1.size(); ++i) {
            if (!components1[i].has_value()) continue;
            
            Entity e = entities1[i];
            
            // skip destroyed entities
            if (destroyedEntities.find(e) != destroyedEntities.end()) {
                continue;
            }
            
            // check if entity has all required components
            if (has<T2>(e) && (has<Rest>(e) && ...)) {
                result.emplace_back(e, &components1[i].value(), get<T2>(e), get<Rest>(e)...);
            }
        }
        
        return result;
    }

    size_t entityCount() const {
        // TODO: is 'entities.size()' misleading since entities contains destroyed ones until cleanup occurs on the main loop
        // TODO: remove from entities on destroy?
        return entities.size() - destroyedEntities.size();
    }
    
    void cleanup() {
        // remove destroyed entities from the entities list
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                [this](Entity e) { return destroyedEntities.find(e) != destroyedEntities.end(); }),
            entities.end()
        );
        destroyedEntities.clear();

        // ... more sophisticated memory management later 
        // for (auto& [_, pool] : pools) {
        //     
        // }
    }
};