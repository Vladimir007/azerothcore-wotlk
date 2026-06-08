#ifndef MATH_UTIL_H
#define MATH_UTIL_H

#include <algorithm>
#include <iterator>
#include <numeric>
#include <vector>

template <typename Container, typename T = std::decay_t<decltype(*std::begin(std::declval<Container>()))>>
T standard_deviation(Container&& c)
{
    auto b = std::begin(c), e = std::end(c);
    auto size = std::distance(b, e);
    auto sum = std::accumulate(b, e, T());
    auto mean = sum / size;

    if (size == 1)
        return static_cast<T>(0);

    T accum = T();
    for (const auto d : c)
    {
        accum += (d - mean) * (d - mean);
    }
    return std::sqrt(accum / (size - 1));
}

template <typename Container, typename T = std::decay_t<decltype(*std::begin(std::declval<Container>()))>>
T mean(Container&& c)
{
    auto b = std::begin(c), e = std::end(c);
    auto size = std::distance(b, e);
    auto sum = std::accumulate(b, e, T());
    return sum / size;
}

template <typename T>
T median(std::vector<T> a)
{
    const std::size_t n = a.size();

    // If size of the arr[] is even
    if (n % 2 == 0)
    {
        // Applying nth_element on (n/2)th index
        std::nth_element(a.begin(), a.begin() + n / 2, a.end());

        // Applying nth_element on (n-1)/2 th index
        std::nth_element(a.begin(), a.begin() + (n - 1) / 2, a.end());

        // Find the average of value at index N/2 and (N-1)/2
        return static_cast<T>(a[(n - 1) / 2] + a[n / 2]) / 2.0;
    }

    // If size of the arr[] is odd

    // Applying nth_element on n/2
    std::nth_element(a.begin(), a.begin() + n / 2, a.end());

    // Value at index (N/2)th is the median
    return static_cast<T>(a[n / 2]);
}

#endif
