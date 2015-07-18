// generator.hpp
// Copyright (c) 2014 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_GENERATOR_HPP
#define PARSERTL_GENERATOR_HPP

#include "narrow.hpp"
#include "nt_state.hpp"
#include <queue>
#include "rules.hpp"
#include "runtime_error.hpp"
#include "state_machine.hpp"

namespace parsertl
{
template<typename rules>
class basic_generator
{
public:
    typedef typename rules::string string;

    static void build(rules &rules_, state_machine &sm_,
        std::string *warnings_ = 0,
        typename detail::size_t_nt_state_map *nt_states_ = 0)
    {
        string start_;
        const grammar &grammar_ = rules_.grammar();
        nt_enum_map nt_enums_;
        const terminal_map &terminals_ = rules_.terminals();
        const nt_map &nts_ = rules_.non_terminals();
        symbol_map symbols_;
        typename nt_map::const_iterator iter_;
        size_t_set_deque configs_;
        dfa dfa_;
        size_t_nt_state_map nt_st_;
        size_t_map new_old_map_;

        rules_.validate();
        start_ = rules_.start();
        sm_.clear();

        if (nt_states_ == 0) nt_states_ = &nt_st_;

        rules_.symbols(symbols_);
        enumerate_non_terminals(grammar_, terminals_.size(), nt_enums_);
        // lookup start
        iter_ = nts_.find(start_);
        build_dfa(grammar_, terminals_, nt_enums_, nts_,
            grammar_[iter_->second._first_production], configs_, dfa_);

        rewrite(rules_, start_, configs_, dfa_, terminals_, nt_enums_,
            new_old_map_);
        rules_.symbols(symbols_);
        enumerate_non_terminals(grammar_, terminals_.size(), nt_enums_);
        iter_ = nts_.find(start_);
        build_dfa(grammar_, terminals_, nt_enums_, nts_,
            grammar_[iter_->second._first_production], configs_, dfa_);

        fill_nt_states(nt_enums_, *nt_states_);
        build_first_sets(grammar_, terminals_, nt_enums_, nts_, *nt_states_);
        // First add EOF to follow_set of start.
        nt_states_->find(nt_enums_.find(start_)->second)->second.
            _follow_set.insert(0);
        build_follow_sets(grammar_, terminals_, nt_enums_, *nt_states_);
        build_table(dfa_, configs_, grammar_, start_,
            *nt_states_, terminals_, nt_enums_, symbols_, sm_, warnings_);
        copy_rules(grammar_, terminals_, nt_enums_, sm_);
        sm_._new_to_old.resize(new_old_map_.size());

        for (typename size_t_map::const_iterator iter_ = new_old_map_.begin(),
            end_ = new_old_map_.end(); iter_ != end_; ++iter_)
        {
            sm_._new_to_old[iter_->first] = iter_->second;
        }
    }

private:
    typedef typename rules::production_deque grammar;
    typedef std::map<string, std::size_t> nt_enum_map;
    typedef std::pair<string, std::size_t> nt_enum_pair;
    typedef typename rules::nt_map nt_map;
    typedef typename detail::nt_state nt_state;
    typedef std::map<std::size_t, std::size_t> size_t_map;
    typedef typename detail::size_t_nt_state_map size_t_nt_state_map;
    typedef typename detail::size_t_nt_state_pair size_t_nt_state_pair;
    typedef typename rules::production production;
    typedef std::pair<std::size_t, std::size_t> size_t_pair;
    typedef std::set<size_t_pair> size_t_set;
    typedef typename rules::symbol symbol;
    typedef typename rules::symbol_deque symbol_deque;
    typedef typename rules::symbol_map symbol_map;
    typedef std::deque<size_t_set> size_t_set_deque;
    typedef typename rules::terminal_map terminal_map;
    typedef typename rules::token_info token_info;

    struct dfa_state
    {
        typedef std::pair<std::size_t, std::size_t> transition_pair;
        typedef std::map<std::size_t, std::size_t> transition_map;

        transition_map _transitions;
    };

    typedef std::deque<dfa_state> dfa;

    struct prod
    {
        string _lhs;
        size_t_pair _lhs_indexes;
        string _precedence;
        symbol_deque _rhs;
        std::vector<size_t_pair> _rhs_indexes;
    };

    typedef std::deque<prod> prod_deque;

