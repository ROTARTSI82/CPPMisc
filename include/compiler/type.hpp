#pragma once

#include <memory>


class Type {
public:
    std::size_t size = 0; // size in bytes

    explicit Type(std::size_t s) : size(s) {}
};

class NumericType : public Type {
public:
    explicit NumericType(std::size_t s) : Type(s) {}
};

class FloatingPointType : public NumericType {
public:
    explicit FloatingPointType(std::size_t s) : NumericType(s) {}
};

class IntegerType : public NumericType {
public:
    explicit IntegerType(std::size_t s) : NumericType(s) {}
};

class SignedIntegerType : public IntegerType {
public:
    explicit SignedIntegerType(std::size_t s) : IntegerType(s) {}
};


class UnsignedIntegerType : public IntegerType {
public:
    explicit UnsignedIntegerType(std::size_t s) : IntegerType(s) {}
};

template <typename T, std::size_t size>
inline static Type *get_type() {
    static std::unique_ptr<T> val = std::make_unique<T>(size);
    return val.get();
}

