// state_machine.hpp
// Copyright (c) 2014-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_STATE_MACHINE_HPP
#define PARSERTL_STATE_MACHINE_HPP

#include <algorithm>
#include "../../../lexertl/include/lexertl/compile_assert.hpp"
#include "enums.hpp"
#include <deque>
#include <vector>

namespace parsertl
{
    template<typename id_ty>
    struct base_state_machine
    {
        typedef id_ty id_type;
        typedef std::vector<std::pair<id_type, id_type> > capture_vector;
        typedef std::pair<std::size_t, capture_vector> capture_vec_pair;
        typedef std::deque<capture_vec_pair> captures_deque;
        typedef std::vector<id_type> id_type_vector;
        typedef std::pair<id_type, id_type_vector> id_type_pair;
        typedef std::deque<id_type_pair> rules;

        std::size_t _columns;
        std::size_t _rows;
        rules _rules;
        captures_deque _captures;

        // If you get a compile error here you have
        // failed to define an unsigned id type.
        lexertl::compile_assert<(static_cast<id_type>(~0) > 0)> _valid_id_type;

        struct entry
        {
            // Qualify action to prevent compilation error
            parsertl::action action;
            id_type param;

            entry() :
                // Qualify action to prevent compilation error
                action(parsertl::error),
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
                // Qualify action to prevent compilation error
                action = parsertl::error;
                param = syntax_error;
            }

            bool operator ==(const entry& rhs_) const
            {
                return action == rhs_.action && param == rhs_.param;
            }
        };

        base_state_machine() :
            _columns(0),
            _rows(0)
        {
        }

        // Just in case someone wants to use a pointer to the base
        virtual ~base_state_machine()
        {
        }

        virtual void clear()
        {
            _columns = _rows = 0;
            _rules.clear();
            _captures.clear();
        }
    };

    // Uses a vector of vectors for the state machine
    template<typename id_ty>
    class basic_state_machine : public base_state_machine<id_ty>
    {
    public:
        typedef base_state_machine<id_ty> base_sm;
        typedef id_ty id_type;
        typedef typename base_sm::entry entry;
        typedef std::pair<id_type, entry> state_pair;
        typedef std::vector<state_pair> pair_vector;
        typedef std::vector<pair_vector> table;

        // No need to specify constructor.
        virtual ~basic_state_machine()
        {
        }

        virtual void clear()
        {
            base_sm::clear();
            _table.clear();
        }

        bool empty() const
        {
            return _table.empty();
        }

        entry at(const std::size_t state_) const
        {
            const pair_vector& s_ = _table[state_];
            typename pair_vector::iterator iter_ = std::find_if(s_.begin(),
                s_.end(), pred(0));

            if (iter_ == s_.end())
                return entry();
            else
                return iter_->second;
        }

        entry at(const std::size_t state_, const std::size_t token_id_) const
        {
            const pair_vector& s_ = _table[state_];
            typename pair_vector::const_iterator iter_ = std::find_if(s_.begin(),
                s_.end(), pred(token_id_));

            if (iter_ == s_.end())
                return entry();
            else
                return iter_->second;
        }

        void set(const std::size_t state_, const std::size_t token_id_,
            const entry& entry_)
        {
            pair_vector& s_ = _table[state_];
            typename pair_vector::iterator iter_ = std::find_if(s_.begin(),
                s_.end(), pred(token_id_));

            if (iter_ == s_.end())
                s_.push_back(std::pair<id_type, entry>
                    (static_cast<id_type>(token_id_), entry_));
            else
                iter_->second = entry_;
        }

        void push()
        {
            _table.resize(base_sm::_rows);
        }

    private:
        struct pred
        {
            std::size_t _token_id;

            pred(const std::size_t token_id_) :
                _token_id(token_id_)
            {
            }

            bool operator()(const state_pair& pair)
            {
                return _token_id == pair.first;
            }
        };

        table _table;
    };

    // Uses uncompressed 2d array for state machine
    template<typename id_ty>
    class basic_uncompressed_state_machine : public base_state_machine<id_ty>
    {
    public:
        typedef base_state_machine<id_ty> base_sm;
        typedef id_ty id_type;
        typedef typename base_sm::entry entry;
        typedef std::vector<entry> table;

        // No need to specify constructor.
        virtual ~basic_uncompressed_state_machine()
        {
        };

        virtual void clear()
        {
            base_sm::clear();
            _table.clear();
        }

        bool empty() const
        {
            return _table.empty();
        }

        entry at(const std::size_t state_) const
        {
            return _table[state_ * base_sm::_columns];
        }

        entry at(const std::size_t state_, const std::size_t token_id_) const
        {
            return _table[state_ * base_sm::_columns + token_id_];
        }

        void set(const std::size_t state_, const std::size_t token_id_,
            const entry& entry_)
        {
            _table[state_ * base_sm::_columns + token_id_] = entry_;
        }

        void push()
        {
            _table.resize(base_sm::_columns * base_sm::_rows);
        }

    private:
        table _table;
    };

    typedef basic_state_machine<std::size_t> state_machine;
    typedef basic_uncompressed_state_machine<std::size_t> uncompressed_state_machine;
}

#endif
