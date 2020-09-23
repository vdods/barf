// 2019.06.06 - Victor Dods

#include "barftest/sem/Vector.hpp"

#include "barftest/sem/TypeArray.hpp"

namespace barftest {
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
} // end namespace barftest
