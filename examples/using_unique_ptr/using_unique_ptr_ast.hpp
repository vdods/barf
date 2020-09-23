// 2019.12.22 - Copyright Victor Dods - Licensed under Apache 2.0

#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Base {
public:

    virtual ~Base () { }

    virtual void print (std::ostream &out, size_t indent_count = 0) const = 0;
};

class Leaf : public Base {
public:

    Leaf (std::string const &text) : m_text(text) { }
    virtual ~Leaf () { }

    virtual void print (std::ostream &out, size_t indent_count = 0) const override;

private:

    std::string m_text;
};

template <typename... Args_>
std::unique_ptr<Leaf> make_leaf (Args_&&... args) {
    // NOTE: In C++14, we could use make_unique, but BARF only requires C++11 for now.
    return std::unique_ptr<Leaf>(new Leaf(std::forward<Args_>(args)...));
}

class Tree : public Base {
public:

    virtual ~Tree () { }

    virtual void print (std::ostream &out, size_t indent_count = 0) const override;

    void append (std::unique_ptr<Base> &&child);

private:

    std::vector<std::unique_ptr<Base>> m_children;
};

template <typename... Args_>
std::unique_ptr<Tree> make_tree (Args_&&... args) {
    // NOTE: In C++14, we could use make_unique, but BARF only requires C++11 for now.
    return std::unique_ptr<Tree>(new Tree(std::forward<Args_>(args)...));
}
