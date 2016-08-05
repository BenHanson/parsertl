// generator.hpp
// Copyright (c) 2014-2016 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_GENERATOR_HPP
#define PARSERTL_GENERATOR_HPP

#include "dfa.hpp"
#include "narrow.hpp"
#include "nt_info.hpp"
#include "rules.hpp"
#include "state_machine.hpp"

namespace parsertl
{
template<typename rules>
class basic_generator
{
public:
    typedef typename rules::symbol_vector symbol_vector;

    struct prod
    {
        std::size_t _lhs;
        size_t_pair _lhs_indexes;
        symbol_vector _rhs;
        size_t_pair_vector _rhs_indexes;

        void swap(prod &rhs_)
        {
            std::swap(_lhs, rhs_._lhs);
            std::swap(_lhs_indexes, rhs_._lhs_indexes);
            _rhs.swap(rhs_._rhs);
            std::swap(_rhs_indexes, rhs_._rhs_indexes);
        }
    };

    typedef std::deque<prod> prod_deque;
    typedef typename rules::string string;

    static void build(rules &rules_, state_machine &sm_,
        std::string *warnings_ = 0)
    {
        dfa dfa_;
        prod_deque new_grammar_;
        rules new_rules_;

        rules_.validate();
        build_dfa(rules_, dfa_);
        rewrite(rules_, dfa_, new_grammar_, new_rules_);

        nt_info_vector new_nt_info_(new_rules_.nt_locations().size(),
            nt_info(new_rules_.tokens_info().size()));

        build_first_sets(new_rules_, new_nt_info_);
        // First add EOF to follow_set of start.
        new_nt_info_[new_rules_.start()]._follow_set[0] = 1;
        build_follow_sets(new_rules_, new_nt_info_);
        sm_.clear();
        build_table(rules_, dfa_, new_grammar_, new_rules_, new_nt_info_, sm_,
            warnings_);
        copy_rules(rules_, sm_);
    }

    static void build_dfa(const rules &rules_, dfa &dfa_)
    {
        const grammar &grammar_ = rules_.grammar();
        const std::size_t terminals_ = rules_.tokens_info().size();
        const std::size_t start_ = rules_.start();
        hash_map hash_map_;

        dfa_.push_back(dfa_state());

        // Only applies if build_dfa() has been called directly
        // from client code (i.e. build() will have already called
        // validate() in the normal case).
        if (start_ == npos())
        {
            dfa_.back()._basis.push_back(size_t_pair(0, 0));
        }
        else
        {
            const std::size_t index_ = rules_.nt_locations()[start_].
                _first_production;

            dfa_.back()._basis.push_back(size_t_pair(index_, 0));
        }

        hash_map_[hash_set(dfa_.back()._basis)].push_back(0);

        for (std::size_t s_ = 0; s_ < dfa_.size(); ++s_)
        {
            dfa_state &state_ = dfa_[s_];
            size_t_vector symbols_;
            typedef std::deque<size_t_pair_vector> item_sets;
            item_sets item_sets_;

            state_._closure.assign(state_._basis.begin(), state_._basis.end());
            closure(rules_, state_);

            for (typename size_t_pair_vector::const_iterator iter_ =
                state_._closure.begin(), end_ = state_._closure.end();
                iter_ != end_; ++iter_)
            {
                const production p_ = grammar_[iter_->first];

                if (iter_->second < p_._rhs.size())
                {
                    const symbol &symbol_ = p_._rhs[iter_->second];
                    const std::size_t id_ = symbol_._type == symbol::TERMINAL ?
                        symbol_._id : terminals_ + symbol_._id;
                    typename size_t_vector::const_iterator sym_iter_ =
                        std::find(symbols_.begin(), symbols_.end(), id_);
                    size_t_pair new_pair_(iter_->first, iter_->second + 1);

                    if (sym_iter_ == symbols_.end())
                    {
                        symbols_.push_back(id_);
                        item_sets_.push_back(size_t_pair_vector());
                        item_sets_.back().push_back(new_pair_);
                    }
                    else
                    {
                        const std::size_t index_ =
                            sym_iter_ - symbols_.begin();
                        size_t_pair_vector &vec_ = item_sets_[index_];
                        typename size_t_pair_vector::const_iterator i_ =
                            std::find(vec_.begin(), vec_.end(), new_pair_);

                        if (i_ == vec_.end())
                        {
                            vec_.push_back(new_pair_);
                        }
                    }
                }
            }

            for (typename size_t_vector::const_iterator iter_ =
                symbols_.begin(), end_ = symbols_.end();
                iter_ != end_; ++iter_)
            {
                std::size_t index_ = iter_ - symbols_.begin();
                size_t_pair_vector &basis_ = item_sets_[index_];

                std::sort(basis_.begin(), basis_.end());
                index_ = add_dfa_state(dfa_, hash_map_, basis_);
                state_._transitions.push_back(size_t_pair(*iter_, index_));
            }
        }
    }

