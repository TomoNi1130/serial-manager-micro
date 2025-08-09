#include <array>

#include "mbed.h"
#include "serial_manager.hpp"

BufferedSerial pc(USBTX, USBRX, 115200);  // PCとの通信に使用するシリアルポート

SerialManager serial(pc, LED1, BUTTON1);  // シリアルマネージャのインスタンスを作成
// SerialManager serial(pc,3,LED1,BUTTON1); //初期IDの指定
// SerialManager serial(pc, 3);  // シリアルマネージャのインスタンスを作成(ID表示機能なし)

float num = 0;

int main() {
  pc.set_blocking(true);
  pc.set_baud(115200);
  serial.send_log("Hello World");  // ログ送信
  while (1) {
    if (serial.received_flags[0]) {
      serial.send_log("button pressed");
    }
    std::vector<float> floats = serial.received_nums;
    std::vector<bool> bools = serial.received_flags;
    serial.send_msg({floats, bools});
    // serial.send_msg(SerialMsg(floats, bools));//これでも良い
  }
  return 0;
}