// 2019.12.24 - Copyright Victor Dods - Licensed under Apache 2.0

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
std::shared_ptr<Leaf> make_leaf (Args_&&... args) {
    return std::make_shared<Leaf>(std::forward<Args_>(args)...);
}

class Tree : public Base {
public:

    virtual ~Tree () { }

    virtual void print (std::ostream &out, size_t indent_count = 0) const override;

    void append (std::shared_ptr<Base> const &child);

private:

    std::vector<std::shared_ptr<Base>> m_children;
};

template <typename... Args_>
std::shared_ptr<Tree> make_tree (Args_&&... args) {
    return std::make_shared<Tree>(std::forward<Args_>(args)...);
}
