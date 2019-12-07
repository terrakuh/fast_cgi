#pragma once

#include "allocator.hpp"

#include <cassert>
#include <cstddef>
#include <cstdlib>

namespace fast_cgi {

class simple_allocator : public allocator
{
public:
    virtual void* allocate(std::size_t size, std::size_t align) override
    {
        assert((align == 1 || align % 2 == 0) && align <= alignof(std::max_align_t));

        return std::malloc(size);
    }
    virtual void deallocate(void* ptr, std::size_t size) override
    {
        std::free(ptr);
    }
};

} // namespace fast_cgi