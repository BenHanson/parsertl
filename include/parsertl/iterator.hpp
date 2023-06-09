// iterator.hpp
// Copyright (c) 2022-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_ITERATOR_HPP
#define PARSERTL_ITERATOR_HPP

#include "../../../lexertl/include/lexertl/iterator.hpp"
#include "lookup.hpp"
#include "match_results.hpp"
#include "token.hpp"

namespace parsertl
{
    template<typename lexer_iterator, typename sm_type,
        typename id_type = std::size_t>
        class iterator
    {
    public:
        typedef basic_match_results<sm_type> results;
        typedef results value_type;
        typedef ptrdiff_t difference_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        typedef std::forward_iterator_tag iterator_category;

        // Qualify token to prevent arg dependant lookup
        typedef parsertl::token<lexer_iterator> token;
        typedef typename token::token_vector token_vector;

        iterator() :
            _sm(0)
        {
        }

        iterator(const lexer_iterator& iter_, const sm_type& sm_) :
            _iter(iter_),
            _results(_iter->id, sm_),
            _sm(&sm_)
        {
            // The first action can only ever be reduce
            // if the grammar treats no input as valid.
            if (_results.entry.action != reduce)
                lookup();
        }

        iterator(const lexer_iterator& iter_, const sm_type& sm_,
            const std::size_t reserved_) :
            _iter(iter_),
            _results(_iter->id, sm_, reserved_),
            _productions(reserved_),
            _sm(&sm_)
        {
            // The first action can only ever be reduce
            // if the grammar treats no input as valid.
            if (_results.entry.action != reduce)
                lookup();
        }

        typename token_vector::value_type dollar(const std::size_t index_) const
        {
            return _results.dollar(index_, *_sm, _productions);
        }

        iterator& operator ++()
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

        const value_type& operator *() const
        {
            return _results;
        }

        const value_type* operator ->() const
        {
            return &_results;
        }

        bool operator ==(const iterator& rhs_) const
        {
            return _sm == rhs_._sm &&
                (_sm == 0 ? true :
                    _results == rhs_._results);
        }

        bool operator !=(const iterator& rhs_) const
        {
            return !(*this == rhs_);
        }

    private:
        lexer_iterator _iter;
        basic_match_results<sm_type> _results;
        token_vector _productions;
        const sm_type* _sm;

        void lookup()
        {
            // do while because we need to move past the current reduce action
            do
            {
                parsertl::lookup(_iter, *_sm, _results, _productions);
            } while (_results.entry.action == shift ||
                _results.entry.action == go_to);

            switch (_results.entry.action)
            {
            case accept:
            case error:
                _sm = 0;
                break;
            default:
                break;
            }
        }
    };

    typedef iterator<lexertl::siterator, state_machine> siterator;
    typedef iterator<lexertl::citerator, state_machine> citerator;
    typedef iterator<lexertl::wsiterator, state_machine> wsiterator;
    typedef iterator<lexertl::wciterator, state_machine> wciterator;
}

#endif
