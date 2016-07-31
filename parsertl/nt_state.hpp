// nt_state.hpp
// Copyright (c) 2014-2016 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_NT_STATE_HPP
#define PARSERTL_NT_STATE_HPP

#include <map>
#include <vector>

namespace parsertl
{
struct nt_state
{
    typedef std::vector<char> size_t_set;
    bool _nullable;
    size_t_set _first_set;
    size_t_set _follow_set;

    nt_state(const std::size_t terminals_) :
        _nullable(false),
        _first_set(terminals_, 0),
        _follow_set(terminals_, 0)
    {
    }
};

typedef std::map<std::size_t, nt_state> size_t_nt_state_map;
typedef std::pair<std::size_t, nt_state> size_t_nt_state_pair;
}

#endif
