// parser.hpp
// Copyright (c) 2014 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_PARSER_HPP
#define PARSERTL_PARSER_HPP

#include <stack>
#include "state_machine.hpp"

namespace parsertl
{
template<typename iterator>
struct parser
{
    struct token
    {
        typedef typename iterator::value_type::char_type char_type;
        typedef typename iterator::value_type::iter_type iter_type;
        typedef std::basic_string<char_type> string;
        std::size_t id;
        iter_type start;
        iter_type end;

        token() :
            id(~0),
            start(),
            end()
        {
        }

        token(const std::size_t id_, const iter_type &start_,
            const iter_type &end_) :
            id(id_),
            start(start_),
            end(end_)
        {
        }

        string str() const
        {
            return string(start, end);
        }
    };

    typedef std::vector<token> token_vector;

    state_machine sm;
    std::vector<std::size_t> stack;
    std::size_t token_id;
    state_machine::entry entry;

    parser() :
        token_id(iterator::value_type::npos())
    {
    }

    void init(iterator &iter_)
    {
        stack.clear();
        stack.push_back(0);
        token_id = iter_->id;

        if (token_id == iterator::value_type::npos())
        {
            entry._action = error;
            entry._param = unknown_token;
        }
        else
        {
            entry = sm._table[stack.back() * sm._columns + token_id];
        }
    }

    std::size_t production_size(const std::size_t index_) const
    {
        return sm._rules[index_].second.size();
    }

    void clear()
    {
        sm.clear();
        stack.clear();
        token_id = iterator::value_type::npos();
        entry.clear();
    }

    // Parse entire sequence and return boolean
    bool parse(iterator &iter_)
    {
        while (entry._action != error && entry._action != accept)
        {
            switch (entry._action)
            {
            case error:
                break;
            case shift:
                stack.push_back(entry._param);
                ++iter_;
                token_id = iter_->id;

                if (token_id == iterator::value_type::npos())
                {
                    entry._action = error;
                    entry._param = unknown_token;
                }
                else
                {
                    entry = sm._table[stack.back() *
                        sm._columns + token_id];
                }

                break;
            case reduce:
            {
                const std::size_t size_ =
                    sm._rules[entry._param].second.size();

                if (size_)
                {
                    stack.resize(stack.size() - size_);
                }

                token_id = sm._rules[entry._param].first;
                entry = sm._table[stack.back() * sm._columns + token_id];
                break;
            }
            case go_to:
                stack.push_back(entry._param);
                token_id = iter_->id;
                entry = sm._table[stack.back() * sm._columns + token_id];
                break;
            case accept:
                break;
            }
        }

        return entry._action == accept;
    }

    // parse sequence but do not keep track of productions
    void next(iterator &iter_)
    {
        switch (entry._action)
        {
            case error:
                break;
            case shift:
                stack.push_back(entry._param);
                ++iter_;
                token_id = iter_->id;

                if (token_id == iterator::value_type::npos())
                {
                    entry._action = error;
                    entry._param = unknown_token;
                }
                else
                {
                    entry = sm._table[stack.back() * sm._columns + token_id];
                }

                break;
            case reduce:
            {
                const std::size_t size_ =
                    sm._rules[entry._param].second.size();
                token token_;

                if (size_)
                {
                    stack.resize(stack.size() - size_);
                }
                else
                {
                    token_.start = token_.end = iter_->start;
                }

                token_id = sm._rules[entry._param].first;
                entry = sm._table[stack.back() * sm._columns + token_id];
                token_.id = token_id;
                break;
            }
            case go_to:
                stack.push_back(entry._param);
                token_id = iter_->id;
                entry = sm._table[stack.back() * sm._columns + token_id];
                break;
            case accept:
                break;
        }
    }

    // Parse sequence and maintain production vector
    void next(iterator &iter_, token_vector &productions)
    {
        switch (entry._action)
        {
        case error:
            break;
        case shift:
            stack.push_back(entry._param);
            productions.push_back
                (token(iter_->id, iter_->start, iter_->end));
            ++iter_;
            token_id = iter_->id;

            if (token_id == iterator::value_type::npos())
            {
                entry._action = error;
                entry._param = unknown_token;
            }
            else
            {
                entry = sm._table[stack.back() * sm._columns + token_id];
            }

            break;
        case reduce:
        {
            const std::size_t size_ =
                sm._rules[entry._param].second.size();
            token token_;

            if (size_)
            {
                token_.start = (productions.end() - size_)->start;
                token_.end = productions.back().end;
                stack.resize(stack.size() - size_);
                productions.resize(productions.size() - size_);
            }
            else
            {
                token_.start = token_.end = iter_->start;
            }

            token_id = sm._rules[entry._param].first;
            entry = sm._table[stack.back() * sm._columns + token_id];
            token_.id = token_id;
            productions.push_back(token_);
            break;
        }
        case go_to:
            stack.push_back(entry._param);
            token_id = iter_->id;
            entry = sm._table[stack.back() * sm._columns + token_id];
            break;
        case accept:
            break;
        }
    }

    const typename parser::token &dollar(const std::size_t index_,
        const token_vector &productions)
    {
        if (entry._action != reduce)
        {
            throw runtime_error("Not in a reduce state!");
        }

        return productions[productions.size() -
            sm._rules[entry._param].second.size() + index_];
    }

    std::size_t reduce_id() const
    {
        if (entry._action != reduce)
        {
            throw runtime_error("Not in a reduce state!");
        }

        return sm._new_to_old[entry._param];
    }
};
}

#endif
