// debug.hpp
// Copyright (c) 2014 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_DEBUG_HPP
#define PARSERTL_DEBUG_HPP

#include "dfa.hpp"
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
    typedef typename rules::nt_enum_map nt_enum_map;
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
            iter_ != end_; ++iter_)
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
            if (seen_.find(iter_->_symbols._lhs) == seen_.end())
            {
                typename production_deque::const_iterator lhs_iter_ = iter_;
                std::size_t index_ = lhs_iter_ - grammar_.begin();

                stream_ << lhs_iter_->_symbols._lhs;
                stream_ << ':';

                while (index_ != ~0)
                {
                    if (lhs_iter_->_symbols._rhs.empty())
                    {
                        stream_ << ' ';
                        empty(stream_);
                    }
                    else
                    {
                        typename symbol_deque::const_iterator rhs_iter_ =
                            lhs_iter_->_symbols._rhs.begin();
                        typename symbol_deque::const_iterator rhs_end_ =
                            lhs_iter_->_symbols._rhs.end();

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
                            _symbols._lhs.size(); i_ < size_; ++i_)
                        {
                            stream_ << ' ';
                        }

                        stream_ << '|';
                    }
                }

                seen_.insert(iter_->_symbols._lhs);
                stream_ << ';' << '\n' << '\n';
            }
        }

        stream_ << '%' << '%' << '\n';
    }

    static void dump(const rules &rules_, size_t_pair_set_deque &configs_,
        const dfa &dfa_, const nt_enum_map &nt_enums_, ostream &stream_)
    {
        const production_deque &grammar_ = rules_.grammar();
        const terminal_map &terminals_ = rules_.terminals();

        for (std::size_t idx_ = 0; idx_ < configs_.size(); ++idx_)
        {
            const size_t_pair_set &config_ = configs_[idx_];

            state(idx_, stream_);

            for (typename size_t_pair_set::const_iterator iter_ =
                config_.begin(), end_ = config_.end(); iter_ != end_; ++iter_)
            {
                const production &p_ = grammar_[iter_->first];
                std::size_t j_ = 0;

                stream_ << ' ' << ' ' << p_._symbols._lhs <<
                    ' ' << '-' << '>';

                for (; j_ < p_._symbols._rhs.size(); ++j_)
                {
                    if (j_ == iter_->second)
                    {
                        stream_ << ' ' << '.';
                    }

                    stream_ << ' ' << p_._symbols._rhs[j_]._name;
                }

                if (j_ == iter_->second)
                {
                    stream_ << ' ' << '.';
                }

                stream_ << '\n';
            }

            const dfa_state &state_ = dfa_[idx_];

            if (!state_._transitions.empty())
                stream_ << '\n';

            for (typename dfa_state::transition_map::const_iterator t_ =
                state_._transitions.begin(), e_ = state_._transitions.end();
                t_ != e_; ++t_)
            {
                stream_ << ' ' << ' ';

                if (t_->first < terminals_.size())
                {
                    typename terminal_map::const_iterator enum_ =
                        terminals_.begin();

                    for (typename terminal_map::const_iterator e_ =
                        terminals_.end(); enum_ != e_; ++enum_)
                    {
                        if (enum_->second._id == t_->first)
                        {
                            break;
                        }
                    }

                    stream_ << enum_->first;
                }
                else
                {
                    typename nt_enum_map::const_iterator enum_ =
                        nt_enums_.begin();

                    for (typename nt_enum_map::const_iterator e_ =
                        nt_enums_.end(); enum_ != e_; ++enum_)
                    {
                        if (enum_->second == t_->first)
                        {
                            break;
                        }
                    }

                    stream_ << enum_->first;
                }

                stream_ << ' ' << '-' << '>' << ' ' << t_->second << '\n';
            }

            stream_ << '\n';
        }
    }

    static void dump(const size_t_nt_state_map &nt_states_,
        const symbol_map &symbols_, ostream &stream_)
    {
        typename size_t_nt_state_map::const_iterator iter_ =
            nt_states_.begin();
        typename size_t_nt_state_map::const_iterator end_ =
            nt_states_.end();
        typename nt_state::size_t_set::const_iterator set_iter_;
        typename nt_state::size_t_set::const_iterator set_end_;

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

            stream_ << '\n';
        }

        stream_ << '\n';
        iter_ = nt_states_.begin();
        end_ = nt_states_.end();

        for (; iter_ != end_; ++iter_)
        {
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

            stream_ << '\n';
        }
    }

    static void dump(const rules &rules_, const state_machine &sm_,
        const symbol_map &symbols_, size_t_pair_set_deque &configs_,
        ostream &stream_)
    {
        const grammar &grammar_ = rules_.grammar();
        const std::size_t terminals_ = rules_.terminals().size();
        typename state_machine::table::const_iterator iter_ =
            sm_._table.begin();

        parse_table(stream_);

        for (std::size_t row_ = 0; row_ < sm_._rows; ++row_)
        {
            string last_lhs_;
            std::size_t max_ = 0;
            typename size_t_pair_set::const_iterator
                citer_ = configs_[row_].begin();
            typename size_t_pair_set::const_iterator
                cend_ = configs_[row_].end();
            bool shift_ = false;
            bool error_ = false;
            bool reduce_ = false;
            bool goto_ = false;

            state(row_, stream_);

            for (; citer_ != cend_; ++citer_)
            {
                ostringstream line_stream_;
                string line_;
                const production &production_ = grammar_[citer_->first];
                const std::size_t rhs_size_ = production_._symbols._rhs.size();

                line_stream_ << citer_->first;
                line_ = line_stream_.str();

                for (std::size_t i_ = 0, len_ = 5 - line_.size(); i_ < len_;
                    ++i_)
                {
                    stream_ << ' ';
                }

                stream_ << citer_->first << ' ';

                if (production_._symbols._lhs == last_lhs_)
                {
                    stream_ << string(last_lhs_.size(), ' ') << '|';
                }
                else
                {
                    stream_ << production_._symbols._lhs << ':';
                    last_lhs_ = production_._symbols._lhs;
                }

                for (std::size_t i_ = 0; i_ < rhs_size_; ++i_)
                {
                    if (i_ == citer_->second)
                    {
                        stream_ << ' ' << '.';
                    }

                    stream_ << ' ' << production_._symbols._rhs[i_]._name;
                }

                if (citer_->second == rhs_size_)
                {
                    stream_ << ' ' << '.';
                }

                stream_ << '\n';
            }

            stream_ << '\n';

            for (std::size_t column_ = 0; column_ < terminals_; ++column_)
            {
                const state_machine::entry &entry_ =
                    (&*iter_)[column_];

                if (entry_._action == shift)
                {
                    max_ = std::max(max_, symbols_.find(column_)->
                        second.size());
                }
            }

            shift_ = max_ > 0;

            if (shift_)
            {
                for (std::size_t column_ = 0; column_ < terminals_; ++column_)
                {
                    const state_machine::entry &entry_ =
                        (&*iter_)[column_];

                    if (entry_._action == shift)
                    {
                        const string &name_ = symbols_.find(column_)->second;

                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << name_;
                        stream_ << string(max_ + 2 - name_.size(), ' ');
                        oshift(stream_);
                        stream_ << entry_._param;
                        stream_ << '\n';
                    }
                }

                stream_ << '\n';
            }

            max_ = 0;

            for (std::size_t column_ = 0; column_ < terminals_; ++column_)
            {
                const state_machine::entry &entry_ =
                    (&*iter_)[column_];

                if (entry_._action == error &&
                    entry_._param == non_associative)
                {
                    max_ = std::max(max_, symbols_.find(column_)->
                        second.size());
                }
            }

            error_ = max_ > 0;

            if (error_)
            {
                for (std::size_t column_ = 0; column_ < terminals_; ++column_)
                {
                    const state_machine::entry &entry_ =
                        (&*iter_)[column_];

                    if (entry_._action == error &&
                        entry_._param == non_associative)
                    {
                        const string &name_ = symbols_.find(column_)->second;

                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << name_;
                        stream_ << string(max_ + 2 - name_.size(), ' ');
                        oerror(stream_);
                        stream_ << '\n';
                    }
                }

                stream_ << '\n';
            }

            max_ = 0;

            bool first_ = true;
            std::size_t rule_ = ~0;
            bool default_ = true;

            for (std::size_t column_ = 0; column_ < terminals_; ++column_)
            {
                const state_machine::entry &entry_ = (&*iter_)[column_];

                if (entry_._action == reduce || entry_._action == accept)
                {
                    typename symbol_map::const_iterator siter_ =
                        symbols_.find(column_);

                    if (first_)
                    {
                        rule_ = entry_._param;
                        first_ = false;
                    }
                    else
                    {
                        if (rule_ != entry_._param)
                        {
                            default_ = false;
                        }
                    }

                    max_ = std::max(max_, siter_->second.size());
                }
            }

            reduce_ = max_ > 0;

            if (reduce_)
            {
                for (std::size_t column_ = 0; column_ < terminals_;
                    ++column_)
                {
                    const state_machine::entry &entry_ =
                        (&*iter_)[column_];

                    if (entry_._action == reduce || entry_._action == accept)
                    {
                        const string &name_ =
                            symbols_.find(column_)->second;

                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << ' ';

                        if (default_)
                        {
                            dollar_default(stream_);
                            stream_ << ' ';
                            stream_ << ' ';
                        }
                        else
                        {
                            stream_ << name_;
                            stream_ << string(max_ + 2 - name_.size(), ' ');
                        }

                        if (entry_._action == reduce)
                        {
                            oreduce(stream_);
                            stream_ << entry_._param;
                            stream_ << ' ';
                            stream_ << '(';
                            stream_ << symbols_.find
                                (sm_._rules[entry_._param].first)->second;
                            stream_ << ')';
                        }
                        else
                        {
                            oaccept(stream_);
                        }

                        stream_ << '\n';

                        if (default_)
                        {
                            break;
                        }
                    }
                }

                stream_ << '\n';
            }

            max_ = 0;

            for (std::size_t column_ = terminals_; column_ < symbols_.size();
                ++column_)
            {
                const state_machine::entry &entry_ = (&*iter_)[column_];

                if (entry_._action == go_to)
                {
                    max_ = std::max(max_, symbols_.find(column_)->
                        second.size());
                }
            }

            goto_ = max_ > 0;

            if (goto_)
            {
                for (std::size_t column_ = terminals_;
                    column_ < symbols_.size(); ++column_)
                {
                    const state_machine::entry &entry_ = (&*iter_)[column_];

                    if (entry_._action == go_to)
                    {
                        const string &name_ = symbols_.find(column_)->second;

                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << ' ';
                        stream_ << name_;
                        stream_ << string(max_ + 2 - name_.size(), ' ');
                        ogoto(stream_);
                        stream_ << entry_._param;
                        stream_ << '\n';
                    }
                }

                stream_ << '\n';
            }

            stream_ << '\n';
            iter_ += symbols_.size();
        }
    }

