// rules.hpp
// Copyright (c) 2014-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_RULES_HPP
#define PARSERTL_RULES_HPP

#include "bison_lookup.hpp"
#include "ebnf_tables.hpp"
#include "enums.hpp"
#include <lexertl/generator.hpp>
#include <lexertl/iterator.hpp>
#include "match_results.hpp"
#include "narrow.hpp"
#include "runtime_error.hpp"
#include "token.hpp"

namespace parsertl
{
    template<typename T, typename id_type = std::size_t>
    class basic_rules
    {
    public:
        typedef T char_type;

        struct nt_location
        {
            std::size_t _first_production;
            std::size_t _last_production;

            nt_location() :
                _first_production(static_cast<std::size_t>(~0)),
                _last_production(static_cast<std::size_t>(~0))
            {
            }
        };

        typedef std::vector<std::pair<id_type, id_type> > capture_vector;
        typedef std::deque<std::pair<std::size_t, capture_vector> >
            captures_deque;
        typedef std::vector<nt_location> nt_location_vector;
        typedef std::basic_string<char_type> string;
        typedef std::map<string, id_type> string_id_type_map;
        typedef std::pair<string, id_type> string_id_type_pair;
        typedef std::vector<string> string_vector;

        struct symbol
        {
            enum type { TERMINAL, NON_TERMINAL };

            type _type;
            std::size_t _id;

            symbol(const type type_, const std::size_t id_) :
                _type(type_),
                _id(id_)
            {
            }

            bool operator<(const symbol& rhs_) const
            {
                return _type < rhs_._type ||
                    (_type == rhs_._type && _id < rhs_._id);
            }

            bool operator==(const symbol& rhs_) const
            {
                return _type == rhs_._type && _id == rhs_._id;
            }
        };

        typedef std::vector<symbol> symbol_vector;
        enum associativity
        {
            token_assoc, precedence_assoc, non_assoc, left_assoc, right_assoc
        };

        struct production
        {
            std::size_t _lhs;

            struct rhs
            {
                symbol_vector _symbols;
                string _prec;

                bool operator==(const rhs& rhs_) const
                {
                    return _symbols == rhs_._symbols &&
                        _prec == rhs_._prec;
                }

                bool operator<(const rhs& rhs_) const
                {
                    return _symbols < rhs_._symbols ||
                        (_symbols == rhs_._symbols && _prec < rhs_._prec);
                }
            };

            rhs _rhs;
            std::size_t _precedence;
            associativity _associativity;
            std::size_t _index;
            std::size_t _next_lhs;

            production(const std::size_t index_) :
                _lhs(static_cast<std::size_t>(~0)),
                _precedence(0),
                _associativity(token_assoc),
                _index(index_),
                _next_lhs(static_cast<std::size_t>(~0))
            {
            }

            void clear()
            {
                _lhs = static_cast<std::size_t>(~0);
                _rhs._symbols.clear();
                _rhs._prec.clear();
                _precedence = 0;
                _associativity = token_assoc;
                _index = static_cast<std::size_t>(~0);
                _next_lhs = static_cast<std::size_t>(~0);
            }
        };

        typedef std::deque<production> production_deque;

        struct token_info
        {
            std::size_t _precedence;
            associativity _associativity;

            token_info() :
                _precedence(0),
                _associativity(token_assoc)
            {
            }

            token_info(const std::size_t precedence_,
                const associativity associativity_) :
                _precedence(precedence_),
                _associativity(associativity_)
            {
            }
        };

        typedef std::vector<token_info> token_info_vector;

