#include "mbed.h"
#include "serial_manager.hpp"

BufferedSerial pc(USBTX, USBRX, 115200);  // PCとの通信に使用するシリアルポート
DigitalIn UserButton(BUTTON1);            // ユーザーボタンのピンを定義

SerialManager serial(pc, 3);  // シリアルマネージャのインスタンスを作成

int main() {
  pc.set_blocking(true);
  pc.set_baud(115200);
  std::vector<float> sending_msg = {0.0, 1.0, 4.4, 22.7, 5555.0, 13000.05, 663.0};
  std::vector<bool> sending_flags = {true, false, true, false, true, false, true, false};
  SerialMsg serial_msg(sending_msg, sending_flags);
  while (1) {
    serial.send_msg(serial_msg);
    std::string button_log_msg;
    for (bool flag : serial.received_flags) {  // bool型の受信内容
      button_log_msg += std::to_string(flag) + " ";
    }
    serial.send_log("joys:" + std::to_string(serial.received_nums[0]) + " " + std::to_string(serial.received_nums[1]) + " " + std::to_string(serial.received_nums[2]) + " " + std::to_string(serial.received_nums[3]));
  }
  return 0;
}