private:
    typedef typename rules::production_deque grammar;
    typedef std::basic_ostringstream<char_type> ostringstream;
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
        stream_ << "state " << row_ << '\n' << '\n';
    }

    static void state(const std::size_t row_, std::wostream &stream_)
    {
        stream_ << L"state " << row_ << L'\n' << L'\n';
    }

    static void oerror(std::ostream &stream_)
    {
        stream_ << "error (nonassociative)";
    }

    static void oerror(std::wostream &stream_)
    {
        stream_ << L"error (nonassociative)";
    }

    static void oshift(std::ostream &stream_)
    {
        stream_ << "shift, and go to state ";
    }

    static void oshift(std::wostream &stream_)
    {
        stream_ << L"shift, and go to state ";
    }

    static void oreduce(std::ostream &stream_)
    {
        stream_ << "reduce using rule ";
    }

    static void oreduce(std::wostream &stream_)
    {
        stream_ << L"reduce using rule ";
    }

    static void ogoto(std::ostream &stream_)
    {
        stream_ << "go to state ";
    }

    static void ogoto(std::wostream &stream_)
    {
        stream_ << L"go to state ";
    }

    static void oaccept(std::ostream &stream_)
    {
        stream_ << "accept";
    }

    static void oaccept(std::wostream &stream_)
    {
        stream_ << L"accept";
    }

    static void dollar_default(std::ostream &stream_)
    {
        stream_ << "$default";
    }

    static void dollar_default(std::wostream &stream_)
    {
        stream_ << L"$default";
    }
};

typedef basic_debug<char> debug;
typedef basic_debug<wchar_t> wdebug;
}

#endif
