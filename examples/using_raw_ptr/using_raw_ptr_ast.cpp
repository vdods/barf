// ///////////////////////////////////////////////////////////////////////////
// using_raw_ptr_ast.cpp by Victor Dods, created 2019/12/25
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "using_raw_ptr_ast.hpp"

void Leaf::print (std::ostream &out, size_t indent_count) const {
    out << std::string(4*indent_count, ' ') << "Leaf(\"" << m_text << "\")";
}

Tree::~Tree () {
    for (auto child : m_children)
        delete child;
}

void Tree::print (std::ostream &out, size_t indent_count) const {
    out << std::string(4*indent_count, ' ') << "Tree(\n";
    for (auto child : m_children) {
        child->print(out, indent_count+1);
        out << '\n';
    }
    out << std::string(4*indent_count, ' ') << ')';
}

void Tree::append (Base *child) {
    m_children.emplace_back(child);
}
