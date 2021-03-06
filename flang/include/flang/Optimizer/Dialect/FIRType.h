//===-- Optimizer/Dialect/FIRType.h -- FIR types ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OPTIMIZER_DIALECT_FIRTYPE_H
#define OPTIMIZER_DIALECT_FIRTYPE_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Types.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {
class raw_ostream;
class StringRef;
template <typename>
class ArrayRef;
class hash_code;
} // namespace llvm

namespace mlir {
class DialectAsmParser;
class DialectAsmPrinter;
} // namespace mlir

namespace fir {

class FIROpsDialect;

using KindTy = int;

namespace detail {
struct BoxTypeStorage;
struct BoxCharTypeStorage;
struct BoxProcTypeStorage;
struct CharacterTypeStorage;
struct CplxTypeStorage;
struct DimsTypeStorage;
struct FieldTypeStorage;
struct HeapTypeStorage;
struct IntTypeStorage;
struct LenTypeStorage;
struct LogicalTypeStorage;
struct PointerTypeStorage;
struct RealTypeStorage;
struct RecordTypeStorage;
struct ReferenceTypeStorage;
struct SequenceTypeStorage;
struct TypeDescTypeStorage;
} // namespace detail

/// Integral identifier for all the types comprising the FIR type system
enum TypeKind {
  // The enum starts at the range reserved for this dialect.
  FIR_TYPE = mlir::Type::FIRST_FIR_TYPE,
  FIR_BOX,       // (static) descriptor
  FIR_BOXCHAR,   // CHARACTER pointer and length
  FIR_BOXPROC,   // procedure with host association
  FIR_CHARACTER, // intrinsic type
  FIR_COMPLEX,   // intrinsic type
  FIR_DERIVED,   // derived
  FIR_DIMS,
  FIR_FIELD,
  FIR_HEAP,
  FIR_INT, // intrinsic type
  FIR_LEN,
  FIR_LOGICAL, // intrinsic type
  FIR_POINTER, // POINTER attr
  FIR_REAL,    // intrinsic type
  FIR_REFERENCE,
  FIR_SEQUENCE, // DIMENSION attr
  FIR_TYPEDESC,
};

// These isa_ routines follow the precedent of llvm::isa_or_null<>

/// Is `t` any of the FIR dialect types?
bool isa_fir_type(mlir::Type t);

/// Is `t` any of the Standard dialect types?
bool isa_std_type(mlir::Type t);

/// Is `t` any of the FIR dialect or Standard dialect types?
bool isa_fir_or_std_type(mlir::Type t);

/// Is `t` a FIR dialect type that implies a memory (de)reference?
bool isa_ref_type(mlir::Type t);

/// Is `t` a FIR dialect aggregate type?
bool isa_aggregate(mlir::Type t);

/// Extract the `Type` pointed to from a FIR memory reference type. If `t` is
/// not a memory reference type, then returns a null `Type`.
mlir::Type dyn_cast_ptrEleTy(mlir::Type t);

/// Boilerplate mixin template
template <typename A, unsigned Id>
struct IntrinsicTypeMixin {
  static constexpr bool kindof(unsigned kind) { return kind == getId(); }
  static constexpr unsigned getId() { return Id; }
};

// Intrinsic types

/// Model of the Fortran CHARACTER intrinsic type, including the KIND type
/// parameter. The model does not include a LEN type parameter. A CharacterType
/// is thus the type of a single character value.
class CharacterType
    : public mlir::Type::TypeBase<CharacterType, mlir::Type,
                                  detail::CharacterTypeStorage>,
      public IntrinsicTypeMixin<CharacterType, TypeKind::FIR_CHARACTER> {
public:
  using Base::Base;
  static CharacterType get(mlir::MLIRContext *ctxt, KindTy kind);
  KindTy getFKind() const;
};

/// Model of a Fortran COMPLEX intrinsic type, including the KIND type
/// parameter. COMPLEX is a floating point type with a real and imaginary
/// member.
class CplxType : public mlir::Type::TypeBase<CplxType, mlir::Type,
                                             detail::CplxTypeStorage>,
                 public IntrinsicTypeMixin<CplxType, TypeKind::FIR_COMPLEX> {
public:
  using Base::Base;
  static CplxType get(mlir::MLIRContext *ctxt, KindTy kind);
  KindTy getFKind() const;
};

/// Model of a Fortran INTEGER intrinsic type, including the KIND type
/// parameter.
class IntType
    : public mlir::Type::TypeBase<IntType, mlir::Type, detail::IntTypeStorage>,
      public IntrinsicTypeMixin<IntType, TypeKind::FIR_INT> {
public:
  using Base::Base;
  static IntType get(mlir::MLIRContext *ctxt, KindTy kind);
  KindTy getFKind() const;
};

/// Model of a Fortran LOGICAL intrinsic type, including the KIND type
/// parameter.
class LogicalType
    : public mlir::Type::TypeBase<LogicalType, mlir::Type,
                                  detail::LogicalTypeStorage>,
      public IntrinsicTypeMixin<LogicalType, TypeKind::FIR_LOGICAL> {
public:
  using Base::Base;
  static LogicalType get(mlir::MLIRContext *ctxt, KindTy kind);
  KindTy getFKind() const;
};

/// Model of a Fortran REAL (and DOUBLE PRECISION) intrinsic type, including the
/// KIND type parameter.
class RealType : public mlir::Type::TypeBase<RealType, mlir::Type,
                                             detail::RealTypeStorage>,
                 public IntrinsicTypeMixin<RealType, TypeKind::FIR_REAL> {
public:
  using Base::Base;
  static RealType get(mlir::MLIRContext *ctxt, KindTy kind);
  KindTy getFKind() const;
};

// FIR support types

/// The type of a Fortran descriptor. Descriptors are tuples of information that
/// describe an entity being passed from a calling context. This information
/// might include (but is not limited to) whether the entity is an array, its
/// size, or what type it has.
class BoxType
    : public mlir::Type::TypeBase<BoxType, mlir::Type, detail::BoxTypeStorage> {
public:
  using Base::Base;
  static BoxType get(mlir::Type eleTy, mlir::AffineMapAttr map = {});
  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_BOX; }
  mlir::Type getEleTy() const;
  mlir::AffineMapAttr getLayoutMap() const;

