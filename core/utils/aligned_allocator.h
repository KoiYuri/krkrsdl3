

#ifndef __ALIGNED_ALLOCATOR_H__
#define __ALIGNED_ALLOCATOR_H__

#include <memory>
#include <new>
#include <cstdlib>

template<typename T, std::size_t Alignment = 16>
struct aligned_allocator
{
    using value_type = T;
    static constexpr std::size_t alignment = Alignment;

    template<typename U>
    struct rebind
    {
        using other = aligned_allocator<U, Alignment>;
    };

    aligned_allocator() = default;

    template<typename U>
    aligned_allocator(const aligned_allocator<U, Alignment>&) noexcept
    {
    }

    T* allocate(std::size_t n)
    {
#if defined(_KRKRSDL3_OHOS)
        void* p;
        posix_memalign(&p, alignment, n * sizeof(T));
        return static_cast<T*>(p);
#else
        return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t(alignment)));
#endif
    }

    void deallocate(T* p, std::size_t) noexcept
    {
#if defined(_KRKRSDL3_OHOS)
        free(p);
#else
        ::operator delete(p, std::align_val_t(alignment));
#endif
    }
};

#endif // __ALIGNED_ALLOCATOR_H__