    // http://www.sqlite.org/src/artifact?ci=trunk&filename=tool/lemon.c
    // FindFirstSets()
    static void build_first_sets(const rules &rules_, nt_info_vector &nt_info_)
    {
        bool progress_ = true;
        const grammar &grammar_ = rules_.grammar();
        const std::size_t terminals_ = rules_.tokens_info().size();

        // First compute all lambdas
        do
        {
            progress_ = 0;

            for (typename grammar::const_iterator iter_ = grammar_.begin(),
                end_ = grammar_.end(); iter_ != end_; ++iter_)
            {
                if (nt_info_[iter_->_lhs]._nullable) continue;

                std::size_t i_ = 0;
                const std::size_t rhs_size_ = iter_->_rhs.size();

                for (; i_ < rhs_size_; i_++)
                {
                    const symbol &symbol_ = iter_->_rhs[i_];

                    if (symbol_._type != symbol::NON_TERMINAL ||
                        !nt_info_[symbol_._id]._nullable)
                    {
                        break;
                    }
                }

                if (i_ == rhs_size_)
                {
                    nt_info_[iter_->_lhs]._nullable = true;
                    progress_ = 1;
                }
            }
        } while (progress_);

        // Now compute all first sets
        do
        {
            progress_ = 0;

            for (typename grammar::const_iterator iter_ = grammar_.begin(),
                end_ = grammar_.end(); iter_ != end_; ++iter_)
            {
                nt_info &lhs_info_ = nt_info_[iter_->_lhs];
                const std::size_t rhs_size_ = iter_->_rhs.size();

                for (std::size_t i_ = 0; i_ < rhs_size_; i_++)
                {
                    const symbol &s2_ = iter_->_rhs[i_];

                    if (s2_._type == symbol::TERMINAL)
                    {
                        progress_ |= set_add(lhs_info_._first_set, s2_._id);
                        break;
                    }
                    else if (iter_->_lhs == s2_._id)
                    {
                        if (!lhs_info_._nullable) break;
                    }
                    else
                    {
                        nt_info &rhs_info_ = nt_info_[s2_._id];

                        progress_ |= set_union(lhs_info_._first_set,
                            rhs_info_._first_set);

                        if (!rhs_info_._nullable) break;
                    }
                }
            }
        } while (progress_);
    }