        basic_rules(const std::size_t flags_ = 0) :
            _flags(flags_),
            _next_precedence(1)
        {
            lexer_rules rules_;

            rules_.insert_macro("TERMINAL",
                "'(\\\\([^0-9cx]|[0-9]{1,3}|c[@a-zA-Z]|x\\d+)|[^\\\\\r\n'])+'|"
                "[\"](\\\\([^0-9cx]|[0-9]{1,3}|c[@a-zA-Z]|x\\d+)|"
                "[^\\\\\r\n\"])+[\"]");
            rules_.insert_macro("IDENTIFIER", "[A-Za-z_.][-A-Za-z_.0-9]*");
            rules_.push("{TERMINAL}", ebnf_tables::TERMINAL);
            rules_.push("{IDENTIFIER}", ebnf_tables::IDENTIFIER);
            rules_.push("\\s+", rules_.skip());
            lexer_generator::build(rules_, _token_lexer);

            rules_.push("[|]", '|');
            rules_.push("\\[", '[');
            rules_.push("\\]", ']');
            rules_.push("[?]", '?');
            rules_.push("[{]", '{');
            rules_.push("[}]", '}');
            rules_.push("[*]", '*');
            rules_.push("-", '-');
            rules_.push("[+]", '+');
            rules_.push("[(]", '(');
            rules_.push("[)]", ')');
            rules_.push("%empty", ebnf_tables::EMPTY);
            rules_.push("%prec", ebnf_tables::PREC);
            rules_.push("[/][*].{+}[\r\n]*?[*][/]|[/][/].*", rules_.skip());
            lexer_generator::build(rules_, _rule_lexer);

            const std::size_t id_ = insert_terminal(string(1, '$'));

            info(id_);
        }

        void clear()
        {
            _flags = 0;
            _next_precedence = 1;

            _non_terminals.clear();
            _nt_locations.clear();
            _new_rule_ids.clear();
            _generated_rules.clear();
            _start.clear();
            _grammar.clear();
            _captures.clear();

            _terminals.clear();
            _tokens_info.clear();

            const std::size_t id_ = insert_terminal(string(1, '$'));

            info(id_);
        }

        void flags(const std::size_t flags_)
        {
            _flags = flags_;
        }

        void token(const char_type* names_)
        {
            lexer_iterator iter_(names_, str_end(names_), _token_lexer);

            token(iter_, 0, token_assoc, "token");
        }

        void token(const string& names_)
        {
            lexer_iterator iter_(names_.c_str(), names_.c_str() + names_.size(),
                _token_lexer);

            token(iter_, 0, token_assoc, "token");
        }

        void left(const char_type* names_)
        {
            lexer_iterator iter_(names_, str_end(names_), _token_lexer);

            token(iter_, _next_precedence, left_assoc, "left");
            ++_next_precedence;
        }

        void left(const string& names_)
        {
            lexer_iterator iter_(names_.c_str(), names_.c_str() + names_.size(),
                _token_lexer);

            token(iter_, _next_precedence, left_assoc, "left");
            ++_next_precedence;
        }

        void right(const char_type* names_)
        {
            lexer_iterator iter_(names_, str_end(names_), _token_lexer);

            token(iter_, _next_precedence, right_assoc, "right");
            ++_next_precedence;
        }

        void right(const string& names_)
        {
            lexer_iterator iter_(names_.c_str(), names_.c_str() + names_.size(),
                _token_lexer);

            token(iter_, _next_precedence, right_assoc, "right");
            ++_next_precedence;
        }

        void nonassoc(const char_type* names_)
        {
            lexer_iterator iter_(names_, str_end(names_), _token_lexer);

            token(iter_, _next_precedence, non_assoc, "nonassoc");
            ++_next_precedence;
        }

        void nonassoc(const string& names_)
        {
            lexer_iterator iter_(names_.c_str(), names_.c_str() + names_.size(),
                _token_lexer);

            token(iter_, _next_precedence, non_assoc, "nonassoc");
            ++_next_precedence;
        }

        void precedence(const char_type* names_)
        {
            lexer_iterator iter_(names_, str_end(names_), _token_lexer);

            token(iter_, _next_precedence, precedence_assoc, "precedence");
            ++_next_precedence;
        }

        void precedence(const string& names_)
        {
            lexer_iterator iter_(names_.c_str(), names_.c_str() + names_.size(),
                _token_lexer);

            token(iter_, _next_precedence, precedence_assoc, "precedence");
            ++_next_precedence;
        }

