// 2016.08.09 - Victor Dods

#pragma once

#include "core.hpp"
#include "Exception.hpp"
#include "sem/Base.hpp"
#include "sem/Declaration.hpp"
#include "sem/Identifier.hpp"
#include "sem/Type.hpp"
#include "sem/Value.hpp"
#include <type_traits>
#include <vector>

namespace cbz {
namespace sem {

// TODO: Make VectorBase ?
// TODO: Rename to Tuple?

template <TypeEnum TYPE_ENUM_, typename ElementType_, typename BaseClass_, char PRINT_DELIMITER_>
struct Vector : public BaseClass_
{
    static TypeEnum constexpr TYPE_ENUM = TYPE_ENUM_;
    typedef ElementType_ ElementType;
    typedef nnup<ElementType_> ElementTypePtr;
    static char constexpr PRINT_DELIMITER = PRINT_DELIMITER_;

    Vector () : BaseClass_(FiRange::INVALID) { }
    Vector (Vector const &) = delete;
//     Vector (FiRange const &firange = FiRange::INVALID) : BaseClass_(firange) { }

    // If you want to set an explicit FiRange, use set_firange.
    template <typename... Args_>
    Vector (Args_&&... args)
    :   BaseClass_(FiRange::INVALID)
    {
        m_elements.reserve(sizeof...(Args_));
        this->push_back(std::forward<Args_>(args)...);
        // TODO: Figure out if this check is already done by nnup
        for (auto const &element : m_elements)
            if (element == nullptr)
                LVD_ABORT("all elements in Vector must be non-null");
    }
    virtual ~Vector () { }

    virtual TypeEnum type_enum__raw () const override { return TYPE_ENUM_; }
    virtual bool equals (Base const &other_) const override
    {
        Vector const &other = dynamic_cast<Vector const &>(other_);
        if (m_elements.size() != other.m_elements.size())
            return false;
        for (auto this_it = m_elements.begin(), other_it = other.m_elements.begin();
             this_it != m_elements.end();
             ++this_it, ++other_it)
        {
            if (!are_equal(*this_it, *other_it))
                return false;
        }
        return true;
    }
    virtual Vector *cloned () const override
    {
        auto retval = new Vector();
        retval->set_firange(this->firange());
        for (auto const &element : m_elements)
            retval->push_back(clone_of(element));
        return retval;
    }
    virtual void print (Log &out) const override
    {
        out << "Vector<" << as_string(TYPE_ENUM_) << ">(" << this->firange() << " with " << m_elements.size() << " elements\n";
        if (!m_elements.empty())
        {
            IndentGuard ig(out);
            for (auto const &element : m_elements)
                out << element << PRINT_DELIMITER_ << '\n';
        }
        out << ")";
    }
    virtual void resolve_symbols (cgen::Context &context) override
    {
        for (auto &element : m_elements)
            element->resolve_symbols(context);
    }
    virtual Determinability generate_determinability (cgen::Context &context) const override
    {
        // An empty tuple is a singleton, and therefore is entirely determined at compiletime.
        if (elements().empty())
            return Determinability::COMPILETIME;

        for (auto const &e : elements())
            if (e->generate_determinability(context) == Determinability::RUNTIME)
                return Determinability::RUNTIME;

        return Determinability::COMPILETIME;
    }
    virtual void generate_code (cgen::Context &context) const override
    {
        LVD_ABORT("Not implemented");
    }

    std::vector<ElementTypePtr> const &elements () const { return m_elements; }

    // TODO: Change this to emplace_back
//     void push_back (ElementTypePtr const &element)
//     {
//         this->grow_firange(element->firange());
//         m_elements.push_back(element);
//     }
    void push_back (ElementTypePtr &&element)
    {
        this->grow_firange(element->firange());
        m_elements.push_back(std::move(element));
    }

private:

    // Note: This does not call grow_firange on the element firanges.
//     template <typename... Args_>
//     void push_back (ElementTypePtr const &first, Args_&&... rest)
//     {
//         this->grow_firange(first->firange());
//         m_elements.push_back(first);
//         this->push_back(std::forward<Args_>(rest)...);
//     }
    template <typename... Args_>
    void push_back (ElementTypePtr &&first, Args_&&... rest)
    {
        this->grow_firange(first->firange());
        m_elements.push_back(std::move(first));
        this->push_back(std::forward<Args_>(rest)...);
    }