    static void enumerate_non_terminals(const grammar &grammar_,
        const std::size_t offset_, nt_enum_map &nt_enums_)
    {
        typename grammar::const_iterator iter_ = grammar_.begin();
        typename grammar::const_iterator end_ = grammar_.end();

        nt_enums_.clear();

        for (; iter_ != end_; ++iter_)
        {
            if (nt_enums_.find(iter_->_lhs) == nt_enums_.end())
            {
                nt_enums_.insert(nt_enum_pair
                    (iter_->_lhs, nt_enums_.size() + offset_));
            }
        }
    }

    static void build_dfa(const grammar &grammar_,
        const terminal_map &terminals_, const nt_enum_map &nt_enums_,
        const nt_map &nts_, const production &start_,
        size_t_set_deque &configs_, dfa &dfa_)
    {
        typedef std::pair<std::size_t, size_t_set> sym_set_pair;
        typedef std::map<std::size_t, size_t_set> sym_set_map;
        // Not owner
        std::queue<size_t_set *> queue_;

        configs_.clear();
        dfa_.clear();
        configs_.push_back(size_t_set());
        closure(size_t_pair(start_._index, 0), grammar_, nts_,
            configs_.back());
        queue_.push(&configs_.back());

        do
        {
            size_t_set *set_ = queue_.front();
            sym_set_map map_;

            queue_.pop();
            dfa_.push_back(dfa_state());

            dfa_state &state_ = dfa_.back();

            for (typename size_t_set::const_iterator iter_ = set_->begin(),
                end_ = set_->end(); iter_ != end_; ++iter_)
            {
                if (iter_->second < grammar_[iter_->first]._rhs.size())
                {
                    size_t_set set_;
                    const symbol &symbol_ =
                        grammar_[iter_->first]._rhs[iter_->second];
                    const std::size_t id_ = symbol_._type == symbol::TERMINAL ?
                        terminals_.find(symbol_._name)->second._id :
                        nt_enums_.find(symbol_._name)->second;

                    closure(size_t_pair(iter_->first, iter_->second + 1),
                        grammar_, nts_, set_);

                    typename sym_set_map::iterator iter_ = map_.find(id_);

                    if (iter_ == map_.end())
                    {
                        iter_ = map_.insert(sym_set_pair
                            (id_, size_t_set())).first;
                        iter_->second.swap(set_);
                    }
                    else
                    {
                        iter_->second.insert(set_.begin(), set_.end());
                    }
                }
            }

            for (typename sym_set_map::iterator iter_ = map_.begin(),
                end_ = map_.end(); iter_ != end_; ++iter_)
            {
                std::size_t index_ = npos();
                typename size_t_set_deque::const_iterator config_iter_ =
                    std::find(configs_.begin(), configs_.end(), iter_->second);

                if (config_iter_ == configs_.end())
                {
                    index_ = configs_.size();
                    configs_.push_back(size_t_set());
                    configs_.back().insert
                        (iter_->second.begin(), iter_->second.end());
                    queue_.push(&configs_.back());
                }
                else
                {
                    index_ = config_iter_ - configs_.begin();
                }

                typename dfa_state::transition_map::iterator tran_iter_ =
                    state_._transitions.find(iter_->first);

                if (tran_iter_ == state_._transitions.end())
                {
                    state_._transitions.insert(typename dfa_state::
                        transition_pair(iter_->first, index_));
                }
            }
        } while (!queue_.empty());
    }

    static void closure(const size_t_pair &config_, const grammar &grammar_,
        const nt_map &nts_, size_t_set &set_)
    {
        std::queue<size_t_pair> queue_;

        queue_.push(config_);

        do
        {
            size_t_pair curr_ = queue_.front();

            queue_.pop();
            set_.insert(curr_);

            if (curr_.second < grammar_[curr_.first]._rhs.size())
            {
                const symbol &symbol_ =
                    grammar_[curr_.first]._rhs[curr_.second];

                if (symbol_._type == symbol::NON_TERMINAL)
                {
                    typename nt_map::const_iterator iter_ =
                        nts_.find(symbol_._name);

                    curr_.second = 0;

                    for (std::size_t index_ = iter_->second._first_production;
                        index_ != npos(); index_ = grammar_[index_]._next_lhs)
                    {
                        curr_.first = index_;

                        if (set_.insert(curr_).second)
                        {
                            queue_.push(curr_);
                        }
                    }
                }
            }
        } while (!queue_.empty());
    }

