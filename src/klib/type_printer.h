// Type for printing types.

#ifndef KLIB_TYPEPRINTER_H_
#define KLIB_TYPEPRINTER_H_

#include "klib/argaccumulator.h"
#include "klib/types.h"

namespace klib {

class IOutputFn {
 public:
  virtual void Print(char c) = 0;
  void Print(const char* msg);
};

// Wrap a function for pinting characters. Used to provide
// higher-level primitives, such as printing values in hex,
// binary, etc.
//
// Errors encountered will be printed directly to fn.
class TypePrinter {
 public:
  explicit TypePrinter(IOutputFn* out);

  void Print(Arg arg);
  void PrintHex(Arg arg);

 private:
  IOutputFn* out_;  // We do not own.
};

}  // namespace klib

#endif  // KLIB_TYPEPRINTER_H_