    std::vector<ElementTypePtr> m_elements;
};

typedef Vector<TypeEnum::PARAMETER_LIST,Base,Base,','> ParameterList;
typedef Vector<TypeEnum::STATEMENT_LIST,Base,Base,';'> StatementList;

typedef Vector<TypeEnum::DECLARATION_TUPLE,Declaration,Base,','> DeclarationTuple;
typedef Vector<TypeEnum::IDENTIFIER_TUPLE,Identifier,Base,','> IdentifierTuple;

// This used to be defined in Vector.cpp, but when building shared libs, the definition
// was apparently not found by the dynamic linker, and instead the "Not implemented"
// default definition was called.  Putting it here in the header fixed that problem.
template <>
inline void StatementList::generate_code (cgen::Context &context) const
{
    for (auto const &statement : elements())
        statement->generate_code(context);
}

struct Tuple : public Vector<TypeEnum::TUPLE,Base,Base,','>
{
    template <typename... Args_>
    Tuple (Args_&&... args) : Vector<TypeEnum::TUPLE,Base,Base,','>(std::forward<Args_>(args)...) { }
    virtual ~Tuple () { }

    virtual Tuple *cloned () const override;
    // TODO: override print ?

    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override;
    virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_rvalue (cgen::Context &context) const override;
};

// Base class for type arrays, type tuples, and structs.
struct TypeAggregate : public TypeBase
{
    template <typename... Args_>
    TypeAggregate (Args_&&... args) : TypeBase(std::forward<Args_>(args)...) { }
    virtual ~TypeAggregate () { }

    virtual TypeAggregate *cloned () const override = 0;

    // Returns the number of elements in this aggregate.
    virtual uint64_t length () const { LVD_ABORT_WITH_FIRANGE("TypeAggregate::length not implemented", firange()); }
    // Returns the given element.  Argument must be in the range [0,length()).
    virtual TypeBase const &element (uint64_t i) const { LVD_ABORT_WITH_FIRANGE("TypeAggregate::element not implemented", firange()); }
//     // Returns false iff there exist two distinct elements.  This means that an empty aggregate IS uniform.
//     virtual bool is_uniform () const { LVD_ABORT_WITH_FIRANGE("TypeAggregate::is_uniform not implemented", firange()); }
    // Returns false iff there exist two distinct elements.  This means that an empty aggregate IS uniform.
    virtual bool has_uniform_element_type () const { LVD_ABORT_WITH_FIRANGE("TypeAggregate::has_uniform_element_type not implemented", firange()); }
    // If length() > 0 and is_uniform() is true, then this returns the uniform element type.  Otherwise throws.
    virtual TypeBase const &uniform_element_type () const { LVD_ABORT_WITH_FIRANGE("TypeAggregate::uniform_element_type not implemented", firange()); }

    // If this is a TypeTuple that has_uniform_element_type, it will return the corresponding TypeArray.
    // Otherwise, this will return itself.
    nnup<TypeAggregate> regularized () const;
};

// These will go away once BinaryOperation handles type constructions.
nnup<TypeAggregate> operator * (TypeAggregate const &lhs, Sint64Value const &rhs);
nnup<TypeAggregate> operator * (Sint64Value const &lhs, TypeAggregate const &rhs);

// This will go away once Tuple handles everything
struct TypeTuple : public Vector<TypeEnum::TYPE_TUPLE,TypeBase,TypeAggregate,','>
{
    template <typename... Args_>
    TypeTuple (Args_&&... args)
    :   Vector<TypeEnum::TYPE_TUPLE,TypeBase,TypeAggregate,','>(std::forward<Args_>(args)...)
    { }
    virtual ~TypeTuple () { }

    virtual TypeTuple *cloned () const override;
    // TODO: override print?

    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override { return ExpressionKind::TYPE; }
    virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;

    virtual uint64_t length () const override;
    virtual TypeBase const &element (uint64_t i) const override;
//     virtual bool is_uniform () const override;
    virtual bool has_uniform_element_type () const override;
    virtual TypeBase const &uniform_element_type () const override;
};

template <typename... Args_>
nnup<ParameterList> make_parameter_list (Args_&&... args)
{
    return make_nnup<ParameterList>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<StatementList> make_statement_list (Args_&&... args)
{
    return make_nnup<StatementList>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<DeclarationTuple> make_declaration_tuple (Args_&&... args)
{
    return make_nnup<DeclarationTuple>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<IdentifierTuple> make_identifier_tuple (Args_&&... args)
{
    return make_nnup<IdentifierTuple>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Tuple> make_tuple (Args_&&... args)
{
    return make_nnup<Tuple>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<TypeTuple> make_type_tuple (Args_&&... args)
{
    return make_nnup<TypeTuple>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