    static void rewrite(rules &rules_, const string &start_,
        const size_t_set_deque &configs_, const dfa &dfa_,
        const terminal_map &terminals_, const nt_enum_map &nt_enums_,
        size_t_map &new_old_map_)
    {
        const grammar &grammar_ = rules_.grammar();
        typedef std::pair<std::size_t, size_t_pair> trie;
        std::queue<trie> queue_;
        typedef typename size_t_set_deque::const_iterator configs_iter;
        std::size_t index_ = 0;
        prod_deque new_grammar_;

        // DFA state, rule, index in rule
        for (configs_iter iter_ = configs_.begin(), end_ = configs_.end();
            iter_ != end_; ++iter_, ++index_)
        {
            typename size_t_set::const_iterator i_ = iter_->begin();
            typename size_t_set::const_iterator e_ = iter_->end();

            for (; i_ != e_; ++i_)
            {
                if (i_->second == 0)
                {
                    queue_.push(trie(index_, *i_));
                }
            }
        }

        for (; !queue_.empty(); queue_.pop())
        {
            trie trie_ = queue_.front();
            const production &production_ = grammar_[trie_.second.first];
            prod *prod_ = 0;

            new_grammar_.push_back(prod());
            prod_ = &new_grammar_.back();
            prod_->_lhs = production_._lhs;
            prod_->_precedence = production_._prec_name;
            prod_->_rhs = production_._rhs;

            if (production_._lhs != start_)
            {
                prod_->_lhs_indexes.first = trie_.first;
                prod_->_lhs_indexes.second =
                    dfa_[trie_.first]._transitions.find(nt_enums_.find
                        (production_._lhs)->second)->second;
            }

            typedef typename symbol_deque::const_iterator rhs_iter;

            index_ = trie_.first;

            for (rhs_iter iter_ = production_._rhs.begin(),
                end_ = production_._rhs.end(); iter_ != end_; ++iter_)
            {
                prod_->_rhs_indexes.push_back(size_t_pair());
                prod_->_rhs_indexes.back().first = index_;

                switch (iter_->_type)
                {
                    case rules::symbol::TERMINAL:
                        index_ = dfa_[index_]._transitions.find
                            (terminals_.find(iter_->_name)->
                                second._id)->second;
                        break;
                    case rules::symbol::NON_TERMINAL:
                        index_ = dfa_[index_]._transitions.find(nt_enums_.find
                            (iter_->_name)->second)->second;
                        break;
                }

                prod_->_rhs_indexes.back().second = index_;
            }
        }

        minimise(new_grammar_, rules_.grammar(), new_old_map_);
        new_rules(rules_, new_grammar_);
    }

    static void minimise(prod_deque &grammar_, const grammar &orig_,
        size_t_map &new_old_map_)
    {
        std::size_t src_idx_ = 0;
        size_t_map del_map_;

        // Find duplicates
        for (typename prod_deque::const_iterator src_ = grammar_.begin(),
            end_ = grammar_.end(); src_ != end_; ++src_, ++src_idx_)
        {
            std::size_t idx_ = src_idx_ + 1;

            // Skip entries that are already marked for deletion
            if (del_map_.find(src_idx_) != del_map_.end())
                continue;

            for (typename prod_deque::const_iterator dest_ = src_ + 1;
                dest_ != end_; ++dest_, ++idx_)
            {
                if (src_->_lhs == dest_->_lhs &&
                    (src_->_rhs_indexes.empty() &&
                    dest_->_rhs_indexes.empty() ||
                    src_->_rhs == dest_->_rhs &&
                    src_->_rhs_indexes.back().second ==
                    dest_->_rhs_indexes.back().second))
                {
                    if (del_map_.find(idx_) == del_map_.end())
                    {
                        del_map_.insert(size_t_pair(idx_, src_idx_));
                    }
                }
            }
        }

        src_idx_ = 0;

        // Renumber
        for (typename prod_deque::iterator iter_ = grammar_.begin(),
            end_ = grammar_.end(); iter_ != end_; ++iter_, ++src_idx_)
        {
            // Only renumber rules that are not to be deleted.
            if (del_map_.find(src_idx_) == del_map_.end())
            {
                std::size_t idx_ = 0;

                for (typename symbol_deque::iterator rhs_iter_ =
                    iter_->_rhs.begin(), rhs_end_ = iter_->_rhs.end();
                    rhs_iter_ != rhs_end_; ++rhs_iter_, ++idx_)
                {
                    for (typename size_t_map::const_iterator del_iter_ =
                        del_map_.begin(), del_end_ = del_map_.end();
                        del_iter_ != del_end_; ++del_iter_)
                    {
                        const prod &prod_ = grammar_[del_iter_->first];

                        if (rhs_iter_->_type == symbol::NON_TERMINAL &&
                            rhs_iter_->_name == prod_._lhs &&
                            iter_->_rhs_indexes[idx_] == prod_._lhs_indexes)
                        {
                            iter_->_rhs_indexes[idx_] =
                                grammar_[del_iter_->second]._lhs_indexes;
                        }
                    }
                }
            }
        }

        // Delete redundant productions
        for (typename size_t_map::const_reverse_iterator del_iter_ =
            del_map_.rbegin(), del_end_ = del_map_.rend();
            del_iter_ != del_end_; ++del_iter_)
        {
            grammar_.erase(grammar_.begin() + del_iter_->first);
        }

        std::size_t dest_idx_ = 0;

        // Map new productions to old
        for (typename prod_deque::const_iterator iter_ = grammar_.begin(),
            end_ = grammar_.end(); iter_ != end_; ++iter_, ++dest_idx_)
        {
            std::size_t idx_ = 0;

            for (typename grammar::const_iterator src_ = orig_.begin(),
                src_end_ = orig_.end(); src_ != src_end_; ++src_, ++idx_)
            {
                if (iter_->_lhs == src_->_lhs && iter_->_rhs == src_->_rhs)
                {
                    new_old_map_[dest_idx_] = idx_;
                }
            }
        }
    }

