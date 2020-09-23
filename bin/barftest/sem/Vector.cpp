// 2019.06.06 - Victor Dods

#include "sem/Vector.hpp"

#include "cbz/cgen/Context.hpp"
#include "sem/TypeArray.hpp"
#include "llvm/ADT/ArrayRef.h"

namespace cbz {
namespace sem {

Tuple *Tuple::cloned () const
{
    // This effectively duplicates Vector<...>::cloned
    auto retval = new Tuple();
    retval->set_firange(firange());
    for (auto const &element : elements())
        retval->push_back(clone_of(element));
    return retval;
}

ExpressionKind Tuple::generate_expression_kind (cgen::Context &context) const
{
    // NOTE: There's no well-defined ExpressionKind for an empty tuple; it could be TYPE or VALUE.
    // This suggests that ExpressionKind::TYPE_OR_VALUE could be a valid ExpressionKind.
    //
    // There's only a well-defined ExpressionKind if there is at least one element, and all elements
    // have the same ExpressionKind.
    if (elements().empty())
        LVD_ABORT_WITH_FIRANGE("No well-defined ExpressionKind for an empty Tuple", firange());

    auto base_ek = elements()[0]->generate_expression_kind(context);
    for (auto const &e : elements())
    {
        auto e_ek = e->generate_expression_kind(context);
        if (e_ek != base_ek)
            throw ProgrammerError("May not have a Tuple containing both types and values; must be all of one or the other", firange());
    }
    return base_ek;
}

llvm::Type *Tuple::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    // TODO: If all the elements are types, then this may be the wrong implementation.

    g_log << Log::dbg() << "Tuple::generate_rvalue_type; firange() = " << firange() << '\n';
    IndentGuard ig(g_log);

    auto type_tuple = sem::make_type_tuple();
    for (auto const &element : elements())
    {
        up<TypeBase> element_abstract_type;
        element->generate_rvalue_type(context, &element_abstract_type);
        assert(element_abstract_type != nullptr);
        type_tuple->push_back(std::move(element_abstract_type));
    }
    // This will convert the TypeTuple into a TypeArray if possible.
    nnup<TypeAggregate> type_aggregate = type_tuple->regularized();

    return type_aggregate->generate_rvalue_type(context, abstract_type);
}

llvm::Value *Tuple::generate_rvalue (cgen::Context &context) const
{
    // TODO: Need to later handle VectorType vs StructType/ArrayType, since VectorType uses insertelement

    // Start with an undef of the correct type and then insertelement/insertvalue for
    // each of the elements.
    llvm::Value *value = llvm::UndefValue::get(generate_rvalue_type(context));
    unsigned i = 0; // This is what IRBuilder::CreateInsertValue calls for
    std::vector<unsigned> index_list;
    for (auto const &element : elements())
    {
        index_list.push_back(i);
        value = context.ir_builder().CreateInsertValue(value, element->generate_rvalue(context), index_list);
        index_list.clear();
        ++i;
    }

    return value;
}

nnup<TypeAggregate> TypeAggregate::regularized () const
{
    if (type_enum__raw() == TypeEnum::TYPE_TUPLE && has_uniform_element_type())
    {
//         g_log << Log::trc() << "it is the case that `type_enum__raw() == TypeEnum::TYPE_TUPLE && has_uniform_element_type()`.  Making a new TypeArray.\n";
        return make_type_array(firange(), clone_of(uniform_element_type()), length());
    }
    else
    {
//         g_log << Log::trc() << "it is not the case that `type_enum__raw() == TypeEnum::TYPE_TUPLE && has_uniform_element_type()`.  Returning a cloned of this.\n";
        return clone_of(this);
    }
}

nnup<TypeAggregate> operator * (TypeAggregate const &lhs, Sint64Value const &rhs)
{
    if (rhs.value() < 0)
        throw ProgrammerError("may not create a negatively-sized type aggregate", rhs.firange());

    if (lhs.has_uniform_element_type())
        return make_type_array(lhs.firange()+rhs.firange(), clone_of(lhs.uniform_element_type()), lhs.length()*rhs.value());

    if (lhs.length() == 0 || rhs.value() == 0)
        return make_type_tuple();

    // TODO: Should probably limit the size of a TypeTuple, since it would be very easy
    // to create a gigantic one like [T]*1000000000000 that would blow the memory.

    if (dynamic_cast<TypeTuple const *>(&lhs) == nullptr)
        LVD_ABORT_WITH_FIRANGE(LVD_LOG_FMT("If TypeAggregate doesn't has_uniform_element_type, then it must be a TypeTuple (or the subclasses of TypeAggregate have changed (offending instance is lhs = " << lhs << ')'), lhs.firange());

    TypeTuple const &lhs_as_type_tuple = static_cast<TypeTuple const &>(lhs);
    nnup<TypeTuple> retval = make_type_tuple();
    // Repeat the sequence in lhs_as_type_tuple rhs times.
    for (int64_t i = 0; i < rhs.value(); ++i)
        for (auto const &e : lhs_as_type_tuple.elements())
            retval->push_back(clone_of(e));
    return retval;
}

nnup<TypeAggregate> operator * (Sint64Value const &lhs, TypeAggregate const &rhs)
{
    // This operation is commutative.
    return rhs*lhs;
}

TypeTuple *TypeTuple::cloned () const
{
    // This effectively duplicates Vector<...>::cloned
    auto retval = new TypeTuple();
    retval->set_firange(firange());
    for (auto const &element : elements())
        retval->push_back(clone_of(element));
    return retval;
}

llvm::Type *TypeTuple::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    // TODO: If all the types are identical, make an ArrayType or a VectorType

    if (abstract_type != nullptr)
        *abstract_type = clone_of(this);

    std::vector<llvm::Type*> types;
    for (auto const &type : elements())
        types.emplace_back(type->generate_rvalue_type(context));
    bool is_packed = false; // This is the default used by llvm.  Maybe it causes the elements to be aligned.
    return llvm::StructType::get(context.llvm_context(), llvm::ArrayRef<llvm::Type*>(types), is_packed);
}

uint64_t TypeTuple::length () const
{
    return elements().size();
}

TypeBase const &TypeTuple::element (uint64_t i) const
{
    if (i >= elements().size())
        throw ProgrammerError("index out of range", firange());
    return *elements()[i];
}

// bool TypeTuple::is_uniform () const
// {
//     if (elements().empty())
//         return true;
//
//     auto const &first = elements()[0];
//     for (auto const &next : elements())
//         if (!::are_equal(first, next))
//             return false;
//
//     return true;
// }

bool TypeTuple::has_uniform_element_type () const
{
    if (elements().empty())
        return false;

    auto const &first = elements()[0];
    for (auto const &next : elements())
        if (!sem::are_equal(first, next))
            return false;

    return true;
}

TypeBase const &TypeTuple::uniform_element_type () const
{
    assert(has_uniform_element_type());
    if (elements().empty())
        LVD_ABORT_WITH_FIRANGE("an empty TypeAggregate has no well-defined uniform_element_type", firange());
    return *elements()[0];
}

} // end namespace sem
} // end namespace cbz
