// 2019.12.25 - Copyright Victor Dods - Licensed under Apache 2.0

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