  static mlir::LogicalResult
  verifyConstructionInvariants(mlir::Location, mlir::Type eleTy,
                               mlir::AffineMapAttr map);
};

/// The type of a pair that describes a CHARACTER variable. Specifically, a
/// CHARACTER consists of a reference to a buffer (the string value) and a LEN
/// type parameter (the runtime length of the buffer).
class BoxCharType : public mlir::Type::TypeBase<BoxCharType, mlir::Type,
                                                detail::BoxCharTypeStorage> {
public:
  using Base::Base;
  static BoxCharType get(mlir::MLIRContext *ctxt, KindTy kind);
  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_BOXCHAR; }
  CharacterType getEleTy() const;
};

/// The type of a pair that describes a PROCEDURE reference. Pointers to
/// internal procedures must carry an additional reference to the host's
/// variables that are referenced.
class BoxProcType : public mlir::Type::TypeBase<BoxProcType, mlir::Type,
                                                detail::BoxProcTypeStorage> {
public:
  using Base::Base;
  static BoxProcType get(mlir::Type eleTy);
  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_BOXPROC; }
  mlir::Type getEleTy() const;

  static mlir::LogicalResult verifyConstructionInvariants(mlir::Location,
                                                          mlir::Type eleTy);
};

/// The type of a runtime vector that describes triples of array dimension
/// information. A triple consists of a lower bound, upper bound, and
/// stride. Each dimension of an array entity may have an associated triple that
/// maps how elements of the array are accessed.
class DimsType : public mlir::Type::TypeBase<DimsType, mlir::Type,
                                             detail::DimsTypeStorage> {
public:
  using Base::Base;
  static DimsType get(mlir::MLIRContext *ctx, unsigned rank);
  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_DIMS; }

  /// returns -1 if the rank is unknown
  int getRank() const;
};

/// The type of a field name. Implementations may defer the layout of a Fortran
/// derived type until runtime. This implies that the runtime must be able to
/// determine the offset of fields within the entity.
class FieldType : public mlir::Type::TypeBase<FieldType, mlir::Type,
                                              detail::FieldTypeStorage> {
public:
  using Base::Base;
  static FieldType get(mlir::MLIRContext *ctxt);
  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_FIELD; }
};

/// The type of a heap pointer. Fortran entities with the ALLOCATABLE attribute
/// may be allocated on the heap at runtime. These pointers are explicitly
/// distinguished to disallow the composition of multiple levels of
/// indirection. For example, an ALLOCATABLE POINTER is invalid.
class HeapType : public mlir::Type::TypeBase<HeapType, mlir::Type,
                                             detail::HeapTypeStorage> {
public:
  using Base::Base;
  static HeapType get(mlir::Type elementType);
  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_HEAP; }

  mlir::Type getEleTy() const;

  static mlir::LogicalResult verifyConstructionInvariants(mlir::Location,
                                                          mlir::Type eleTy);
};

/// The type of a LEN parameter name. Implementations may defer the layout of a
/// Fortran derived type until runtime. This implies that the runtime must be
/// able to determine the offset of LEN type parameters related to an entity.
class LenType
    : public mlir::Type::TypeBase<LenType, mlir::Type, detail::LenTypeStorage> {
public:
  using Base::Base;
  static LenType get(mlir::MLIRContext *ctxt);
  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_LEN; }
};