    static void build_follow_sets(const rules &rules_, nt_info_vector &nt_info_)
    {
        const grammar &grammar_ = rules_.grammar();

        for (;;)
        {
            bool changes_ = false;
            typename grammar::const_iterator iter_ = grammar_.begin();
            typename grammar::const_iterator end_ = grammar_.end();

            for (; iter_ != end_; ++iter_)
            {
                const production &production_ = *iter_;
                const std::size_t lhs_id_ = production_._lhs;
                typename symbol_vector::const_iterator rhs_iter_ =
                    production_._rhs.begin();
                typename symbol_vector::const_iterator rhs_end_ =
                    production_._rhs.end();

                for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
                {
                    if (rhs_iter_->_type == symbol::NON_TERMINAL)
                    {
                        typename symbol_vector::const_iterator next_iter_ =
                            rhs_iter_ + 1;
                        const std::size_t rhs_id_ = rhs_iter_->_id;
                        nt_info *lstate_iter_ = &nt_info_[rhs_id_];
                        bool nullable_ = next_iter_ == rhs_end_;

                        if (next_iter_ != rhs_end_)
                        {
                            if (next_iter_->_type == symbol::TERMINAL)
                            {
                                const std::size_t id_ = next_iter_->_id;

                                // Just add terminal.
                                changes_ |=
                                    set_add(lstate_iter_->_follow_set, id_);
                            }
                            else
                            {
                                // If there is a production A -> aBb
                                // then everything in FIRST(b) is
                                // placed in FOLLOW(B).
                                nt_info *rstate_iter_ =
                                    &nt_info_[next_iter_->_id];

                                changes_ |= set_union
                                    (lstate_iter_->_follow_set,
                                        rstate_iter_->_first_set);
                                ++next_iter_;

                                // If nullable, keep going
                                if (rstate_iter_->_nullable)
                                {
                                    for (; next_iter_ != rhs_end_; ++next_iter_)
                                    {
                                        std::size_t next_id_ =
                                            static_cast<std::size_t>(~0);

                                        if (next_iter_->_type ==
                                            symbol::TERMINAL)
                                        {
                                            next_id_ = next_iter_->_id;

                                            // Just add terminal.
                                            changes_ |= set_add
                                                (lstate_iter_->_follow_set,
                                                    next_id_);
                                            break;
                                        }
                                        else
                                        {
                                            next_id_ = next_iter_->_id;
                                            rstate_iter_ = &nt_info_[next_id_];
                                            changes_ |= set_union
                                                (lstate_iter_->_follow_set,
                                                    rstate_iter_->_first_set);

                                            if (!rstate_iter_->_nullable)
                                            {
                                                break;
                                            }
                                        }
                                    }

                                    nullable_ = next_iter_ == rhs_end_;
                                }
                            }
                        }

                        if (nullable_)
                        {
                            // If there is a production A -> aB
                            // then everything in FOLLOW(A) is in FOLLOW(B).
                            nt_info *rstate_iter_ = &nt_info_[lhs_id_];

                            changes_ |= set_union
                                (lstate_iter_->_follow_set,
                                    rstate_iter_->_follow_set);
                        }
                    }
                }
            }

            if (!changes_) break;
        }
    }

