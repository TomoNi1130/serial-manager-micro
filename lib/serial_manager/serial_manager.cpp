#include "serial_manager.hpp"

SerialManager::SerialManager(BufferedSerial& serial, uint8_t id) : men_serial(serial), serial_id(id) {
  heartbeat_thread.start(callback(this, &SerialManager::heartbeat));
  receive_thread.start(callback(this, &SerialManager::receive_msg));
}

void SerialManager::send_msg(const SerialMsg& send_msg) {
  std::vector<uint8_t> send_bytes;
  send_bytes = cobs_encode(send_msg.numbers);
  men_serial.write(send_bytes.data(), send_bytes.size());  // 小数を送信
  std::vector<uint8_t> booldata(send_msg.flags.begin(), send_msg.flags.end());
  send_bytes = cobs_encode(booldata);
  men_serial.write(send_bytes.data(), send_bytes.size());  // boolを送信
}

std::vector<uint8_t> SerialManager::cobs_string(const std::string& input_str) {
  std::vector<uint8_t> encoded;
  encoded.push_back(0xfe);
  encoded.push_back(0x00);
  uint8_t count = 0;  // 次にsource_data[i]に0x00が出るまでの配列番号をカウント
  int mark = 1;       // 最後に0x00が出たsource_data[i]の配列番号をキープ
  for (size_t i = 0; i < input_str.size(); ++i) {
    const uint8_t raw = uint8_t(input_str[i]);
    if (raw != 0x00) {
      encoded.push_back(raw);
      count++;
      if (count == 0xFF) {
        encoded[mark] = count;
        mark = encoded.size();
        encoded.push_back(0x00);
        count = 0;
      }
    } else {
      encoded[mark] = count + 1;
      mark = encoded.size();
      encoded.push_back(0x00);
      count = 0;
    }
  }
  count++;
  encoded[mark] = count;
  encoded.push_back(0x00);

  return encoded;
}
void SerialManager::send_log(const std::string& log_msg) {
  std::vector<uint8_t> encoded_msg = cobs_string(log_msg);
  men_serial.write(encoded_msg.data(), encoded_msg.size());  // ログメッセージを送信
}

void SerialManager::heartbeat() {
  while (1) {
    std::vector<uint8_t> heartbeat_msg = {0xaa, 0x05, 0x24, 0x08, 0x60, serial_id, 0x00};  // ハート
    men_serial.write(heartbeat_msg.data(), heartbeat_msg.size());                          // ハートビート
    ThisThread::sleep_for(2000ms);                                                         // 2.0秒ごとにハートビートを送信
  }
}

void SerialManager::receive_msg() {
  std::vector<uint8_t> receive_bytes;
  while (1) {
    if (men_serial.readable()) {
      uint8_t buf[1];
      men_serial.read(buf, 1);
      receive_bytes.push_back(buf[0]);
      if (buf[0] == 0x00) {
        uint8_t type_keeper = receive_bytes[0];
        receive_bytes.erase(receive_bytes.begin());
        uint8_t OBH;  // ゼロが出るまでの数
        std::vector<uint8_t> decorded_data;
        OBH = receive_bytes[0];
        for (uint8_t i = 1; i < receive_bytes.size(); i++) {
          if (i == OBH) {
            decorded_data.push_back(0);
            OBH = receive_bytes[i] + OBH;
          } else {
            decorded_data.push_back(receive_bytes[i]);
          }
        }
        decorded_data.pop_back();  // 最後の0x00を削除
        if (type_keeper == 0x01) {
          std::vector<float> results;
          for (size_t i = 0; i < decorded_data.size() / sizeof(float); i++) {
            float result;
            memcpy(&result, &decorded_data[i * sizeof(float)], sizeof(float));
            results.push_back(result);
          }
          received_msg.numbers.clear();
          received_msg.numbers.resize(results.size());
          received_msg.numbers.assign(results.begin(), results.end());
        } else if (type_keeper == 0x02) {
          received_msg.flags.clear();
          received_msg.flags.resize(decorded_data.size());
          received_msg.flags.assign(decorded_data.begin(), decorded_data.end());
        }
        receive_bytes.clear();
      }
    }
  }
}