#ifndef SERIAL_HPP_INCLUDED
#define SERIAL_HPP_INCLUDED

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include <mutex>
#include <exception>
//#include "Global.hpp"

using namespace std;

/** \brief Enumerator of errors that may rise during work with SerialPort*/
enum class SerialPortError{
    EGOOD, /**< All good */
    EISOPEN, /**< Can't work when port is open */
    EISCLOSED, /**< Can't work when port is closed */
    ECANTOPEN, /**< Can't open port */
    ECANTSETTIMEOUTS, /**< Can't set timeouts (COMMTIEOUTS) */
    ECANTSETSTATE, /**< Can't set state of port (DCB) */
    EREADTIMEOUT, /**< Got timeout when trying to read port */
    EWRITETIMEOUT,/**< Got timeout when trying to write port */
    EPARTREAD, /**< Read occurs, but with less characters than required */
    EPARTWRITE, /**< Write occurs, but with less characters than required*/
    ECANTSETBREAK, /**< Can't force port into Break mode */
    ECANTRESETBREAK /**< Can't force port out of Break mode */
};

/** \brief Exception class for work with SerialPort errors*/
class SerialPortException: public exception{
    string exc_desc;
    SerialPortError errn;
public:
    explicit SerialPortException(const string& what_arg, SerialPortError err): exc_desc(what_arg), errn(err) {};
    explicit SerialPortException(const char* what_arg, SerialPortError err): exc_desc(what_arg), errn(err) {};

    const char* what() const noexcept{ return exc_desc.c_str(); }
    SerialPortError err(){ return errn; }
};

/** \brief Enumerator of parity check types*/
enum class SerialPortParity{
    No,
    Odd,
    Even,
    Mark,
    Space
};

/** \brief Enumerator of stop bits amounts*/
enum class SerialPortStop{
    One,
    OneWithHalf,
    Two
};

/** \brief Class for platform-independent low-level work with COM ports*/
class SerialPort{
    string port; /**< Platform-dependent string that defines port we work with*/
    unsigned int baudrate; /**< Baudrate, in bps*/
    unsigned char data_bits; /**< Amount of data bits in one COM byte*/
    SerialPortStop stop_bits; /**< Stop bits amount in one transmission */
    SerialPortParity parity_bits; /**< Parity check type */
    unsigned int timeout; /**< Read/Write timeout, msec.*/

    bool opened; /**< Port status, opened(true) - closed(false) */

    HANDLE hnd; /**< Windows port handler*/
    std::mutex ocmtx;
public:
    /** \brief Default constructor, sets port's values to defaults*/
    SerialPort(): SerialPort(" ", 115200, 8, SerialPortStop::One, SerialPortParity::No, 500) {};

    /** \brief Main constructor
     *  Sets port's initial values
     * \param prt Port address
     * \param b Baudrate
     * \param d Data bits
     * \param s Stop bits
     * \param p Parity bit
     * \param t Timeout
     */
    SerialPort(const char* prt, unsigned int b, unsigned char d, SerialPortStop s, SerialPortParity p, unsigned int t): port(prt), baudrate(b), data_bits(d), stop_bits(s), parity_bits(p), timeout(t), opened(false), hnd(0), ocmtx() {};

    /** \brief Deleted copy constructor
     * We don't want our port to be able to copy
     */
    SerialPort(SerialPort& other) = delete;

    /** \brief Move constructor*/
    SerialPort(SerialPort&& other): port(other.port), baudrate(other.baudrate), data_bits(other.data_bits), stop_bits(other.stop_bits), parity_bits(other.parity_bits), timeout(other.timeout), opened(other.opened) {};

    /** \brief Set port address we are working with
     * \param p Port address
     */
    void SetPort(const char* p);

    /** \brief Set port speed
     * \param baud Baudrate
     */
    void SetBaud(unsigned int baud);

    /** \brief Set data bits (byte length)
     * \param data Byte length
     */
    void SetData(unsigned char data);

    /** \brief Set stop bits amount
     * \param stop Stop bits amount
     */
    void SetStop(SerialPortStop stop);

    /** \brief Set parity check type
     * \param par Parity check type
     */
    void SetParity(SerialPortParity par);

    /** \brief Set r/w timeout
     * \param timeout Timeout, msec.
     */
    void SetTimeout(unsigned int timeout);

    /** \brief Initialize and open port*/
    void Open();

    /** \brief Close port*/
    void Close();

    /** \brief Get AOB from port
     * \param buf Receiving buffer
     * \param len Length of AOB
     * \throw SerialPortException
     * \warning Blocks on bytes receiving, max time - timeout
     */
    void Read(uint8_t* buf, unsigned int len);

    /** \brief Write AOB to port
     * \param buf Incoming buffer
     * \param len Length of AOB
     * \throw SerialPortException
     * \warning Blocks on bytes transmission, max time - timeout
     */
    void Write(uint8_t* buf, unsigned int len);

    /** \brief Force Break condition to port */
    void SetBreak();

    /** \brief Clean Break condition from port*/
    void ResetBreak();

    /** \brief Clear RX buffer and abort RX operations*/
    void Clean();

    /** \brief Renew and get port state */
    bool Query();

    /** Non-standard destructor*/
    ~SerialPort();
};

#endif // SERIAL_HPP_INCLUDED
