// rules.hpp
// Copyright (c) 2014 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_RULES_HPP
#define PARSERTL_RULES_HPP

#include "../../lexertl/lexertl/generator.hpp"
#include "../../lexertl/lexertl/iterator.hpp"
#include "narrow.hpp"
#include "runtime_error.hpp"

namespace parsertl
{
template<typename T>
class basic_rules
{
public:
    typedef T char_type;
    typedef std::basic_string<char_type> string;

    struct symbol
    {
        enum type {TERMINAL, NON_TERMINAL};

        type _type;
        string _name;

        symbol(const type type_, const string &name_) :
            _type(type_),
            _name(name_)
        {
        }

        bool operator<(const symbol &rhs_) const
        {
            return _type < rhs_._type ||
                _type == rhs_._type && _name < rhs_._name;
        }

        bool operator==(const symbol &rhs_) const
        {
            return _type == rhs_._type && _name == rhs_._name;
        }
    };

    typedef std::deque<symbol> symbol_deque;

    struct non_terminal
    {
        std::size_t _first_production;
        std::size_t _last_production;

        non_terminal() :
            _first_production(~0),
            _last_production(~0)
        {
        }
    };

    typedef std::map<string, non_terminal> nt_map;
    typedef std::pair<string, non_terminal> nt_pair;

    struct production
    {
        string _lhs;
        std::size_t _precedence;
        string _prec_name;
        symbol_deque _rhs;
        std::size_t _index;
        std::size_t _next_lhs;

        production(const std::size_t index_) :
            _precedence(0),
            _index(index_),
            _next_lhs(~0)
        {
        }

        void clear()
        {
            _lhs.clear();
            _precedence = 0;
            _prec_name.clear();
            _rhs.clear();
            _index = ~0;
            _next_lhs = ~0;
        }
    };

    typedef std::deque<production> production_deque;

    struct token_info
    {
        enum associativity {precedence, nonassoc, left, right};
        std::size_t _id;
        std::size_t _precedence;
        associativity _associativity;

        token_info(const std::size_t id_, const std::size_t precedence_,
            const associativity associativity_) :
            _id(id_),
            _precedence(precedence_),
            _associativity(associativity_)
        {
        }
    };

    typedef std::map<string, token_info> terminal_map;
    typedef std::map<std::size_t, string> symbol_map;
    typedef std::pair<std::size_t, string> symbol_pair;

    basic_rules() :
        _next_precedence(1)
    {
        lexer_rules rules_;

        rules_.push("'(\\\\([^0-9cx]|[0-9]{1,3}|c[@a-zA-Z]|x\\d+)|[^'])'",
            LITERAL);
        rules_.push("[A-Za-z_.][-A-Za-z_.0-9]*", SYMBOL);
        rules_.push("\\s+", rules_.skip());
        lexer_generator::build(rules_, _token_lexer);

        rules_.push_state("CODE");
        rules_.push_state("EMPTY");
        rules_.push_state("PREC");

        rules_.push("INITIAL,CODE", "[{]", ">CODE");
        rules_.push("CODE", "'(\\\\.|[^'])*'", ".");
        rules_.push("CODE", "[\"](\\\\.|[^\"])*[\"]", ".");
        rules_.push("CODE", "<%", ">CODE");
        rules_.push("CODE", "%>", "<");
        rules_.push("CODE", "[^}]", ".");
        rules_.push("CODE", "[}]", rules_.skip(), "<");

        rules_.push("INITIAL", "%empty", rules_.skip(), "EMPTY");
        rules_.push("INITIAL", "%prec", rules_.skip(), "PREC");
        rules_.push("PREC", "[A-Za-z_.][-A-Za-z_.0-9]*", PREC, "INITIAL");
        rules_.push("INITIAL,EMPTY", "[|]", OR, "INITIAL");
        rules_.push("INITIAL,CODE,EMPTY,PREC", "[/][*](.|\n)*?[*][/]|[/][/].*",
            rules_.skip(), ".");
        rules_.push("EMPTY,PREC", "\\s+", rules_.skip(), ".");
        lexer_generator::build(rules_, _rule_lexer);
        _terminals.insert(string_tinfo_pair(string(1, '$'),
            token_info(0, 0, token_info::nonassoc)));
    }

