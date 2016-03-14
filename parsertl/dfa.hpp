// dfa.hpp
// Copyright (c) 2014-2016 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_DFA_HPP
#define PARSERTL_DFA_HPP

#include <deque>
#include <map>
#include <set>

namespace parsertl
{
    struct dfa_state
    {
        typedef std::pair<std::size_t, std::size_t> transition_pair;
        typedef std::map<std::size_t, std::size_t> transition_map;

        transition_map _transitions;
    };

    typedef std::deque<dfa_state> dfa;
    typedef std::pair<std::size_t, std::size_t> size_t_pair;
    typedef std::set<size_t_pair> size_t_pair_set;
    typedef std::deque<size_t_pair_set> size_t_pair_set_deque;
};

#endif
