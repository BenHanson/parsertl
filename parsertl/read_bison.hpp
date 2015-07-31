// read_bison.hpp
// Copyright (c) 2014 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_READ_BISON_HPP
#define PARSERTL_READ_BISON_HPP

#include "generator.hpp"
#include "parser.hpp"

namespace parsertl
{
void read_bison(const char *start_, const char *end_, rules &rules_)
{
    typedef parser<lexertl::criterator> parser;
    rules grules_;
    parser parser_;
    lexertl::rules lrules_;
    lexertl::state_machine lsm_;

    grules_.token("':' ';' '|' EMPTY LEFT LITERAL NAME NEWLINE NONASSOC "
        "PREC PRECEDENCE RIGHT START TOKEN");
    grules_.push("start", "list");
    grules_.push("list", "directives rules");
    grules_.push("directives", "directives directive "
        "| directive");

    const std::size_t token_index_ =
        grules_.push("directive", "TOKEN tokens NEWLINE");
    const std::size_t left_index_ =
        grules_.push("directive", "LEFT tokens NEWLINE");
    const std::size_t right_index_ =
        grules_.push("directive", "RIGHT tokens NEWLINE");
    const std::size_t nonassoc_index_ = grules_.push("directive",
        "NONASSOC tokens NEWLINE");
    const std::size_t precedence_index_ =
        grules_.push("directive", "PRECEDENCE tokens NEWLINE");
    const std::size_t start_index_ =
        grules_.push("directive", "START NAME NEWLINE");

    grules_.push("directive", "NEWLINE");
    grules_.push("tokens", "tokens name "
        "| name");
    grules_.push("name", "LITERAL | NAME");
    grules_.push("rules", "rules rule "
        "| rule");

    const std::size_t prod_index_ =
        grules_.push("rule", "NAME ':' productions ';'");

    // Meh
    grules_.push("rule", "';'");
    grules_.push("productions", "productions '|' production prec "
        "| production prec");
    grules_.push("production", "EMPTY "
        "| prod_list");
    grules_.push("prod_list",
        "prod_list token "
        "| token "
        "| %empty");
    grules_.push("token", "LITERAL | NAME");
    grules_.push("prec", "%empty | PREC NAME");
    generator::build(grules_, parser_.sm);

    lrules_.push_state("CODE");
    lrules_.push_state("FINISH");
    lrules_.push_state("PRODUCTIONS");
    lrules_.push_state("PREC");

    lrules_.push("%left", grules_.token_id("LEFT"));
    lrules_.push("\n", grules_.token_id("NEWLINE"));
    lrules_.push("%nonassoc", grules_.token_id("NONASSOC"));
    lrules_.push("%precedence", grules_.token_id("PRECEDENCE"));
    lrules_.push("%right", grules_.token_id("RIGHT"));
    lrules_.push("%start", grules_.token_id("START"));
    lrules_.push("%token", grules_.token_id("TOKEN"));
    lrules_.push("%union[^{]*[{](.|\n)*?[}]", lrules_.skip());
    lrules_.push("<[^>]+>", lrules_.skip());
    lrules_.push("%[{](.|\n)*?%[}]", lrules_.skip());
    lrules_.push("[ \t\r]+", lrules_.skip());

    lrules_.push("CODE,PRODUCTIONS", "[{]", ">CODE");
    lrules_.push("CODE", "'(\\\\.|[^'])*'", ".");

    lrules_.push("CODE", "[\"](\\\\.|[^\"])*[\"]", ".");
    lrules_.push("CODE", "<%", ">CODE");
    lrules_.push("CODE", "%>", "<");
    lrules_.push("CODE", "[^}]", ".");
    lrules_.push("CODE", "[}]", lrules_.skip(), "<");

    lrules_.push("INITIAL", "%%", lrules_.skip(), "PRODUCTIONS");
    lrules_.push("PRODUCTIONS", ":", grules_.token_id("':'"), ".");
    lrules_.push("PRODUCTIONS", ";", grules_.token_id("';'"), ".");
    lrules_.push("PRODUCTIONS", "[|]", grules_.token_id("'|'"), "PRODUCTIONS");
    lrules_.push("PRODUCTIONS", "%empty", grules_.token_id("EMPTY"), ".");
    lrules_.push("INITIAL,PRODUCTIONS",
        "'(\\\\([^0-9cx]|[0-9]{1,3}|c[@a-zA-Z]|x\\d+)|[^'])'",
        grules_.token_id("LITERAL"), ".");
    lrules_.push("INITIAL,PRODUCTIONS",
        "[A-Za-z_.][-A-Za-z_.0-9]*", grules_.token_id("NAME"), ".");
    lrules_.push("PRODUCTIONS", "%%", lrules_.skip(), "FINISH");
    lrules_.push("PRODUCTIONS", "%prec", grules_.token_id("PREC"), "PREC");
    lrules_.push("PREC",
        "[A-Za-z_.][-A-Za-z_.0-9]*", grules_.token_id("NAME"), "PRODUCTIONS");
    // Always skip comments
    lrules_.push("CODE,INITIAL,PREC,PRODUCTIONS",
        "[/][*](.|\n)*?[*][/]|[/][/].*", lrules_.skip(), ".");
    // All whitespace in PRODUCTIONS mode is skipped.
    lrules_.push("PREC,PRODUCTIONS", "\\s+", lrules_.skip(), ".");
    lrules_.push("FINISH", "(.|\n)+", lrules_.skip(), "INITIAL");
    lexertl::generator::build(lrules_, lsm_);

    lexertl::criterator iter_(start_, end_, lsm_);
    parser::token_vector productions_;

    parser_.init(iter_);

    while (parser_.entry._action != parsertl::error &&
        parser_.entry._action != parsertl::accept)
    {
        switch (parser_.entry._action)
        {
            case parsertl::error:
                throw std::runtime_error("Syntax error");
                break;
            case parsertl::reduce:
                if (parser_.entry._param == token_index_)
                {
                    const parser::token &token_ =
                        parser_.dollar(1, productions_);
                    const std::string str_(token_.start, token_.end);

                    rules_.token(str_.c_str());
                }
                else if (parser_.entry._param == left_index_)
                {
                    const parser::token &token_ =
                        parser_.dollar(1, productions_);
                    const std::string str_(token_.start, token_.end);

                    rules_.left(str_.c_str());
                }
                else if (parser_.entry._param == right_index_)
                {
                    const parser::token &token_ =
                        parser_.dollar(1, productions_);
                    const std::string str_(token_.start, token_.end);

                    rules_.right(str_.c_str());
                }
                else if (parser_.entry._param == nonassoc_index_)
                {
                    const parser::token &token_ =
                        parser_.dollar(1, productions_);
                    const std::string str_(token_.start, token_.end);

                    rules_.nonassoc(str_.c_str());
                }
                else if (parser_.entry._param == precedence_index_)
                {
                    const parser::token &token_ =
                        parser_.dollar(1, productions_);
                    const std::string str_(token_.start, token_.end);

                    rules_.precedence(str_.c_str());
                }
                else if (parser_.entry._param == start_index_)
                {
                    const parser::token &name_ =
                        parser_.dollar(1, productions_);

                    rules_.start(std::string(name_.start, name_.end).c_str());
                }
                else if (parser_.entry._param == prod_index_)
                {
                    const parser::token &lhs_ = parser_.dollar(0, productions_);
                    const parser::token &rhs_ = parser_.dollar(2, productions_);
                    const std::string lhs_str_(lhs_.start, lhs_.end);
                    const std::string rhs_str_(rhs_.start, rhs_.end);

                    rules_.push(lhs_str_.c_str(), rhs_str_.c_str());
                }

                break;
        }

        parser_.next(iter_, productions_);
    }
}
}

#endif
