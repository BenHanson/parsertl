// iterator.hpp
// Copyright (c) 2018-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SEARCH_ITERATOR_HPP
#define PARSERTL_SEARCH_ITERATOR_HPP

#include "../../../lexertl/include/lexertl/iterator.hpp"
#include "match_results.hpp"
#include "search.hpp"

namespace parsertl
{
    template<typename lexer_iterator, typename sm_type,
        typename id_type = std::size_t>
    class search_iterator
    {
    public:
        typedef typename lexer_iterator::value_type::iter_type iter_type;
        typedef std::vector<std::vector<std::pair<iter_type, iter_type> > >
            value_type;
        typedef ptrdiff_t difference_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        typedef std::forward_iterator_tag iterator_category;

        search_iterator() :
            _sm(0)
        {
        }

        search_iterator(const lexer_iterator& iter_, const sm_type& sm_) :
            _iter(iter_),
            _sm(&sm_)
        {
            _captures.push_back(std::vector<std::pair
                <iter_type, iter_type> >());
            _captures.back().push_back(std::pair<iter_type, iter_type>
                (iter_->first, iter_->first));
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
            return _sm == rhs_._sm &&
                (_sm == 0 ? true :
                    _captures == rhs_._captures);
        }

        bool operator !=(const search_iterator& rhs_) const
        {
            return !(*this == rhs_);
        }

    private:
        lexer_iterator _iter;
        value_type _captures;
        const sm_type* _sm;

        void lookup()
        {
            lexer_iterator end;

            _captures.clear();

            if (search(_iter, end, *_sm, _captures))
            {
                _iter = end;
            }
            else
            {
                _sm = 0;
            }
        }
    };

    typedef search_iterator<lexertl::siterator, state_machine>
        ssearch_iterator;
    typedef search_iterator<lexertl::citerator, state_machine>
        csearch_iterator;
    typedef search_iterator<lexertl::wsiterator, state_machine>
        wssearch_iterator;
    typedef search_iterator<lexertl::wciterator, state_machine>
        wcsearch_iterator;
}

#endif