    void clear()
    {
        _terminals.clear();
        _terminals.insert(string_tinfo_pair(string(1, '$'),
            token_info(0, 0, token_info::nonassoc)));
        _non_terminals.clear();
        _grammar.clear();
        _start.clear();
        _next_precedence = 1;
    }

    void token(const char_type *names_)
    {
        token(names_, 0, token_info::nonassoc, "token");
    }

    void left(const char_type *names_)
    {
        token(names_, _next_precedence, token_info::left, "left");
        ++_next_precedence;
    }

    void right(const char_type *names_)
    {
        token(names_, _next_precedence, token_info::right, "right");
        ++_next_precedence;
    }

    void nonassoc(const char_type *names_)
    {
        token(names_, _next_precedence, token_info::nonassoc, "nonassoc");
        ++_next_precedence;
    }

    void precedence(const char_type *names_)
    {
        token(names_, _next_precedence, token_info::precedence, "precedence");
        ++_next_precedence;
    }

    std::size_t token_id(const char_type *name_) const
    {
        typename terminal_map::const_iterator iter_ = _terminals.find(name_);

        if (iter_ == _terminals.end())
        {
            std::ostringstream ss_;

            ss_ << "Unknown token '";
            narrow(name_, ss_);
            ss_ << "'.";
            throw runtime_error(ss_.str());
        }

        return iter_->second._id;
    }

    std::size_t push(const char_type *lhs_, const char_type *rhs_)
    {
        const string lhs_str_ = lhs_;

        validate(lhs_);

        if (_terminals.find(lhs_str_) != _terminals.end())
        {
            std::ostringstream ss_;

            ss_ << "Rule ";
            narrow(lhs_, ss_);
            ss_ << " is already defined as a TERMINAL.";
            throw runtime_error(ss_.str());
        }

        push_production(lhs_str_, rhs_);
        return _grammar.size() - 1;
    }

    void start(const char_type *start_)
    {
        validate(start_);
        _start = start_;
    }

    const string &start() const
    {
        return _start;
    }

    void validate()
    {
        if (_grammar.empty())
        {
            throw runtime_error("No productions are defined.");
        }

        if (_start.empty())
        {
            _start = _grammar[0]._lhs;
        }

        // Validate start rule
        typename nt_map::const_iterator nt_iter_ = _non_terminals.find(_start);
        typename nt_map::const_iterator nt_end_ = _non_terminals.end();

        if (nt_iter_ == nt_end_)
        {
            throw runtime_error("Specified start rule does not exist.");
        }

        if (nt_iter_->second._first_production !=
            nt_iter_->second._last_production ||
            // Check for %directives?
            _grammar[nt_iter_->second._first_production]._rhs.size() != 1)
        {
            static char_type accept_[] =
                {'$', 'a', 'c', 'c', 'e', 'p', 't', 0};

            push_production(accept_, _start);
            _start = accept_;
        }
        else
        {
            typename production_deque::const_iterator iter_ = _grammar.begin();
            typename production_deque::const_iterator end_ = _grammar.end();

            for (; iter_ != end_; ++iter_)
            {
                typename symbol_deque::const_iterator sym_iter_ =
                    iter_->_rhs.begin();
                typename symbol_deque::const_iterator sym_end_ =
                    iter_->_rhs.end();

                for (; sym_iter_ != sym_end_; ++sym_iter_)
                {
                    if (sym_iter_->_name == _start)
                    {
                        std::ostringstream ss_;
                        const char_type *name_ = iter_->_lhs.c_str();

                        ss_ << "The start symbol occurs on the RHS of rule '";
                        narrow(name_, ss_);
                        ss_ << "'.";
                        throw runtime_error(ss_.str());
                    }
                }
            }
        }

        // Validate all non-terminals.
        nt_iter_ = _non_terminals.begin();

        for (; nt_iter_ != nt_end_; ++nt_iter_)
        {
            if (nt_iter_->second._first_production == npos())
            {
                std::ostringstream ss_;
                const char_type *name_ = nt_iter_->first.c_str();

                ss_ << "Non-terminal '";
                narrow(name_, ss_);
                ss_ << "' does not have any productions.";
                throw runtime_error(ss_.str());
            }
        }
    }

