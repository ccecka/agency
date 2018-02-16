// Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/requires.hpp>
#include <agency/detail/type_traits.hpp>
#include <agency/execution/executor/detail/adaptors/basic_executor_adaptor.hpp>
#include <agency/execution/executor/executor_traits/detail/has_twoway_execute_member.hpp>
#include <agency/execution/executor/detail/utility/twoway_execute.hpp>
#include <utility>


namespace agency
{
namespace detail
{


template<class Executor>
class twoway_executor : public basic_executor_adaptor<Executor>
{
  private:
    using super_t = basic_executor_adaptor<Executor>;

  public:
    template<class T>
    using future = typename super_t::template future<T>;

    __AGENCY_ANNOTATION
    twoway_executor(const Executor& ex) noexcept : super_t{ex} {}

    template<class Function>
    __AGENCY_ANNOTATION
    future<result_of_t<decay_t<Function>()>>
      twoway_execute(Function&& f) const
    {
      return detail::twoway_execute(super_t::base_executor(), std::forward<Function>(f));
    }
};


} // end detail


struct twoway_t
{
  constexpr static bool is_requirable = true;
  constexpr static bool is_preferable = false;

  // Agency is a C++11-compatible library,
  // so we can't implement static_query_v as a variable template
  // use a constexpr static function instead
  template<class E>
  __AGENCY_ANNOTATION
  constexpr static bool static_query()
  {
    return detail::has_twoway_execute_member<E>::value;
  }

  template<class Executor>
  __AGENCY_ANNOTATION
  friend detail::twoway_executor<Executor> require(Executor ex, twoway_t)
  {
    return detail::twoway_executor<Executor>{ex};
  }
};


constexpr twoway_t twoway{};


} // end agency

