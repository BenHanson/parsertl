// nt_info.hpp
// Copyright (c) 2016 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_NT_INFO_HPP
#define PARSERTL_NT_INFO_HPP

#include <vector>

namespace parsertl
{
typedef std::vector<char> char_vector;

struct nt_info
{
    bool _nullable;
    char_vector _first_set;
    char_vector _follow_set;

    nt_info(const std::size_t terminals_) :
        _nullable(false),
        _first_set(terminals_, 0),
        _follow_set(terminals_, 0)
    {
    }
};

typedef std::vector<nt_info> nt_info_vector;
}

#endif
