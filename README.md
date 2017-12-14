# Template Ring Buffer

- C++11 and above
- no exceptions/RTTI
- designed for compile time (static) allocation and type evaluation
- atomic operation for SPSC cases without disabling interrupts
- underrun and overrun checks in insert/remove functions
- currently implemented as indexed table with `head == tail` empty state (n-1 available elements)
- highly efficient on most microcontroller architectures (eg. avr, cortex-m)

## notes

- if ring buffer is allocated on the stack (local scope) or heap, it have to be explicitly cleared before use due to empty constructor
- index_t of size less than architecture reg size (size_t) might not be most efficient (arm gcc can still generate `uxth` when not necessary)
- 8 bit architectures other than AVR are not fixed yet

## todo:
- block write/read
- insert overwrite
- add clearing constructor
- pick appropriate namespace that will not end in "using namespace"
- fix 8bit archs
- pointer based implementation