-*- C -*-

b4_percent_define_default([[api.push_pull]], [[pull]])
b4_percent_define_check_values([[[[api.push_pull]],
                               [[pull]], [[push]], [[both]]]])
b4_define_flag_if([pull]) m4_define([b4_pull_flag], [[1]])
b4_define_flag_if([push]) m4_define([b4_push_flag], [[1]])
#m4_case(b4_percent_define_get([[api.push_pull]]),
#        [pull], [m4_define([b4_push_flag], [[0]])],
#        [push], [m4_define([b4_pull_flag], [[0]])])

# Handle BISON_USE_PUSH_FOR_PULL for the test suite.  So that push parsing
# tests function as written, don't let BISON_USE_PUSH_FOR_PULL modify Bison's
# behavior at all when push parsing is already requested.
b4_define_flag_if([use_push_for_pull])
b4_use_push_for_pull_if([
  b4_push_if([m4_define([b4_use_push_for_pull_flag], [[0]])],
             [m4_define([b4_push_flag], [[1]])])])

m4_include(b4_pkgdatadir/[c.m4])

## ---------------- ##
## Default values.  ##
## ---------------- ##

# Stack parameters.
m4_define_default([b4_stack_depth_max], [10000])
m4_define_default([b4_stack_depth_init],  [200])


## ------------------------ ##
## Pure/impure interfaces.  ##
## ------------------------ ##

b4_percent_define_default([[api.pure]], [[false]])
b4_define_flag_if([pure])
m4_define([b4_pure_flag],
          [b4_percent_define_flag_if([[api.pure]], [[1]], [[0]])])

# b4_yacc_pure_if(IF-TRUE, IF-FALSE)
# ----------------------------------
# Expand IF-TRUE, if %pure-parser and %parse-param, IF-FALSE otherwise.
m4_define([b4_yacc_pure_if],
[b4_pure_if([m4_ifset([b4_parse_param],
              [$1], [$2])],
        [$2])])


# b4_yyerror_args
# ---------------
# Arguments passed to yyerror: user args plus yylloc.
m4_define([b4_yyerror_args],
[b4_yacc_pure_if([b4_locations_if([&yylloc, ])])dnl
m4_ifset([b4_parse_param], [b4_c_args(b4_parse_param), ])])


# b4_lex_param
# ------------
# Accumulate in b4_lex_param all the yylex arguments.
# b4_lex_param arrives quoted twice, but we want to keep only one level.
m4_define([b4_lex_param],
m4_dquote(b4_pure_if([[[[YYSTYPE *]], [[&yylval]]][]dnl
b4_locations_if([, [[YYLTYPE *], [&yylloc]]])m4_ifdef([b4_lex_param], [, ])])dnl
m4_ifdef([b4_lex_param], b4_lex_param)))


## ------------ ##
## Data Types.  ##
## ------------ ##

# b4_int_type(MIN, MAX)
# ---------------------
# Return the smallest int type able to handle numbers ranging from
# MIN to MAX (included).  Overwrite the version from c.m4, which
# uses only C89 types, so that the user can override the shorter
# types, and so that pre-C89 compilers are handled correctly.
m4_define([b4_int_type],
[m4_if(b4_ints_in($@,      [0],   [255]), [1], [yytype_uint8],
       b4_ints_in($@,   [-128],   [127]), [1], [yytype_int8],

       b4_ints_in($@,      [0], [65535]), [1], [yytype_uint16],
       b4_ints_in($@, [-32768], [32767]), [1], [yytype_int16],

       m4_eval([0 <= $1]),                [1], [unsigned int],

                           [int])])

# We do want M4 expansion after # for CPP macros.
m4_changecom()
m4_divert_push(0)dnl
@output(b4_parser_file_name@)
#pragma once

#include <vector>

struct tables
{
    using yytype_uint8 = unsigned char;
    using yytype_int8 = signed char;
    using yytype_uint16 = unsigned short int;
    using yytype_int16 = short int;

    enum
    {
        YYFINAL = b4_final_state_number,
        YYLAST = b4_last,
        YYNTOKENS = b4_tokens_number,
        YYPACT_NINF = b4_pact_ninf,
        YYTABLE_NINF = b4_table_ninf
    };
m4_define([b4_token_enums],
[    enum yytokentype
    {
m4_map_sep([        b4_token_enum], [,
],
       [$@])
    };
])
b4_token_enums(b4_tokens)
    static const b4_int_type_for([b4_translate]) translate[[]];
    static const b4_int_type_for([b4_r1]) r1[[]];
    static const b4_int_type_for([b4_r2]) r2[[]];
    static const b4_int_type_for([b4_defact]) defact[[]];
    static const b4_int_type_for([b4_defgoto]) defgoto[[]];
    static const b4_int_type_for([b4_pact]) pact[[]];
    static const b4_int_type_for([b4_pgoto]) pgoto[[]];
    static const b4_int_type_for([b4_table]) table[[]];
    static const b4_int_type_for([b4_check]) check[[]];
};

const tables::b4_int_type_for([b4_translate]) tables::translate[[]] =
{
  b4_translate
};

const tables::b4_int_type_for([b4_r1]) tables::r1[[]] =
{
  b4_r1
};

const tables::b4_int_type_for([b4_r2]) tables::r2[[]] =
{
  b4_r2
};

const tables::b4_int_type_for([b4_defact]) tables::defact[[]] =
{
  b4_defact
};

const tables::b4_int_type_for([b4_defgoto]) tables::defgoto[[]] =
{
  b4_defgoto
};

const tables::b4_int_type_for([b4_pact]) tables::pact[[]] =
{
  b4_pact
};

const tables::b4_int_type_for([b4_pgoto]) tables::pgoto[[]] =
{
  b4_pgoto
};

const tables::b4_int_type_for([b4_table]) tables::table[[]] =
{
  b4_table
};

const tables::b4_int_type_for([b4_check]) tables::check[[]] =
{
  b4_check
};

m4_divert_pop(0)