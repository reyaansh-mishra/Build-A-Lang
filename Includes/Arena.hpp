#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <vector>
#include <cstddef>

class Arena {
public:
    explicit Arena(size_t chunk_size = 65536) : m_chunk_size(chunk_size) {
        alloc_chunk();
    }

    // Allocate memory for a type T and construct it
    template <typename T, typename... Args>
    T* alloc(Args&&... args) {
        void* ptr = allocate_raw(sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }

    // No individual deletes! Just clear the whole arena at once.
    ~Arena() {
        for (auto chunk : m_chunks) free(chunk);
    }


    std::string_view dup_string(std::string_view s) {
        char* mem = static_cast<char*>(allocate_raw(s.size(), 1));
        std::memcpy(mem, s.data(), s.size());
        return std::string_view(mem, s.size());
    }


private:
    void* allocate_raw(size_t size, size_t alignment) {
        size_t padding = (alignment - (reinterpret_cast<uintptr_t>(m_current_ptr) % alignment)) % alignment;
        if (m_offset + size + padding > m_chunk_size) {
            alloc_chunk();
            padding = 0; // Fresh chunk is always aligned
        }
        m_current_ptr = static_cast<char*>(m_current_ptr) + padding;
        void* result = m_current_ptr;
        m_current_ptr = static_cast<char*>(m_current_ptr) + size;
        m_offset += size + padding;
        return result;
    }

    void alloc_chunk() {
        m_chunks.push_back(malloc(m_chunk_size));
        m_current_ptr = m_chunks.back();
        m_offset = 0;
    }

    std::vector<void*> m_chunks;
    void* m_current_ptr;
    size_t m_offset;
    size_t m_chunk_size;
};