// search.hpp
// Copyright (c) 2017-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SEARCH_HPP
#define PARSERTL_SEARCH_HPP

#include "../../../lexertl/include/lexertl/iterator.hpp"
#include <map>
#include "match_results.hpp"
#include "parse.hpp"
#include <set>
#include "token.hpp"

namespace parsertl
{
    // Forward declarations:
    namespace details
    {
        template<typename sm_type, typename iterator>
        void next(const sm_type& sm_, iterator& iter_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_,
            iterator& last_eoi_, basic_match_results<sm_type>& last_results_);
        template<typename sm_type, typename iterator, typename token_vector>
        void next(const sm_type& sm_, iterator& iter_,
            basic_match_results<sm_type>& results_, iterator& last_eoi_,
            token_vector& productions_);
        template<typename sm_type, typename iterator>
        bool parse(const sm_type& sm_, iterator& iter_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_);
        template<typename sm_type, typename iterator, typename token_vector>
        bool parse(const sm_type& sm_, iterator& iter_,
            basic_match_results<sm_type>& results_, token_vector& productions_,
            std::multimap<typename sm_type::id_type, token_vector>* prod_map_);
    }

    template<typename iterator, typename sm_type, typename lsm>
    bool search(iterator first_, iterator second_, const lsm& lsm_,
        const sm_type& gsm_)
    {
        typedef lexertl::iterator<iterator, lsm, lexertl::
            match_results<iterator> > lex_iterator;
        lex_iterator iter_(first_, second_, lsm_);
        lex_iterator end_;

        return search(gsm_, iter_, end_);
    }

    template<typename iterator, typename captures, typename sm_type,
        typename lsm>
        bool search(iterator first_, iterator second_, captures& captures_,
            lsm& lsm_, const sm_type& gsm_)
    {
        typedef lexertl::iterator<iterator, lsm, lexertl::
            match_results<iterator> > lex_iterator;
        lex_iterator iter_(first_, second_, lsm_);
        lex_iterator end_;
        basic_match_results<sm_type> results_(iter_->id, gsm_);
        // Qualify token to prevent arg dependant lookup
        typedef parsertl::token<lex_iterator> token;
        typedef typename token::token_vector token_vector;
        typedef std::multimap<typename sm_type::id_type, token_vector> prod_map;
        prod_map prod_map_;
        bool success_ = search(gsm_, iter_, end_, &prod_map_);

        captures_.clear();

        if (success_)
        {
            iterator last_ = iter_->first;
            typename prod_map::const_iterator pi_ = prod_map_.begin();
            typename prod_map::const_iterator pe_ = prod_map_.end();

            captures_.resize((gsm_._captures.empty() ? 0 :
                gsm_._captures.back().first +
                gsm_._captures.back().second.size()) + 1);
            captures_[0].push_back(std::pair
                <typename lex_iterator::value_type::iter_type,
                typename lex_iterator::value_type::iter_type>
                (iter_->first, iter_->first));

            for (; pi_ != pe_; ++pi_)
            {
                if (gsm_._captures.size() > pi_->first)
                {
                    const typename sm_type::capture_vec_pair& row_ =
                        gsm_._captures[pi_->first];

                    if (!row_.second.empty())
                    {
                        typedef typename sm_type::capture_vector capture_vector;
                        typename capture_vector::const_iterator ti_ =
                            row_.second.begin();
                        typename capture_vector::const_iterator te_ =
                            row_.second.end();
                        std::size_t index_ = 0;

                        for (; ti_ != te_; ++ti_)
                        {
                            const token& token1_ = pi_->second[ti_->first];
                            const token& token2_ = pi_->second[ti_->second];

                            captures_[row_.first + index_ + 1].
                                push_back(std::pair<typename token::iter_type,
                                    typename token::iter_type>(token1_.first,
                                    token2_.second));
                            ++index_;
                        }
                    }
                }
            }

            pi_ = prod_map_.begin();
            pe_ = prod_map_.end();

            for (; pi_ != pe_; ++pi_)
            {
                typename token::iter_type second_ = pi_->second.back().second;

                if (second_ > last_)
                {
                    last_ = second_;
                }
            }

            captures_.front().back().second = last_;
        }

        return success_;
    }

