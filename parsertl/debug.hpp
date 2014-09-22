// debug.hpp
// Copyright (c) 2014 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_DEBUG_HPP
#define PARSERTL_DEBUG_HPP

#include "nt_state.hpp"
#include <ostream>
#include "rules.hpp"
#include "state_machine.hpp"

namespace parsertl
{
template<typename char_type>
class basic_debug
{
public:
    typedef basic_rules<char_type> rules;
    typedef std::basic_ostream<char_type> ostream;
    typedef typename rules::production production;
    typedef typename rules::production_deque production_deque;
    typedef std::basic_string<char_type> string;
    typedef typename rules::symbol symbol;
    typedef typename rules::symbol_deque symbol_deque;
    typedef std::map<std::size_t, string> symbol_map;
    typedef typename rules::terminal_map terminal_map;

    static void dump(const rules &rules_, ostream &stream_)
    {
        string start_ = rules_.start();
        const production_deque &grammar_ = rules_.grammar();
        const terminal_map &terminals_ = rules_.terminals();
        std::set<string> seen_;
        token_map map_;

        for (typename terminal_map::const_iterator iter_ =
            terminals_.begin(), end_ = terminals_.end();
            iter_ != end_;  ++iter_)
        {
            if (iter_->second._id == 0) continue;

            token_info info_(iter_->second._precedence,
                iter_->second._associativity);
            typename token_map::iterator map_iter_ = map_.find(info_);

            if (map_iter_ == map_.end())
            {
                map_.insert(token_pair(info_, iter_->first));
            }
            else
            {
                map_iter_->second += ' ';
                map_iter_->second += iter_->first;
            }
        }

        for (typename token_map::const_iterator iter_ =
            map_.begin(), end_ = map_.end(); iter_ != end_; ++iter_)
        {
            switch (iter_->first.second)
            {
                case rules::token_info::precedence:
                    precedence(stream_);
                    break;
                case rules::token_info::nonassoc:
                    nonassoc(stream_);
                    break;
                case rules::token_info::left:
                    left(stream_);
                    break;
                case rules::token_info::right:
                    right(stream_);
                    break;
            }

            stream_ << iter_->second << '\n';
        }

        if (!start_.empty())
        {
            stream_ << '\n';
            start(stream_);
            stream_ << start_ << '\n' << '\n';
        }

        stream_ << '%' << '%' << '\n' << '\n';

        for (typename production_deque::const_iterator iter_ =
            grammar_.begin(), end_ = grammar_.end();
            iter_ != end_; ++iter_)
        {
            if (seen_.find(iter_->_lhs) == seen_.end())
            {
                typename production_deque::const_iterator lhs_iter_ =
                    iter_;
                std::size_t index_ = lhs_iter_ - grammar_.begin();

                stream_ << lhs_iter_->_lhs;
                stream_ << ':';

                while (index_ != ~0)
                {
                    if (lhs_iter_->_rhs.empty())
                    {
                        stream_ << ' ';
                        empty(stream_);
                    }
                    else
                    {
                        typename symbol_deque::const_iterator rhs_iter_ =
                            lhs_iter_->_rhs.begin();
                        typename symbol_deque::const_iterator rhs_end_ =
                            lhs_iter_->_rhs.end();

                        for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
                        {
                            stream_ << ' ' << rhs_iter_->_name;
                        }
                    }

                    index_ = lhs_iter_->_next_lhs;

                    if (index_ != ~0)
                    {
                        lhs_iter_ = grammar_.begin() + index_;
                        stream_ << '\n';

                        for (std::size_t i_ = 0, size_ = lhs_iter_->
                            _lhs.size(); i_ < size_; ++i_)
                        {
                            stream_ << ' ';
                        }

                        stream_ << '|';
                    }
                }

                seen_.insert(iter_->_lhs);
                stream_ << ';' << '\n' << '\n';
            }
        }

        stream_ << '%' << '%' << '\n';
    }

    static void dump(const typename detail::size_t_nt_state_map &nt_states_,
        const symbol_map &symbols_, ostream &stream_)
    {
        typename detail::size_t_nt_state_map::const_iterator iter_ =
            nt_states_.begin();
        typename detail::size_t_nt_state_map::const_iterator end_ =
            nt_states_.end();
        typename detail::nt_state::size_t_set::const_iterator set_iter_;
        typename detail::nt_state::size_t_set::const_iterator set_end_;

        first_follow(stream_);

        for (; iter_ != end_; ++iter_)
        {
            first(stream_);
            stream_ << symbols_.find(iter_->first)->second;
            bracket_equals(stream_);
            set_iter_ = iter_->second._first_set.begin();
            set_end_ = iter_->second._first_set.end();

            if (set_iter_ != set_end_)
            {
                stream_ << symbols_.find(*set_iter_++)->second;
            }

            for (; set_iter_ != set_end_; ++set_iter_)
            {
                comma(stream_);
                stream_ << symbols_.find(*set_iter_)->second;
            }

            stream_ << '\n' << '\n';
            follow(stream_);
            stream_ << symbols_.find(iter_->first)->second;
            bracket_equals(stream_);
            set_iter_ = iter_->second._follow_set.begin();
            set_end_ = iter_->second._follow_set.end();

            if (set_iter_ != set_end_)
            {
                stream_ << symbols_.find(*set_iter_++)->second;
            }

            for (; set_iter_ != set_end_; ++set_iter_)
            {
                comma(stream_);
                stream_ << symbols_.find(*set_iter_)->second;
            }

            stream_ << '\n' << '\n';
        }
    }