        id_type push(const string& lhs_, const string& rhs_)
        {
            // Return the first index of any EBNF/rule with ors.
            id_type index_ = static_cast<id_type>(_grammar.size());
            const std::size_t old_size_ = _grammar.size();

            validate(lhs_.c_str());

            if (_terminals.find(lhs_) != _terminals.end())
            {
                std::ostringstream ss_;

                ss_ << "Rule ";
                narrow(lhs_.c_str(), ss_);
                ss_ << " is already defined as a TERMINAL.";
                throw runtime_error(ss_.str());
            }

            if (_generated_rules.find(lhs_) != _generated_rules.end())
            {
                std::ostringstream ss_;

                ss_ << "Rule ";
                narrow(lhs_.c_str(), ss_);
                ss_ << " is already defined as a generated rule.";
                throw runtime_error(ss_.str());
            }

            lexer_iterator iter_(rhs_.c_str(), rhs_.c_str() + rhs_.size(),
                _rule_lexer);
            basic_match_results<basic_state_machine<id_type> > results_;
            // Qualify token to prevent arg dependant lookup
            typedef parsertl::token<lexer_iterator> token_t;
            typename token_t::token_vector productions_;
            std::stack<string> rhs_stack_;
            std::stack<std::pair<string, string> > new_rules_;
            static const char_type empty_or_[] =
            { '%', 'e', 'm', 'p', 't', 'y', ' ', '|', ' ', '\0' };
            static const char_type or_[] = { ' ', '|', ' ', '\0' };

            bison_next(_ebnf_tables, iter_, results_);

            while (results_.entry.action != error &&
                results_.entry.action != accept)
            {
                if (results_.entry.action == reduce)
                {
                    switch (static_cast<ebnf_indexes>(results_.entry.param))
                    {
                    case rhs_or_2_idx:
                    {
                        // rhs_or: rhs_or '|' opt_list
                        const std::size_t size_ =
                            _ebnf_tables.yyr2[results_.entry.param];
                        const std::size_t idx_ = productions_.size() - size_;
                        const token_t& token_ = productions_[idx_ + 1];
                        const string r_ = token_.str() + char_type(' ') +
                            rhs_stack_.top();

                        rhs_stack_.pop();

                        if (rhs_stack_.empty())
                        {
                            rhs_stack_.push(r_);
                        }
                        else
                        {
                            rhs_stack_.top() += char_type(' ') + r_;
                        }

                        break;
                    }
                    case opt_prec_list_idx:
                    {
                        // opt_prec_list: opt_list opt_prec
                        const std::size_t size_ =
                            _ebnf_tables.yyr2[results_.entry.param];
                        const std::size_t idx_ = productions_.size() - size_;
                        const token_t& token_ = productions_[idx_ + 1];

                        // Check if %prec is present
                        if (token_.first != token_.second)
                        {
                            string r_ = rhs_stack_.top();

                            rhs_stack_.pop();
                            rhs_stack_.top() += char_type(' ') + r_;
                        }

                        break;
                    }
                    case opt_list_1_idx:
                        // opt_list: %empty
                        rhs_stack_.push(string());
                        break;
                    case rhs_list_2_idx:
                    {
                        // rhs_list: rhs_list rhs
                        string r_ = rhs_stack_.top();

                        rhs_stack_.pop();
                        rhs_stack_.top() += char_type(' ') + r_;
                        break;
                    }
                    case opt_list_2_idx:
                    case identifier_idx:
                    case terminal_idx:
                    {
                        // opt_list: %empty
                        // rhs: IDENTIFIER
                        // rhs: TERMINAL
                        const std::size_t size_ =
                            _ebnf_tables.yyr2[results_.entry.param];
                        const std::size_t idx_ = productions_.size() - size_;
                        const token_t& token_ = productions_[idx_];

                        rhs_stack_.push(token_.str());
                        break;
                    }
                    case optional_1_idx:
                    case optional_2_idx:
                    {
                        // rhs: '[' rhs_or ']'
                        // rhs: rhs '?'
                        std::size_t& counter_ = _new_rule_ids[lhs_];
                        std::basic_ostringstream<char_type> ss_;
                        std::pair<string, string> pair_;

                        ++counter_;
                        ss_ << counter_;
                        pair_.first = lhs_ + char_type('_') + ss_.str();
                        _generated_rules.insert(pair_.first);
                        pair_.second = empty_or_ + rhs_stack_.top();
                        rhs_stack_.top() = pair_.first;
                        new_rules_.push(pair_);
                        break;
                    }
                    case zom_1_idx:
                    case zom_2_idx:
                    {
                        // rhs: '{' rhs_or '}'
                        // rhs: rhs '*'
                        std::size_t& counter_ = _new_rule_ids[lhs_];
                        std::basic_ostringstream<char_type> ss_;
                        std::pair<string, string> pair_;

                        ++counter_;
                        ss_ << counter_;
                        pair_.first = lhs_ + char_type('_') + ss_.str();
                        _generated_rules.insert(pair_.first);
                        pair_.second = empty_or_ + pair_.first +
                            char_type(' ') + rhs_stack_.top();
                        rhs_stack_.top() = pair_.first;
                        new_rules_.push(pair_);
                        break;
                    }
                    case oom_1_idx:
                    case oom_2_idx:
                    {
                        // rhs: '{' rhs_or '}' '-'
                        // rhs: rhs '+'
                        std::size_t& counter_ = _new_rule_ids[lhs_];
                        std::basic_ostringstream<char_type> ss_;
                        std::pair<string, string> pair_;

                        ++counter_;
                        ss_ << counter_;
                        pair_.first = lhs_ + char_type('_') + ss_.str();
                        _generated_rules.insert(pair_.first);
                        pair_.second = rhs_stack_.top() + or_ +
                            pair_.first + char_type(' ') + rhs_stack_.top();
                        rhs_stack_.top() = pair_.first;
                        new_rules_.push(pair_);
                        break;
                    }
                    case bracketed_idx:
                    {
                        // rhs: '(' rhs_or ')'
                        std::size_t& counter_ = _new_rule_ids[lhs_];
                        std::basic_ostringstream<char_type> ss_;
                        std::pair<string, string> pair_;

                        ++counter_;
                        ss_ << counter_;
                        pair_.first = lhs_ + char_type('_') + ss_.str();
                        _generated_rules.insert(pair_.first);
                        pair_.second = rhs_stack_.top();

                        if (_flags & enable_captures)
                        {
                            rhs_stack_.top() = char_type('(') + pair_.first +
                                char_type(')');
                        }
                        else
                        {
                            rhs_stack_.top() = pair_.first;
                        }

                        new_rules_.push(pair_);
                        break;
                    }
                    case prec_ident_idx:
                    case prec_term_idx:
                    {
                        // opt_prec: PREC IDENTIFIER
                        // opt_prec: PREC TERMINAL
                        const std::size_t size_ =
                            _ebnf_tables.yyr2[results_.entry.param];
                        const std::size_t idx_ = productions_.size() - size_;
                        const token_t& token_ = productions_[idx_];

                        rhs_stack_.push(token_.str() + char_type(' ') +
                            productions_[idx_ + 1].str());
                        break;
                    }
                    }
                }

                bison_lookup(_ebnf_tables, iter_, results_, productions_);
                bison_next(_ebnf_tables, iter_, results_);
            }

            if (results_.entry.action == error)
            {
                std::ostringstream ss_;

                ss_ << "Syntax error in rule ";
                narrow(lhs_.c_str(), ss_);
                ss_ << '(' << iter_->first - rhs_.c_str() + 1 << "): '";
                narrow(rhs_.c_str(), ss_);
                ss_ << "'.";
                throw runtime_error(ss_.str());
            }

            assert(rhs_stack_.size() == 1);
            push_production(lhs_, rhs_stack_.top());

            while (!new_rules_.empty())
            {
                push_production(new_rules_.top().first,
                    new_rules_.top().second);
                new_rules_.pop();
            }

            if (!_captures.empty() && old_size_ != _grammar.size())
            {
                resize_captures();
            }

            return index_;
        }

