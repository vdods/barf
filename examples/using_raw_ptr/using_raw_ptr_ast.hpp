// 2019.12.25 - Copyright Victor Dods - Licensed under Apache 2.0

#pragma once

#include <iostream>
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

class Tree : public Base {
public:

    virtual ~Tree ();

    virtual void print (std::ostream &out, size_t indent_count = 0) const override;

    void append (Base *child);

private:

    std::vector<Base*> m_children;
};
