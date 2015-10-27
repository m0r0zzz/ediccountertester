#include "Config.hpp"
#include "Serial.hpp"
#include <iostream>
#include <cstdint>
#include <cstdlib>

int main(int argc, char** argv){
    SerialPort port(COM_DEFAULT_PORT, COM_DEFAULT_BRATE, COM_DEFAULT_DATA, COM_DEFAULT_STOP, COM_DEFAULT_PARITY, COM_DEFAULT_TIMEOUT);

    port.Open();

    uint8_t in1, in2, in3[2];
    int16_t prev = 0;
    bool nFirst = false;
    while(1){
        port.Read(&in1, 1);
        if(in1 == ' ' ) continue;
        else if(in1 == 'K' || in1 == 'S'){
            port.Read(in3, 2);
            std::cout << "\nE: " << in3[0] << in3[1] << " (odd)" << endl;
        }
        else{
            port.Read(&in2, 1);
            char _tmp[] = {'0', 'x', (char)in1, (char)in2, '\0'};
            int16_t curr = std::strtoull(_tmp, NULL, 16);
            if(prev - curr == 0xFF){
                cout << "\nI: Wrap(FF -> 00)" << endl;
            }
            else if(curr - prev == 1){
                std::cout << "*";
                nFirst = true;
            }
            else{
                if(nFirst) std::cout << "\nE: " << _tmp << " (even)" << endl;
                else nFirst = true;
            }
            prev = curr;
        }
    }

}