        id_type token_id(const string& name_) const
        {
            typename string_id_type_map::const_iterator iter_ =
                _terminals.find(name_);

            if (iter_ == _terminals.end())
            {
                std::ostringstream ss_;

                ss_ << "Unknown token \"";
                narrow(name_.c_str(), ss_);
                ss_ << "\".";
                throw runtime_error(ss_.str());
            }

            return iter_->second;
        }

        string name_from_token_id(const std::size_t id_) const
        {
            string name_;

            for (typename string_id_type_map::const_iterator iter_ =
                _terminals.begin(), end_ = _terminals.end();
                iter_ != end_; ++iter_)
            {
                if (iter_->second == id_)
                {
                    name_ = iter_->first;
                    break;
                }
            }

            return name_;
        }

        string name_from_nt_id(const std::size_t id_) const
        {
            string name_;

            for (typename string_id_type_map::const_iterator iter_ =
                _non_terminals.begin(), end_ = _non_terminals.end();
                iter_ != end_; ++iter_)
            {
                if (iter_->second == id_)
                {
                    name_ = iter_->first;
                    break;
                }
            }

            return name_;
        }

        void start(const char_type* start_)
        {
            validate(start_);
            _start = start_;
        }

        void start(const string& start_)
        {
            validate(start_.c_str());
            _start = start_;
        }

