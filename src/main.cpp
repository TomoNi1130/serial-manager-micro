#include "mbed.h"
#include "serial_manager.hpp"

BufferedSerial pc(USBTX, USBRX, 115200);
SerialManager serial(pc, 3);  // idは0以外

int main() {
  pc.set_blocking(false);
  pc.set_baud(115200);
  std::vector<uint8_t> send_msg = {0, 5, 29, 111, 222, 0, 66, 0, 44};
  std::vector<float> send_msg2 = {0.0, 1.0, 4.4, 22.7, 5555.0, 13000.05, 663.0};
  std::vector<bool> send_flags = {true, false, true, false, true, false, true, false};
  SerialMsg serial_msg(send_msg2, send_flags);
  serial.send_log("setup!!");  // ログは表示されるだけで使われない
  while (1) {
    // serial.send_msg(send_msg);//std::vector<uint8_t>,SerialMsgどちらでも送信可能
    // serial.send_msg(serial_msg);
    std::string button_log_msg;
    for (bool flag : serial.received_msg.flags) {
      button_log_msg += std::to_string(flag) + " ";
    }
    serial.send_log("buttons:" + button_log_msg);
    serial.send_log("joys:" + std::to_string(serial.received_msg.numbers[0]) + " " + std::to_string(serial.received_msg.numbers[1]) + " " + std::to_string(serial.received_msg.numbers[2]) + " " + std::to_string(serial.received_msg.numbers[3]));
    ThisThread::sleep_for(10ms);
  }
  return 0;
}