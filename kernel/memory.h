// Memory map.

#ifndef KERNEL_MEMORY_H_
#define KERNEL_MEMORY_H_

#include "klib/types.h"

namespace kernel {

// 32-bit / 4KiB page directory table entry.
class PageDirectoryEntry {
 public:
  explicit PageDirectoryEntry();

  uint32 GetPageTableAddress();
  void SetPageTableAddress(uint32 page_table_address);

#define BIT_FLAG_PROPS(name)     \
  bool Get##name##Bit();         \
  void Set##name##Bit(bool bit);

  BIT_FLAG_PROPS(Present)
  BIT_FLAG_PROPS(ReadWrite)
  BIT_FLAG_PROPS(User)
  BIT_FLAG_PROPS(WriteThrough)
  BIT_FLAG_PROPS(DisableCache)
  BIT_FLAG_PROPS(Dirty)
  BIT_FLAG_PROPS(Size)
#undef BIT_FLAG_PROPS

 private:
  // Bits
  // 31 - 11: 4KiB aligned pointer to a PageTableEntry.
  // 7: (S) Size. If set, pages are 4MiB in size. Otherwise 4KiB. 4MiB also
  //        need PSE to be enabled.
  // 5: (A) Accessed. Hold if the page has been read/written to.
  // 4: (D) If set the page will not be cached. If not, could be.
  // 3: (W) Write-Through. If set write-through cache is enabled. Else
  //        write-back is enabled.
  // 2: (U) User.If set, page can be read by user space. If NOT set, can
  //        only be read by privledged code.
  // 1: (R) Read/Write. If set, page is read/write, otherwise read-only.
  // 0: (P) Present. Does the page exist in pysical memory?
  uint32 data_;
};

}  // namespace kernel

#endif  // KERNEL_MEMORY_H_
