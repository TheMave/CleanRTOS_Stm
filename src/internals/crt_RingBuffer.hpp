#pragma once
#include <cstddef>
#include <cassert>

template<typename T, size_t CAPACITY>
class RingBuffer {
private:
    T buffer[CAPACITY];
    size_t head;
    size_t tail;
    bool bFull;
    bool bEndlessAdd;

public:
    // if bEndlessAdd == true, if the ringbuffer is full, it first pops the oldest
    // element before pushing a new one (thus automatically discarding the oldest element).
    RingBuffer(bool bEndlessAdd=false) : head(0), tail(0), bFull(false), bEndlessAdd(bEndlessAdd) {}

    bool push(const T& item) {
        if (bFull) {
            if (!bEndlessAdd) {
                // Klassiek gedrag: weigeren als vol
                return false;
            }

            // Endless mode: oudste element weggooien door tail vooruit te zetten
            tail = (tail + 1) % CAPACITY;
            bFull = false; // nu is er ruimte voor één element
        }
        buffer[head] = item;
        head = (head + 1) % CAPACITY;
        if (head == tail) bFull = true;
        return true;
    }

    bool pop(T& item) {
        if (isEmpty()) return false;
        item = buffer[tail];
        tail = (tail + 1) % CAPACITY;
        bFull = false;
        return true;
    }

    // remove item from tail/front, if possible.
    void removeFirst()
    {
        if (isEmpty()) return;
        tail = (tail + 1) % CAPACITY;
        bFull = false;
    }

    // remove item from head/back, if possible.
    void removeLast()
    {
        if (isEmpty()) return;
        head = (head + CAPACITY - 1) % CAPACITY; // first adding CAPACITY to prevent negative operand for % operation.
        bFull = false;
    }

    bool peekTail(T& item) const {
        if (isEmpty()) return false;
        item = buffer[tail];
        return true;
    }

    bool isEmpty() const { return (!bFull && head == tail); }
    bool isFull() const { return bFull; }
    size_t getSizeUsed() const {
        if (bFull) return CAPACITY;
        if (head >= tail) return head - tail;
        return CAPACITY + head - tail;
    }
    size_t getSizeFree() const { return getCapacity()-getSizeUsed(); }
    size_t getCapacity() const { return CAPACITY; }

    // ---------------- Iterator ----------------
    class Iterator {
    public:
        Iterator(RingBuffer<T, CAPACITY>* buf, size_t pos, bool isEnd = false)
            : buffer(buf), index(pos), ended(isEnd) {}

        T& getItem() {
            assert(!ended);
            return buffer->buffer[index];
        }

        Iterator next() {
            if (ended) return *this;

            size_t nextIndex = (index + 1) % CAPACITY;
            bool nextEnded = (nextIndex == buffer->head && !buffer->bFull);
            return Iterator(buffer, nextIndex, nextEnded);
        }

        bool operator!=(const Iterator& other) const {
            return ended != other.ended || index != other.index || buffer != other.buffer;
        }

        bool operator==(const Iterator& other) const {
            return ended == other.ended && index == other.index && buffer == other.buffer;
        }

        static Iterator end(RingBuffer<T, CAPACITY>* buf) {
            return Iterator(buf, buf->head, true);
        }

    private:
        RingBuffer<T, CAPACITY>* buffer;
        size_t index;
        bool ended;
    };

    Iterator getFirst() {
        if (isEmpty()) return Iterator::end(this);
        return Iterator(this, tail, false);
    }
    
    Iterator getLast() {
        if (isEmpty()) return Iterator::end(this);
        return Iterator(this, head, false);
    }
};
