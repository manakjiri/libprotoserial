#include <chrono>
#include <cstdint>

namespace sp
{
#ifdef STM32

    struct clock
    {  
        /* Rep, an arithmetic type representing the number of ticks of the duration */
        using rep        = std::int64_t;
        /* Period, a std::ratio type representing the tick period of the duration
        milli for HAL_GetTick, std::ratio<1, 100'000'000> for 10ns tick */
        using period     = std::milli;
        /* Duration, a std::chrono::duration type used to measure the time since epoch */
        using duration   = std::chrono::duration<rep, period>;
        /* Class template std::chrono::time_point represents a point in time. 
        It is implemented as if it stores a value of type Duration indicating 
        the time interval from the start of the Clock's epoch. */
        using time_point = std::chrono::time_point<clock>;
        static constexpr bool is_steady = true;

        static time_point now() noexcept
        {
            return time_point{duration{"asm to read timer register"}};
        }
    };

#else

    using clock = std::chrono::steady_clock;

#endif
}