/// The type of entities with the POINTER attribute.  These pointers are
/// explicitly distinguished to disallow the composition of multiple levels of
/// indirection. For example, an ALLOCATABLE POINTER is invalid.
class PointerType : public mlir::Type::TypeBase<PointerType, mlir::Type,
                                                detail::PointerTypeStorage> {
public:
  using Base::Base;
  static PointerType get(mlir::Type elementType);
  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_POINTER; }

  mlir::Type getEleTy() const;

  static mlir::LogicalResult verifyConstructionInvariants(mlir::Location,
                                                          mlir::Type eleTy);
};

/// The type of a reference to an entity in memory.
class ReferenceType
    : public mlir::Type::TypeBase<ReferenceType, mlir::Type,
                                  detail::ReferenceTypeStorage> {
public:
  using Base::Base;
  static ReferenceType get(mlir::Type elementType);
  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_REFERENCE; }

  mlir::Type getEleTy() const;

  static mlir::LogicalResult verifyConstructionInvariants(mlir::Location,
                                                          mlir::Type eleTy);
};

/// A sequence type is a multi-dimensional array of values. The sequence type
/// may have an unknown number of dimensions or the extent of dimensions may be
/// unknown. A sequence type models a Fortran array entity, giving it a type in
/// FIR. A sequence type is assumed to be stored in a column-major order, which
/// differs from LLVM IR and other dialects of MLIR.
class SequenceType : public mlir::Type::TypeBase<SequenceType, mlir::Type,
                                                 detail::SequenceTypeStorage> {
public:
  using Base::Base;
  using Extent = int64_t;
  using Shape = llvm::SmallVector<Extent, 8>;

  /// Return a sequence type with the specified shape and element type
  static SequenceType get(const Shape &shape, mlir::Type elementType,
                          mlir::AffineMapAttr map = {});

  /// The element type of this sequence
  mlir::Type getEleTy() const;

  /// The shape of the sequence. If the sequence has an unknown shape, the shape
  /// returned will be empty.
  Shape getShape() const;

  mlir::AffineMapAttr getLayoutMap() const;

  /// The number of dimensions of the sequence
  unsigned getDimension() const { return getShape().size(); }

  /// The value `-1` represents an unknown extent for a dimension
  static constexpr Extent getUnknownExtent() { return -1; }

  static bool kindof(unsigned kind) { return kind == TypeKind::FIR_SEQUENCE; }

  static mlir::LogicalResult
  verifyConstructionInvariants(mlir::Location loc, const Shape &shape,
                               mlir::Type eleTy, mlir::AffineMapAttr map);
};

bool operator==(const SequenceType::Shape &, const SequenceType::Shape &);
llvm::hash_code hash_value(const SequenceType::Extent &);
llvm::hash_code hash_value(const SequenceType::Shape &);

/// The type of a type descriptor object. The runtime may generate type
/// descriptor objects to determine the type of an entity at runtime, etc.
class TypeDescType : public mlir::Type::TypeBase<TypeDescType, mlir::Type,
                                                 detail::TypeDescTypeStorage> {
public:
  using Base::Base;
  static TypeDescType get(mlir::Type ofType);
  static constexpr bool kindof(unsigned kind) {
    return kind == TypeKind::FIR_TYPEDESC;
  }
  mlir::Type getOfTy() const;

  static mlir::LogicalResult verifyConstructionInvariants(mlir::Location,
                                                          mlir::Type ofType);
};

// Derived types

/// Model of Fortran's derived type, TYPE. The name of the TYPE includes any
/// KIND type parameters. The record includes runtime slots for LEN type
/// parameters and for data components.
class RecordType : public mlir::Type::TypeBase<RecordType, mlir::Type,
                                               detail::RecordTypeStorage> {
public:
  using Base::Base;
  using TypePair = std::pair<std::string, mlir::Type>;
  using TypeList = std::vector<TypePair>;

  llvm::StringRef getName();
  TypeList getTypeList();
  TypeList getLenParamList();

  mlir::Type getType(llvm::StringRef ident);
  mlir::Type getType(unsigned index) {
    assert(index < getNumFields());
    return getTypeList()[index].second;
  }
  unsigned getNumFields() { return getTypeList().size(); }
  unsigned getNumLenParams() { return getLenParamList().size(); }

  static RecordType get(mlir::MLIRContext *ctxt, llvm::StringRef name);
  void finalize(llvm::ArrayRef<TypePair> lenPList,
                llvm::ArrayRef<TypePair> typeList);
  static constexpr bool kindof(unsigned kind) { return kind == getId(); }
  static constexpr unsigned getId() { return TypeKind::FIR_DERIVED; }

  detail::RecordTypeStorage const *uniqueKey() const;

  static mlir::LogicalResult verifyConstructionInvariants(mlir::Location,
                                                          llvm::StringRef name);
};

mlir::Type parseFirType(FIROpsDialect *, mlir::DialectAsmParser &parser);

void printFirType(FIROpsDialect *, mlir::Type ty, mlir::DialectAsmPrinter &p);

} // namespace fir

#endif // OPTIMIZER_DIALECT_FIRTYPE_H
