// 2019.06.30 - Victor Dods

#include "sem/Type.hpp"

namespace cbz {
namespace sem {

TypeKeyword *TypeKeyword::cloned () const { return new TypeKeyword(firange()); }

void TypeKeyword::print (Log &out) const { out << "Type(" << firange() << ')'; }

template <> VoidType *VoidType::cloned () const { return new VoidType(firange()); }
template <> NullType *NullType::cloned () const { return new NullType(firange()); }
template <> Boolean *Boolean::cloned () const { return new Boolean(firange()); }
template <> Sint8 *Sint8::cloned () const { return new Sint8(firange()); }
template <> Sint16 *Sint16::cloned () const { return new Sint16(firange()); }
template <> Sint32 *Sint32::cloned () const { return new Sint32(firange()); }
template <> Sint64 *Sint64::cloned () const { return new Sint64(firange()); }
template <> Uint8 *Uint8::cloned () const { return new Uint8(firange()); }
template <> Uint16 *Uint16::cloned () const { return new Uint16(firange()); }
template <> Uint32 *Uint32::cloned () const { return new Uint32(firange()); }
template <> Uint64 *Uint64::cloned () const { return new Uint64(firange()); }
template <> Float32 *Float32::cloned () const { return new Float32(firange()); }
template <> Float64 *Float64::cloned () const { return new Float64(firange()); }
template <> TypeDummy *TypeDummy::cloned () const { return new TypeDummy(firange()); }

template <> void VoidType::print (Log &out) const { out << "VoidType(" << firange() << ')'; }
template <> void NullType::print (Log &out) const { out << "NullType(" << firange() << ')'; }
template <> void Boolean::print (Log &out) const { out << "Boolean(" << firange() << ')'; }
template <> void Sint8::print (Log &out) const { out << "Sint8(" << firange() << ')'; }
template <> void Sint16::print (Log &out) const { out << "Sint16(" << firange() << ')'; }
template <> void Sint32::print (Log &out) const { out << "Sint32(" << firange() << ')'; }
template <> void Sint64::print (Log &out) const { out << "Sint64(" << firange() << ')'; }
template <> void Uint8::print (Log &out) const { out << "Uint8(" << firange() << ')'; }
template <> void Uint16::print (Log &out) const { out << "Uint16(" << firange() << ')'; }
template <> void Uint32::print (Log &out) const { out << "Uint32(" << firange() << ')'; }
template <> void Uint64::print (Log &out) const { out << "Uint64(" << firange() << ')'; }
template <> void Float32::print (Log &out) const { out << "Float32(" << firange() << ')'; }
template <> void Float64::print (Log &out) const { out << "Float64(" << firange() << ')'; }
template <> void TypeDummy::print (Log &out) const { out << "TypeDummy(" << firange() << ')'; }

} // end namespace sem
} // end namespace cbz
