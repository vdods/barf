// 2019.12.24 - Copyright Victor Dods - Licensed under Apache 2.0

#include "using_shared_ptr_ast.hpp"

void Leaf::print (std::ostream &out, size_t indent_count) const {
    out << std::string(4*indent_count, ' ') << "Leaf(\"" << m_text << "\")";
}

void Tree::print (std::ostream &out, size_t indent_count) const {
    out << std::string(4*indent_count, ' ') << "Tree(\n";
    for (auto const &child : m_children) {
        child->print(out, indent_count+1);
        out << '\n';
    }
    out << std::string(4*indent_count, ' ') << ')';
}

void Tree::append (std::shared_ptr<Base> const &child) {
    m_children.emplace_back(child);
}