        const size_t start() const
        {
            return _start.empty() ?
                npos() :
                _non_terminals.find(_start)->second;
        }

        void validate()
        {
            if (_grammar.empty())
            {
                throw runtime_error("No productions are defined.");
            }

            std::size_t start_ = npos();

            // Determine id of start rule
            if (_start.empty())
            {
                const std::size_t id_ = _grammar[0]._lhs;

                _start = name_from_nt_id(id_);

                if (!_start.empty())
                    start_ = id_;
            }
            else
            {
                typename string_id_type_map::const_iterator iter_ =
                    _non_terminals.find(_start);

                if (iter_ != _non_terminals.end())
                    start_ = iter_->second;
            }

            if (start_ == npos())
            {
                throw runtime_error("Specified start rule does not exist.");
            }

            // Look for unused rules
            for (typename string_id_type_map::const_iterator iter_ =
                _non_terminals.begin(), end_ = _non_terminals.end();
                iter_ != end_; ++iter_)
            {
                bool found_ = false;

                // Skip start_
                if (iter_->second == start_) continue;

                for (typename production_deque::const_iterator iter2_ =
                    _grammar.begin(), end2_ = _grammar.end();
                    !found_ && iter2_ != end2_; ++iter2_)
                {
                    if (iter_->second == iter2_->_lhs)
                        continue;

                    for (typename symbol_vector::const_iterator iter3_ =
                        iter2_->_rhs._symbols.begin(),
                        end3_ = iter2_->_rhs._symbols.end();
                        !found_ && iter3_ != end3_; ++iter3_)
                    {
                        if (iter3_->_type == symbol::NON_TERMINAL &&
                            iter3_->_id == iter_->second)
                        {
                            found_ = true;
                        }
                    }
                }

                if (!found_)
                {
                    std::ostringstream ss_;
                    const string name_ = iter_->first;

                    ss_ << '\'';
                    narrow(name_.c_str(), ss_);
                    ss_ << "' is an unused rule.";
                    throw runtime_error(ss_.str());
                }
            }

            // Validate all non-terminals.
            for (std::size_t i_ = 0, size_ = _nt_locations.size();
                i_ < size_; ++i_)
            {
                if (_nt_locations[i_]._first_production == npos())
                {
                    std::ostringstream ss_;
                    const string name_ = name_from_nt_id(i_);

                    ss_ << "Non-terminal '";
                    narrow(name_.c_str(), ss_);
                    ss_ << "' does not have any productions.";
                    throw runtime_error(ss_.str());
                }
            }

            static const char_type accept_[] =
            {
                '$', 'a', 'c', 'c', 'e', 'p', 't', '\0'
            };

            // Validate start rule
            if (_non_terminals.find(accept_) == _non_terminals.end())
            {
                string rhs_ = _start;

                push_production(accept_, rhs_);
                _grammar.back()._rhs._symbols.push_back(symbol(symbol::TERMINAL,
                    insert_terminal(string(1, '$'))));
            }

            _start = accept_;
        }

