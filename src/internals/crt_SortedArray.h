#ifdef std
#pragma message("MACRO ALERT: std = " STR(std))
#endif

#ifdef iterator_traits
#pragma message("MACRO ALERT: iterator_traits = " STR(iterator_traits))
#endif

#ifdef __half
#pragma message("MACRO ALERT: __half = " STR(__half))
#endif

#include <algorithm>

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <functional>
#include <utility>

// SortedArray by cgpt... todo thorough review by human/me. (for now, has only been reviewed by gemini3 and gpt5.2 extended thinking)
#include <cstddef>
#include <type_traits>

#define STR2(x) #x
#define STR(x) STR2(x)

// !! LET OP !! : T moet operator< implementeren, anders krijg je een slurry van
// onbegrijpelijke foutmeldingen.
template <typename T, std::size_t CAPACITY, typename Comparator = std::less<T>>
class SortedArray {
public:
    static_assert(CAPACITY > 0);

    explicit SortedArray(Comparator cmp = Comparator()) : cmp_(cmp) {}

    std::size_t size() const { return size_; }
    constexpr std::size_t capacity() const { return CAPACITY; }
    bool empty() const { return size_ == 0; }
    bool full()  const { return size_ >= CAPACITY; }

    const T& operator[](std::size_t i) const { return items_[i]; }

    bool addItem(const T& item) { return addImpl(item); }
    bool addItem(T&& item)      { return addImpl(std::move(item)); }

    template <typename Predicate>
    const T* search(Predicate check) const {
        for (std::size_t i = 0; i < size_; ++i) {
            if (check(items_[i])) return &items_[i];
        }
        return nullptr;
    }

    template <typename Predicate>
    const T* searchAfter(Predicate check) const {
        for (std::size_t i = 0; i < size_; ++i) {
            if (check(items_[i])) 
            {
                if(i<(size_-1))
                {
                    return &items_[i+1];
                }
                else
                {
                    return nullptr;
                }
            }
        }
        return nullptr;
    }

    template <typename Predicate>
    bool remove(Predicate check) {
        for (std::size_t i = 0; i < size_; ++i) {
            if (check(items_[i])) {
                std::move(items_ + i + 1, items_ + size_, items_ + i);
                --size_;
                return true;
            }
        }
        return false;
    }

    void clear() { size_ = 0; }

    // Returns pointer to first (smallest) element, or nullptr if empty.
    T* first() {
        return empty() ? nullptr : &items_[0];
    }

    const T* first() const {
        return empty() ? nullptr : &items_[0];
    }

    T* last() {
    return empty() ? nullptr : &items_[size_ - 1];
    }

    const T* last() const {
        return empty() ? nullptr : &items_[size_ - 1];
    }

private:
    template <typename U>
    bool addImpl(U&& item) {
        if (full()) return false;

        std::size_t idx = 0;
        while (idx < size_ && cmp_(items_[idx], item)) ++idx;

        std::move_backward(items_ + idx, items_ + size_, items_ + size_ + 1);
        items_[idx] = std::forward<U>(item);
        ++size_;
        return true;
    }

    T items_[CAPACITY]{};
    std::size_t size_ = 0;
    Comparator cmp_;
};