    static void new_rules(rules &rules_, const prod_deque &new_grammar_)
    {
        const string &start_ = rules_.start();
        const bool dollar_ = !start_.empty() && start_[0] == '$';
        rules new_rules_;
        typename prod_deque::const_iterator iter_ = new_grammar_.begin();
        typename prod_deque::const_iterator end_ = new_grammar_.end();

        rules_.copy_terminals(new_rules_);

        for (; iter_ != end_; ++iter_)
        {
            string lhs_;
            string rhs_;
            const std::size_t size_ = iter_->_rhs.size();

            if (iter_->_lhs_indexes.first == 0 &&
                iter_->_lhs_indexes.second == 0)
            {
                lhs_ = iter_->_lhs;
            }
            else
            {
                std::basic_ostringstream<typename rules::char_type> ss_;

                ss_ << iter_->_lhs << iter_->_lhs_indexes.first << '_' <<
                    iter_->_lhs_indexes.second;
                lhs_ = ss_.str();
            }

            for (std::size_t index_ = 0; index_ < size_; ++index_)
            {
                const symbol &symbol_ = iter_->_rhs[index_];

                if (!rhs_.empty())
                {
                    rhs_ += ' ';
                }

                if (symbol_._type == symbol::TERMINAL ||
                    symbol_._name == start_)
                {
                    rhs_ += symbol_._name;
                }
                else
                {
                    std::basic_ostringstream<typename rules::char_type> ss_;

                    ss_ << symbol_._name <<
                        iter_->_rhs_indexes[index_].first <<
                        '_' << iter_->_rhs_indexes[index_].second;
                    rhs_ += ss_.str();
                }
            }

            if (!(dollar_ && lhs_ == start_))
            {
                if (!iter_->_precedence.empty())
                {
                    typename rules::char_type prec_[] =
                        {' ', '%', 'p', 'r', 'e', 'c', ' ', 0};

                    rhs_ += prec_;
                    rhs_ += iter_->_precedence;
                }

                new_rules_.push(lhs_.c_str(), rhs_.c_str());
            }
        }

        if (dollar_)
        {
            new_rules_.validate();
        }
        else
        {
            new_rules_.start(start_.c_str());
        }

        rules_.swap(new_rules_);
    }

    static void fill_nt_states(const nt_enum_map &nt_enums_,
        size_t_nt_state_map &nt_states_)
    {
        typename nt_enum_map::const_iterator iter_ = nt_enums_.begin();
        typename nt_enum_map::const_iterator end_ = nt_enums_.end();

        for (; iter_ != end_; ++iter_)
        {
            nt_states_.insert(size_t_nt_state_pair(iter_->second, nt_state()));
        }
    }