        const production_deque& grammar() const
        {
            return _grammar;
        }

        const token_info_vector& tokens_info() const
        {
            return _tokens_info;
        }

        const nt_location_vector& nt_locations() const
        {
            return _nt_locations;
        }

        void terminals(string_vector& vec_) const
        {
            typename string_id_type_map::const_iterator iter_ =
                _terminals.begin();
            typename string_id_type_map::const_iterator end_ =
                _terminals.end();

            vec_.clear();
            vec_.resize(_terminals.size());

            for (; iter_ != end_; ++iter_)
            {
                vec_[iter_->second] = iter_->first;
            }
        }

        std::size_t terminals_count() const
        {
            return _terminals.size();
        }

        void non_terminals(string_vector& vec_) const
        {
            const std::size_t size_ = vec_.size();
            typename string_id_type_map::const_iterator iter_ =
                _non_terminals.begin();
            typename string_id_type_map::const_iterator end_ =
                _non_terminals.end();

            vec_.resize(size_ + _non_terminals.size());

            for (; iter_ != end_; ++iter_)
            {
                vec_[size_ + iter_->second] = iter_->first;
            }
        }

        std::size_t non_terminals_count() const
        {
            return _non_terminals.size();
        }

        void symbols(string_vector& vec_) const
        {
            vec_.clear();
            terminals(vec_);
            non_terminals(vec_);
        }

        const captures_deque& captures() const
        {
            return _captures;
        }

        std::size_t npos() const
        {
            return static_cast<std::size_t>(~0);
        }

    private:
        enum ebnf_indexes
        {
            rule_idx = 2,
            rhs_or_1_idx,
            rhs_or_2_idx,
            opt_prec_list_idx,
            opt_list_1_idx,
            opt_list_2_idx,
            opt_list_3_idx,
            rhs_list_1_idx,
            rhs_list_2_idx,
            identifier_idx,
            terminal_idx,
            optional_1_idx,
            optional_2_idx,
            zom_1_idx,
            zom_2_idx,
            oom_1_idx,
            oom_2_idx,
            bracketed_idx,
            prec_empty_idx,
            prec_ident_idx,
            prec_term_idx
        };

        typedef typename lexertl::basic_rules<char, char_type> lexer_rules;
        typedef typename lexertl::basic_state_machine<char_type>
            lexer_state_machine;
        typedef typename lexertl::basic_generator<lexer_rules,
            lexer_state_machine> lexer_generator;
        typedef typename lexertl::iterator<const char_type*,
            lexer_state_machine, typename lexertl::match_results
            <const char_type*> > lexer_iterator;

        std::size_t _flags;
        ebnf_tables _ebnf_tables;
        std::size_t _next_precedence;
        lexer_state_machine _rule_lexer;
        lexer_state_machine _token_lexer;
        string_id_type_map _terminals;
        token_info_vector _tokens_info;
        string_id_type_map _non_terminals;
        nt_location_vector _nt_locations;
        std::map<string, std::size_t> _new_rule_ids;
        std::set<string> _generated_rules;
        string _start;
        production_deque _grammar;
        captures_deque _captures;

        token_info& info(const std::size_t id_)
        {
            if (_tokens_info.size() <= id_)
            {
                _tokens_info.resize(id_ + 1);
            }

            return _tokens_info[id_];
        }

