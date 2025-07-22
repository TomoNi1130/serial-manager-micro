#include <array>

#include "mbed.h"
#include "serial_manager.hpp"

BufferedSerial pc(USBTX, USBRX, 115200);  // PCとの通信に使用するシリアルポート

SerialManager serial(pc, LED1, BUTTON1);  // シリアルマネージャのインスタンスを作成
// SerialManager serial(pc,3,LED1,BUTTON1); //初期IDの指定
// SerialManager serial(pc, 3);  // シリアルマネージャのインスタンスを作成(ID表示機能なし)

int main() {
  pc.set_blocking(true);
  pc.set_baud(115200);
  std::vector<float> sending_msg = {11.2789, 25.7345, 66.867, 44.9, 3.4567, 11.2789, 25.7345, 66.867, 44.9, 3.4567, 11.2789, 25.7345, 66.867, 44.9, 3.4567, 11.2789, 25.7345, 66.867, 44.9, 3.4567, 11.2789, 25.7345, 66.867, 44.9, 3.4567, 11.2789, 25.7345, 66.867, 44.9, 3.4567, 11.2789, 25.7345, 66.867, 44.9, 3.4567, 11.2789, 25.7345, 66.867, 44.9, 3.4567};
  while (1) {
    std::string log_msg;
    for (int i = 0; i < 4; i++)
      log_msg += std::to_string(serial.received_nums[i]) + ":";
    serial.send_log(log_msg);  // ログ送信
    serial.send_msg(sending_msg);
  }
  return 0;
}