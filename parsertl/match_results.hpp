// match_results.hpp
// Copyright (c) 2017-2018 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_MATCH_RESULTS_HPP
#define PARSERTL_MATCH_RESULTS_HPP

#include "runtime_error.hpp"
#include "state_machine.hpp"
#include <vector>

namespace parsertl
{
template<typename sm_type>
struct basic_match_results
{
    typedef typename sm_type::id_type id_type;
    std::vector<id_type> stack;
    id_type token_id;
    typename sm_type::entry entry;

    basic_match_results() :
        token_id(static_cast<id_type>(~0))
    {
        stack.push_back(0);
        entry.action = error;
        entry.param = unknown_token;
    }

    basic_match_results(const id_type token_id_, const sm_type &sm_)
    {
        reset(token_id_, sm_);
    }

    void clear()
    {
        stack.clear();
        stack.push_back(0);
        token_id = static_cast<id_type>(~0);
        entry.clear();
    }

    void reset(const id_type token_id_, const sm_type &sm_)
    {
        stack.clear();
        stack.push_back(0);
        token_id = token_id_;

        if (token_id == static_cast<id_type>(~0))
        {
            entry.action = error;
            entry.param = unknown_token;
        }
        else
        {
            entry = sm_._table[stack.back() * sm_._columns + token_id];
        }
    }

    id_type reduce_id() const
    {
        if (entry.action != reduce)
        {
            throw runtime_error("Not in a reduce state!");
        }

        return entry.param;
    }

    template<typename token_vector>
    typename token_vector::value_type &dollar(const sm_type &sm_,
        const std::size_t index_, token_vector &productions) const
    {
        if (entry.action != reduce)
        {
            throw runtime_error("Not in a reduce state!");
        }

        return productions[productions.size() -
            production_size(sm_, entry.param) + index_];
    }

    template<typename token_vector>
    const typename token_vector::value_type &dollar(const sm_type &sm_,
        const std::size_t index_, const token_vector &productions) const
    {
        if (entry.action != reduce)
        {
            throw runtime_error("Not in a reduce state!");
        }

        return productions[productions.size() -
            production_size(sm_, entry.param) + index_];
    }

    std::size_t production_size(const sm_type &sm,
        const std::size_t index_) const
    {
        return sm._rules[index_].second.size();
    }
};

typedef basic_match_results<state_machine> match_results;
}

#endif
