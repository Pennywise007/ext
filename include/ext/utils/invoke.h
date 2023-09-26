// Helpers for invoking functions and reducing arguments move/copying count

#pragma once

#include <memory>
#include <type_traits>
#include <tuple>
#include <utility>

namespace ext {

template <typename Function, typename... Types>
std::invoke_result_t<Function, Types...> Invoke(Function&& function, std::tuple<Types...>&& tuple)
{
    return std::apply(std::forward<Function>(function), std::forward<std::tuple<Types...>>(tuple));
}

template <typename Function, typename Tuple, size_t... ArgumentsIndexes>
std::invoke_result_t<Function, decltype(std::get<ArgumentsIndexes>(std::declval<Tuple>()))...>
    Invoke(Function&& function, Tuple&& tuple, std::index_sequence<ArgumentsIndexes...>)
{
    return std::invoke(
        std::forward<Function>(function),
        std::get<ArgumentsIndexes>(std::move<Tuple>(tuple))...);
}

// Wrap params and function in a unique_ptr decay which allows to avoid extra copying parameters during thread creation
template <typename Function, typename... Args>
struct ThreadInvoker
{
public:
    static_assert(std::is_invocable_v<std::decay_t<Function>, std::decay_t<Args>...>,
         "Arguments must be invocable after conversion to rvalues");
    
    using _Tuple = std::tuple<std::decay_t<Function>, std::decay_t<Args>...>;

    ThreadInvoker(Function&& function, Args&&...args)
        : decay_(std::make_shared<_Tuple>(std::forward<Function>(function), std::forward<Args>(args)...))
    {}
    ThreadInvoker(ThreadInvoker &&other) noexcept = default;
    ThreadInvoker(const ThreadInvoker &other) noexcept = default;

    std::invoke_result_t<Function, Args...> operator()()
    {
        return invoke(std::make_index_sequence<1 + sizeof...(Args)>{});
    }

private:
	template<size_t... _Ind>
	std::invoke_result_t<Function, Args...> invoke(std::index_sequence<_Ind...>)
    {
        return std::invoke(std::get<_Ind>(std::move(*decay_))...);
    }

private:
    std::shared_ptr<_Tuple> decay_;
};

template<typename Function, typename... Args>
ThreadInvoker(Function&&, Args&&...) -> ThreadInvoker<Function, Args...>;

} // namespace ext