        void token(lexer_iterator& iter_, const std::size_t precedence_,
            const associativity associativity_, const char* func_)
        {
            lexer_iterator end_;
            string token_;
            std::size_t id_ = static_cast<std::size_t>(~0);

            for (; iter_ != end_; ++iter_)
            {
                if (iter_->id == _token_lexer.npos())
                {
                    std::ostringstream ss_;

                    ss_ << "Unrecognised char in " << func_ << "().";
                    throw runtime_error(ss_.str());
                }

                token_ = iter_->str();

                if (_terminals.find(token_) != _terminals.end())
                    throw runtime_error("token " + token_ +
                        " is already defined.");

                id_ = insert_terminal(token_);

                token_info& token_info_ = info(id_);

                token_info_._precedence = precedence_;
                token_info_._associativity = associativity_;
            }
        }

        void validate(const char_type* name_) const
        {
            const char_type* start_ = name_;

            while (*name_)
            {
                if (!(*name_ >= 'A' && *name_ <= 'Z') &&
                    !(*name_ >= 'a' && *name_ <= 'z') &&
                    *name_ != '_' && *name_ != '.' &&
                    !(*name_ >= '0' && *name_ <= '9') &&
                    *name_ != '-')
                {
                    std::ostringstream ss_;

                    ss_ << "Invalid name '";
                    name_ = start_;
                    narrow(name_, ss_);
                    ss_ << "'.";
                    throw runtime_error(ss_.str());
                }

                ++name_;
            }
        }

        id_type insert_terminal(const string& str_)
        {
            return _terminals.insert(string_id_type_pair(str_,
                static_cast<id_type>(_terminals.size()))).first->second;
        }

        id_type insert_non_terminal(const string& str_)
        {
            return _non_terminals.insert
            (string_id_type_pair(str_,
                static_cast<id_type>(_non_terminals.size()))).first->second;
        }

        const char_type* str_end(const char_type* str_)
        {
            while (*str_) ++str_;

            return str_;
        }

