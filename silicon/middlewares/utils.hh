#pragma once

# include <iod/utils.hh>

namespace sl {
namespace mw {
namespace utils {

auto remove_members_with_attribute = [] (const auto& o, const auto& a)
{
  typedef std::decay_t<decltype(a)> A; 
  return foreach2(o) | [&] (auto& m)
  {
    typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
    return ::iod::static_if<!has_symbol<attrs, A>::value>(
      [&] () { return m; },
      [&] () {});
  };
};

auto extract_members_with_attribute = [] (const auto& o, const auto& a)
{
  typedef std::decay_t<decltype(a)> A; 
  return foreach2(o) | [&] (auto& m)
  {
    typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
    return ::iod::static_if<has_symbol<attrs, A>::value>(
      [&] () { return m; },
      [&] () {});
  };
};
    
} /** !utils */
} /** !mw */
} /** !sl */
