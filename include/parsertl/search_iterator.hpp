// iterator.hpp
// Copyright (c) 2018 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SEARCH_ITERATOR_HPP
#define PARSERTL_SEARCH_ITERATOR_HPP

#include "match_results.hpp"
#include "search.hpp"

namespace parsertl
{
    template<typename iter, typename lsm_type, typename gsm_type,
        typename id_type = std::size_t>
        class search_iterator
    {
    public:
        typedef std::vector<std::vector<std::pair<iter, iter> > > results;
        typedef results value_type;
        typedef ptrdiff_t difference_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        typedef std::forward_iterator_tag iterator_category;

        search_iterator() :
            _end(),
            _lsm(0),
            _gsm(0)
        {
        }

        search_iterator(const iter& first_, const iter& second_, const lsm_type& lsm_,
            const gsm_type& gsm_) :
            _end(second_),
            _lsm(&lsm_),
            _gsm(&gsm_)
        {
            _captures.push_back(std::vector<std::pair<iter, iter> >());
            _captures.back().push_back(std::pair<iter, iter>(first_, first_));
            lookup();
        }

        search_iterator& operator ++()
        {
            lookup();
            return *this;
        }

        search_iterator operator ++(int)
        {
            search_iterator iter_ = *this;

            lookup();
            return iter_;
        }

        const value_type& operator *() const
        {
            return _captures;
        }

        const value_type* operator ->() const
        {
            return &_captures;
        }

        bool operator ==(const search_iterator& rhs_) const
        {
            return _lsm == rhs_._lsm && _gsm == rhs_._gsm &&
                (_gsm == 0 ? true :
                    _captures == rhs_._captures);
        }

        bool operator !=(const search_iterator& rhs_) const
        {
            return !(*this == rhs_);
        }

    private:
        iter _end;
        results _captures;
        const lsm_type* _lsm;
        const gsm_type* _gsm;

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
        state_machine> ssearch_iterator;
    typedef iterator<const char*, lexertl::state_machine, state_machine>
        csearch_iterator;
    typedef iterator<std::wstring::const_iterator, lexertl::state_machine,
        state_machine> wssearch_iterator;
    typedef iterator<const wchar_t*, lexertl::state_machine, state_machine>
        wcsearch_iterator;
}

#endif