    static void build_first_sets(const grammar &grammar_,
        const terminal_map &terminals_, const nt_enum_map &nt_enums_,
        const nt_map &nts_, size_t_nt_state_map &nt_states_)
    {
        typename grammar::const_iterator iter_;
        typename grammar::const_iterator end_;

        calc_nullable(grammar_, nt_enums_, nt_states_);

        // Start with LHS of productions.
        for (;;)
        {
            bool changes_ = false;

            iter_ = grammar_.begin();
            end_ = grammar_.end();

            for (; iter_ != end_; ++iter_)
            {
                const production &production_ = *iter_;
                typename symbol_deque::const_iterator rhs_iter_ =
                    production_._rhs.begin();
                typename symbol_deque::const_iterator rhs_end_ =
                    production_._rhs.end();
                nt_state &lhs_ = nt_states_.find(nt_enums_.find
                    (production_._lhs)->second)->second;

                for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
                {
                    if (rhs_iter_->_type == symbol::TERMINAL)
                    {
                        if (lhs_._first_set.insert(terminals_.find
                            (rhs_iter_->_name)->second._id).second)
                        {
                            changes_ = true;
                        }

                        break;
                    }
                    else if (production_._lhs == rhs_iter_->_name)
                    {
                        if (lhs_._nullable == false)
                            break;
                    }
                    else
                    {
                        const std::size_t size_ = lhs_._first_set.size();
                        nt_state &rhs_ = nt_states_.find
                            (nt_enums_.find(rhs_iter_->_name)->second)->second;

                        lhs_._first_set.insert
                            (rhs_._first_set.begin(), rhs_._first_set.end());

                        if (size_ != lhs_._first_set.size())
                        {
                            changes_ = true;
                        }

                        if (rhs_._nullable == false)
                            break;
                    }
                }
            }

            if (!changes_) break;
        }

        // Now process RHS of productions
        for (iter_ = grammar_.begin(), end_ = grammar_.end();
            iter_ != end_; ++iter_)
        {
            const production &production_ = *iter_;
            typename symbol_deque::const_iterator rhs_iter_ =
                production_._rhs.begin();
            typename symbol_deque::const_iterator rhs_end_ =
                production_._rhs.end();

            for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
            {
                if (rhs_iter_->_type == symbol::NON_TERMINAL)
                {
                    nt_state *rhs_ = &nt_states_.find(nt_enums_.find
                        (rhs_iter_->_name)->second)->second;
                    typename symbol_deque::const_iterator next_iter_ =
                        rhs_iter_ + 1;

                    for (; rhs_->_nullable && next_iter_ != rhs_end_;
                        ++next_iter_)
                    {
                        if (next_iter_ != rhs_end_)
                        {
                            if (next_iter_->_type == symbol::TERMINAL)
                            {
                                rhs_->_first_set.insert(terminals_.find
                                    (next_iter_->_name)->second._id);
                                break;
                            }
                            else if (next_iter_->_type == symbol::NON_TERMINAL)
                            {
                                nt_state &next_rhs_ = nt_states_.find
                                    (nt_enums_.find(next_iter_->_name)->
                                    second)->second;

                                rhs_->_first_set.insert
                                    (next_rhs_._first_set.begin(),
                                    next_rhs_._first_set.end());
                                rhs_ = &nt_states_.find(nt_enums_.find
                                    (next_iter_->_name)->second)->second;
                            }
                        }
                    }

                    if (rhs_->_nullable && next_iter_ == rhs_end_)
                    {
                        nt_states_.find(nt_enums_.find
                            (rhs_iter_->_name)->second)->
                            second._nullable = true;
                    }
                }
            }
        }
    }

    static void calc_nullable(const grammar &grammar_,
        const nt_enum_map &nt_enums_, size_t_nt_state_map &nt_states_)
    {
        for (;;)
        {
            bool changes_ = false;
            typename grammar::const_iterator iter_ = grammar_.begin();
            typename grammar::const_iterator end_ = grammar_.end();

            for (; iter_ != end_; ++iter_)
            {
                const production &production_ = *iter_;
                typename size_t_nt_state_map::iterator state_iter_ =
                    nt_states_.find(nt_enums_.find(production_._lhs)->second);

                if (state_iter_->second._nullable) continue;

                typename symbol_deque::const_iterator rhs_iter_ =
                    production_._rhs.begin();
                typename symbol_deque::const_iterator rhs_end_ =
                    production_._rhs.end();

                for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
                {
                    const symbol &symbol_ = *rhs_iter_;

                    if (symbol_._type != symbol::NON_TERMINAL) break;

                    typename size_t_nt_state_map::iterator state_iter_ =
                        nt_states_.find(nt_enums_.find(symbol_._name)->second);

                    if (state_iter_->second._nullable == false) break;
                }

                if (rhs_iter_ == rhs_end_)
                {
                    state_iter_->second._nullable = true;
                    changes_ = true;
                }
            }

            if (!changes_) break;
        }
    }