    static void rewrite(const rules &rules_, dfa &dfa_,
        prod_deque &new_grammar_, rules &new_rules_)
    {
        const grammar &grammar_ = rules_.grammar();
        string_vector terminals_;
        string_vector non_terminals_;
        string start_;

        rules_.copy_terminals(new_rules_);
        rules_.terminals(terminals_);
        rules_.non_terminals(non_terminals_);
        start_ = non_terminals_[rules_.start()];

        for (std::size_t sidx_ = 0, ssize_ = dfa_.size();
            sidx_ != ssize_; ++sidx_)
        {
            const dfa_state &state_ = dfa_[sidx_];

            for (std::size_t cidx_ = 0, csize_ = state_._closure.size();
                cidx_ != csize_; ++cidx_)
            {
                const size_t_pair &pair_ = state_._closure[cidx_];

                if (pair_.second != 0) continue;

                const production &production_ = grammar_[pair_.first];
                std::string lhs_ = non_terminals_[production_._lhs];
                std::string rhs_;
                prod prod_;

                prod_._lhs = production_._lhs;
                prod_._rhs = production_._rhs;

                if (lhs_ != start_)
                {
                    ostringstream ss_;
                    const std::size_t id_ = production_._lhs +
                        terminals_.size();

                    ss_ << '_' << sidx_ << '_';
                    prod_._lhs_indexes.first = sidx_;

                    for (std::size_t tidx_ = 0,
                        tsize_ = state_._transitions.size();
                        tidx_ != tsize_; ++tidx_)
                    {
                        const size_t_pair &pr_ =
                            state_._transitions[tidx_];

                        if (pr_.first == id_)
                        {
                            ss_ << pr_.second;
                            prod_._lhs_indexes.second = pr_.second;
                            break;
                        }
                    }

                    lhs_ += ss_.str();
                }

                std::size_t index_ = sidx_;

                if (production_._rhs.empty())
                {
                    prod_._rhs_indexes.push_back(size_t_pair(sidx_, sidx_));
                }

                for (std::size_t ridx_ = 0, rsize_ = production_._rhs.size();
                    ridx_ != rsize_; ++ridx_)
                {
                    const symbol &symbol_ = production_._rhs[ridx_];

                    prod_._rhs_indexes.push_back(size_t_pair(index_, 0));

                    if (ridx_ > 0) rhs_ += ' ';

                    if (symbol_._type == symbol::TERMINAL ||
                        symbol_._id == rules_.start())
                    {
                        const string &str_ =
                            symbol_._type == symbol::TERMINAL ?
                            terminals_[symbol_._id] :
                            non_terminals_[symbol_._id];

                        if (str_[0] != '$')
                        {
                            rhs_ += str_;
                        }

                        for (std::size_t tidx_ = 0,
                            tsize_ = dfa_[index_]._transitions.size();
                            tidx_ != tsize_; ++tidx_)
                        {
                            const std::size_t id_ = symbol_._id +
                                (symbol_._type == symbol::TERMINAL ? 0 :
                                    terminals_.size());
                            const size_t_pair &pr_ =
                                dfa_[index_]._transitions[tidx_];

                            if (pr_.first == id_)
                            {
                                index_ = pr_.second;
                                break;
                            }
                        }
                    }
                    else
                    {
                        ostringstream ss_;

                        rhs_ += non_terminals_[symbol_._id] + '_';
                        ss_ << index_ << '_';

                        for (std::size_t tidx_ = 0,
                            tsize_ = dfa_[index_]._transitions.size();
                            tidx_ != tsize_; ++tidx_)
                        {
                            const std::size_t id_ = symbol_._id +
                                (symbol_._type == symbol::TERMINAL ? 0 :
                                    terminals_.size());
                            const size_t_pair &pr_ =
                                dfa_[index_]._transitions[tidx_];

                            if (pr_.first == id_)
                            {
                                index_ = pr_.second;
                                ss_ << index_;
                                break;
                            }
                        }

                        rhs_ += ss_.str();
                    }

                    prod_._rhs_indexes.back().second = index_;
                }

                if (lhs_[0] != '$')
                {
                    new_grammar_.push_back(prod());
                    new_grammar_.back().swap(prod_);
                    new_rules_.push(lhs_.c_str(), rhs_.c_str());
                }
            }
        }

        if (start_[0] != '$')
        {
            new_rules_.start(start_.c_str());
        }

        new_rules_.validate();
    }

private:
    typedef typename rules::char_type char_type;
    typedef std::deque<char_vector> char_vector_deque;
    typedef typename rules::production_deque grammar;
    typedef std::basic_ostringstream<char_type> ostringstream;
    typedef typename rules::production production;
    typedef std::vector<std::size_t> size_t_vector;
    typedef std::map<std::size_t, size_t_vector> hash_map;
    typedef typename rules::string_size_t_map string_size_t_map;
    typedef typename rules::string_vector string_vector;
    typedef typename rules::symbol symbol;
    typedef typename rules::token_info token_info;
    typedef typename rules::token_info_vector token_info_vector;

