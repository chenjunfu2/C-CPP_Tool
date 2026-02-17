#include <stdio.h>

#include <array>
#include <tuple>

template<typename T, std::size_t N, std::size_t... Is>
constexpr auto array_to_tuple(const std::array<T, N>& arr, std::index_sequence<Is...>)
{
    return std::make_tuple(arr[Is]...);
}

template<std::copy_constructible T, std::size_t... N>
constexpr auto array_concat(const std::array<T, N>&... arrays)
{
    return std::apply([](auto... elems){ return std::array<T, sizeof...(elems)>{elems...}; }, std::tuple_cat(array_to_tuple(arrays, std::make_index_sequence<N>{})...));
}

int main(void)
{
    std::array<int,3> a0{0,1,2};
    std::array<int,2> a1{3,4};

    auto ret = array_concat(a0,a1);

    for(auto&it:ret)
    {
        printf("%d,",it);
    }

    return 0;
}