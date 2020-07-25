// state_machine.hpp
// Copyright (c) 2014-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_STATE_MACHINE_HPP
#define PARSERTL_STATE_MACHINE_HPP

#include "../../../lexertl/include/lexertl/compile_assert.hpp"
#include "enums.hpp"
#include <deque>
#include <map>
#include <vector>

namespace parsertl
{
    template<typename id_ty>
    struct basic_state_machine
    {
        typedef id_ty id_type;
        // If you get a compile error here you have
        // failed to define an unsigned id type.
        lexertl::compile_assert<(static_cast<id_type>(~0) > 0)> _valid_id_type;

        struct entry
        {
            action action;
            id_type param;

            entry() :
                action(error),
                param(syntax_error)
            {
            }

            // Qualify action to prevent compilation error
            entry(const parsertl::action action_, const id_type param_) :
                action(action_),
                param(param_)
            {
            }

            void clear()
            {
                action = error;
                param = syntax_error;
            }
        };

        typedef std::vector<std::pair<id_type, id_type> > capture_vector;
        typedef std::pair<std::size_t, capture_vector> capture_vec_pair;
        typedef std::deque<capture_vec_pair> captures_deque;
        typedef std::vector<entry> table;
        typedef std::vector<id_type> id_type_vector;
        typedef std::pair<id_type, id_type_vector> id_type_pair;
        typedef std::deque<id_type_pair> rules;

        table _table;
        std::size_t _columns;
        std::size_t _rows;
        rules _rules;
        captures_deque _captures;

        basic_state_machine() :
            _columns(0),
            _rows(0)
        {
        }

        void clear()
        {
            _table.clear();
            _columns = _rows = 0;
            _rules.clear();
            _captures.clear();
        }

        bool empty() const
        {
            return _table.empty();
        }
    };

    typedef basic_state_machine<std::size_t> state_machine;
}

#endif
