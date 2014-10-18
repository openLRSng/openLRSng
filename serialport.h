// Interrupt-driven serial transmit/receive library.
//
// Library changed slighly to fit openLRSng project by martin.collberg@gmail.com (gitsly).
// Changes made:
// - Move all code into header to have it compiling in Arduino IDE without using libraries
// - Optimize for flash space
// - Memory allocation for ringbuffers can be made outside of this class (no malloc).
//
//      Copyright (c) 2010 Michael Smith. All rights reserved.
//
// Receive and baudrate calculations derived from the Arduino
// HardwareSerial driver:
//
//      Copyright (c) 2006 Nicholas Zambetti.  All right reserved.
//
// Transmit algorithm inspired by work:
//
//      Code Jose Julio and Jordi Munoz. DIYDrones.com
//
//      This library is free software; you can redistribute it and/or
//      modify it under the terms of the GNU Lesser General Public
//      License as published by the Free Software Foundation; either
//      version 2.1 of the License, or (at your option) any later
//      version.
//
//      This library is distributed in the hope that it will be
//      useful, but WITHOUT ANY WARRANTY; without even the implied
//      warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//      PURPOSE.  See the GNU Lesser General Public License for more
//      details.
//
//      You should have received a copy of the GNU Lesser General
//      Public License along with this library; if not, write to the
//      Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//      Boston, MA 02110-1301 USA
//

//
// Note that this library does not pre-declare drivers for serial
// ports; the user must explicitly create drivers for the ports they
// wish to use.  This is less friendly than the stock Arduino driver,
// but it saves a few bytes of RAM for every unused port and frees up
// the vector for another driver (e.g. MSPIM on USARTs).
//

// Disable the stock Arduino serial driver
#ifdef HardwareSerial_h
# error Must include FastSerial.h before the Arduino serial driver is defined.
#endif
#ifndef __AVR_ATmega32U4__
#define HardwareSerial_h
#endif

#include <inttypes.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Stream.h>


/// @file	FastSerial.h
/// @brief	An enhanced version of the Arduino HardwareSerial class
///			implementing interrupt-driven transmission and flexible
///			buffer management.
///
/// Because Arduino libraries aren't really libraries, but we want to
/// only define interrupt handlers for serial ports that are actually
/// used, we have to force our users to define them using a macro.
///
/// FastSerialPort(<port name>, <port number>)
///
/// <port name> is the name of the object that will be created by the
/// macro.  <port number> is the 0-based number of the port that will
/// be managed by the object.
///
/// Previously ports were defined with a different macro for each port,
/// and these macros are retained for compatibility:
///
/// FastSerialPort0(<port name>)         creates <port name> referencing serial port 0
/// FastSerialPort1(<port name>)         creates <port name> referencing serial port 1
/// FastSerialPort2(<port name>)         creates <port name> referencing serial port 2
/// FastSerialPort3(<port name>)         creates <port name> referencing serial port 3
///
/// Note that compatibility macros are only defined for ports that
/// exist on the target device.
///

/// Transmit/receive buffer descriptor.
///
/// Public so the interrupt handlers can see it
struct RingBuffer {
  volatile uint16_t head, tail;	///< head and tail pointers
  volatile uint16_t overflow;		///< Incremented every time the buffer can't fit a character.
  uint16_t mask;					///< buffer size mask for pointer wrap
  uint8_t *bytes;					///< pointer to allocated buffer
};

#if   defined(UDR3)
# define FS_MAX_PORTS   4
#elif defined(UDR2)
# define FS_MAX_PORTS   3
#elif defined(UDR1)
# define FS_MAX_PORTS   2
#else
# define FS_MAX_PORTS   1
#endif

//#ifndef min
//#define min(a, b) (a < b ? a : b)
//#endif


RingBuffer __FastSerial__rxBuffer[FS_MAX_PORTS];
RingBuffer __FastSerial__txBuffer[FS_MAX_PORTS];
uint8_t FastSerial_serialInitialized = 0; 	/// Bit mask for initialized ports

