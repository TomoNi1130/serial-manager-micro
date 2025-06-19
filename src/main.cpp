#include <array>

#include "mbed.h"
#include "serial_manager.hpp"

BufferedSerial pc(USBTX, USBRX, 115200);  // PCとの通信に使用するシリアルポート

// SerialManager serial(pc, LED1, BUTTON1);  // シリアルマネージャのインスタンスを作成
// SerialManager serial(pc,3,LED1,BUTTON1); //初期IDの指定
SerialManager serial(pc, 3);  // シリアルマネージャのインスタンスを作成(ID表示機能なし)

DigitalIn sw(BUTTON1);

int main() {
  pc.set_blocking(true);
  pc.set_baud(115200);
  std::vector<float> sending_msg = {11.27, 25.7, 66.8, 44.9, 8.786};
  while (1) {
    bool now_sw = !sw;
    std::array<bool, 1> sending_flags = {now_sw};
    SerialMsg serial_msg(sending_msg, sending_flags);
    serial.send_msg(serial_msg);
    serial.send_log("log test");  // ログ送信
    ThisThread::sleep_for(500);
  }
  return 0;
}