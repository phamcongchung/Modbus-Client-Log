#include "ModbusClient.h"
using namespace std;

template<typename T>
T ModbusClient::holdingRegisterRead(int id, int address, ByteOrder byteOrder) {
// Template implementation code here
    uint16_t values[4];
    int numRegisters = 0;

    if (is_same<T, uint16_t>::value) {
        numRegisters = 1;
    } else if (is_same<T, float>::value) {
        numRegisters = 2;
    } else if (is_same<T, double>::value) {
        numRegisters = 4;
    } else {
        throw runtime_error("Unsupported data type");
    }

    modbus_set_slave(_mb, id);
  
    if (modbus_read_registers(_mb, address, numRegisters, values) < 0) {
        return -1;
    }
  
    if (is_same<T, uint16_t>::value) {
        return static_cast<uint16_t>(values[0]);
    } else if (is_same<T, float>::value) {
        uint32_t temp = ((uint32_t)values[0] << 16) | values[1];
        if (byteOrder == BIGEND) {
            return *reinterpret_cast<float*>(&temp);
        } else if (byteOrder == LILEND) {
            temp = (temp << 24) |
                   ((temp << 8) & 0x00FF0000) |
                   ((temp >> 8) & 0x0000FF00) |
                   (temp >> 24);
            return *reinterpret_cast<float*>(&temp);
        } else if (byteOrder == BSWAP) {
            temp = ((temp << 8) & 0xFF00FF00) |
                   ((temp >> 8) & 0x00FF00FF);
            return *reinterpret_cast<float*>(&temp);
        } else if (byteOrder == LSWAP) {
            temp = (temp << 16) | (temp >> 16);
            return *reinterpret_cast<float*>(&temp);
        }
    } else if (is_same<T, double>::value) {
        uint64_t temp = ((uint64_t)values[0] << 48) |
                        ((uint64_t)values[1] << 32) |
                        ((uint64_t)values[2] << 16) |
                        values[3];
        if (byteOrder == BIGEND) {
            return *reinterpret_cast<double*>(&temp);
        } else if (byteOrder == LILEND) {
            temp = (temp << 56) |
                   ((temp << 40) & 0x00FF000000000000) |
                   ((temp << 24) & 0x0000FF0000000000) |
                   ((temp << 8 ) & 0x000000FF00000000) |
                   ((temp >> 8 ) & 0x00000000FF000000) |
                   ((temp >> 24) & 0x0000000000FF0000) |
                   ((temp >> 40) & 0x000000000000FF00) |
                   (temp >> 56);
            return *reinterpret_cast<double*>(&temp);
        } else if (byteOrder == BSWAP) {
            temp = ((temp << 8) & 0xFF00FF00FF00FF00) |
                   ((temp >> 8) & 0x00FF00FF00FF00FF);
            return *reinterpret_cast<double*>(&temp);
        } else if (byteOrder == LSWAP) {
            temp = (temp << 48) |
                   ((temp << 16) & 0x0000FFFF00000000) |
                   ((temp >> 16) & 0x00000000FFFF0000) |
                   (temp >> 48);
            return *reinterpret_cast<double*>(&temp);
        }
    }
    return 0;
}