class FastSerial: public Stream
{
public:

  /// Constructor
  __attribute__ ((noinline)) FastSerial(const uint8_t portNumber, volatile uint8_t *ubrrh, volatile uint8_t *ubrrl, volatile uint8_t *ucsra,
                                        volatile uint8_t *ucsrb, const uint8_t u2x, const uint8_t portEnableBits, const uint8_t portTxBits)  :
    _ubrrh(ubrrh),
    _ubrrl(ubrrl),
    _ucsra(ucsra),
    _ucsrb(ucsrb),
    _u2x(u2x),
    _portEnableBits(portEnableBits),
    _portTxBits(portTxBits),
    _rxBuffer(&__FastSerial__rxBuffer[portNumber]),
    _txBuffer(&__FastSerial__txBuffer[portNumber]) {
    FastSerial_serialInitialized |= (1 << portNumber);
  }

  __attribute__ ((noinline)) virtual void end(void) {
    *_ucsrb &= ~(_portEnableBits | _portTxBits);
    _open = false;
  }

  __attribute__ ((noinline)) virtual int available(void) {
    if (!_open) {
      return (-1);
    }
    return ((_rxBuffer->head - _rxBuffer->tail) & _rxBuffer->mask);
  }

  __attribute__ ((noinline)) virtual int txspace(void) {
    if (!_open) {
      return (-1);
    }
    return ((_txBuffer->mask+1) - ((_txBuffer->head - _txBuffer->tail) & _txBuffer->mask));
  }

  __attribute__ ((noinline)) virtual int read(void) {
    uint8_t c;

    // if the head and tail are equal, the buffer is empty
    if (!_open || (_rxBuffer->head == _rxBuffer->tail)) {
      return (-1);
    }

    // pull character from tail
    c = _rxBuffer->bytes[_rxBuffer->tail];
    _rxBuffer->tail = (_rxBuffer->tail + 1) & _rxBuffer->mask;

    return (c);
  }

  __attribute__ ((noinline)) virtual int peek(void) {

    // if the head and tail are equal, the buffer is empty
    if (!_open || (_rxBuffer->head == _rxBuffer->tail)) {
      return (-1);
    }

    // pull character from tail
    return (_rxBuffer->bytes[_rxBuffer->tail]);
  }

  __attribute__ ((noinline)) virtual void flush(void) {
    // don't reverse this or there may be problems if the RX interrupt
    // occurs after reading the value of _rxBuffer->head but before writing
    // the value to _rxBuffer->tail; the previous value of head
    // may be written to tail, making it appear as if the buffer
    // don't reverse this or there may be problems if the RX interrupt
    // occurs after reading the value of head but before writing
    // the value to tail; the previous value of rx_buffer_head
    // may be written to tail, making it appear as if the buffer
    // were full, not empty.
    _rxBuffer->head = _rxBuffer->tail;

    // don't reverse this or there may be problems if the TX interrupt
    // occurs after reading the value of _txBuffer->tail but before writing
    // the value to _txBuffer->head.
    _txBuffer->tail = _txBuffer->head;
  }


  __attribute__ ((noinline)) uint16_t rxOverflowCounter(void) {
    if (!_open) {
      return 0;
    }
    return _rxBuffer->overflow;
  }

  __attribute__ ((noinline)) virtual size_t write(uint8_t c) {
    uint16_t i;

    if (!_open) { // drop bytes if not open
      return 0;
    }

    // wait for room in the tx buffer
    i = (_txBuffer->head + 1) & _txBuffer->mask;

    // if the port is set into non-blocking mode, then drop the byte
    // if there isn't enough room for it in the transmit buffer
    if (_nonblocking_writes && i == _txBuffer->tail) {
      return 0;
    }

    while (i == _txBuffer->tail);

    // add byte to the buffer
    _txBuffer->bytes[_txBuffer->head] = c;
    _txBuffer->head = i;

    // enable the data-ready interrupt, as it may be off if the buffer is empty
    *_ucsrb |= _portTxBits;

    // return number of bytes written (always 1)
    return 1;
  }