    static void build_table(const rules &rules_, const dfa &dfa_,
        const prod_deque &new_grammar_, const rules &new_rules_,
        const nt_info_vector &new_nt_info_, state_machine &sm_,
        std::string *warnings_)
    {
        const grammar &grammar_ = rules_.grammar();
        const std::size_t start_ = rules_.start();
        const std::size_t terminals_ = rules_.tokens_info().size();
        const std::size_t non_terminals_ = rules_.nt_locations().size();
        string_vector symbols_;
        const std::size_t columns_ = terminals_ + non_terminals_;
        const std::size_t dfa_size_ = dfa_.size();
        const string_size_t_map &new_nt_map_ = new_rules_.non_terminals();

        rules_.terminals(symbols_);
        rules_.non_terminals(symbols_);
        sm_._columns = columns_;
        sm_._rows = dfa_.size();
        sm_._table.resize(sm_._columns * sm_._rows);

        for (std::size_t index_ = 0; index_ < dfa_size_; ++index_)
        {
            // shift and gotos
            for (typename size_t_pair_vector::const_iterator iter_ =
                dfa_[index_]._transitions.begin(),
                end_ = dfa_[index_]._transitions.end();
                iter_ != end_; ++iter_)
            {
                const std::size_t id_ = iter_->first;
                typename state_machine::entry &lhs_ =
                    sm_._table[index_ * columns_ + id_];
                typename state_machine::entry rhs_;

                if (id_ < terminals_)
                {
                    // TERMINAL
                    rhs_._action = shift;
                }
                else
                {
                    // NON_TERMINAL
                    rhs_._action = go_to;
                }

                rhs_._param = iter_->second;
                fill_entry(rules_, dfa_[index_]._closure, symbols_,
                    lhs_, id_, rhs_, warnings_);
            }

            // reductions
            for (typename size_t_pair_vector::const_iterator iter_ =
                dfa_[index_]._closure.begin(),
                end_ = dfa_[index_]._closure.end(); iter_ != end_; ++iter_)
            {
                const production &production_ = grammar_[iter_->first];

                if (production_._rhs.size() == iter_->second)
                {
                    char_vector follow_set_(terminals_, 0);

                    // config is reduction
                    for (typename prod_deque::const_iterator p_iter_ =
                        new_grammar_.begin(), p_end_ = new_grammar_.end();
                        p_iter_ != p_end_; ++p_iter_)
                    {
                        if (production_._lhs == p_iter_->_lhs &&
                            production_._rhs == p_iter_->_rhs &&
                            index_ == p_iter_->_rhs_indexes.back().second)
                        {
                            ostringstream lhs_;
                            std::size_t lhs_id_ = static_cast<std::size_t>(~0);

                            lhs_ << symbols_[terminals_ + production_._lhs];

                            if (production_._lhs != start_)
                            {
                                lhs_ << '_' << p_iter_->_lhs_indexes.first <<
                                    '_' << p_iter_->_lhs_indexes.second;
                            }

                            lhs_id_ = new_nt_map_.find(lhs_.str())->second;
                            set_union(follow_set_,
                                new_nt_info_[lhs_id_]._follow_set);
                        }
                    }

                    for (std::size_t i_ = 0, size_ = follow_set_.size();
                        i_ < size_; ++i_)
                    {
                        if (!follow_set_[i_]) continue;

                        typename state_machine::entry &lhs_ =
                            sm_._table[index_ * columns_ + i_];
                        typename state_machine::entry rhs_
                            (reduce, production_._index);

                        if (production_._lhs == start_)
                        {
                            rhs_._action = accept;
                        }

                        fill_entry(rules_, dfa_[index_]._closure, symbols_,
                            lhs_, i_, rhs_, warnings_);
                    }
                }
            }
        }
    }

    static void copy_rules(const rules &rules_, state_machine &sm_)
    {
        const grammar &grammar_ = rules_.grammar();
        const std::size_t terminals_ = rules_.tokens_info().size();
        typename grammar::const_iterator iter_ = grammar_.begin();
        typename grammar::const_iterator end_ = grammar_.end();

        for (; iter_ != end_; ++iter_)
        {
            const production &production_ = *iter_;
            typename symbol_vector::const_iterator rhs_iter_ =
                production_._rhs.begin();
            typename symbol_vector::const_iterator rhs_end_ =
                production_._rhs.end();

            sm_._rules.push_back(typename state_machine::size_t_size_t_pair());

            typename state_machine::size_t_size_t_pair &pair_ =
                sm_._rules.back();

            pair_.first = terminals_ + production_._lhs;

            for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
            {
                const symbol &symbol_ = *rhs_iter_;

                if (symbol_._type == symbol::TERMINAL)
                {
                    pair_.second.push_back(symbol_._id);
                }
                else
                {
                    pair_.second.push_back(terminals_ + symbol_._id);
                }
            }
        }
    }

    // Helper functions:

    // Add a new element to the set. Return true if the element was added
    // and false if it was already there.
    static bool set_add(char_vector &s_, const std::size_t e_)
    {
        const char rv_ = s_[e_];

        assert(e_ < s_.size());
        s_[e_] = 1;
        return !rv_;
    }

    // Add every element of rhs_ to lhs_. Return true if lhs_ changes.
    static bool set_union(char_vector &lhs_, const char_vector &rhs_)
    {
        const std::size_t size_ = lhs_.size();
        bool progress_ = false;
        char *s1_ = &lhs_.front();
        const char *s2_ = &rhs_.front();

        for (std::size_t i_ = 0; i_ < size_; i_++)
        {
            if (s2_[i_] == 0) continue;

            if (s1_[i_] == 0)
            {
                progress_ = true;
                s1_[i_] = 1;
            }
        }

        return progress_;
    }

