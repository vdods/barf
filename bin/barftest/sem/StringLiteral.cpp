// 2019.11.12 - Victor Dods

#include "sem/StringLiteral.hpp"

#include "cbz/cgen/Context.hpp"
#include "cbz/literal.hpp"
#include "sem/TypeArray.hpp"

namespace cbz {
namespace sem {

bool StringLiteral::equals (Base const &other_) const
{
    StringLiteral const &other = dynamic_cast<StringLiteral const &>(other_);
    return m_text == other.m_text;
}

StringLiteral *StringLiteral::cloned () const
{
    return new StringLiteral(firange(), m_text);
}

void StringLiteral::print (Log &out) const
{
    out << "StringLiteral(" << firange() << '\n';
    out << IndentGuard()
        << string_literal_of(m_text) << '\n';
    out << ')';
}

llvm::Type *StringLiteral::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    // The type of a string literal is [Uint8]*N, where N is length(m_text)+1.
    // The +1 is to account for the null terminator.
    return make_type_array(firange(), make_uint8(firange().end_as_firange()), m_text.size()+1)->generate_rvalue_type(context, abstract_type);
}

llvm::Value *StringLiteral::generate_rvalue (cgen::Context &context) const
{
    bool add_null = true;
    return llvm::ConstantDataArray::getString(context.llvm_context(), m_text, add_null);
}

} // end namespace sem
} // end namespace cbz
