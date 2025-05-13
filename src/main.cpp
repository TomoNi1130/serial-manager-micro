#include "mbed.h"
#include "serial_manager.hpp"

BufferedSerial pc(USBTX, USBRX, 115200);
SerialManager serial(pc, 2);  // idは0以外

DigitalIn UserButton(BUTTON1, PullUp);

bool button_pushing = false;

int main() {
  pc.set_baud(115200);
  pc.set_blocking(false);
  serial.send_log("setup!!");  // ログは表示されるだけで使われない
  while (1) {
    if (std::optional<SerialMsg> msg = serial.receive_msg(); msg.has_value()) {  // 通信が来たとき
      SerialMsg receive_msg = msg.value();
      serial.send_log("これがきたぜ" + std::to_string(receive_msg.flags[0]));
    }
    if (!UserButton && !button_pushing) {
      button_pushing = true;
      std::vector<float> f_v = {0.2, 0.6};  // doublreでも可
      std::vector<std::string> str_v = {"aa", "bb", "cc"};
      std::vector<bool> b_v = {true, false, false, true};
      serial.send_msg(SerialMsg(f_v, str_v, b_v));  // 順番はどれでもいい
      serial.send_log("send_msg");
    }
    if (UserButton) {
      button_pushing = false;
    }
  }
  return 0;
}