    static void closure(const rules &rules_, dfa_state &state_)
    {
        const typename rules::nt_location_vector &nt_locations_ =
            rules_.nt_locations();
        const grammar &grammar_ = rules_.grammar();

        for (std::size_t c_ = 0; c_ < state_._closure.size(); ++c_)
        {
            const size_t_pair pair_ = state_._closure[c_];
            const production *p_ = &grammar_[pair_.first];
            const std::size_t rhs_size_ = p_->_rhs.size();

            if (pair_.second < rhs_size_)
            {
                // SHIFT
                const symbol &symbol_ = p_->_rhs[pair_.second];

                if (symbol_._type == symbol::NON_TERMINAL)
                {
                    for (std::size_t rule_ =
                        nt_locations_[symbol_._id]._first_production;
                        rule_ != npos(); rule_ = grammar_[rule_]._next_lhs)
                    {
                        const size_t_pair new_pair_(rule_, 0);
                        typename size_t_pair_vector::const_iterator i_ =
                            std::find(state_._closure.begin(),
                                state_._closure.end(), new_pair_);

                        if (i_ == state_._closure.end())
                        {
                            state_._closure.push_back(new_pair_);
                        }
                    }
                }
            }
        }
    }

    static std::size_t add_dfa_state(dfa &dfa_, hash_map &hash_map_,
        size_t_pair_vector &basis_)
    {
        size_t_vector &states_ = hash_map_[hash_set(basis_)];
        std::size_t index_ = npos();

        if (!states_.empty())
        {
            for (typename size_t_vector::const_iterator iter_ =
                states_.begin(), end_ = states_.end(); iter_ != end_; ++iter_)
            {
                dfa_state &state_ = dfa_[*iter_];

                if (state_._basis == basis_)
                {
                    index_ = *iter_;
                    break;
                }
            }
        }

        if (states_.empty() || index_ == npos())
        {
            index_ = dfa_.size();
            states_.push_back(index_);
            dfa_.push_back(dfa_state());
            dfa_.back()._basis.swap(basis_);
        }

        return index_;
    }

    static std::size_t hash_set(size_t_pair_vector &vec_)
    {
        std::size_t hash_ = 0;

        for (typename size_t_pair_vector::const_iterator iter_ = vec_.begin(),
            end_ = vec_.end(); iter_ != end_; ++iter_)
        {
            hash_ *= 571;
            hash_ += iter_->first * 37 + iter_->second;
        }

        return hash_;
    }