  using Stream::write;

  __attribute__ ((noinline)) void setBuffers(uint8_t* rxPtr, uint16_t rxSpace, uint8_t* txPtr, uint16_t txSpace) {
    // allocate buffers
    _allocBuffer(_rxBuffer, rxPtr, rxSpace);
    _allocBuffer(_txBuffer, txPtr, txSpace);
  }

  __attribute__ ((noinline)) virtual void begin(long baud) {
    uint16_t ubrr;
    bool use_u2x = true;

    // if we are currently open...
    if (_open) {
      // close the port in its current configuration, clears _open
      end();
    }

    // reset buffer pointers
    _txBuffer->head = _txBuffer->tail = 0;
    _rxBuffer->head = _rxBuffer->tail = 0;

    // mark the port as open
    _open = true;

    // If the user has supplied a new baud rate, compute the new UBRR value.
    if (baud > 0) {
#if F_CPU == 16000000UL
      // hardcoded exception for compatibility with the bootloader shipped
      // with the Duemilanove and previous boards and the firmware on the 8U2
      // on the Uno and Mega 2560.
      if (baud == 57600) {
        use_u2x = false;
      }
#endif

      if (use_u2x) {
        *_ucsra = 1 << _u2x;
        ubrr = (F_CPU / 4 / baud - 1) / 2;
      } else {
        *_ucsra = 0;
        ubrr = (F_CPU / 8 / baud - 1) / 2;
      }

      *_ubrrh = ubrr >> 8;
      *_ubrrl = ubrr;
    }

    *_ucsrb |= _portEnableBits;
  }

  // ask for writes to be blocking or non-blocking
  void set_blocking_writes(bool blocking) {
    _nonblocking_writes = !blocking;
  }

private:

  // register accessors
  volatile uint8_t * const _ubrrh;
  volatile uint8_t * const _ubrrl;
  volatile uint8_t * const _ucsra;
  volatile uint8_t * const _ucsrb;

  // register magic numbers
  const uint8_t	_u2x;
  const uint8_t	_portEnableBits;		///< rx, tx and rx interrupt enables
  const uint8_t	_portTxBits;			///< tx data and completion interrupt enables


  // ring buffers
  RingBuffer			* const _rxBuffer;
  RingBuffer			* const _txBuffer;
  bool 			_open;

  // whether writes to the port should block waiting
  // for enough space to appear
  bool			_nonblocking_writes;

  /// Set ringbuffer pointer to buffer allocated elsewhere, and calculate mask.
  ///
  /// @param	buffer		The buffer descriptor for which the buffer will
  ///						will be allocated.
  /// @param	size		The desired buffer size. (max 512)
  ///
  __attribute__ ((noinline)) void _allocBuffer(RingBuffer *buffer, uint8_t* bufPtr, unsigned int size) {
    uint8_t i;
    for (i = 1; (1U << i) <= size; ++i);

    buffer->mask = (1U << (i - 1)) - 1;
    buffer->bytes = bufPtr;
  }

};


