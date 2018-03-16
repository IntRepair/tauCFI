#ifndef __MPL_H
#define __MPL_H

#include <type_traits>

namespace mpl
{
template <typename type_t> struct unqualified_type
{
    using type = typename std::remove_const<typename std::remove_reference<type_t>::type>::type;
};
};

#endif /* __MPL_H */