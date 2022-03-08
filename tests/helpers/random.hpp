

#include <cstdlib>
#include "libprotoserial/container.hpp"

uint random(uint from, uint to)
{
    return from + (std::rand() % (to - from + 1));
}

/* bool chance(uint percent)
{
    return random(1, 100) <= percent;
} */

bool chance(double percent)
{
    return ((std::rand() * 1.0) / (RAND_MAX / 100.0)) < percent;
}

sp::byte random_byte()
{
    return (sp::byte)std::rand();
}

sp::bytes random_bytes(sp::bytes::size_type size)
{
    sp::bytes b(size);
    std::generate(b.begin(), b.end(), random_byte);
    return b;    
}

sp::bytes random_bytes(uint from, uint to)
{
    return random_bytes(random(from, to));
}
