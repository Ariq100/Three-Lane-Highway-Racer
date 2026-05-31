#ifndef DYNAMIC_ARRAY_HPP_INCLUDED
#define DYNAMIC_ARRAY_HPP_INCLUDED

#include "splashkit.h"

template <typename T>
class dynamic_array {
private:
    int size;
    int current_capacity;
    T* data;

    void resize() {
        current_capacity *= 2;
        T* new_data = new T[current_capacity];
        for (int i = 0; i < size; i++) {
            new_data[i] = data[i];
        }
        delete[] data;
        data = new_data;
    }

public:
    dynamic_array(int initial_capacity = 10) {
        size = 0;
        current_capacity = initial_capacity;
        data = new T[current_capacity];
    }

    ~dynamic_array() {
        delete[] data;
    }

    dynamic_array(const dynamic_array& other) {
        size = other.size;
        current_capacity = other.current_capacity;
        data = new T[current_capacity];
        for (int i = 0; i < size; i++) {
            data[i] = other.data[i];
        }
    }

    dynamic_array& operator=(const dynamic_array& other) {
        if (this != &other) {
            delete[] data;
            size = other.size;
            current_capacity = other.current_capacity;
            data = new T[current_capacity];
            for (int i = 0; i < size; i++) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    void add(T value) {
        if (size == current_capacity) {
            resize();
        }
        data[size] = value;
        size++;
    }

    int length() const {
        return size;
    }

    int capacity() const {
        return current_capacity;
    }

    int remove_element(int index) {
        if (index >= 0 && index < size) {
            for (int i = index; i < size - 1; i++) {
                data[i] = data[i + 1];
            }
            size--;
            return index;
        } else {
            throw std::out_of_range("Failed to remove element: index is out of bounds.");
        }
    }

    // Alias for older name used in codebase
    int remove_at(int index) { return remove_element(index); }

    // Clear contents
    void clear() { size = 0; }

    void fill(T value) {
        for (int i = 0; i < current_capacity; i++) {
            data[i] = value;
        }
        size = current_capacity;
    }

    T& get(int index) {
        if (index >= 0 && index < size) {
            return data[index];
        } else {
            throw std::out_of_range("Failed to get value: index is out of bounds.");
        }
    }

    const T& get(int index) const {
        if (index >= 0 && index < size) {
            return data[index];
        } else {
            throw std::out_of_range("Failed to get value: index is out of bounds.");
        }
    }

    T& operator[](int index) {
        return get(index);
    }

    const T& operator[](int index) const {
        return get(index);
    }
};

#endif // DYNAMIC_ARRAY_HPP_INCLUDED