    static void dump(const state_machine &sm_, const symbol_map &symbols_,
        ostream &stream_)
    {
        typename state_machine::table::const_iterator iter_ =
            sm_._table.begin();

        parse_table(stream_);

        for (std::size_t row_ = 0; row_ < sm_._rows; ++row_)
        {
            state(row_, stream_);

            for (std::size_t colummn_ = 0; colummn_ < sm_._columns; ++colummn_)
            {
                const parsertl::state_machine::entry &entry_ =
                    *iter_++;

                if (entry_._action != parsertl::error)
                {
                    stream_ << ' ';
                    stream_ << ' ';
                    stream_ << symbols_.find(colummn_)->second;
                    arrow(stream_);
                }

                switch (entry_._action)
                {
                case parsertl::shift:
                    shift(stream_);
                    stream_ << entry_._param;
                    break;
                case parsertl::reduce:
                    reduce(stream_);
                    stream_ << entry_._param;
                    break;
                case parsertl::go_to:
                    ogoto(stream_);
                    stream_ << entry_._param;
                    break;
                case parsertl::accept:
                    accept(stream_);
                    break;
                }

                if (entry_._action != parsertl::error)
                {
                    stream_ << '\n';
                }
            }

            stream_ << '\n';
        }
    }

private:
    typedef std::pair<std::size_t, typename rules::token_info::associativity>
        token_info;
    typedef std::map<token_info, string> token_map;
    typedef std::pair<token_info, string> token_pair;

    static void precedence(std::ostream &stream_)
    {
        stream_ << "%precedence ";
    }

    static void precedence(std::wostream &stream_)
    {
        stream_ << L"%precedence ";
    }

    static void nonassoc(std::ostream &stream_)
    {
        stream_ << "%nonassoc ";
    }

    static void nonassoc(std::wostream &stream_)
    {
        stream_ << L"%nonassoc ";
    }

    static void left(std::ostream &stream_)
    {
        stream_ << "%left ";
    }

    static void left(std::wostream &stream_)
    {
        stream_ << L"%left ";
    }

    static void right(std::ostream &stream_)
    {
        stream_ << "%right ";
    }

    static void right(std::wostream &stream_)
    {
        stream_ << L"%right ";
    }

    static void start(std::ostream &stream_)
    {
        stream_ << "%start ";
    }

    static void start(std::wostream &stream_)
    {
        stream_ << L"%start ";
    }

    static void empty(std::ostream &stream_)
    {
        stream_ << "%empty";
    }

    static void empty(std::wostream &stream_)
    {
        stream_ << L"%empty";
    }

    static void first_follow(std::ostream &stream_)
    {
        stream_ << "First and Follow sets:\n\n";
    }

    static void first_follow(std::wostream &stream_)
    {
        stream_ << L"First and Follow sets:\n\n";
    }

    static void first(std::ostream &stream_)
    {
        stream_ << "FIRST(";
    }

    static void first(std::wostream &stream_)
    {
        stream_ << L"FIRST(";
    }

    static void follow(std::ostream &stream_)
    {
        stream_ << "FOLLOW(";
    }

    static void follow(std::wostream &stream_)
    {
        stream_ << L"FOLLOW(";
    }

    static void bracket_equals(std::ostream &stream_)
    {
        stream_ << ") = ";
    }

    static void bracket_equals(std::wostream &stream_)
    {
        stream_ << L") = ";
    }

    static void comma(std::ostream &stream_)
    {
        stream_ << ", ";
    }

    static void comma(std::wostream &stream_)
    {
        stream_ << L", ";
    }

    static void parse_table(std::ostream &stream_)
    {
        stream_ << "Parse Table:\n\n";
    }

    static void parse_table(std::wostream &stream_)
    {
        stream_ << L"Parse Table:\n\n";
    }

    static void state(const std::size_t row_, std::ostream &stream_)
    {
        stream_ << "State " << row_ << ":\n";
    }

    static void state(const std::size_t row_, std::wostream &stream_)
    {
        stream_ << L"State " << row_ << L":\n";
    }

    static void arrow(std::ostream &stream_)
    {
        stream_ << " -> ";
    }

    static void arrow(std::wostream &stream_)
    {
        stream_ << L" -> ";
    }

    static void shift(std::ostream &stream_)
    {
        stream_ << "SHIFT ";
    }

    static void shift(std::wostream &stream_)
    {
        stream_ << L"SHIFT ";
    }

    static void reduce(std::ostream &stream_)
    {
        stream_ << "REDUCE ";
    }

    static void reduce(std::wostream &stream_)
    {
        stream_ << L"REDUCE ";
    }

    static void ogoto(std::ostream &stream_)
    {
        stream_ << "GOTO ";
    }

    static void ogoto(std::wostream &stream_)
    {
        stream_ << L"GOTO ";
    }

    static void accept(std::ostream &stream_)
    {
        stream_ << "ACCEPT";
    }

    static void accept(std::wostream &stream_)
    {
        stream_ << L"ACCEPT";
    }
};

typedef basic_debug<char> debug;
typedef basic_debug<wchar_t> wdebug;
}

#endif