    const production_deque &grammar() const
    {
        return _grammar;
    }

    const terminal_map &terminals() const
    {
        return _terminals;
    }

    const nt_map &non_terminals() const
    {
        return _non_terminals;
    }

    void symbols(symbol_map &map_) const
    {
        std::set<string> nts_;

        map_.clear();

        for (typename terminal_map::const_iterator iter_ =
            _terminals.begin(), end_ = _terminals.end();
            iter_ != end_; ++iter_)
        {
            map_.insert(symbol_pair(iter_->second._id, iter_->first));
        }

        for (typename production_deque::const_iterator iter_ =
            _grammar.begin(), end_ = _grammar.end(); iter_ != end_; ++iter_)
        {
            if (nts_.find(iter_->_lhs) == nts_.end())
            {
                map_.insert(symbol_pair(map_.size(), iter_->_lhs));
                nts_.insert(iter_->_lhs);
            }
        }
    }

    std::size_t npos() const
    {
        return ~0;
    }

    void copy_terminals(basic_rules &rhs_) const
    {
        rhs_._terminals = _terminals;
        rhs_._next_precedence = _next_precedence;
    }

    void swap(basic_rules &rhs_)
    {
        _terminals.swap(rhs_._terminals);
        _non_terminals.swap(rhs_._non_terminals);
        _grammar.swap(rhs_._grammar);
        _start.swap(rhs_._start);
        std::swap(_next_precedence, rhs_._next_precedence);
    }

private:
    typedef typename lexertl::basic_rules<char, char_type> lexer_rules;
    typedef typename lexertl::basic_state_machine<char_type>
        lexer_state_machine;
    typedef typename lexertl::basic_generator<lexer_rules, lexer_state_machine>
        lexer_generator;
    typedef typename lexertl::iterator<const char_type *, lexer_state_machine,
        typename lexertl::recursive_match_results<const char_type *> >
        lexer_iterator;
    typedef std::pair<string, token_info> string_tinfo_pair;

    enum type {LITERAL = 1, SYMBOL, PREC, OR};

    lexer_state_machine _rule_lexer;
    lexer_state_machine _token_lexer;
    terminal_map _terminals;
    nt_map _non_terminals;
    production_deque _grammar;
    string _start;
    std::size_t _next_precedence;

