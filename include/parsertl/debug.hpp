// debug.hpp
// Copyright (c) 2014-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_DEBUG_HPP
#define PARSERTL_DEBUG_HPP

#include "dfa.hpp"
#include <ostream>
#include "rules.hpp"

namespace parsertl
{
    template<typename char_type>
    class basic_debug
    {
    public:
        typedef basic_rules<char_type> rules;
        typedef std::basic_ostream<char_type> ostream;

        static void dump(const rules& rules_, ostream& stream_)
        {
            const std::size_t start_ = rules_.start();
            const production_deque& grammar_ = rules_.grammar();
            const token_info_vector& tokens_info_ = rules_.tokens_info();
            const std::size_t terminals_ = tokens_info_.size();
            string_vector symbols_;
            std::set<std::size_t> seen_;
            token_map map_;

            rules_.symbols(symbols_);

            // Skip EOI token
            for (std::size_t idx_ = 1, size_ = tokens_info_.size();
                idx_ < size_; ++idx_)
            {
                const token_info& token_info_ = tokens_info_[idx_];
                token_prec_assoc info_(token_info_._precedence,
                    token_info_._associativity);
                typename token_map::iterator map_iter_ = map_.find(info_);

                if (map_iter_ == map_.end())
                {
                    map_.insert(token_pair(info_, symbols_[idx_]));
                }
                else
                {
                    map_iter_->second += ' ';
                    map_iter_->second += symbols_[idx_];
                }
            }

            for (typename token_map::const_iterator iter_ =
                map_.begin(), end_ = map_.end(); iter_ != end_; ++iter_)
            {
                switch (iter_->first.second)
                {
                case rules::token_assoc:
                    token(stream_);
                    break;
                case rules::precedence_assoc:
                    precedence(stream_);
                    break;
                case rules::non_assoc:
                    nonassoc(stream_);
                    break;
                case rules::left_assoc:
                    left(stream_);
                    break;
                case rules::right_assoc:
                    right(stream_);
                    break;
                }

                stream_ << iter_->second << '\n';
            }

            if (start_ != static_cast<std::size_t>(~0))
            {
                stream_ << '\n';
                start(stream_);
                stream_ << symbols_[terminals_ + start_] << '\n' << '\n';
            }

            stream_ << '%' << '%' << '\n' << '\n';

            for (typename production_deque::const_iterator iter_ =
                grammar_.begin(), end_ = grammar_.end();
                iter_ != end_; ++iter_)
            {
                if (seen_.find(iter_->_lhs) == seen_.end())
                {
                    typename production_deque::const_iterator lhs_iter_ = iter_;
                    std::size_t index_ = lhs_iter_ - grammar_.begin();

                    stream_ << symbols_[terminals_ + lhs_iter_->_lhs];
                    stream_ << ':';

                    while (index_ != static_cast<std::size_t>(~0))
                    {
                        if (lhs_iter_->_rhs._symbols.empty())
                        {
                            stream_ << ' ';
                            empty(stream_);
                        }
                        else
                        {
                            typename symbol_vector::const_iterator rhs_iter_ =
                                lhs_iter_->_rhs._symbols.begin();
                            typename symbol_vector::const_iterator rhs_end_ =
                                lhs_iter_->_rhs._symbols.end();

                            for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
                            {
                                const std::size_t id_ =
                                    rhs_iter_->_type == symbol::TERMINAL ?
                                    rhs_iter_->_id :
                                    terminals_ + rhs_iter_->_id;

                                // Don't dump '$'
                                if (id_ > 0)
                                {
                                    stream_ << ' ' << symbols_[id_];
                                }
                            }
                        }

                        if (!lhs_iter_->_rhs._prec.empty())
                        {
                            stream_ << ' ';
                            prec(stream_);
                            stream_ << lhs_iter_->_rhs._prec;
                        }

                        index_ = lhs_iter_->_next_lhs;

                        if (index_ != static_cast<std::size_t>(~0))
                        {
                            const string& lhs_ =
                                symbols_[terminals_ + lhs_iter_->_lhs];

                            lhs_iter_ = grammar_.begin() + index_;
                            stream_ << '\n';

                            for (std::size_t i_ = 0, size_ = lhs_.size();
                                i_ < size_; ++i_)
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

        static void dump(const rules& rules_, const dfa& dfa_, ostream& stream_)
        {
            const production_deque& grammar_ = rules_.grammar();
            const std::size_t terminals_ = rules_.tokens_info().size();
            string_vector symbols_;

            rules_.symbols(symbols_);

            for (std::size_t idx_ = 0, dfa_size_ = dfa_.size();
                idx_ < dfa_size_; ++idx_)
            {
                const dfa_state& state_ = dfa_[idx_];
                const cursor_vector& config_ = state_._closure;

                state(idx_, stream_);

                for (typename cursor_vector::const_iterator iter_ =
                    config_.begin(), end_ = config_.end(); iter_ != end_;
                    ++iter_)
                {
                    const production& p_ = grammar_[iter_->_id];
                    std::size_t j_ = 0;

                    stream_ << ' ' << ' ' << symbols_[terminals_ + p_._lhs] <<
                        ' ' << '-' << '>';

                    for (; j_ < p_._rhs.size(); ++j_)
                    {
                        const symbol& symbol_ = p_._rhs[j_];
                        const std::size_t id_ =
                            symbol_._type == symbol::TERMINAL ? symbol_._id :
                            terminals_ + symbol_._id;

                        if (j_ == iter_->_index)
                        {
                            stream_ << ' ' << '.';
                        }

                        stream_ << ' ' << symbols_[id_];
                    }

                    if (j_ == iter_->_index)
                    {
                        stream_ << ' ' << '.';
                    }

                    stream_ << '\n';
                }

                if (!state_._transitions.empty())
                    stream_ << '\n';

                for (typename cursor_vector::const_iterator t_ =
                    state_._transitions.begin(), e_ = state_._transitions.end();
                    t_ != e_; ++t_)
                {
                    stream_ << ' ' << ' ' << symbols_[t_->_id] << ' ' <<
                        '-' << '>' << ' ' << t_->_index << '\n';
                }

                stream_ << '\n';
            }
        }

    private:
        typedef typename rules::production production;
        typedef typename rules::production_deque production_deque;
        typedef std::basic_string<char_type> string;
        typedef typename rules::string_vector string_vector;
        typedef typename rules::symbol symbol;
        typedef typename rules::symbol_vector symbol_vector;
        typedef std::pair<std::size_t, typename rules::associativity>
            token_prec_assoc;
        typedef typename rules::token_info token_info;
        typedef typename rules::token_info_vector token_info_vector;
        typedef std::map<token_prec_assoc, string> token_map;
        typedef std::pair<token_prec_assoc, string> token_pair;

        static void empty(std::ostream& stream_)
        {
            stream_ << "%empty";
        }

        static void empty(std::wostream& stream_)
        {
            stream_ << L"%empty";
        }

        static void left(std::ostream& stream_)
        {
            stream_ << "%left ";
        }

        static void left(std::wostream& stream_)
        {
            stream_ << L"%left ";
        }

        static void nonassoc(std::ostream& stream_)
        {
            stream_ << "%nonassoc ";
        }

        static void nonassoc(std::wostream& stream_)
        {
            stream_ << L"%nonassoc ";
        }

        static void prec(std::ostream& stream_)
        {
            stream_ << "%prec ";
        }

        static void prec(std::wostream& stream_)
        {
            stream_ << L"%prec ";
        }

        static void precedence(std::ostream& stream_)
        {
            stream_ << "%precedence ";
        }

        static void precedence(std::wostream& stream_)
        {
            stream_ << L"%precedence ";
        }

        static void right(std::ostream& stream_)
        {
            stream_ << "%right ";
        }

        static void right(std::wostream& stream_)
        {
            stream_ << L"%right ";
        }

        static void start(std::ostream& stream_)
        {
            stream_ << "%start ";
        }

        static void start(std::wostream& stream_)
        {
            stream_ << L"%start ";
        }

        static void state(const std::size_t row_, std::ostream& stream_)
        {
            stream_ << "state " << row_ << '\n' << '\n';
        }

        static void state(const std::size_t row_, std::wostream& stream_)
        {
            stream_ << L"state " << row_ << L'\n' << L'\n';
        }

        static void token(std::ostream& stream_)
        {
            stream_ << "%token ";
        }

        static void token(std::wostream& stream_)
        {
            stream_ << L"%token ";
        }
    };

    typedef basic_debug<char> debug;
    typedef basic_debug<wchar_t> wdebug;
}

#endif
