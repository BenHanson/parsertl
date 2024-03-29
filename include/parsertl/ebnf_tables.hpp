// ebnf_tables.hpp
// Copyright (c) 2018-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_EBNF_TABLES_HPP
#define PARSERTL_EBNF_TABLES_HPP

#include <vector>

namespace parsertl
{
    struct ebnf_tables
    {
        enum
        {
            YYFINAL = 16,
            YYLAST = 32,
            YYNTOKENS = 18,
            YYPACT_NINF = -4,
            YYTABLE_NINF = -1
        };

        enum yytokentype
        {
            EMPTY = 258,
            IDENTIFIER = 259,
            PREC = 260,
            TERMINAL = 261
        };

        std::vector<unsigned char> yytranslate;
        std::vector<unsigned char> yyr1;
        std::vector<unsigned char> yyr2;
        std::vector<unsigned char> yydefact;
        std::vector<char> yydefgoto;
        std::vector<char> yypact;
        std::vector<char> yypgoto;
        std::vector<unsigned char> yytable;
        std::vector<char> yycheck;

        ebnf_tables()
        {
            const unsigned char translate[] =
            {
                   0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                  16,    17,    13,    15,     2,    14,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,    10,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     8,     2,     9,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,    11,     7,    12,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                   2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
                   5,     6
            };
            const unsigned char r1[] =
            {
                   0,    18,    19,    20,    20,    21,    22,    22,    22,    23,
                  23,    24,    24,    24,    24,    24,    24,    24,    24,    24,
                  25,    25,    25
            };
            const unsigned char r2[] =
            {
                   0,     2,     1,     1,     3,     2,     0,     1,     1,     1,
                   2,     1,     1,     3,     2,     3,     2,     4,     2,     3,
                   0,     2,     2
            };
            const unsigned char defact[] =
            {
                   6,     7,    11,    12,     6,     6,     6,     0,     2,     3,
                  20,     8,     9,     0,     0,     0,     1,     6,     0,     5,
                  10,    14,    16,    18,    13,    15,    19,     4,    21,    22,
                  17
            };
            const char defgoto[] =
            {
                  -1,     7,     8,     9,    10,    11,    12,    19
            };
            const char pact[] =
            {
                  -3,    -4,    -4,    -4,    -3,    -3,    -3,    19,    18,    -4,
                  22,    -2,     5,     3,     4,     0,    -4,    -3,    20,    -4,
                   5,    -4,    -4,    -4,    -4,    14,    -4,    -4,    -4,    -4,
                  -4
            };
            const char pgoto[] =
            {
                  -4,    -4,    17,    12,    -4,    -4,    21,    -4
            };
            const unsigned char table[] =
            {
                   1,     2,     2,     3,     3,     4,     4,    17,     5,     5,
                  17,    17,    24,     6,     6,    21,    25,    26,    22,    16,
                  23,    13,    14,    15,    28,    17,    29,    18,    30,    27,
                   0,     0,    20
            };
            const char check[] =
            {
                   3,     4,     4,     6,     6,     8,     8,     7,    11,    11,
                   7,     7,     9,    16,    16,    10,    12,    17,    13,     0,
                  15,     4,     5,     6,     4,     7,     6,     5,    14,    17,
                  -1,    -1,    11
            };

            yytranslate.assign(translate, translate + sizeof(translate) /
                sizeof(translate[0]));
            yyr1.assign(r1, r1 + sizeof(r1) / sizeof(r1[0]));
            yyr2.assign(r2, r2 + sizeof(r2) / sizeof(r2[0]));
            yydefact.assign(defact, defact + sizeof(defact) /
                sizeof(defact[0]));
            yydefgoto.assign(defgoto, defgoto + sizeof(defgoto) +
                sizeof(defgoto[0]));
            yypact.assign(pact, pact + sizeof(pact) / sizeof(pact[0]));
            yypgoto.assign(pgoto, pgoto + sizeof(pgoto) + sizeof(pgoto[0]));
            yytable.assign(table, table + sizeof(table) + sizeof(table[0]));
            yycheck.assign(check, check + sizeof(check) / sizeof(check[0]));
        }
    };
}

#endif
