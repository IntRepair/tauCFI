#ifndef __UTILS_H
#define __UTILS_H
#include <vector>

#include <BPatch_object.h>
#include <Region.h>

namespace util
{

template <typename element_t, typename alloc_t, typename pred_t>
static bool contains_if(std::vector<element_t, alloc_t> const &container,
                        pred_t predicate)
{
    return std::end(container) !=
           std::find_if(std::begin(container), std::end(container), predicate);
}

template <typename element_t, typename alloc_t>
static bool contains(std::vector<element_t, alloc_t> const &container,
                     element_t const &element)
{
    return std::end(container) !=
           std::find(std::begin(container), std::end(container), element);
}

template <typename element_t, typename... element_ts>
auto make_vector(element_t &&element, element_ts &&... elements)
    -> std::vector<typename std::decay<element_t>::type>
{
    return {std::forward<element_t>(element), std::forward<element_ts>(elements)...};
}
};

#endif /* __UTILS_H */