    static void fill_entry(const rules &rules_,
        const size_t_pair_vector &config_, const string_vector &symbols_,
        typename state_machine::entry &lhs_, const std::size_t id_,
        const typename state_machine::entry &rhs_, std::string *warnings_)
    {
        const grammar &grammar_ = rules_.grammar();
        const token_info_vector &tokens_info_ = rules_.tokens_info();
        const std::size_t terminals_ = tokens_info_.size();
        static const char *actions_ [] =
        { "ERROR", "SHIFT", "REDUCE", "GOTO", "ACCEPT" };
        bool error_ = false;

        if (lhs_._action == error)
        {
            if (lhs_._param == syntax_error)
            {
                // No conflict
                lhs_ = rhs_;
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
                token_info::token;
            std::size_t rhs_prec_ = 0;
            typename token_info::associativity rhs_assoc_ =
                token_info::token;
            const token_info *iter_ = &tokens_info_[id_];

            if (lhs_._action == shift)
            {
                lhs_prec_ = iter_->_precedence;
                lhs_assoc_ = iter_->_associativity;
            }
            else if (lhs_._action == reduce)
            {
                lhs_prec_ = grammar_[lhs_._param]._precedence;
            }

            if (rhs_._action == shift)
            {
                rhs_prec_ = iter_->_precedence;
                rhs_assoc_ = iter_->_associativity;
            }
            else if (rhs_._action == reduce)
            {
                rhs_prec_ = grammar_[rhs_._param]._precedence;
            }

            if (lhs_._action == shift && rhs_._action == reduce)
            {
                if (lhs_prec_ == 0 || rhs_prec_ == 0)
                {
                    // Favour shift (leave rhs as it is).
                    if (warnings_)
                    {
                        std::ostringstream ss_;

                        ss_ << actions_[lhs_._action];
                        dump_action(grammar_, terminals_, config_, symbols_,
                            id_, lhs_, ss_);
                        ss_ << '/' << actions_[rhs_._action];
                        dump_action(grammar_, terminals_, config_, symbols_,
                            id_, rhs_, ss_);
                        ss_ << " conflict.\n";
                        *warnings_ += ss_.str();
                    }
                }
                else if (lhs_prec_ == rhs_prec_)
                {
                    switch (lhs_assoc_)
                    {
                    case token_info::precedence:
                        // Favour shift (leave rhs as it is).
                        if (warnings_)
                        {
                            std::ostringstream ss_;

                            ss_ << actions_[lhs_._action];
                            dump_action(grammar_, terminals_, config_,
                                symbols_, id_, lhs_, ss_);
                            ss_ << '/' << actions_[rhs_._action];
                            dump_action(grammar_, terminals_, config_,
                                symbols_, id_, rhs_, ss_);
                            ss_ << " conflict.\n";
                            *warnings_ += ss_.str();
                        }

                        break;
                    case token_info::nonassoc:
                        lhs_._action = error;
                        lhs_._param = non_associative;
                        break;
                    case token_info::left:
                        lhs_ = rhs_;
                        break;
                    }
                }
                else if (rhs_prec_ > lhs_prec_)
                {
                    lhs_ = rhs_;
                }
            }
            else if (lhs_._action == reduce && rhs_._action == reduce)
            {
                if (lhs_prec_ == 0 || rhs_prec_ == 0 || lhs_prec_ == rhs_prec_)
                {
                    error_ = true;
                }
                else if (rhs_prec_ > lhs_prec_)
                {
                    lhs_ = rhs_;
                }
            }
            else
            {
                error_ = true;
            }
        }

        if (error_ && warnings_)
        {
            std::ostringstream ss_;

            ss_ << actions_[lhs_._action];
            dump_action(grammar_, terminals_, config_, symbols_, id_, lhs_,
                ss_);
            ss_ << '/' << actions_[rhs_._action];
            dump_action(grammar_, terminals_, config_, symbols_, id_, rhs_,
                ss_);
            ss_ << " conflict.\n";
            *warnings_ += ss_.str();
        }
    }

    static void dump_action(const grammar &grammar_,
        const std::size_t terminals_, const size_t_pair_vector &config_,
        const string_vector &symbols_, const std::size_t id_,
        const typename state_machine::entry &entry_, std::ostringstream &ss_)
    {
        if (entry_._action == shift)
        {
            typename size_t_pair_vector::const_iterator iter_ = config_.begin();
            typename size_t_pair_vector::const_iterator end_ = config_.end();

            for (; iter_ != end_; ++iter_)
            {
                const production &production_ = grammar_[iter_->first];

                if (production_._rhs.size() > iter_->second &&
                    production_._rhs[iter_->second]._id == id_)
                {
                    dump_production(production_, iter_->second, terminals_,
                        symbols_, ss_);
                }
            }
        }
        else if (entry_._action == reduce)
        {
            const production &production_ = grammar_[entry_._param];

            dump_production(production_, static_cast<std::size_t>(~0),
                terminals_, symbols_, ss_);
        }
    }

    static void dump_production(const production &production_,
        const std::size_t dot_, const std::size_t terminals_,
        const string_vector &symbols_, std::ostringstream &ss_)
    {
        typename symbol_vector::const_iterator sym_iter_ =
            production_._rhs.begin();
        typename symbol_vector::const_iterator sym_end_ =
            production_._rhs.end();
        std::size_t index_ = 0;

        ss_ << " (";
        narrow(symbols_[terminals_ + production_._lhs].c_str(), ss_);
        ss_ << " -> ";

        if (sym_iter_ != sym_end_)
        {
            const std::size_t id_ = sym_iter_->_type == symbol::TERMINAL ?
                sym_iter_->_id :
                terminals_ + sym_iter_->_id;

            if (index_ == dot_) ss_ << ". ";

            narrow(symbols_[id_].c_str(), ss_);
            ++sym_iter_;
            ++index_;
        }

        for (; sym_iter_ != sym_end_; ++sym_iter_, ++index_)
        {
            const std::size_t id_ = sym_iter_->_type == symbol::TERMINAL ?
                sym_iter_->_id :
                terminals_ + sym_iter_->_id;

            ss_ << ' ';

            if (index_ == dot_) ss_ << ". ";

            narrow(symbols_[id_].c_str(), ss_);
        }

        ss_ << ')';
    }

    static std::size_t npos()
    {
        return static_cast<std::size_t>(~0);
    }
};

typedef basic_generator<rules> generator;
typedef basic_generator<wrules> wgenerator;
}

#endif