    void validate(const char_type *name_,
        const char_type *end_ = 0) const
    {
        const char_type *start_ = name_;

        if (!(*name_ >= 'A' && *name_ <= 'Z') &&
            !(*name_ >= 'a' && *name_ <= 'z') &&
            *name_ != '_' && *name_ != '.')
        {
            std::ostringstream ss_;

            ss_ << "Invalid name '";
            narrow(name_, ss_);
            ss_ << "'.";
            throw runtime_error(ss_.str());
        }
        else if (*name_)
        {
            ++name_;
        }

        while (*name_ && name_ != end_)
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

    void token(const char_type *names_, const std::size_t precedence_,
        const typename token_info::associativity associativity_,
        const char *func_)
    {
        lexer_iterator iter_(names_, str_end(names_),
            _token_lexer);
        lexer_iterator end_;
        string token_;

        for (; iter_ != end_; ++iter_)
        {
            if (iter_->id == _token_lexer.npos())
            {
                std::ostringstream ss_;

                ss_ << "Unrecognised char in " << func_ << "().";
                throw runtime_error(ss_.str());
            }

            token_ = iter_->str();

            typename terminal_map::iterator term_iter_ =
                _terminals.find(token_);

            if (term_iter_ == _terminals.end())
            {
                _terminals.insert
                    (string_tinfo_pair(token_,
                    token_info(_terminals.size(), precedence_,
                    associativity_)));
            }
            else
            {
                // Real yacc/bison scripts sometimes
                // specify terminals more than once...
                term_iter_->second._precedence = precedence_;
                term_iter_->second._associativity = associativity_;
            }
        }
    }

    const char_type *str_end(const char_type *str_)
    {
        while (*str_) ++str_;

        return str_;
    }

    void push_production(const string &lhs_, const string &rhs_)
    {
        typename nt_map::iterator nt_iter_ = _non_terminals.find(lhs_);
        production production_(_grammar.size());
        lexer_iterator iter_(rhs_.c_str(), rhs_.c_str() + rhs_.size(),
            _rule_lexer);
        lexer_iterator end_;

        if (nt_iter_ == _non_terminals.end())
        {
            nt_iter_ = _non_terminals.insert(nt_pair(lhs_,
                non_terminal())).first;
            nt_iter_->second._first_production = production_._index;
            nt_iter_->second._last_production = production_._index;
        }
        else
        {
            if (nt_iter_->second._first_production == npos())
            {
                nt_iter_->second._first_production = production_._index;
            }

            if (nt_iter_->second._last_production != npos())
            {
                _grammar[nt_iter_->second._last_production]._next_lhs =
                    production_._index;
            }

            nt_iter_->second._last_production = production_._index;
        }

        production_._lhs = nt_iter_->first;

        for (; iter_ != end_; ++iter_)
        {
            switch (iter_->id)
            {
                case LITERAL:
                {
                    string token_ = iter_->str();
                    typename terminal_map::const_iterator terminal_iter_ =
                        _terminals.find(token_);

                    if (terminal_iter_ == _terminals.end())
                    {
                        terminal_iter_ = _terminals.insert
                            (string_tinfo_pair (token_, token_info
                            (_terminals.size(), 0,
                            token_info::nonassoc))).first;
                    }

                    production_._precedence =
                        terminal_iter_->second._precedence;
                    production_._rhs.push_back
                        (symbol(symbol::TERMINAL, token_));
                    break;
                }
                case SYMBOL:
                {
                    string token_ = iter_->str();
                    typename terminal_map::const_iterator terminal_iter_ =
                        _terminals.find(token_);

                    if (terminal_iter_ == _terminals.end())
                    {
                        // NON_TERMINAL
                        nt_iter_ = _non_terminals.find(token_);

                        if (nt_iter_ == _non_terminals.end())
                        {
                            nt_iter_ = _non_terminals.insert(nt_pair(token_,
                                non_terminal())).first;
                        }

                        production_._rhs.push_back
                            (symbol(symbol::NON_TERMINAL, token_));
                    }
                    else
                    {
                        production_._precedence =
                            terminal_iter_->second._precedence;
                        production_._rhs.push_back
                            (symbol(symbol::TERMINAL, token_));
                    }

                    break;
                }
                case PREC:
                {
                    string token_ = iter_->str();
                    typename terminal_map::const_iterator term_iter_ =
                        _terminals.find(token_);

                    if (term_iter_ == _terminals.end())
                    {
                        std::ostringstream ss_;

                        ss_ << "Unknown token ";
                        narrow(token_.c_str(), ss_);
                        ss_ << '.';
                        throw runtime_error(ss_.str());
                    }

                    production_._precedence = term_iter_->second._precedence;
                    production_._prec_name = token_;
                    break;
                }
                case OR:
                {
                    string old_lhs_ = production_._lhs;
                    std::size_t index_ = _grammar.size() + 1;
                    typename nt_map::iterator iter_ =
                        _non_terminals.find(old_lhs_);

                    production_._next_lhs =
                        iter_->second._last_production = index_;
                    _grammar.push_back(production_);
                    production_.clear();
                    production_._lhs = old_lhs_;
                    production_._index = index_;
                    break;
                }
                default:
                {
                    std::ostringstream ss_;
                    const char_type *l_ = lhs_.c_str();
                    const char_type *r_ = rhs_.c_str();

                    assert(0);
                    ss_ << "Syntax error in rule '";
                    narrow(l_, ss_);
                    ss_ << "': '";
                    narrow(r_, ss_);
                    ss_ << "'.";
                    throw runtime_error(ss_.str());
                    break;
                }
            }
        }

        _grammar.push_back(production_);
    }
};

typedef basic_rules<char> rules;
typedef basic_rules<wchar_t> wrules;
}

#endif
