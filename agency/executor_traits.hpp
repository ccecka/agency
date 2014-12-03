#pragma once

#include <agency/detail/type_traits.hpp>
#include <agency/detail/tuple_of_references.hpp>
#include <agency/execution_categories.hpp>


namespace agency
{
namespace detail
{


__DEFINE_HAS_NESTED_TYPE(has_index_type, index_type);
__DEFINE_HAS_NESTED_TYPE(has_shape_type, shape_type);


} // end detail


template<class Executor>
struct executor_traits
{
  private:
    template<class T>
    struct executor_index
    {
      using type = typename T::index_type;
    };

    template<class T>
    struct executor_shape
    {
      using type = typename T::shape_type;
    };

  public:
    using executor_type = Executor;

    using execution_category = typename Executor::execution_category;

    using index_type = typename detail::lazy_conditional<
      detail::has_index_type<executor_type>::value,
      executor_index<executor_type>,
      detail::identity<size_t>
    >::type;

    using shape_type = typename detail::lazy_conditional<
      detail::has_shape_type<executor_type>::value,
      executor_shape<executor_type>,
      detail::identity<index_type>
    >::type;


    // XXX we could make .bulk_async(f, shape, shared_arg) optional
    //     the default implementation could create a launcher agent to own the shared arg and wait for the
    //     workers
    template<class Function, class T>
    static std::future<void> bulk_async(executor_type& ex, Function f, shape_type shape, T shared_arg)
    {
      return ex.bulk_async(f, shape, shared_arg);
    }

  private:
    template<class Function>
    struct test_for_bulk_async
    {
      template<
        class Executor2,
        typename = decltype(std::declval<Executor2*>()->bulk_async(
        std::declval<Function>(),
        std::declval<shape_type>()))
      >
      static std::true_type test(int);

      template<class>
      static std::false_type test(...);

      using type = decltype(test<executor_type>(0));
    };

    template<class Function>
    using has_bulk_async = typename test_for_bulk_async<Function>::type;

    template<class Function>
    static std::future<void> bulk_async_impl(executor_type& ex, Function f, shape_type shape, std::true_type)
    {
      return ex.bulk_async(f, shape);
    }

    template<class Function>
    static std::future<void> bulk_async_impl(executor_type& ex, Function f, shape_type shape, std::false_type)
    {
      return bulk_async(ex, [=](index_type index, const shape_type&) mutable
      {
        f(index);
      },
      shape, shape
      );
    }

  public:
    template<class Function>
    static std::future<void> bulk_async(executor_type& ex, Function f, shape_type shape)
    {
      return bulk_async_impl(ex, f, shape, has_bulk_async<Function>());
    }

  private:
    template<class Function, class T>
    struct test_for_bulk_invoke_with_shared_arg
    {
      template<
        class Executor2,
        typename = decltype(std::declval<Executor2*>()->bulk_invoke(
        std::declval<Function>(),
        std::declval<shape_type>(),
        std::declval<T>()))
      >
      static std::true_type test(int);

      template<class>
      static std::false_type test(...);

      using type = decltype(test<executor_type>(0));
    };

    template<class Function, class T>
    using has_bulk_invoke_with_shared_arg = typename test_for_bulk_invoke_with_shared_arg<Function,T>::type;

    template<class Function, class T>
    static void bulk_invoke_with_shared_arg_impl(executor_type& ex, Function f, shape_type shape, T shared_arg, std::true_type)
    {
      ex.bulk_invoke(f, shape, shared_arg);
    }

    template<class Function, class T>
    static void bulk_invoke_with_shared_arg_impl(executor_type& ex, Function f, shape_type shape, T shared_arg, std::false_type)
    {
      bulk_async(ex, f, shape, shared_arg).wait();
    }

  public:
    template<class Function, class T>
    static void bulk_invoke(executor_type& ex, Function f, shape_type shape, T shared_arg)
    {
      bulk_invoke_with_shared_arg_impl(ex, f, shape, shared_arg, has_bulk_invoke_with_shared_arg<Function,T>());
    }

  private:
    template<class Function>
    struct test_for_bulk_invoke
    {
      template<
        class Executor2,
        class Function2,
        typename = decltype(std::declval<Executor2*>()->bulk_invoke(
        std::declval<Function2>(),
        std::declval<shape_type>()))
      >
      static std::true_type test(int);

      template<class,class>
      static std::false_type test(...);

      using type = decltype(test<executor_type,Function>(0));
    };

    template<class Function>
    using has_bulk_invoke = typename test_for_bulk_invoke<Function>::type;

    template<class Function>
    static void bulk_invoke_impl(executor_type& ex, Function f, shape_type shape, std::true_type)
    {
      ex.bulk_invoke(f, shape);
    }

    template<class Function>
    static void bulk_invoke_impl(executor_type& ex, Function f, shape_type shape, std::false_type)
    {
      bulk_async(ex, f, shape).wait();
    }

  public:
    template<class Function>
    static void bulk_invoke(executor_type& ex, Function f, shape_type shape)
    {
      bulk_invoke_impl(ex, f, shape, has_bulk_invoke<Function>());
    }

  private:
    template<class Executor1, class T1>
    struct test_for_shared_param_type
    {
      template<
        class Executor2,
        class T2
      >
      static std::true_type test(typename Executor2::template shared_param_type<T2>*);

      template<class,class>
      static std::false_type test(...);

      using type = decltype(test<Executor1,T1>(0));
    };

    template<class T>
    using has_shared_param_type = typename test_for_shared_param_type<executor_type,T>::type;

    template<class T>
    struct tuple_of_references_t
    {
      using type = decltype(detail::tuple_of_references(*std::declval<T*>()));
    };

    template<class Executor1, class T>
    struct executor_shared_param_type
    {
      using type = typename Executor1::template shared_param_type<T>;
    };

    // check if executor_type has a declared shared_param_type for T
    // if so, use it
    // else, check if execution_category is nested
    // if so, interpret T as a tuple and the shared param type is a tuple of references to T's elements
    // else, the shared param type is just a reference to T
    template<class T>
    struct shared_param_type_impl
      : detail::lazy_conditional<
          has_shared_param_type<T>::value,
          executor_shared_param_type<executor_type,T>,         
          detail::lazy_conditional<
            detail::is_nested_execution_category<execution_category>::value,
            tuple_of_references_t<T>,
            std::add_lvalue_reference<T>
          >
        >
    {
    };

  public:
    template<class T>
    using shared_param_type = typename shared_param_type_impl<T>::type;
};


template<class Executor, class Function, class... Args>
std::future<void> bulk_async(Executor& ex,
                             typename executor_traits<Executor>::shape_type shape,
                             Function&& f,
                             Args&&... args)
{
  auto g = std::bind(std::forward<Function>(f), std::placeholders::_1, std::forward<Args>(args)...);
  return executor_traits<Executor>::bulk_async(ex, f, shape);
}


template<class Executor, class Function, class... Args>
void bulk_invoke(Executor& ex,
                 typename executor_traits<Executor>::shape_type shape,
                 Function&& f,
                 Args&&... args)
{
  auto g = std::bind(std::forward<Function>(f), std::placeholders::_1, std::forward<Args>(args)...);
  return executor_traits<Executor>::bulk_invoke(ex, f, shape);
}


} // end agency

