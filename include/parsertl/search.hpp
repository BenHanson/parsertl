// search.hpp
// Copyright (c) 2017-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SEARCH_HPP
#define PARSERTL_SEARCH_HPP

#include "capture.hpp"
#include "enums.hpp"
#include "match_results.hpp"
#include "parse.hpp"
#include "token.hpp"

#include <map>
#include <set>
#include <utility>
#include <vector>

namespace parsertl
{
    // Forward declarations:
    namespace details
    {
        // A point in the input where the table has an action on '$'.
        // This is only a *candidate* end of match (LALR lookaheads are
        // merged, so '$' can be present when it is not really valid).
        template<typename lexer_iterator, typename sm_type>
        struct eoi_candidate
        {
            lexer_iterator _iter;
            basic_match_results<sm_type> _results;
        };

        template<typename lexer_iterator, typename sm_type>
        void next(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_,
            std::vector<eoi_candidate<lexer_iterator, sm_type> >& candidates_);
        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        void next(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::vector<lexer_iterator>& candidates_,
            token_vector& productions_);
        template<typename lexer_iterator, typename sm_type>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_);
        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_, token_vector& productions_,
            std::multimap<typename sm_type::id_type, token_vector>* prod_map_);
        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_, token_vector& productions_,
            std::vector<std::pair<typename sm_type::id_type, token_vector> >*
            prod_vec_);
    }

    template<typename lexer_iterator, typename sm_type, typename captures>
    bool search(lexer_iterator& iter_, lexer_iterator& end_, const sm_type& sm_,
        captures& captures_)
    {
        basic_match_results<sm_type> results_(iter_->id, sm_);
        // Qualify token to prevent arg dependant lookup
        typedef parsertl::token<lexer_iterator> token;
        typedef typename token::token_vector token_vector;
        typedef std::multimap<typename sm_type::id_type, token_vector> prod_map;
        prod_map prod_map_;
        bool success_ = search(iter_, end_, sm_, &prod_map_);

        captures_.clear();

        if (success_)
        {
            typename token::iter_type last_ = iter_->first;
            typename prod_map::const_iterator pi_ = prod_map_.begin();
            typename prod_map::const_iterator pe_ = prod_map_.end();

            captures_.resize((sm_._captures.empty() ? 0 :
                sm_._captures.back().first +
                sm_._captures.back().second.size()) + 1);
            captures_[0].push_back(capture<typename token::iter_type>
                (iter_->first, iter_->first));

            for (; pi_ != pe_; ++pi_)
            {
                if (sm_._captures.size() > pi_->first)
                {
                    const typename sm_type::capture& row_ =
                        sm_._captures[pi_->first];

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
                                push_back(capture<typename token::iter_type>
                                    (token1_.first, token2_.second));
                            ++index_;
                        }
                    }
                }
            }

            pi_ = prod_map_.begin();
            pe_ = prod_map_.end();

            for (; pi_ != pe_; ++pi_)
            {
                typename token::iter_type sec_ = pi_->second.back().second;

                if (sec_ > last_)
                {
                    last_ = sec_;
                }
            }

            captures_.front().back().second = last_;
        }

        return success_;
    }

    // Equivalent of std::search().
    template<typename lexer_iterator, typename sm_type>
    bool search(lexer_iterator& iter_, lexer_iterator& end_, const sm_type& sm_,
        std::set<typename sm_type::id_type>* prod_set_ = 0)
    {
        bool hit_ = false;
        lexer_iterator curr_ = iter_;
        // results_ defined here so that allocated memory can be reused.
        basic_match_results<sm_type> results_;
        typedef std::vector<details::eoi_candidate<lexer_iterator, sm_type> >
            candidate_vector;
        candidate_vector candidates_;

        end_ = lexer_iterator();

        while (curr_ != end_)
        {
            if (prod_set_)
            {
                prod_set_->clear();
            }

            results_.reset(curr_->id, sm_);
            candidates_.clear();

            while (results_.entry.action != accept &&
                results_.entry.action != error)
            {
                details::next(curr_, sm_, results_, prod_set_, candidates_);
            }

            hit_ = results_.entry.action == accept;

            if (hit_)
            {
                end_ = curr_;
                break;
            }

            // Try each candidate, longest first. A candidate sitting at
            // end of input has already been tried by the loop above.
            for (typename candidate_vector::reverse_iterator i_ =
                candidates_.rbegin(), e_ = candidates_.rend();
                i_ != e_; ++i_)
            {
                if (i_->_iter->id == 0)
                    continue;

                lexer_iterator eoi_;

                hit_ = details::parse(eoi_, sm_, i_->_results, prod_set_);

                if (hit_)
                {
                    end_ = i_->_iter;
                    break;
                }
            }

            if (hit_)
                break;

            if (iter_->id != 0)
                ++iter_;

            curr_ = iter_;
        }

        return hit_;
    }

    template<typename lexer_iterator, typename sm_type, typename token_vector>
    bool search(lexer_iterator& iter_, lexer_iterator& end_, const sm_type& sm_,
        std::multimap<typename sm_type::id_type, token_vector>*
        prod_map_ = 0)
    {
        bool hit_ = false;
        lexer_iterator curr_ = iter_;
        typedef std::vector<lexer_iterator> candidate_vector;
        candidate_vector candidates_;
        // results_ and productions_ defined here so that
        // allocated memory can be reused.
        basic_match_results<sm_type> results_;
        token_vector productions_;

        end_ = lexer_iterator();

        while (curr_ != end_)
        {
            if (prod_map_)
            {
                prod_map_->clear();
            }

            results_.reset(curr_->id, sm_);
            productions_.clear();
            candidates_.clear();

            while (results_.entry.action != accept &&
                results_.entry.action != error)
            {
                details::next(curr_, sm_, results_, candidates_,
                    productions_);
            }

            hit_ = results_.entry.action == accept;

            if (hit_)
            {
                if (prod_map_)
                {
                    lexer_iterator again_(iter_->first, curr_->first,
                        iter_.sm());

                    results_.reset(iter_->id, sm_);
                    productions_.clear();
                    details::parse(again_, sm_, results_, productions_,
                        prod_map_);
                }

                end_ = curr_;
                break;
            }

            // Try each candidate, longest first. A candidate sitting at
            // end of input has already been tried by the loop above.
            for (typename candidate_vector::reverse_iterator i_ =
                candidates_.rbegin(), e_ = candidates_.rend();
                i_ != e_; ++i_)
            {
                if ((*i_)->id == 0)
                    continue;

                lexer_iterator again_(iter_->first, (*i_)->first,
                    iter_.sm());

                results_.reset(iter_->id, sm_);
                productions_.clear();

                if (prod_map_)
                    prod_map_->clear();

                hit_ = details::parse(again_, sm_, results_, productions_,
                    prod_map_);

                if (hit_)
                {
                    end_ = *i_;
                    break;
                }
            }

            if (hit_)
                break;

            if (iter_->id != 0)
                ++iter_;

            curr_ = iter_;
        }

        return hit_;
    }

    template<typename lexer_iterator, typename sm_type, typename token_vector>
    bool search(lexer_iterator& iter_, lexer_iterator& end_, const sm_type& sm_,
        std::vector<std::pair<typename sm_type::id_type, token_vector> >*
        prod_vec_ = 0)
    {
        bool hit_ = false;
        lexer_iterator curr_ = iter_;
        std::vector<lexer_iterator> candidates_;
        // results_ and productions_ defined here so that
        // allocated memory can be reused.
        basic_match_results<sm_type> results_;
        token_vector productions_;

        end_ = lexer_iterator();

        while (curr_ != end_)
        {
            if (prod_vec_)
            {
                prod_vec_->clear();
            }

            results_.reset(curr_->id, sm_);
            productions_.clear();
            candidates_.clear();

            while (results_.entry.action != accept &&
                results_.entry.action != error)
            {
                details::next(curr_, sm_, results_, candidates_,
                    productions_);
            }

            hit_ = results_.entry.action == accept;

            if (hit_)
            {
                if (prod_vec_)
                {
                    lexer_iterator again_(iter_->first, curr_->first,
                        iter_.sm());

                    results_.reset(iter_->id, sm_);
                    productions_.clear();
                    details::parse(again_, sm_, results_, productions_,
                        prod_vec_);
                }

                end_ = curr_;
                break;
            }

            // Try each candidate, longest first. A candidate sitting at
            // end of input has already been tried by the loop above.
            for (typename std::vector<lexer_iterator>::reverse_iterator i_ =
                    candidates_.rbegin(), e_ = candidates_.rend();
                i_ != e_; ++i_)
            {
                if ((*i_)->id == 0)
                    continue;

                lexer_iterator again_(iter_->first, (*i_)->first,
                    iter_.sm());

                results_.reset(iter_->id, sm_);
                productions_.clear();

                if (prod_vec_)
                    prod_vec_->clear();

                hit_ = details::parse(again_, sm_, results_, productions_,
                    prod_vec_);

                if (hit_)
                {
                    end_ = *i_;
                    break;
                }
            }

            if (hit_)
                break;

            if (iter_->id != 0)
                ++iter_;

            curr_ = iter_;
        }

        return hit_;
    }

    namespace details
    {
        template<typename lexer_iterator, typename sm_type>
        void next(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_,
            std::vector<eoi_candidate<lexer_iterator, sm_type> >& candidates_)
        {
            switch (results_.entry.action)
            {
            case shift:
            {
                const typename sm_type::entry eoi_ =
                    sm_.at(results_.entry.param);

                results_.stack.push_back(results_.entry.param);

                if (iter_->id != 0)
                    ++iter_;

                results_.token_id = iter_->id;

                if (results_.token_id == lexer_iterator::value_type::npos())
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
                    eoi_candidate<lexer_iterator, sm_type> candidate;

                    candidate._iter = iter_;
                    candidate._results = results_;
                    candidates_.push_back(candidate);
                    candidates_.back()._results.token_id = 0;
                    candidates_.back()._results.entry = eoi_;
                }

                break;
            }
            case reduce:
            {
                const std::size_t size_ =
                    sm_._rules[results_.entry.param]._rhs.size();

                if (prod_set_)
                {
                    prod_set_->insert(results_.entry.param);
                }

                if (size_)
                {
                    results_.stack.resize(results_.stack.size() - size_);
                }

                results_.token_id = sm_._rules[results_.entry.param]._lhs;
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
                    sm_._rules[results_.entry.param]._rhs.size();

                if (size_)
                {
                    results_.stack.resize(results_.stack.size() - size_);
                }

                break;
            }
            default:
                // error
                break;
            }
        }

        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        void next(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::vector<lexer_iterator>& candidates_,
            token_vector& productions_)
        {
            switch (results_.entry.action)
            {
            case shift:
            {
                const typename sm_type::entry eoi_ =
                    sm_.at(results_.entry.param);

                results_.stack.push_back(results_.entry.param);
                productions_.push_back(typename token_vector::
                    value_type(iter_->id, iter_->first, iter_->second));

                if (iter_->id != 0)
                    ++iter_;

                results_.token_id = iter_->id;

                if (results_.token_id == lexer_iterator::value_type::npos())
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
                    candidates_.push_back(iter_);

                break;
            }
            case reduce:
            {
                const std::size_t size_ =
                    sm_._rules[results_.entry.param]._rhs.size();
                token<lexer_iterator> token_;

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

                results_.token_id = sm_._rules[results_.entry.param]._lhs;
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
                    sm_._rules[results_.entry.param]._rhs.size();

                if (size_)
                {
                    results_.stack.resize(results_.stack.size() - size_);
                }

                break;
            }
            default:
                // error
                break;
            }
        }

        template<typename lexer_iterator, typename sm_type>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_)
        {
            while (results_.entry.action != error)
            {
                switch (results_.entry.action)
                {
                case shift:
                    results_.stack.push_back(results_.entry.param);

                    if (iter_->id != 0)
                        ++iter_;

                    results_.token_id = iter_->id;

                    if (results_.token_id == lexer_iterator::value_type::npos())
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
                        sm_._rules[results_.entry.param]._rhs.size();

                    if (prod_set_)
                    {
                        prod_set_->insert(results_.entry.param);
                    }

                    if (size_)
                    {
                        results_.stack.resize(results_.stack.size() - size_);
                    }

                    results_.token_id = sm_._rules[results_.entry.param]._lhs;
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
                default:
                    // error
                    break;
                }

                if (results_.entry.action == accept)
                {
                    const std::size_t size_ =
                        sm_._rules[results_.entry.param]._rhs.size();

                    if (size_)
                    {
                        results_.stack.resize(results_.stack.size() - size_);
                    }

                    break;
                }
            }

            return results_.entry.action == accept;
        }

        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_, token_vector& productions_,
            std::multimap<typename sm_type::id_type, token_vector>* prod_map_)
        {
            while (results_.entry.action != error)
            {
                switch (results_.entry.action)
                {
                case shift:
                    results_.stack.push_back(results_.entry.param);
                    productions_.push_back(typename token_vector::
                        value_type(iter_->id, iter_->first, iter_->second));

                    if (iter_->id != 0)
                        ++iter_;

                    results_.token_id = iter_->id;

                    if (results_.token_id == lexer_iterator::value_type::npos())
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
                        sm_._rules[results_.entry.param]._rhs.size();
                    token<lexer_iterator> token_;

                    if (size_)
                    {
                        if (prod_map_)
                        {
                            prod_map_->insert(std::pair
                                <typename sm_type::id_type, token_vector>
                                (results_.entry.param,
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

                    results_.token_id = sm_._rules[results_.entry.param]._lhs;
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
                default:
                    // accept
                    // error
                    break;
                }

                if (results_.entry.action == accept)
                {
                    const std::size_t size_ =
                        sm_._rules[results_.entry.param]._rhs.size();

                    if (size_)
                    {
                        results_.stack.resize(results_.stack.size() - size_);
                    }

                    break;
                }
            }

            return results_.entry.action == accept;
        }

        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_, token_vector& productions_,
            std::vector<std::pair<typename sm_type::id_type, token_vector> >*
            prod_vec_)
        {
            while (results_.entry.action != error)
            {
                switch (results_.entry.action)
                {
                case shift:
                    results_.stack.push_back(results_.entry.param);
                    productions_.emplace_back(iter_->id, iter_->first,
                        iter_->second);

                    if (iter_->id != 0)
                        ++iter_;

                    results_.token_id = iter_->id;

                    if (results_.token_id == lexer_iterator::value_type::npos())
                    {
                        results_.entry.action = error;
                        results_.entry.param =
                            static_cast<typename sm_type::id_type>
                            (unknown_token);
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
                        sm_._rules[results_.entry.param]._rhs.size();
                    token<lexer_iterator> token_;

                    if (size_)
                    {
                        if (prod_vec_)
                        {
                            prod_vec_->emplace_back(results_.entry.
                                param, token_vector(productions_.end() - size_,
                                    productions_.end()));
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

                    results_.token_id = sm_._rules[results_.entry.param]._lhs;
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
                default:
                    // accept
                    // error
                    break;
                }

                if (results_.entry.action == accept)
                {
                    const std::size_t size_ = sm_._rules[results_.entry.param].
                        _rhs.size();

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