    // Equivalent of std::search().
    template<typename sm_type, typename iterator>
    bool search(const sm_type& sm_, iterator& iter_, iterator& end_,
        std::set<typename sm_type::id_type>* prod_set_ = 0)
    {
        bool hit_ = false;
        iterator curr_ = iter_;
        iterator last_eoi_;
        // results_ defined here so that allocated memory can be reused.
        basic_match_results<sm_type> results_;
        basic_match_results<sm_type> last_results_;

        end_ = iterator();

        while (curr_ != end_)
        {
            if (prod_set_)
            {
                prod_set_->clear();
            }

            results_.reset(curr_->id, sm_);
            last_results_.clear();

            while (results_.entry.action != accept &&
                results_.entry.action != error)
            {
                details::next(sm_, curr_, results_, prod_set_, last_eoi_,
                    last_results_);
            }

            hit_ = results_.entry.action == accept;

            if (hit_)
            {
                end_ = curr_;
                break;
            }
            else if (last_eoi_->id != 0)
            {
                iterator eoi_;

                hit_ = details::parse(sm_, eoi_, last_results_, prod_set_);

                if (hit_)
                {
                    end_ = last_eoi_;
                    break;
                }
            }

            ++iter_;
            curr_ = iter_;
        }

        return hit_;
    }

    template<typename sm_type, typename iterator, typename token_vector>
    bool search(const sm_type& sm_, iterator& iter_, iterator& end_,
        std::multimap<typename sm_type::id_type, token_vector>* prod_map_ = 0)
    {
        bool hit_ = false;
        iterator curr_ = iter_;
        iterator last_eoi_;
        // results_ and productions_ defined here so that
        // allocated memory can be reused.
        basic_match_results<sm_type> results_;
        token_vector productions_;

        end_ = iterator();

        while (curr_ != end_)
        {
            if (prod_map_)
            {
                prod_map_->clear();
            }

            results_.reset(curr_->id, sm_);
            productions_.clear();

            while (results_.entry.action != accept &&
                results_.entry.action != error)
            {
                details::next(sm_, curr_, results_, last_eoi_, productions_);
            }

            hit_ = results_.entry.action == accept;

            if (hit_)
            {
                if (prod_map_)
                {
                    iterator again_(iter_->first, last_eoi_->first, iter_.sm());

                    results_.reset(iter_->id, sm_);
                    productions_.clear();
                    details::parse(sm_, again_, results_, productions_,
                        prod_map_);
                }

                end_ = curr_;
                break;
            }
            else if (last_eoi_->id != 0)
            {
                iterator again_(iter_->first, last_eoi_->first, iter_.sm());

                results_.reset(iter_->id, sm_);
                productions_.clear();
                hit_ = details::parse(sm_, again_, results_, productions_,
                    prod_map_);

                if (hit_)
                {
                    end_ = last_eoi_;
                    break;
                }
            }

            ++iter_;
            curr_ = iter_;
        }

        return hit_;
    }

