

#include <algorithm>
#include <iostream>
#include <cassert>

#include <sys/types.h>

#include "ringbuffer.hpp"

using namespace jnk0le;

/* Variable that defines the simulated bufsize for sockets */
static unsigned bufsize = 0;

/* Emulates a socket read function */
int test_read( int fd, void * data, size_t length )
{
    size_t ret  = std::min( bufsize, unsigned(length));
    uint8_t * iter = static_cast<uint8_t*>(data);
    std::cout << "Writing " << ret << " bytes into ring buffer." << std::endl;
    for ( uint8_t i = 0; i < ret; ++i )
    {
        * iter = static_cast<uint8_t>(fd);
        iter++;
    }
    bufsize -= ret;
    return ret;
}

/* Emulates a socket write function */
int test_write( int fd, void const * data, size_t length )
{
    size_t ret = std::min( bufsize, unsigned(length));
    std::cout << "Reading " << ret << " bytes from ring buffer" << std::endl;
    bufsize -= ret;
    return ret;
}

/* Print ringbuffer ontent for debug purposes */
template<typename RB>
void print_ringbuf(RB & rb )
{
    std::cout << "{ ";
    if (!rb.isEmpty())
    {
        std::cout << *rb.at(0);
        for (unsigned i = 1; i < rb.readAvailable();++i)
        {
            std::cout << ", " << *rb.at(i);
        }
    }
    std::cout << " };" << std::endl;
}
    
int main()
{
    Ringbuffer<uint8_t, 16> buf;

    std::cout << "Write 22 elements to buffer (fills up buffer compeltely)\n";
    bufsize = 22; 
    ssize_t nwrite = writeBuff( buf, test_read, 'a' );
    std::cout << "data_buff: ";
    buf.print();
    std::cout << "Ring_Buff: ";
    print_ringbuf(buf);
    assert( nwrite == 16);

    assert( buf.readAvailable() == 16);

    std::cout << "\nRead 16 elements from buffer. Empties buffer completely\n";
    bufsize = 16; 
    ssize_t nread = readBuff( buf, test_write, 'b' );
    std::cout << "data_buff: ";
    buf.print();
    std::cout << "Ring_Buff: ";
    print_ringbuf(buf);
    assert( nread == 16);
    std::cout << "buf.readAvailable() = " << buf.readAvailable() << std::endl;
    assert( buf.readAvailable() == 0);
    assert( buf.writeAvailable() == 16);
    assert( buf.writeAvailableContinous() == 16);
    assert( buf.readAvailableContinous() == 0);

    std::cout << "\nWrite 8 bytes to fill the buffer halfway\n";
    bufsize = 8;
    nwrite = writeBuff( buf, test_read, 'c' );
    std::cout << "data_buff: ";
    buf.print();
    std::cout << "Ring_Buff: ";
    print_ringbuf(buf);
    assert( nwrite == 8);
    std::cout << "buf.readAvailable() = " << buf.readAvailable() << std::endl;

    std::cout << "\nRead 6 bytes to make space at the beginning of the data_buff array";
    std::cout << "Read from buffer once\n";
    bufsize = 6;
    nread = readBuff( buf, test_write, 'd' );
    std::cout << "data_buff: ";
    buf.print();
    std::cout << "Ring_Buff: ";
    print_ringbuf(buf);
    assert( nread == 6);
    std::cout << "buf.readAvailable() = " << buf.readAvailable() << std::endl;

    std::cout << "\nWriting 10 e's into buffer. This requires two accesses as the sequence not is continous\n";
    bufsize = 10;
    nwrite = writeBuff( buf, test_read, 'e' );
    std::cout << "data_buff: ";
    buf.print();
    std::cout << "Ring_Buff: ";
    print_ringbuf(buf);
    assert( nwrite == 10);
    std::cout << "buf.readAvailable() = " << buf.readAvailable() << std::endl;
}

    