    static void build_follow_sets(const grammar &grammar_,
        const terminal_map &terminals_, const nt_enum_map &nt_enums_,
        size_t_nt_state_map &nt_states_)
    {
        for (;;)
        {
            bool changes_ = false;
            typename grammar::const_iterator iter_ = grammar_.begin();
            typename grammar::const_iterator end_ = grammar_.end();

            for (; iter_ != end_; ++iter_)
            {
                const production &production_ = *iter_;
                typename symbol_deque::const_iterator rhs_iter_ =
                    production_._rhs.begin();
                typename symbol_deque::const_iterator rhs_end_ =
                    production_._rhs.end();

                for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
                {
                    if (rhs_iter_->_type == symbol::NON_TERMINAL)
                    {
                        typename symbol_deque::const_iterator next_iter_ =
                            rhs_iter_ + 1;
                        typename size_t_nt_state_map::iterator
                            lstate_iter_ = nt_states_.find
                            (nt_enums_.find(rhs_iter_->_name)->second);
                        const std::size_t size_ =
                            lstate_iter_->second._follow_set.size();

                        if (next_iter_ != rhs_end_)
                        {
                            if (next_iter_->_type == symbol::TERMINAL)
                            {
                                // Just add terminal.
                                lstate_iter_->second._follow_set.insert
                                    (terminals_.find(next_iter_->_name)->
                                    second._id);
                            }
                            else
                            {
                                // If there is a production A -> aBb
                                // then everything in FIRST(b) is
                                // placed in FOLLOW(B).
                                typename size_t_nt_state_map::const_iterator
                                    rstate_iter_ = nt_states_.find
                                    (nt_enums_.find(next_iter_->_name)->
                                    second);

                                lstate_iter_->second._follow_set.insert
                                    (rstate_iter_->second._first_set.begin(),
                                    rstate_iter_->second._first_set.end());
                                ++next_iter_;

                                // If nullable, keep going
                                for (; rstate_iter_->second._nullable &&
                                    next_iter_ != rhs_end_; ++next_iter_)
                                {
                                    if (next_iter_->_type == symbol::TERMINAL)
                                    {
                                        // Just add terminal.
                                        lstate_iter_->second._follow_set.insert
                                            (terminals_.find(next_iter_->_name)->
                                            second._id);
                                        break;
                                    }
                                    else
                                    {
                                        rstate_iter_ = nt_states_.find
                                            (nt_enums_.find(next_iter_->_name)->
                                            second);
                                        lstate_iter_->second._follow_set.insert
                                            (rstate_iter_->second._first_set.begin(),
                                            rstate_iter_->second._first_set.end());
                                    }
                                }
                            }
                        }

                        if (next_iter_ == rhs_end_)
                        {
                            // If there is a production A -> aB
                            // then everything in FOLLOW(A) is in FOLLOW(B).
                            typename size_t_nt_state_map::const_iterator
                                rstate_iter_ = nt_states_.find
                                    (nt_enums_.find(production_._lhs)->second);

                            lstate_iter_->second._follow_set.insert
                                (rstate_iter_->second._follow_set.begin(),
                                rstate_iter_->second._follow_set.end());
                        }

                        if (size_ != lstate_iter_->second.
                            _follow_set.size())
                        {
                            changes_ = true;
                        }
                    }
                }
            }

            if (!changes_) break;
        }
    }

