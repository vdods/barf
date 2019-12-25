// ///////////////////////////////////////////////////////////////////////////
// using_shared_ptr_ast.hpp by Victor Dods, created 2019/12/24
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

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