    namespace details
    {
        template<typename sm_type, typename iterator>
        void next(const sm_type& sm_, iterator& iter_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_,
            iterator& last_eoi_, basic_match_results<sm_type>& last_results_)
        {
            switch (results_.entry.action)
            {
            case error:
                break;
            case shift:
            {
                const typename sm_type::entry eoi_ =
                    sm_.at(results_.entry.param);

                results_.stack.push_back(results_.entry.param);

                if (results_.token_id != 0)
                {
                    ++iter_;
                }

                results_.token_id = iter_->id;

                if (results_.token_id == iterator::value_type::npos())
                {
                    results_.entry.action = error;
                    results_.entry.param = unknown_token;
                }
                else
                {
                    results_.entry =
                        sm_.at(results_.entry.param, results_.token_id);
                }

                if (eoi_.action != error)
                {
                    last_eoi_ = iter_;
                    last_results_.stack = results_.stack;
                    last_results_.token_id = 0;
                    last_results_.entry = eoi_;
                }

                break;
            }
            case reduce:
            {
                const std::size_t size_ =
                    sm_._rules[results_.entry.param].second.size();

                if (prod_set_)
                {
                    prod_set_->insert(results_.entry.param);
                }

                if (size_)
                {
                    results_.stack.resize(results_.stack.size() - size_);
                }

                results_.token_id = sm_._rules[results_.entry.param].first;
                results_.entry =
                    sm_.at(results_.stack.back(), results_.token_id);
                break;
            }
            case go_to:
                results_.stack.push_back(results_.entry.param);
                results_.token_id = iter_->id;
                results_.entry =
                    sm_.at(results_.stack.back(), results_.token_id);
                break;
            case accept:
            {
                const std::size_t size_ =
                    sm_._rules[results_.entry.param].second.size();

                if (size_)
                {
                    results_.stack.resize(results_.stack.size() - size_);
                }

                break;
            }
            }
        }

        template<typename sm_type, typename iterator, typename token_vector>
        void next(const sm_type& sm_, iterator& iter_,
            basic_match_results<sm_type>& results_, iterator& last_eoi_,
            token_vector& productions_)
        {
            switch (results_.entry.action)
            {
            case error:
                break;
            case shift:
            {
                const typename sm_type::entry eoi_ = sm_.at(results_.entry.param);

                results_.stack.push_back(results_.entry.param);
                productions_.push_back(typename token_vector::
                    value_type(iter_->id, iter_->first, iter_->second));

                if (results_.token_id != 0)
                {
                    ++iter_;
                }

                results_.token_id = iter_->id;

                if (results_.token_id == iterator::value_type::npos())
                {
                    results_.entry.action = error;
                    results_.entry.param = unknown_token;
                }
                else
                {
                    results_.entry =
                        sm_.at(results_.entry.param, results_.token_id);
                }

                if (eoi_.action != error)
                {
                    last_eoi_ = iter_;
                }

                break;
            }
            case reduce:
            {
                const std::size_t size_ =
                    sm_._rules[results_.entry.param].second.size();
                token<iterator> token_;

                if (size_)
                {
                    token_.first = (productions_.end() - size_)->first;
                    token_.second = productions_.back().second;
                    results_.stack.resize(results_.stack.size() - size_);
                    productions_.resize(productions_.size() - size_);
                }
                else
                {
                    if (productions_.empty())
                    {
                        token_.first = token_.second = iter_->first;
                    }
                    else
                    {
                        token_.first = token_.second =
                            productions_.back().second;
                    }
                }

                results_.token_id = sm_._rules[results_.entry.param].first;
                results_.entry =
                    sm_.at(results_.stack.back(), results_.token_id);
                token_.id = results_.token_id;
                productions_.push_back(token_);
                break;
            }
            case go_to:
                results_.stack.push_back(results_.entry.param);
                results_.token_id = iter_->id;
                results_.entry =
                    sm_.at(results_.stack.back(), results_.token_id);
                break;
            case accept:
            {
                const std::size_t size_ =
                    sm_._rules[results_.entry.param].second.size();

                if (size_)
                {
                    results_.stack.resize(results_.stack.size() - size_);
                }

                break;
            }
            }
        }

