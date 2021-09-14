#include <debug.h>

struct SourceLocation {
  const char* filename;
  unsigned int line;
  unsigned int column;
};

struct TypeDescriptor {
  unsigned short kind;
  unsigned short info;
  char name[1];
};

struct TypeMismatchData {
  SourceLocation loc;
  const TypeDescriptor& type;
  unsigned long Alignment;
  unsigned char TypeCheckKind;
};

struct OverflowData {
  SourceLocation loc;
  const TypeDescriptor& lhs;
};

struct FloatCastOverflowData {
  SourceLocation loc;
  const TypeDescriptor& lhs;
  const TypeDescriptor& rhs;
};

struct PointerOverflowData {
  SourceLocation loc;
};

struct UnreachableData {
  SourceLocation loc;
};

struct FunctionTypeMismatchData {
  SourceLocation loc;
  const TypeDescriptor& type;
};

struct InvalidValueData {
  SourceLocation loc;
  const TypeDescriptor& type;
};

struct NonNullReturnData {
  SourceLocation loc;
};

struct OutOfBoundsData {
  SourceLocation loc;
  const TypeDescriptor& array;
  const TypeDescriptor& index;
};

struct ShiftOutOfBoundsData {
  SourceLocation loc;
  const TypeDescriptor& lhs;
  const TypeDescriptor& rhs;
};

void kubsan_abort(const SourceLocation& loc) {
  (void)loc;
  debug("Kubsan triggered\n");
  debug("{s}:{}:{}\n", loc.filename, loc.line, loc.column);
  //while(1) {}
}

extern "C" void __ubsan_handle_add_overflow(OverflowData* Data, unsigned long lhs, unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_sub_overflow(OverflowData* Data, unsigned long lhs, unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_mul_overflow(OverflowData* Data, unsigned long lhs, unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_negate_overflow(OverflowData* Data, unsigned long val) {
  (void)val;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_divrem_overflow(OverflowData* Data, unsigned long lhs, unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_float_cast_overflow(FloatCastOverflowData *Data, unsigned long Base, unsigned long Result) {
  (void)Base;
  (void)Result;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_pointer_overflow(PointerOverflowData *Data, unsigned long Base, unsigned long Result) {
  (void)Base;
  (void)Result;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_builtin_unreachable(UnreachableData *Data) {
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_function_type_mismatch_v1(FunctionTypeMismatchData *Data, unsigned long function) {
  (void)function;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_load_invalid_value(InvalidValueData* Data, unsigned long val) {
  (void)val;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_nonnull_return_v1(NonNullReturnData *Data, SourceLocation *LocPtr) {
  (void)LocPtr;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_out_of_bounds(OutOfBoundsData* Data, unsigned long val) {
  (void)val;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_shift_out_of_bounds(ShiftOutOfBoundsData* Data, unsigned long val) {
  (void)val;
  kubsan_abort(Data->loc);
}

extern "C" void __ubsan_handle_type_mismatch_v1(TypeMismatchData *Data, unsigned long val) {
  (void)val;
  kubsan_abort(Data->loc);
}


