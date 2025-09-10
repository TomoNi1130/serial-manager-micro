#include <array>

#include "mbed.h"
#include "serial_manager.hpp"

BufferedSerial pc(USBTX, USBRX, 115200);  // PCとの通信に使用するシリアルポート

SerialManager serial(pc, LED1, BUTTON1);  // シリアルマネージャのインスタンスを作成
// SerialManager serial(pc,3,LED1,BUTTON1); //初期IDの指定
// SerialManager serial(pc, 3);  // シリアルマネージャのインスタンスを作成(ID表示機能なし)

float num = 0.0f;

int main() {
  pc.set_blocking(true);
  pc.set_baud(115200);

  while (1) {
    std::vector<float> floats = serial.received_nums;
    std::vector<bool> bools = serial.received_flags;
    serial.send_log("Hello World");  // ログ送信
    serial.send_msg({floats, bools});
    std::vector<float> new_floats = {0.0, 0.01, 111.0, 42330.0};
    std::vector<bool> new_bools = {true, false, 0, 1};
    serial.send_msg(new_floats);
    serial.send_msg(new_bools);  // 個別に分けることもできる
    // serial.send_msg(SerialMsg(floats, bools));//これでも良い
  }
  return 0;
}