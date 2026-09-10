#include "system_architecture.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string_view>
#if defined(_WIN32)
#include <malloc.h>
#endif

namespace {
// Injection is thread-local and armed only around an operation with no workers.
thread_local long allocationCountdown = -1;
void failAllocation() {
    if (allocationCountdown == 0) throw std::bad_alloc{};
    if (allocationCountdown > 0) --allocationCountdown;
}
void* allocate(std::size_t size) {
    failAllocation();
    if (void* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc{};
}
void* allocateAligned(std::size_t size, std::size_t alignment) {
    failAllocation();
    void* p = nullptr;
#if defined(_WIN32)
    p = _aligned_malloc(size == 0 ? 1 : size, alignment);
#else
    if (posix_memalign(&p, alignment, size == 0 ? 1 : size) != 0) p = nullptr;
#endif
    if (!p) throw std::bad_alloc{};
    return p;
}
void freeAligned(void* p) noexcept {
#if defined(_WIN32)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
}
void* operator new(std::size_t n) { return allocate(n); }
void* operator new[](std::size_t n) { return allocate(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void* operator new(std::size_t n, std::align_val_t a) { return allocateAligned(n, static_cast<std::size_t>(a)); }
void* operator new[](std::size_t n, std::align_val_t a) { return allocateAligned(n, static_cast<std::size_t>(a)); }
void operator delete(void* p, std::align_val_t) noexcept { freeAligned(p); }
void operator delete[](void* p, std::align_val_t) noexcept { freeAligned(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { freeAligned(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { freeAligned(p); }

namespace {
using AdaptiveMesh::SpatialAdaptiveMesh;
void numericFailure(bool automatic) {
    SpatialAdaptiveMesh mesh(1);
    const double coordinates[] = {0.0, 1.0, 1e308, -1e308};
    for (std::size_t i = 0; i < 4; ++i) mesh.addNode(i, {coordinates[i], 0, 0}, 0);
    mesh.simulationStep(); // Warm buffers before the failed topology change.
    bool rejected = false;
    try {
        if (automatic) mesh.autoConnectNearbyNodes(2.0);
        else mesh.connectNodePairs({{0, 1}, {2, 3}});
    } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "late geometry failure was not rejected");
    for (std::size_t i = 0; i < 4; ++i) {
        require(mesh.getNodeBridgesCount(i) == 0, "partial topology after numeric exception");
        require(mesh.getNodeState(i) == 0 && mesh.getNodeHealth(i) == 1,
                "numeric exception changed state/health");
    }
    mesh.simulationStep();
    mesh.connectNodes(0, 1);
    mesh.simulationStep();
}
void allocationSweep(int operation) {
    std::size_t failures = 0;
    for (long failureIndex = 0; failureIndex < 4096; ++failureIndex) {
        SpatialAdaptiveMesh mesh(1);
        for (std::size_t i = 0; i < 4; ++i) mesh.addNode(i, {static_cast<double>(i), 0, 0}, 0);
        // Existing records must survive rollback of a later operation.
        mesh.connectNodes(0, 1);
        const std::vector<std::pair<int, int>> pairs{{1, 2}, {2, 3}};
        auto perform = [&] {
            switch (operation) {
            case 0: mesh.addNode(4, {4, 0, 0}, 0); break;
            case 1: mesh.connectNodes(1, 2); break;
            case 2: mesh.connectNodePairs(pairs); break;
            default: mesh.autoConnectNearbyNodes(1.1); break;
            }
        };
        bool failed = false;
        allocationCountdown = failureIndex;
        try { perform(); }
        catch (const std::bad_alloc&) { failed = true; }
        catch (...) { allocationCountdown = -1; throw; }
        allocationCountdown = -1;
        if (failed) {
            ++failures;
            for (std::size_t i = 0; i < 4; ++i) {
                require(mesh.getNodeBridgesCount(i) == (i < 2 ? 1U : 0U), "allocation failure changed edges");
                require(mesh.getNodeState(i) == 0 && mesh.getNodeHealth(i) == 1, "allocation failure changed values");
            }
            if (operation == 0) {
                bool absent = false;
                try { static_cast<void>(mesh.getNodeState(4)); }
                catch (const std::out_of_range&) { absent = true; }
                require(absent, "failed insertion published node");
            }
            mesh.simulationStep();
            perform(); // Retry also checks that provisional lifecycle entries were erased.
        }
        require(mesh.getNodeBridgesCount(0) == 1, "existing edge lost");
        if (operation == 0) require(mesh.getNodeState(4) == 0, "retry insertion failed");
        else require(mesh.getNodeBridgesCount(1) == 2, "retry connection failed");
        mesh.simulationStep();
        if (!failed) {
            require(failures != 0, "allocation injection did not exercise failures");
            std::cout << "allocation operation=" << operation << " failures=" << failures << '\n';
            return;
        }
    }
    throw std::runtime_error("allocation sweep bound exhausted");
}
}
int main(int argc, char** argv) {
    try {
        if (argc > 1) {
            numericFailure(std::string_view(argv[1]) == "automatic");
        } else {
            numericFailure(false);
            numericFailure(true);
            for (int op = 0; op < 4; ++op) allocationSweep(op);
        }
        std::cout << "Topology exception safety passed\n";
        return 0;
    } catch (const std::exception& error) {
        allocationCountdown = -1;
        std::cerr << error.what() << '\n';
        return 1;
    }
}
