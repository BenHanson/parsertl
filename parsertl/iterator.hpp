// iterator.hpp
// Copyright (c) 2018 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_ITERATOR_HPP
#define PARSERTL_ITERATOR_HPP

#include "match_results.hpp"
#include "search.hpp"

namespace parsertl
{
template<typename iter, typename lsm_type, typename gsm_type,
    typename id_type = std::size_t>
class iterator
{
public:
    typedef std::vector<std::vector<std::pair<iter, iter> > > results;
    typedef results value_type;
    typedef ptrdiff_t difference_type;
    typedef const value_type *pointer;
    typedef const value_type &reference;
    typedef std::forward_iterator_tag iterator_category;

    iterator() :
        _end(),
        _lsm(0),
        _gsm(0)
    {
    }

    iterator(const iter &start_, const iter &end_, const lsm_type &lsm,
        const gsm_type &gsm) :
        _lsm(&lsm),
        _gsm(&gsm)
    {
        _end = end_;
        _captures.push_back(std::vector<std::pair<iter, iter> >());
        _captures.back().push_back(std::make_pair(start_, start_));
        lookup();
    }

    iterator &operator ++()
    {
        lookup();
        return *this;
    }

    iterator operator ++(int)
    {
        iterator iter_ = *this;

        lookup();
        return iter_;
    }

    const value_type &operator *() const
    {
        return _captures;
    }

    const value_type *operator ->() const
    {
        return &_captures;
    }

    bool operator ==(const iterator &rhs_) const
    {
        return _lsm == rhs_._lsm && _gsm == rhs_._gsm &&
            (_gsm == 0 ? true :
            _captures == rhs_._captures);
    }

    bool operator !=(const iterator &rhs_) const
    {
        return !(*this == rhs_);
    }

private:
    iter _end;
    results _captures;
    const lsm_type *_lsm;
    const gsm_type *_gsm;

    void lookup()
    {
        const std::pair<iter, iter> pair_ = _captures[0].back();

        _captures.clear();

        if (!search(pair_.second, _end, _captures, *_lsm, *_gsm))
        {
            _lsm = 0;
            _gsm = 0;
        }
    }
};

typedef iterator<std::string::const_iterator, lexertl::state_machine,
    state_machine> siterator;
typedef iterator<const char *, lexertl::state_machine, state_machine>
    citerator;
typedef iterator<std::wstring::const_iterator, lexertl::state_machine,
    state_machine> wsiterator;
typedef iterator<const wchar_t *, lexertl::state_machine, state_machine>
    wciterator;
}

#endif
