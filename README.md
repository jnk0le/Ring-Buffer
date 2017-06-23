# AVR Optimized Ring Buffer

- provides underrun and overrun checks in insert/remove/load functions
- only 2 bytes memory footprint except actuall buffer
- atomic acces for SPSC cases without disabling interrupts
- buffer size implemented as a compile time constant to make use of AVR immediate instructions
- currently implemented as an direct replacement for LUFA ring buff.

## notes



## todo
- example code
- comapre sizes
- isr