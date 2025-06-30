#pragma once
#include <windows.h>
#include <vector>
#include <memory>

class HybridAllocator {
public:
    static HybridAllocator& instance() {
        static HybridAllocator alloc;
        return alloc;
    }

    
    void* allocate(size_t size) {
        
        for (auto& alloc : smallAllocs) {
            if (size <= alloc->getBlockSize()) {
                return alloc->allocate();
            }
        }

        
        if (size <= 1024 * 1024) {
            return HeapAlloc(GetProcessHeap(), 0, size);
        }

        
        return VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE);
    }

    
    void deallocate(void* ptr, size_t size) {
        if (!ptr) return;

        
        for (auto& alloc : smallAllocs) {
            if (size <= alloc->getBlockSize()) {
                alloc->deallocate(ptr);
                return;
            }
        }

        
        if (size <= 1024 * 1024) {
            HeapFree(GetProcessHeap(), 0, ptr);
            return;
        }

       
        VirtualFree(ptr, 0, MEM_RELEASE);
    }


    template <typename T>
    struct allocator {
        using value_type = T;

        allocator() = default;

        template <typename U>
        allocator(const allocator<U>&) {}

        T* allocate(size_t n) {
            return static_cast<T*>(instance().allocate(n * sizeof(T)));
        }

        void deallocate(T* p, size_t n) {
            instance().deallocate(p, n * sizeof(T));
        }

        bool operator==(const allocator&) const { return true; }
        bool operator!=(const allocator&) const { return false; }
    };

private:
    HybridAllocator() {
        for (size_t size = 8; size <= 256; size *= 2) {
            smallAllocs.emplace_back(std::make_unique<SlabAllocator>(size));
        }
    }

    
    class SlabAllocator {
    public:
        SlabAllocator(size_t blockSize) : blockSize(blockSize) {}

        void* allocate() {
            if (freeList.empty()) {
                allocateNewChunk();
            }
            void* block = freeList.back();
            freeList.pop_back();
            return block;
        }

        void deallocate(void* ptr) {
            freeList.push_back(ptr);
        }

        size_t getBlockSize() const { return blockSize; }

    private:
        size_t blockSize;
        std::vector<void*> freeList;

        void allocateNewChunk() {
            constexpr size_t BLOCKS_PER_CHUNK = 64;
            void* chunk = HeapAlloc(GetProcessHeap(), 0, blockSize * BLOCKS_PER_CHUNK);
            if (!chunk) throw std::bad_alloc();

            for (size_t i = 0; i < BLOCKS_PER_CHUNK; ++i) {
                freeList.push_back(static_cast<char*>(chunk) + i * blockSize);
            }
        }
    };

    std::vector<std::unique_ptr<SlabAllocator>> smallAllocs;
};


template <typename T>
using HybridVector = std::vector<T, HybridAllocator::allocator<T>>;