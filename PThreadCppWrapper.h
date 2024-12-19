#pragma once

#include <bits/shared_ptr.h>
#include <functional>
#include <pthread.h>
#include <tuple>

namespace PThreadBySohrab
{

template <class... Types>
using DecayedTuple = std::tuple<std::decay_t<Types>...>;

template <class Class, class Function, class... Args>
void
invoke(Class *instance, Function &&func, Args &&...args)
{
    (instance->*func)(std::forward<Args>(args)...);
}

template <class Function, class... Args>
static void *
invoker(void *arg)
{
    DecayedTuple<Function, Args...> *raw = reinterpret_cast<DecayedTuple<Function, Args...> *>(arg);

    std::shared_ptr<DecayedTuple<Function, Args...>> data =
      std::make_shared<DecayedTuple<Function, Args...>>(*raw);

    constexpr auto invoke = [](std::decay_t<Function> function,
                               std::decay_t<Args>... args) -> auto {
        return std::invoke(function, args...);
    };

    std::apply(invoke, std::move(*data.get()));

    return nullptr;
}

template <class Function, class... Args>
auto
run(pthread_t &thread, Function &&func, Args &&...args)
{
    DecayedTuple<Function, Args...> tuple {std::forward<Function>(func),
                                           std::forward<Args>(args)...};

    auto  tuplePtr   = std::make_shared<DecayedTuple<Function, Args...>>(tuple);

    void *pthreadArg = tuplePtr.get();

    pthread_create(&thread, nullptr, &invoker<Function, Args...>, pthreadArg);
}

template <class Class, typename Function, typename... Args>
auto
run(pthread_t &thread, Class *instance, Function &&func, Args &&...args)
{
    struct ThreadData
    {
        Class                *instance;
        Function              func;
        DecayedTuple<Args...> args;
    };

    auto *data =
      new ThreadData {instance, std::forward<Function>(func), {std::forward<Args>(args)...}};

    run(thread, [&]() {
        invoke(data->instance, data->func, std::forward<DecayedTuple<Args>>(data->args)...);
        // delete data;
    });
}
};    // namespace PThreadBySohrab