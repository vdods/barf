// 2019.12.22 - Copyright Victor Dods - Licensed under Apache 2.0

#include "using_unique_ptr_ast.hpp"

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

void Tree::append (std::unique_ptr<Base> &&child) {
    m_children.emplace_back(std::move(child));
}
