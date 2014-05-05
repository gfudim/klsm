/*
 *  Copyright 2014 Jakob Gruber
 *
 *  This file is part of kpqueue.
 *
 *  kpqueue is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  kpqueue is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with kpqueue.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "block.h"

#include <cassert>

namespace kpq
{


template <class K, class V>
block<K, V>::block(const size_t power_of_2) :
    m_next(nullptr),
    m_prev(nullptr),
    m_size(0),
    m_power_of_2(power_of_2),
    m_capacity(1 << power_of_2),
    m_item_pairs(new item_pair_t[m_capacity]),
    m_used(false)
{
}

template <class K, class V>
block<K, V>::~block()
{
    delete[] m_item_pairs;
}

template <class K, class V>
void
block<K, V>::insert(item<K, V> *it)
{
    assert(m_used);
    assert(m_size == 0);
    assert(m_capacity == 1);

    m_item_pairs->first  = it;
    m_item_pairs->second = it->version();

    m_size = 1;
}

template <class K, class V>
void
block<K, V>::merge(const block<K, V> *lhs,
                   const block<K, V> *rhs)
{
    assert(m_power_of_2 == lhs->power_of_2() + 1);
    assert(lhs->power_of_2() == rhs->power_of_2());
    assert(m_used);
    assert(m_size == 0);

    /* Merge. */

    size_t l = 0, r = 0, dst = 0;

    while (l < lhs->capacity() && r < rhs->capacity()) {
        auto &lelem = lhs->m_item_pairs[l];
        auto &relem = rhs->m_item_pairs[r];

        if (!item_owned(lelem)) {
            l++;
            continue;
        }

        if (!item_owned(relem)) {
            r++;
            continue;
        }

        if (lelem.first->key() < relem.first->key()) {
            m_item_pairs[dst++] = lelem;
            l++;
        } else {
            m_item_pairs[dst++] = relem;
            r++;
        }
    }

    while (l < lhs->capacity()) {
        auto &lelem = lhs->m_item_pairs[l];
        if (!item_owned(lelem)) {
            l++;
            continue;
        }
        m_item_pairs[dst++] = lelem;
        l++;
    }

    while (r < rhs->capacity()) {
        auto &relem = rhs->m_item_pairs[r];
        if (!item_owned(relem)) {
            r++;
            continue;
        }
        m_item_pairs[dst++] = relem;
        r++;
    }

    m_size = dst;
}

template <class K, class V>
typename block<K, V>::peek_t
block<K, V>::peek()
{
    peek_t p;
    for (size_t i = 0; i < m_size; i++) {
        p.m_item    = m_item_pairs[i].first;
        p.m_key     = m_item_pairs[i].first->key();
        p.m_version = m_item_pairs[i].second;

        if (item_owned(m_item_pairs[i])) {
            return p;
        }

        /* TODO: Clean reference to unowned item. */
    }

    p.m_item = nullptr;
    return p;
}

template <class K, class V>
size_t
block<K, V>::size() const
{
    return m_size;
}

template <class K, class V>
size_t
block<K, V>::power_of_2() const
{
    return m_power_of_2;
}

template <class K, class V>
size_t
block<K, V>::capacity() const
{
    return m_capacity;
}

template <class K, class V>
bool
block<K, V>::used() const
{
    return m_used;
}

template <class K, class V>
void
block<K, V>::set_unused()
{
    assert(m_used);
    m_used = false;

    m_next.store(nullptr, std::memory_order_relaxed);
    m_prev = nullptr;
}

template <class K, class V>
void
block<K, V>::set_used()
{
    assert(!m_used);
    m_used = true;
}

template <class K, class V>
bool
block<K, V>::item_owned(const item_pair_t &item_pair)
{
    return (item_pair.first->version() == item_pair.second);
}

template class block<uint32_t, uint32_t>;

}