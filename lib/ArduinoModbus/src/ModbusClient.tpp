#include "ModbusClient.h"
using namespace std;

template<typename T>
T ModbusClient::holdingRegisterRead(int id, int address, ByteOrder byteOrder){
  // Khai báo buffer để lưu trữ các giá trị thanh ghi đọc từ thiết bị Modbus
  uint16_t values[4];
  int numRegisters = 0;
  // Xác định số lượng thanh ghi cần đọc dựa trên kiểu dữ liệu T
  if(is_same<T, uint16_t>::value){
    numRegisters = 1;
  } else if(is_same<T, float>::value){
    numRegisters = 2;
  } else if(is_same<T, double>::value){
    numRegisters = 4;
  } else {
    // Ném ngoại lệ cho kiểu dữ liệu không được hỗ trợ
    throw runtime_error("Unsupported data type");
  }
  // Thiết lập ID của thiết bị Modbus slave cần giao tiếp
  modbus_set_slave(_mb, id);
  // Đọc số lượng thanh ghi được chỉ định bắt đầu từ địa chỉ cho trước
  if(modbus_read_registers(_mb, address, numRegisters, values) < 0){
    return -1;
  }
  // Xử lý chuyển đổi dựa trên kiểu dữ liệu T
  if(is_same<T, uint16_t>::value){
    // Đối với uint16_t, chỉ cần trả về giá trị của thanh ghi đầu tiên
    return static_cast<uint16_t>(values[0]);
  } else if (is_same<T, float>::value) {
    // Đối với float, kết hợp hai thanh ghi 16-bit thành một giá trị 32-bit
    uint32_t temp = ((uint32_t)values[0] << 16) | values[1];
    if (byteOrder == BIGEND){
      // Đối với big-endian, trả về giá trị trực tiếp dưới dạng float
      return *reinterpret_cast<float*>(&temp);
    } else if(byteOrder == LILEND){
      // Đối với little-endian, đảo ngược thứ tự byte
      temp = (temp << 24) |
            ((temp << 8) & 0x00FF0000) |
            ((temp >> 8) & 0x0000FF00) |
            (temp >> 24);
      return *reinterpret_cast<float*>(&temp);
    } else if(byteOrder == BSWAP){
      // Đối với big-endian (byte-swapped), hoán đổi byte trong mỗi khối 16-bit
      temp = ((temp << 8) & 0xFF00FF00) |
            ((temp >> 8) & 0x00FF00FF);
      return *reinterpret_cast<float*>(&temp);
    } else if(byteOrder == LSWAP){
      // Đối với little-endian (byte-swapped), đảo ngược hai khối 16-bit
      temp = (temp << 16) | (temp >> 16);
      return *reinterpret_cast<float*>(&temp);
    }
  } else if(is_same<T, double>::value){
    // Đối với double, kết hợp bốn thanh ghi 16-bit thành một giá trị 64-bit
    uint64_t temp = ((uint64_t)values[0] << 48) |
                    ((uint64_t)values[1] << 32) |
                    ((uint64_t)values[2] << 16) |
                    values[3];
    if(byteOrder == BIGEND){
    // Đối với big-endian, trả về giá trị trực tiếp dưới dạng double
      return *reinterpret_cast<double*>(&temp);
    } else if(byteOrder == LILEND){
      // Đối với little-endian, đảo ngược thứ tự byte
      temp = (temp << 56) |
            ((temp << 40) & 0x00FF000000000000) |
            ((temp << 24) & 0x0000FF0000000000) |
            ((temp << 8 ) & 0x000000FF00000000) |
            ((temp >> 8 ) & 0x00000000FF000000) |
            ((temp >> 24) & 0x0000000000FF0000) |
            ((temp >> 40) & 0x000000000000FF00) |
            (temp >> 56);
      return *reinterpret_cast<double*>(&temp);
    } else if(byteOrder == BSWAP){
      // Đối với big-endian (byte-swapped), hoán đổi byte trong mỗi khối 16-bit
      temp = ((temp << 8) & 0xFF00FF00FF00FF00) |
            ((temp >> 8) & 0x00FF00FF00FF00FF);
      return *reinterpret_cast<double*>(&temp);
    } else if(byteOrder == LSWAP){
      // Đối với little-endian (byte-swapped), đảo ngược hai khối 32-bit
      temp = (temp << 48) |
            ((temp << 16) & 0x0000FFFF00000000) |
            ((temp >> 16) & 0x00000000FFFF0000) |
            (temp >> 48);
      return *reinterpret_cast<double*>(&temp);
    }
  }
  return 0;
}