    static void build_table(const dfa &dfa_, const size_t_set_deque &configs_,
        const grammar &grammar_, const string &start_,
        const size_t_nt_state_map &nt_states_, const terminal_map &terminals_,
        const nt_enum_map &nt_enums_, const symbol_map &symbols_,
        state_machine &sm_, std::string *warnings_)
    {
        const std::size_t row_size_ = terminals_.size() + nt_enums_.size();
        const std::size_t size_ = dfa_.size();
        typename dfa::const_iterator end_ = dfa_.end();

        sm_._columns = terminals_.size() + nt_enums_.size();
        sm_._rows = dfa_.size();
        sm_._table.resize(sm_._columns * sm_._rows);

        for (std::size_t index_ = 0; index_ < size_; ++index_)
        {
            for (typename size_t_set::const_iterator iter_ =
                configs_[index_].begin(), end_ = configs_[index_].end();
                iter_ != end_; ++iter_)
            {
                const production &production_ = grammar_[iter_->first];

                if (production_._rhs.size() == iter_->second)
                {
                    // Cursor at end
                    if (production_._lhs == start_)
                    {
                        typename state_machine::entry lhs_;
                        typename state_machine::entry &rhs_ =
                            sm_._table[index_ * row_size_];

                        // Accepting state
                        lhs_._action = accept;
                        fill_entry(configs_[index_], grammar_, terminals_,
                            symbols_, 0, lhs_, rhs_, warnings_);
                    }
                    else
                    {
                        const std::size_t lhs_id_ =
                            nt_enums_.find(production_._lhs)->second;
                        typename size_t_nt_state_map::const_iterator nt_iter_ =
                            nt_states_.find(lhs_id_);
                        typename nt_state::size_t_set::const_iterator
                            follow_iter_ = nt_iter_->second.
                                _follow_set.begin();
                        typename nt_state::size_t_set::const_iterator
                            follow_end_ = nt_iter_->second._follow_set.end();

                        for (; follow_iter_ != follow_end_; ++follow_iter_)
                        {
                            typename state_machine::entry lhs_
                                (reduce, production_._index);
                            typename state_machine::entry &rhs_ = sm_._table
                                [index_ * row_size_ + *follow_iter_];

                            fill_entry(configs_[index_], grammar_, terminals_,
                                symbols_, *follow_iter_, lhs_, rhs_,
                                warnings_);
                        }
                    }
                }
            }

            for (typename dfa_state::transition_map::const_iterator iter_ =
                dfa_[index_]._transitions.begin(),
                end_ = dfa_[index_]._transitions.end();
                iter_ != end_; ++iter_)
            {
                const std::size_t &id_ = iter_->first;
                typename state_machine::entry lhs_;
                typename state_machine::entry &rhs_ =
                    sm_._table[index_ * row_size_ + id_];

                if (id_ < terminals_.size())
                {
                    // TERMINAL
                    lhs_._action = shift;
                }
                else
                {
                    // NON_TERMINAL
                    lhs_._action = go_to;
                }

                lhs_._param = iter_->second;
                fill_entry(configs_[index_], grammar_, terminals_, symbols_,
                    id_, lhs_, rhs_, warnings_);
            }
        }
    }

    static void fill_entry(const size_t_set &config_,
        const grammar &grammar_, const terminal_map &terminals_,
        const symbol_map &symbols_, const std::size_t id_,
        const typename state_machine::entry &lhs_,
        typename state_machine::entry &rhs_, std::string *warnings_)
    {
        static const char *actions_ [] =
        { "ERROR", "SHIFT", "REDUCE", "GOTO", "ACCEPT" };
        bool error_ = false;
        bool left_ = true;

        if (rhs_._action == error)
        {
            if (rhs_._param == syntax_error)
            {
                // No conflict
                rhs_ = lhs_;
            }
            else
            {
                error_ = true;
            }
        }
        else
        {
            std::size_t lhs_prec_ = 0;
            typename token_info::associativity lhs_assoc_ =
                token_info::precedence;
            std::size_t rhs_prec_ = 0;
            typename token_info::associativity rhs_assoc_ =
                token_info::precedence;

            if (lhs_._action == shift)
            {
                typename terminal_map::const_iterator iter_ =
                    terminals_.find(symbols_.find(id_)->second);

                lhs_prec_ = iter_->second._precedence;
                lhs_assoc_ = iter_->second._associativity;
            }
            else if (lhs_._action == reduce)
            {
                lhs_prec_ = grammar_[lhs_._param]._precedence;
            }

            if (rhs_._action == shift)
            {
                typename terminal_map::const_iterator iter_ =
                    terminals_.find(symbols_.find(id_)->second);

                rhs_prec_ = iter_->second._precedence;
                rhs_assoc_ = iter_->second._associativity;
            }
            else if (rhs_._action == reduce)
            {
                rhs_prec_ = grammar_[rhs_._param]._precedence;
            }

            if (lhs_._action == shift && rhs_._action == reduce)
            {
                if (lhs_prec_ == 0 || rhs_prec_ == 0)
                {
                    // Favour shift
                    if (warnings_)
                    {
                        std::ostringstream ss_;

                        ss_ << actions_[lhs_._action];
                        dump_action(config_, grammar_, symbols_,
                            id_, lhs_, ss_);
                        ss_ << '/' << actions_[rhs_._action];
                        dump_action(config_, grammar_, symbols_,
                            id_, rhs_, ss_);
                        ss_ << " conflict.\n";
                        *warnings_ += ss_.str();
                    }

                    // Assign new value after warning!
                    rhs_ = lhs_;
                }
                else if (lhs_prec_ == rhs_prec_)
                {
                    switch (lhs_assoc_)
                    {
                        case token_info::precedence:
                            // Favour shift
                            if (warnings_)
                            {
                                std::ostringstream ss_;

                                ss_ << actions_[lhs_._action];
                                dump_action(config_, grammar_, symbols_,
                                    id_, lhs_, ss_);
                                ss_ << '/' << actions_[rhs_._action];
                                dump_action(config_, grammar_, symbols_,
                                    id_, rhs_, ss_);
                                ss_ << " conflict.\n";
                                *warnings_ += ss_.str();
                            }

                            // Assign new value after warning!
                            rhs_ = lhs_;
                            break;
                        case token_info::nonassoc:
                            rhs_._action = error;
                            rhs_._param = non_associative;
                            break;
                        case token_info::right:
                            rhs_ = lhs_;
                            break;
                    }
                }
                else if (lhs_prec_ > rhs_prec_)
                {
                    rhs_ = lhs_;
                }
            }
            else if (lhs_._action == reduce && rhs_._action == reduce)
            {
                if (lhs_prec_ == 0 || rhs_prec_ == 0 || lhs_prec_ == rhs_prec_)
                {
                    error_ = true;
                }
                else if (lhs_prec_ > rhs_prec_)
                {
                    rhs_ = lhs_;
                }
            }
            else
            {
                error_ = true;
            }
        }

        if (error_)
        {
            std::ostringstream ss_;

            ss_ << actions_[lhs_._action];
            dump_action(config_, grammar_, symbols_, id_, lhs_, ss_);
            ss_ << '/' << actions_[rhs_._action];
            dump_action(config_, grammar_, symbols_, id_, rhs_, ss_);
            ss_ << " conflict.";
            throw runtime_error(ss_.str());
        }
    }