/// Generic Rx/Tx vectors for a serial port - needs to know magic numbers
///
#define FastSerialHandler(_PORT, _RXVECTOR, _TXVECTOR, _UDR, _UCSRB, _TXBITS) \
ISR(_RXVECTOR, ISR_BLOCK)                                               \
{                                                                       \
	uint8_t c;                                                      \
	uint16_t i;                                                      \
	\
	/* read the byte as quickly as possible */                      \
	c = _UDR;                                                       \
	/* work out where the head will go next */                      \
	i = (__FastSerial__rxBuffer[_PORT].head + 1) & __FastSerial__rxBuffer[_PORT].mask; \
	/* decide whether we have space for another byte */             \
	if (i != __FastSerial__rxBuffer[_PORT].tail) {                  \
		/* we do, move the head */                              \
		__FastSerial__rxBuffer[_PORT].bytes[__FastSerial__rxBuffer[_PORT].head] = c; \
		__FastSerial__rxBuffer[_PORT].head = i;                 \
	}                                                               \
	else															\
	{																\
		__FastSerial__rxBuffer[_PORT].overflow++;				\
	}																\
}                                                                       \
ISR(_TXVECTOR, ISR_BLOCK)                                               \
{                                                                       \
	/* if there is another character to send */                     \
	if (__FastSerial__txBuffer[_PORT].tail != __FastSerial__txBuffer[_PORT].head) { \
		_UDR = __FastSerial__txBuffer[_PORT].bytes[__FastSerial__txBuffer[_PORT].tail]; \
		/* increment the tail */                                \
		__FastSerial__txBuffer[_PORT].tail =                    \
		(__FastSerial__txBuffer[_PORT].tail + 1) & __FastSerial__txBuffer[_PORT].mask; \
		} else {                                                        \
		/* there are no more bytes to send, disable the interrupt */ \
		if (__FastSerial__txBuffer[_PORT].head == __FastSerial__txBuffer[_PORT].tail) \
		_UCSRB &= ~_TXBITS;                             \
	}                                                               \
}                                                                       \
struct hack

//
// Portability; convert various older sets of defines for U(S)ART0 up
// to match the definitions for the 1280 and later devices.
//
#if !defined(USART0_RX_vect)
# if defined(USART_RX_vect)
#  define USART0_RX_vect        USART_RX_vect
#  define USART0_UDRE_vect      USART_UDRE_vect
# elif defined(UART0_RX_vect)
#  define USART0_RX_vect        UART0_RX_vect
#  define USART0_UDRE_vect      UART0_UDRE_vect
# endif
#endif

#if !defined(USART1_RX_vect)
# if defined(UART1_RX_vect)
#  define USART1_RX_vect        UART1_RX_vect
#  define USART1_UDRE_vect      UART1_UDRE_vect
# endif
#endif

#if !defined(UDR0)
# if defined(UDR)
#  define UDR0                  UDR
#  define UBRR0H                UBRRH
#  define UBRR0L                UBRRL
#  define UCSR0A                UCSRA
#  define UCSR0B                UCSRB
#  define U2X0                  U2X
#  define RXEN0                 RXEN
#  define TXEN0                 TXEN
#  define RXCIE0                RXCIE
#  define UDRIE0                UDRIE
# endif
#endif

///
/// Macro defining a FastSerial port instance.
///
#define FastSerialPort(_name, _num)                                     \
FastSerial _name(_num,                                          \
&UBRR##_num##H,                                \
&UBRR##_num##L,                                \
&UCSR##_num##A,                                \
&UCSR##_num##B,                                \
U2X##_num,                                     \
(_BV(RXEN##_num) |  _BV(TXEN##_num) | _BV(RXCIE##_num)), \
(_BV(UDRIE##_num)));                           \
FastSerialHandler(_num,                                         \
USART##_num##_RX_vect,                        \
USART##_num##_UDRE_vect,                      \
UDR##_num,                                    \
UCSR##_num##B,                                \
_BV(UDRIE##_num))

///
/// Compatibility macros for previous FastSerial versions.
///
/// Note that these are not conditionally defined, as the errors
/// generated when using these macros for a board that does not support
/// the port are better than the errors generated for a macro that's not
/// defined at all.
///
#define FastSerialPort0(_portName)     FastSerialPort(_portName, 0)
#define FastSerialPort1(_portName)     FastSerialPort(_portName, 1)
#define FastSerialPort2(_portName)     FastSerialPort(_portName, 2)
#define FastSerialPort3(_portName)     FastSerialPort(_portName, 3)