        void push_production(const string& lhs_, const string& rhs_)
        {
            const id_type lhs_id_ = insert_non_terminal(lhs_);
            nt_location& location_ = location(lhs_id_);
            lexer_iterator iter_(rhs_.c_str(), rhs_.c_str() +
                rhs_.size(), _rule_lexer);
            basic_match_results<basic_state_machine<id_type> > results_;
            // Qualify token to prevent arg dependant lookup
            typedef parsertl::token<lexer_iterator> token_t;
            typename token_t::token_vector productions_;
            production production_(_grammar.size());
            int curr_bracket_ = 0;
            std::stack<int> bracket_stack_;

            if (location_._first_production == npos())
            {
                location_._first_production = production_._index;
            }

            if (location_._last_production != npos())
            {
                _grammar[location_._last_production]._next_lhs =
                    production_._index;
            }

            location_._last_production = production_._index;
            production_._lhs = lhs_id_;
            bison_next(_ebnf_tables, iter_, results_);

            while (results_.entry.action != error &&
                results_.entry.action != accept)
            {
                if (results_.entry.action == shift)
                {
                    switch (iter_->id)
                    {
                    case '(':
                        if (_captures.size() <= _grammar.size())
                        {
                            resize_captures();
                            curr_bracket_ = 0;
                        }

                        ++curr_bracket_;
                        bracket_stack_.push(curr_bracket_);
                        _captures.back().second.push_back(std::pair
                            <id_type, id_type>(static_cast<id_type>
                                (production_._rhs._symbols.size()),
                                static_cast<id_type>(0)));
                        break;
                    case ')':
                        _captures.back().second[bracket_stack_.top() - 1].
                            second = static_cast<id_type>(production_.
                                _rhs._symbols.size() - 1);
                        bracket_stack_.pop();
                        break;
                    case '|':
                    {
                        std::size_t old_lhs_ = production_._lhs;
                        std::size_t index_ = _grammar.size() + 1;
                        nt_location& loc_ = location(old_lhs_);

                        production_._next_lhs = loc_._last_production = index_;
                        _grammar.push_back(production_);
                        production_.clear();
                        production_._lhs = old_lhs_;
                        production_._index = index_;
                        break;
                    }
                    }
                }
                else if (results_.entry.action == reduce)
                {
                    switch (static_cast<ebnf_indexes>(results_.entry.param))
                    {
                    case identifier_idx:
                    {
                        // rhs: IDENTIFIER;
                        const std::size_t size_ =
                            _ebnf_tables.yyr2[results_.entry.param];
                        const std::size_t idx_ = productions_.size() - size_;
                        const string token_ = productions_[idx_].str();
                        typename string_id_type_map::const_iterator
                            terminal_iter_ = _terminals.find(token_);

                        if (terminal_iter_ == _terminals.end())
                        {
                            const id_type id_ = insert_non_terminal(token_);

                            // NON_TERMINAL
                            location(id_);
                            production_._rhs._symbols.
                                push_back(symbol(symbol::NON_TERMINAL, id_));
                        }
                        else
                        {
                            const std::size_t id_ = terminal_iter_->second;
                            token_info& token_info_ = info(id_);

                            if (token_info_._precedence)
                            {
                                production_._precedence =
                                    token_info_._precedence;
                                production_._associativity =
                                    token_info_._associativity;
                            }

                            production_._rhs._symbols.
                                push_back(symbol(symbol::TERMINAL, id_));
                        }

                        break;
                    }
                    case terminal_idx:
                    {
                        // rhs: TERMINAL;
                        const std::size_t size_ =
                            _ebnf_tables.yyr2[results_.entry.param];
                        const std::size_t idx_ = productions_.size() - size_;
                        const string token_ = productions_[idx_].str();
                        const std::size_t id_ = insert_terminal(token_);
                        token_info& token_info_ = info(id_);

                        if (token_info_._precedence)
                        {
                            production_._precedence = token_info_._precedence;
                            production_._associativity =
                                token_info_._associativity;
                        }

                        production_._rhs._symbols.push_back(symbol(symbol::
                            TERMINAL, id_));
                        break;
                    }
                    case prec_ident_idx:
                    case prec_term_idx:
                    {
                        // opt_prec: PREC IDENTIFIER;
                        // opt_prec: PREC TERMINAL;
                        const std::size_t size_ =
                            _ebnf_tables.yyr2[results_.entry.param];
                        const std::size_t idx_ = productions_.size() - size_;
                        const string token_ = productions_[idx_ + 1].str();
                        const id_type id_ = token_id(token_);
                        token_info& token_info_ = info(id_);

                        // Explicit %prec, so no conditional
                        production_._precedence = token_info_._precedence;
                        production_._associativity =
                            token_info_._associativity;
                        production_._rhs._prec = token_;
                        break;
                    }
                    }
                }

                bison_lookup(_ebnf_tables, iter_, results_, productions_);
                bison_next(_ebnf_tables, iter_, results_);
            }

            // As rules passed in are generated,
            // they have already been validated.
            assert(results_.entry.action == accept);
            _grammar.push_back(production_);
        }

        void resize_captures()
        {
            const std::size_t old_size_ = _captures.size();

            _captures.resize(_grammar.size() + 1);

            if (old_size_ > 0)
            {
                for (std::size_t i_ = old_size_ - 1,
                    size_ = _captures.size() - 1; i_ < size_; ++i_)
                {
                    _captures[i_ + 1].first = _captures[i_].first +
                        _captures[i_].second.size();
                }
            }
        }

        nt_location& location(const std::size_t id_)
        {
            if (_nt_locations.size() <= id_)
            {
                _nt_locations.resize(id_ + 1);
            }

            return _nt_locations[id_];
        }
    };

    typedef basic_rules<char> rules;
    typedef basic_rules<wchar_t> wrules;
}

#endif
