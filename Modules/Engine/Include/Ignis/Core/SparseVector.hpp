#pragma once

#include <Ignis/Core/Logger.hpp>

namespace Ignis {
    template <typename TKey, typename TValue>
    class SparseVector final {
       public:
        struct iterator {
            using difference_type   = std::ptrdiff_t;
            using value_type        = std::pair<TKey &, TValue &>;
            using iterator_category = std::forward_iterator_tag;

            std::vector<TKey>   *Keys;
            std::vector<TValue> *Values;
            size_t               Index;

            value_type operator*() const {
                return {(*Keys)[Index], (*Values)[Index]};
            }

            iterator &operator++() {
                ++Index;
                return *this;
            }

            iterator operator++(int) {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const iterator &other) const {
                return Index == other.Index;
            }
        };

        struct const_iterator {
            using difference_type   = std::ptrdiff_t;
            using value_type        = std::pair<const TKey &, const TValue &>;
            using iterator_category = std::forward_iterator_tag;

            const std::vector<TKey>   *Keys;
            const std::vector<TValue> *Values;
            size_t                     Index;

            value_type operator*() const {
                return {(*Keys)[Index], (*Values)[Index]};
            }

            const_iterator &operator++() {
                ++Index;
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const const_iterator &other) const {
                return Index == other.Index;
            }
        };

       public:
        SparseVector()  = default;
        ~SparseVector() = default;

        void clear() {
            m_Data.clear();
            m_Keys.clear();
            m_LookUp.clear();
        }

        template <typename... Args>
        void insert(const TKey &key, Args &&...args) {
            DIGNIS_ASSERT(!m_LookUp.contains(key), "Ignis::SparseVector<{}>: key already inserted.", typeid(TKey).name());

            m_LookUp[key] = m_Data.size();
            m_Keys.push_back(key);
            m_Data.emplace_back(std::forward<Args>(args)...);
        }

        void remove(const TKey &key) {
            DIGNIS_ASSERT(m_LookUp.contains(key), "Ignis::SparseVector<{}>: key is not inserted.", typeid(TKey).name());

            const size_t index = m_LookUp[key];

            if (index < m_Data.size() - 1) {
                m_Data[index] = std::move(m_Data.back());
                m_Keys[index] = std::move(m_Keys.back());

                m_LookUp[m_Keys[index]] = index;
            }

            m_LookUp.erase(key);
            m_Keys.pop_back();
            m_Data.pop_back();
        }

        bool contains(const TKey &key) const {
            return m_LookUp.find(key) != std::end(m_LookUp);
        }

        bool isEmpty() const {
            return m_Data.empty();
        }

        size_t getSize() const {
            return m_Data.size();
        }

        TValue &operator[](const TKey &key) {
            DIGNIS_ASSERT(m_LookUp.contains(key), "Ignis::SparseVector<{}>: key is not inserted.", typeid(TKey).name());
            return m_Data[m_LookUp[key]];
        }

        const TValue &operator[](const TKey &key) const {
            DIGNIS_ASSERT(m_LookUp.contains(key), "Ignis::SparseVector<{}>: key is not inserted.", typeid(TKey).name());
            return m_Data.at(m_LookUp.at(key));
        }

        iterator begin() { return iterator{&m_Keys, &m_Data, 0}; }

        iterator end() { return iterator{&m_Keys, &m_Data, m_Data.size()}; }

        const_iterator cbegin() const { return const_iterator{&m_Keys, &m_Data, 0}; }

        const_iterator cend() const { return const_iterator{&m_Keys, &m_Data, m_Data.size()}; }

        std::span<TValue> getData() { return m_Data; }

        std::span<const TValue> getConstData() const { return m_Data; }

       private:
        std::vector<TValue> m_Data;
        std::vector<TKey>   m_Keys;

        gtl::flat_hash_map<TKey, size_t> m_LookUp;
    };
}  // namespace Ignis