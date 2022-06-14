#include "MemoryManager.h"
#include "Vector.h"

#include <cstdlib>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifdef ARENA_DEBUG
#include <iostream>
#endif

static Vector* MEMORY_STACK = nullptr;
static Vector* LARGE_OBJECT_LIST = nullptr;
static volatile int LOCK = 0;

typedef Arena* ArenaPtr;
typedef void* VoidPtr;

RegisterVectorStatics(Vec)
RegisterVectorFor(ArenaPtr, Vec)
RegisterVectorFor(VoidPtr, Vec)

// Ensure the memory manager is ready. It starts with an empty stack
void StartManagedMemory() {
    if (LOCK != 0) return; // weak protection, yes.
    if (MEMORY_STACK != nullptr) return;
    LOCK = 1;

    auto baseArena = NewArena((128 KILOBYTES));
    MEMORY_STACK = VecAllocateArena_ArenaPtr(baseArena);
    LARGE_OBJECT_LIST = VecAllocateArena_VoidPtr(baseArena);
    VecPush_ArenaPtr(MEMORY_STACK, baseArena);

    LOCK = 0;
}
// Close all arenas and return to stdlib memory
void ShutdownManagedMemory() {
    if (LOCK != 0) return;
    LOCK = 1;

    if (LARGE_OBJECT_LIST != nullptr){
        auto *vec = (Vector *) LARGE_OBJECT_LIST;
        LARGE_OBJECT_LIST = nullptr;
        void* a = nullptr;
        while (VecPop_VoidPtr(vec, &a)) {
            free(a);
        }
        VecDeallocate(vec);
    }

    if (MEMORY_STACK != nullptr) {
        auto *vec = (Vector *) MEMORY_STACK;
        MEMORY_STACK = nullptr;
        ArenaPtr a = nullptr;
        ArenaPtr baseArena = nullptr;

        // cut the base arena out of the vector
        VecDequeue_ArenaPtr(vec, &baseArena);

        // drop all other arenas
        while (VecPop_ArenaPtr(vec, &a)) {
            DropArena(&a);
        }

        // drop the base arena (takes MEMORY_STACK and LARGE_OBJECT_LIST with it)
        DropArena(&baseArena);
    }

    LOCK = 0;
}

// Start a new arena, keeping memory and state of any existing ones
bool MMPush(size_t arenaMemory) {
    if (MEMORY_STACK == nullptr) return false;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
    while (LOCK != 0) {}
#pragma clang diagnostic pop
    LOCK = 1;

    auto* vec = (Vector*)MEMORY_STACK;
    auto a = NewArena(arenaMemory);
    bool result = false;
    if (a != nullptr) {
        result = VecPush_ArenaPtr(vec, a);
        if (!result) DropArena(&a);
    }

    LOCK = 0;
    return result;
}

// Deallocate the most recent arena, restoring the previous
void MMPop() {
    if (MEMORY_STACK == nullptr) return;
    if (VecLength(MEMORY_STACK) <= 1) return; // don't pop off our own arena
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
    while (LOCK != 0) {}
#pragma clang diagnostic pop
    LOCK = 1;

    auto* vec = (Vector*)MEMORY_STACK;
    ArenaPtr a = nullptr;
    if (VecPop_ArenaPtr(vec, &a)) {
        DropArena(&a);
    }

    LOCK = 0;
}

// Deallocate the most recent arena, copying a data item to the next one down (or permanent memory if at the bottom of the stack)
void* MMPopReturn(void* ptr, size_t size) {
    if (MEMORY_STACK == nullptr) return nullptr;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
    while (LOCK != 0) {}
#pragma clang diagnostic pop
    LOCK = 1;

    void* result;
    auto* vec = (Vector*)MEMORY_STACK;
    ArenaPtr a = nullptr;
    if (VecPop_ArenaPtr(vec, &a)) {
        if (VecPeek_ArenaPtr(vec, &a)) { // there is another arena. Copy there
            result = CopyToArena(ptr, size, a);
        } else { // no more arenas. Dump in regular memory
            result = MakePermanent(ptr, size);
        }
        DropArena(&a);
    } else { // nothing to pop. Raise null to signal stack underflow
        result = nullptr;
    }

    LOCK = 0;
    return result;
}

// Return the current arena, or nullptr if none pushed
// TODO: if nothing pushed, push a new small arena
Arena* MMCurrent() {
    if (MEMORY_STACK == nullptr) return nullptr;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
    while (LOCK != 0) {}
#pragma clang diagnostic pop
    LOCK = 1;

    auto* vec = (Vector*)MEMORY_STACK;
    ArenaPtr result = nullptr;
    VecPeek_ArenaPtr(vec, &result);

    LOCK = 0;
    return result;
}

void *MMAllocate(size_t byteCount) {
    if (byteCount > ARENA_ZONE_SIZE) { // stdlib allocation and add to large object list
        auto ptr = malloc(byteCount);
        if (ptr != nullptr) VecPush_VoidPtr(LARGE_OBJECT_LIST, ptr);
        return ptr;
    } else { // use the small bump allocator
        auto current = MMCurrent();
        if (current == nullptr) return nullptr;
        return ArenaAllocate(current, byteCount);
    }
}

// Check if the current area has this pointer, then scan down the stack.
// Finally, try the large object list (which should not see alloc/dealloc in common code)
void MMDrop(void *ptr) {
    auto current = MMCurrent();
    if (current != nullptr){
        auto offset = ArenaPtrToOffset(current, ptr);
        if (offset > 0) { // found it
            ArenaDereference(current, ptr);
            return;
        }
        // TODO: hunt further down the stacks, return if found
    }

    // scan through the large object list, `free` if found
    uint32_t len = VectorLength(LARGE_OBJECT_LIST);
    for (uint32_t i = 0; i < len; ++i) {
        auto lob = VecGet_VoidPtr(LARGE_OBJECT_LIST, i);
        if (lob == ptr){
            VecSet_VoidPtr(LARGE_OBJECT_LIST, i, nullptr, nullptr); // null this item so we don't double free. We could trim the array, but it shouldn't be needed.
            free(lob);
            return;
        }
    }
}

// Allocate memory array, cleared to zeros
void* mcalloc(int count, size_t size) {
    ArenaPtr a = MMCurrent();
    if (a != nullptr) {
        return ArenaAllocateAndClear(a, count*size);
    } else {
        return calloc(count, size);
    }
}

// Free memory
void mfree(void* ptr) {
    if (ptr == nullptr) return;
    // we might not be freeing from the current arena, so this can get complex
    ArenaPtr a = MMCurrent();
    if (a == nullptr) { // no arenas. stdlib free
        free(ptr);
        return;
    }
    if (ArenaContainsPointer(a, ptr)) { // in the most recent arena
        ArenaDereference(a, ptr);
        return;
    }

    // otherwise, scan through all the arenas until we find it
    // it might be simpler to leak the memory and let the arena get cleaned up whenever

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
    while (LOCK != 0) {}
#pragma clang diagnostic pop
    LOCK = 1;

    auto* vec = (Vector*)MEMORY_STACK;
    int count = VecLength(vec);
    for (int i = 0; i < count; i++) {
        ArenaPtr af = *VecGet_ArenaPtr(vec, i);
        if (af == nullptr) continue;
        if (ArenaContainsPointer(af, ptr)) {
            ArenaDereference(af, ptr);
            return;
        }
    }
    // never found it. Either bad call or we've leaked some memory

#ifdef ARENA_DEBUG
    std::cout << "mfree failed. Memory leaked.\n";
#endif

    LOCK = 0;
}
#pragma clang diagnostic pop