        template<typename sm_type, typename iterator>
        bool parse(const sm_type& sm_, iterator& iter_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_)
        {
            while (results_.entry.action != error)
            {
                switch (results_.entry.action)
                {
                case error:
                    break;
                case shift:
                    results_.stack.push_back(results_.entry.param);

                    if (results_.token_id != 0)
                    {
                        ++iter_;
                    }

                    results_.token_id = iter_->id;

                    if (results_.token_id == iterator::value_type::npos())
                    {
                        results_.entry.action = error;
                        results_.entry.param = unknown_token;
                    }
                    else
                    {
                        results_.entry =
                            sm_.at(results_.stack.back(), results_.token_id);
                    }

                    break;
                case reduce:
                {
                    const std::size_t size_ =
                        sm_._rules[results_.entry.param].second.size();

                    if (prod_set_)
                    {
                        prod_set_->insert(results_.entry.param);
                    }

                    if (size_)
                    {
                        results_.stack.resize(results_.stack.size() - size_);
                    }

                    results_.token_id = sm_._rules[results_.entry.param].first;
                    results_.entry =
                        sm_.at(results_.stack.back(), results_.token_id);
                    break;
                }
                case go_to:
                    results_.stack.push_back(results_.entry.param);
                    results_.token_id = iter_->id;
                    results_.entry =
                        sm_.at(results_.stack.back(), results_.token_id);
                    break;
                }

                if (results_.entry.action == accept)
                {
                    const std::size_t size_ =
                        sm_._rules[results_.entry.param].second.size();

                    if (size_)
                    {
                        results_.stack.resize(results_.stack.size() - size_);
                    }

                    break;
                }
            }

            return results_.entry.action == accept;
        }

        template<typename sm_type, typename iterator, typename token_vector>
        bool parse(const sm_type& sm_, iterator& iter_,
            basic_match_results<sm_type>& results_, token_vector& productions_,
            std::multimap<typename sm_type::id_type, token_vector>* prod_map_)
        {
            while (results_.entry.action != error)
            {
                switch (results_.entry.action)
                {
                case error:
                    break;
                case shift:
                    results_.stack.push_back(results_.entry.param);
                    productions_.push_back(typename token_vector::
                        value_type(iter_->id, iter_->first, iter_->second));

                    if (results_.token_id != 0)
                    {
                        ++iter_;
                    }

                    results_.token_id = iter_->id;

                    if (results_.token_id == iterator::value_type::npos())
                    {
                        results_.entry.action = error;
                        results_.entry.param = unknown_token;
                    }
                    else
                    {
                        results_.entry =
                            sm_.at(results_.stack.back(), results_.token_id);
                    }

                    break;
                case reduce:
                {
                    const std::size_t size_ =
                        sm_._rules[results_.entry.param].second.size();
                    token<iterator> token_;

                    if (size_)
                    {
                        if (prod_map_)
                        {
                            prod_map_->insert(std::pair
                                <typename sm_type::entry::id_type,
                                token_vector>(results_.entry.param,
                                    token_vector(productions_.end() - size_,
                                    productions_.end())));
                        }

                        token_.first = (productions_.end() - size_)->first;
                        token_.second = productions_.back().second;
                        results_.stack.resize(results_.stack.size() - size_);
                        productions_.resize(productions_.size() - size_);
                    }
                    else
                    {
                        if (productions_.empty())
                        {
                            token_.first = token_.second = iter_->first;
                        }
                        else
                        {
                            token_.first = token_.second =
                                productions_.back().second;
                        }
                    }

                    results_.token_id = sm_._rules[results_.entry.param].first;
                    results_.entry =
                        sm_.at(results_.stack.back(), results_.token_id);
                    token_.id = results_.token_id;
                    productions_.push_back(token_);
                    break;
                }
                case go_to:
                    results_.stack.push_back(results_.entry.param);
                    results_.token_id = iter_->id;
                    results_.entry =
                        sm_.at(results_.stack.back(), results_.token_id);
                    break;
                }

                if (results_.entry.action == accept)
                {
                    const std::size_t size_ =
                        sm_._rules[results_.entry.param].second.size();

                    if (size_)
                    {
                        results_.stack.resize(results_.stack.size() - size_);
                    }

                    break;
                }
            }

            return results_.entry.action == accept;
        }
    }
}

#endif
