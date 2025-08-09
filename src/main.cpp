#include <array>

#include "mbed.h"
#include "serial_manager.hpp"

BufferedSerial pc(USBTX, USBRX, 115200);  // PCとの通信に使用するシリアルポート

// SerialManager serial(pc, LED1, BUTTON1);  // シリアルマネージャのインスタンスを作成
// SerialManager serial(pc,3,LED1,BUTTON1); //初期IDの指定
SerialManager serial(pc, 1);  // シリアルマネージャのインスタンスを作成(ID表示機能なし)

DigitalOut led(LED1);  // LEDの制御用
bool pre_button = false;

float num = 0;

int main() {
  pc.set_blocking(true);
  pc.set_baud(115200);
  serial.send_log("Hello World");  // ログ送信
  while (1) {
    if (serial.received_flags[0] != pre_button) {  // ボタンが押されたとき
      // serial.send_log("button pressed");
      led = serial.received_flags[0];
    }
    pre_button = serial.received_flags[0];
    std::vector<float> floats = serial.received_nums;
    std::vector<bool> bools = serial.received_flags;
    serial.send_msg({floats, bools});
    // serial.send_msg(SerialMsg(floats, bools));//これでも良い
  }
  return 0;
}