    static void dump_action(const size_t_set &config_, const grammar &grammar_,
        const symbol_map &symbols_, const std::size_t id_,
        const typename state_machine::entry &entry_, std::ostringstream &ss_)
    {
        if (entry_._action == shift)
        {
            typename size_t_set::const_iterator iter_ =
                config_.begin();
            typename size_t_set::const_iterator end_ =
                config_.end();

            for (; iter_ != end_; ++iter_)
            {
                const production &production_ = grammar_[iter_->first];

                if (production_._rhs.size() > iter_->second &&
                    production_._rhs[iter_->second]._name ==
                    symbols_.find(id_)->second)
                {
                    dump_production(production_, iter_->second, ss_);
                }
            }
        }
        else if (entry_._action == reduce)
        {
            const production &production_ = grammar_[entry_._param];

            dump_production(production_, ~0, ss_);
        }
    }

    static void dump_production(const production &production_,
        const std::size_t dot_, std::ostringstream &ss_)
    {
        typename symbol_deque::const_iterator sym_iter_ =
            production_._rhs.begin();
        typename symbol_deque::const_iterator sym_end_ =
            production_._rhs.end();
        std::size_t index_ = 0;

        ss_ << " (";
        narrow(production_._lhs.c_str(), ss_);
        ss_ << " -> ";

        if (sym_iter_ != sym_end_)
        {
            if (index_ == dot_) ss_ << ". ";

            narrow(sym_iter_++->_name.c_str(), ss_);
            ++index_;
        }

        for (; sym_iter_ != sym_end_; ++sym_iter_, ++index_)
        {
            ss_ << ' ';

            if (index_ == dot_) ss_ << ". ";

            narrow(sym_iter_->_name.c_str(), ss_);
        }

        ss_ << ')';
    }

    static void copy_rules(const grammar &grammar_,
        const terminal_map &terminals_, const nt_enum_map &nt_enums_,
        state_machine &sm_)
    {
        typename grammar::const_iterator iter_ = grammar_.begin();
        typename grammar::const_iterator end_ = grammar_.end();

        for (; iter_ != end_; ++iter_)
        {
            const production &production_ = *iter_;
            typename symbol_deque::const_iterator rhs_iter_ =
                production_._rhs.begin();
            typename symbol_deque::const_iterator rhs_end_ =
                production_._rhs.end();

            sm_._rules.push_back(typename state_machine::size_t_size_t_pair());

            typename state_machine::size_t_size_t_pair &pair_ =
                sm_._rules.back();

            pair_.first = nt_enums_.find(production_._lhs)->second;

            for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
            {
                const symbol &symbol_ = *rhs_iter_;

                if (symbol_._type == symbol::TERMINAL)
                {
                    pair_.second.push_back(terminals_.find
                        (symbol_._name)->second._id);
                }
                else
                {
                    pair_.second.push_back(nt_enums_.find
                        (symbol_._name)->second);
                }
            }
        }
    }

    static std::size_t npos()
    {
        return ~0;
    }
};

typedef basic_generator<rules> generator;
typedef basic_generator<wrules> wgenerator;
}

#endif
