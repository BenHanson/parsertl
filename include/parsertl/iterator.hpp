// iterator.hpp
// Copyright (c) 2022 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_ITERATOR_HPP
#define PARSERTL_ITERATOR_HPP

#include "lookup.hpp"
#include "match_results.hpp"
#include "token.hpp"

namespace parsertl
{
    template<typename iter, typename lsm_type, typename gsm_type,
        typename id_type = std::size_t>
        class iterator
    {
    public:
        typedef basic_match_results<gsm_type> results;
        typedef results value_type;
        typedef ptrdiff_t difference_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        typedef std::forward_iterator_tag iterator_category;

        typedef lexertl::iterator<iter, lsm_type,
            lexertl::match_results<iter> > lex_iterator;
        // Qualify token to prevent arg dependant lookup
        typedef parsertl::token<lex_iterator> token;
        typedef typename token::token_vector token_vector;

        iterator() :
            _gsm(0)
        {
        }

        iterator(const iter& first_, const iter& second_,
            const lsm_type& lsm_, const gsm_type& gsm_) :
            _iter(first_, second_, lsm_),
            _results(_iter->id, gsm_),
            _gsm(&gsm_)
        {
            // The first action can only ever be reduce
            // if the grammar treats no input as valid.
            if (_results.entry.action != reduce)
                lookup();
        }

        typename token_vector::value_type dollar(const std::size_t index_) const
        {
            return _results.dollar(*_gsm, index_, _productions);
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
            return _gsm == rhs_._gsm &&
                (_gsm == 0 ? true :
                    _results == rhs_._results);
        }

        bool operator !=(const iterator& rhs_) const
        {
            return !(*this == rhs_);
        }

    private:
        lex_iterator _iter;
        basic_match_results<gsm_type> _results;
        token_vector _productions;
        const gsm_type* _gsm;

        void lookup()
        {
            // do while because we need to move past the current reduce action
            do
            {
                parsertl::lookup(*_gsm, _iter, _results, _productions);
            } while (_results.entry.action == shift ||
                _results.entry.action == go_to);

            switch (_results.entry.action)
            {
            case accept:
            case error:
                _gsm = 0;
                break;
            }
        }
    };

    typedef iterator<std::string::const_iterator, lexertl::state_machine,
        state_machine> siterator;
    typedef iterator<const char*, lexertl::state_machine,
        state_machine> citerator;
    typedef iterator<std::wstring::const_iterator, lexertl::wstate_machine,
        state_machine> wsiterator;
    typedef iterator<const wchar_t*, lexertl::wstate_machine,
        state_machine> wciterator;
}

#endif
