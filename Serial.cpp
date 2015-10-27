#include "Serial.hpp"

void SerialPort::SetPort(const char* port){
    if(Query()) throw SerialPortException("Can't set port: interface is opened.", SerialPortError::EISOPEN);
    this->port = port;
}

void SerialPort::SetBaud(unsigned int baud){
    if(Query()) throw SerialPortException("Can't set baudrate: interface is opened.", SerialPortError::EISOPEN);
    baudrate = baud;
}

void SerialPort::SetData(unsigned char data){
    if(Query()) throw SerialPortException("Can't set data bits: interface is opened.", SerialPortError::EISOPEN);
    data_bits = data;
}

void SerialPort::SetStop(SerialPortStop stop){
    if(Query()) throw SerialPortException("Can't set stop bits: interface is opened.", SerialPortError::EISOPEN);
    stop_bits = stop;
}

void SerialPort::SetParity(SerialPortParity par){
    if(Query()) throw SerialPortException("Can't set parity bit: interface is opened.", SerialPortError::EISOPEN);
    parity_bits = par;
}

void SerialPort::SetTimeout(unsigned int timeout){
    if(Query()) throw SerialPortException("Can't set timeout: interface is opened.", SerialPortError::EISOPEN);
    this->timeout = timeout;
}

SerialPort::~SerialPort(){
    Close();
}


/**Platform specific functions*/
void SerialPort::Open(){

    if(opened) Close();

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    ocmtx.lock();

    /*DCB dcb = { .DCBlength = sizeof(DCB), .BaudRate = DWORD(baudrate), .ByteSize = BYTE(data_bits), .fAbortOnError = FALSE, .fDtrControl = DTR_CONTROL_DISABLE,
                .fRtsControl = RTS_CONTROL_DISABLE, .fBinary = TRUE, .fParity = TRUE, .fInX = FALSE, .fOutX = FALSE, .XonChar = 0, .XoffChar = 0xFF,
                .fErrorChar = FALSE, .fNull = FALSE, .fOutxCtsFlow = FALSE, .fOutxDsrFlow = FALSE, .XonLim = 128, .XoffLim = 128};*/

    DCB dcb = {sizeof(DCB), DWORD(baudrate), TRUE, TRUE, FALSE, FALSE, DTR_CONTROL_DISABLE, 0, 0, FALSE, FALSE, FALSE, FALSE, RTS_CONTROL_DISABLE, FALSE,
                0, 0, 128, 128, BYTE(data_bits), 0, 0, 0x00, (char)0xFF, 0, 0, 0, 0};

    switch(stop_bits){
        case SerialPortStop::One: dcb.StopBits = ONESTOPBIT;
            break;
        case SerialPortStop::OneWithHalf: dcb.StopBits = ONE5STOPBITS;
            break;
        case SerialPortStop::Two: dcb.StopBits = TWOSTOPBITS;
            break;
    }
    switch(parity_bits){
        case SerialPortParity::No: dcb.Parity = NOPARITY;
            break;
        case SerialPortParity::Even: dcb.Parity = EVENPARITY;
            break;
        case SerialPortParity::Odd: dcb.Parity = ODDPARITY;
            break;
        case SerialPortParity::Mark: dcb.Parity = MARKPARITY;
            break;
        case SerialPortParity::Space: dcb.Parity = SPACEPARITY;
            break;
    }

    /*COMMTIMEOUTS ctm = {.ReadIntervalTimeout = 0, .ReadTotalTimeoutMultiplier = 0, .ReadTotalTimeoutConstant = DWORD(timeout),
                        .WriteTotalTimeoutMultiplier = 0, .WriteTotalTimeoutConstant = DWORD(timeout)};*/

    COMMTIMEOUTS ctm = {0, 0, DWORD(timeout), 0, DWORD(timeout)};
    try{
        hnd = CreateFile(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if(hnd == (HANDLE)-1){
//            cout << GetLastError() << endl;
            throw SerialPortException("Can't open port", SerialPortError::ECANTOPEN);
        }
        SetupComm(hnd, 16384, 16384);
        if(!SetCommTimeouts(hnd, &ctm)) throw SerialPortException("Can't set timeouts", SerialPortError::ECANTSETTIMEOUTS);
        if(!SetCommState(hnd, &dcb)) throw SerialPortException("Can't set DCB", SerialPortError::ECANTSETSTATE);
        Clean();
        opened = true;
    }catch(SerialPortException& e){
        ocmtx.unlock();
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        throw e;
    }
    ocmtx.unlock();
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
}

void SerialPort::Close(){
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    ocmtx.lock();
    if(hnd != 0) CloseHandle(hnd);
    hnd = 0;
    opened = false;
    ocmtx.unlock();
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
}

void SerialPort::Clean(){
    PurgeComm(hnd, PURGE_RXABORT);
    PurgeComm(hnd, PURGE_RXCLEAR);
}

void SerialPort::Read(uint8_t* buf, unsigned int len){
    unsigned char er;
    DWORD alen;

    if(Query() == false) throw SerialPortException("Can't read port when it's closed", SerialPortError::EISCLOSED);

    er = ReadFile(hnd, buf, len, &alen, NULL);

    if( !er && !alen ) throw SerialPortException("Read timeout!",SerialPortError::EREADTIMEOUT);
    else if(!er && alen) throw SerialPortException("Partial read w/timeout!",SerialPortError::EPARTREAD);
    else if(alen < len) throw SerialPortException("Partial read!",SerialPortError::EPARTREAD);

//    cout.write((char *)buf, alen); //Debug feature, prints incoming bits
}

void SerialPort::Write(uint8_t *buf, unsigned int len){
    unsigned char er;
    DWORD alen;

    if(Query() == false) throw SerialPortException("Can't write port when it's closed", SerialPortError::EISCLOSED);

    er = WriteFile(hnd, buf, len, &alen, NULL);

    if( !er && !alen ) throw SerialPortException("Write timeout!",SerialPortError::EWRITETIMEOUT);
    else if(!er && alen) throw SerialPortException("Partial write w/timeout!",SerialPortError::EPARTWRITE);
    else if(alen < len) throw SerialPortException("Partial write!",SerialPortError::EPARTWRITE);
}


void SerialPort::SetBreak(){
    if(!Query()) throw SerialPortException("Can't set break on when it's closed", SerialPortError::EISCLOSED);
    if(!SetCommBreak(hnd))
        throw SerialPortException("Can't set break",SerialPortError::ECANTSETBREAK);
}

void SerialPort::ResetBreak(){
    if(!Query()) throw SerialPortException("Can't reset break on port when it's closed", SerialPortError::EISCLOSED);
    if(!ClearCommBreak(hnd))
        throw SerialPortException("Can't reset break",SerialPortError::ECANTRESETBREAK);
}

bool SerialPort::Query(){
    //Possibly it is ACCESS_DENIED condition
    if(GetLastError() == 5){
        opened = false;
        Close();
    }
    return opened;
}
