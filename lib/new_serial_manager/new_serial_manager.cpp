#include "new_serial_manager.hpp"

SerialManager::SerialManager(BufferedSerial& serial, uint8_t id) : men_serial(serial), serial_id(id), state_(STANBY) {
  send_msg_thread.start(callback(this, &SerialManager::serial_send));
  receive_msg_thread.start(callback(this, &SerialManager::serial_callback));
  state_ = SETUP;
}

void SerialManager::send_log(const std::string& log_msg) {
  sending_log = log_msg;
  ThisThread::sleep_for(5ms);
}

int SerialManager::get_id() const {
  return serial_id;
}
bool SerialManager::is_connected() const {
  return state_ == CONNECT;
}

std::vector<uint8_t> SerialManager::make_msg(const std::string& input) {
  std::vector<uint8_t> encoded;
  encoded.push_back(config::LOG_HEADER);
  std::vector<uint8_t> encoded_data = cobs_encode(std::vector<uint8_t>(input.begin(), input.end()));
  encoded.insert(encoded.end(), encoded_data.begin(), encoded_data.end());
  return encoded;
}

void SerialManager::serial_send() {
  std::vector<uint8_t> send_bytes;
  std::vector<uint8_t> booldata;
  std::vector<uint8_t> encoded_msg;
  std::vector<uint8_t> send_id_msg;
  std::vector<uint8_t> self_introduction_msg;
  while (1) {
    if (men_serial.writable()) {
      switch (state_) {
        case CONNECT: {
          if (first_msg) {
            first_msg = false;
            for (int i = 0; i < 20; i++) {  // 5はなんとなく、適当な値
              send_bytes = cobs_encode(config::START_COM_BYTES);
              men_serial.write(send_bytes.data(), send_bytes.size());  // 通信開始の合図を送る
              ThisThread::sleep_for(10ms);
            }
          } else {
            if (!sending_msg.numbers.empty()) {  // 小数を送信
              send_bytes = make_msg(sending_msg.numbers);
              men_serial.write(send_bytes.data(), send_bytes.size());
              sending_msg.numbers.clear();
              ThisThread::sleep_for(10ms);
            }
            if (!sending_msg.flags.empty()) {  // boolを送信
              booldata.assign(sending_msg.flags.begin(), sending_msg.flags.end());
              send_bytes = make_msg(booldata);
              men_serial.write(send_bytes.data(), send_bytes.size());
              sending_msg.flags.clear();
              ThisThread::sleep_for(10ms);
            }
            if (!sending_log.empty()) {  // ログメッセージを送信
              encoded_msg = make_msg(sending_log);
              men_serial.write(encoded_msg.data(), encoded_msg.size());
              sending_log.clear();
              sending_log = "";
              ThisThread::sleep_for(10ms);
            }
          }
          break;
        }
        case STANBY: {
          send_id_msg = config::INTRODUCTION_BYTES;
          send_id_msg.push_back(serial_id);  // マイコンIDを追加
          self_introduction_msg = cobs_encode(send_id_msg);
          men_serial.write(self_introduction_msg.data(), self_introduction_msg.size());
          ThisThread::sleep_for(std::chrono::milliseconds(static_cast<int>(self_introduction_msg.size() * WaitTimePerByte_)));
          break;
        }
        case SETUP: {
          // 待機状態、PCからの信号待ち
          break;
        }
        default:
          break;
      }
    }
  }
}

void SerialManager::serial_callback() {
  std::vector<uint8_t> receive_bytes;
  std::vector<uint8_t> decorded_data;

  while (1) {
    if (men_serial.readable()) {
      uint8_t buf[1];
      men_serial.read(buf, 1);
      receive_bytes.push_back(buf[0]);

      if (buf[0] == 0x00) {
        uint8_t type_keeper = 0;
        if (state_ == CONNECT) {
          type_keeper = receive_bytes[0];  // 型識別用のデータ(使わない)
          receive_bytes.erase(receive_bytes.begin());
        }
        uint8_t OBH;  // ゼロが出るまでの数
        OBH = receive_bytes[0];
        for (uint8_t i = 1; i < receive_bytes.size(); i++) {
          if (i == OBH) {
            decorded_data.push_back(0x00);
            OBH = receive_bytes[i] + OBH;
          } else {
            decorded_data.push_back(receive_bytes[i]);
          }
        }
        decorded_data.erase(decorded_data.end() - 1);
        // デコード完了
        switch (state_) {
          case CONNECT: {
            if (type_keeper == config::FLOAT_HEADER) {
              received_nums.clear();
              for (size_t i = 0; i < decorded_data.size() / sizeof(float); i++) {
                float result;
                memcpy(&result, &decorded_data[i * sizeof(float)], sizeof(float));
                received_nums.push_back(result);
              }
            } else if (type_keeper == config::BOOL_HAEDER) {
              received_flags.clear();
              for (size_t i = 0; i < decorded_data.size(); i++)
                if (decorded_data[i] == 0x01) {
                  received_flags.push_back(true);
                } else if (decorded_data[i] == 0x00) {
                  received_flags.push_back(false);
                }
            }
            break;
          }
          case STANBY: {
            if (std::equal(config::RECORL_BYTES.begin(), config::RECORL_BYTES.end(), decorded_data.begin()) && decorded_data.back() == serial_id) {
              state_ = CONNECT;  // PCが存在することを確認
              first_msg = true;
            }
            break;
          }
          case SETUP: {
            if (std::equal(config::INTRODUCTION_BYTES.begin(), config::INTRODUCTION_BYTES.end(), decorded_data.begin())) {
              state_ = STANBY;  // PCが存在することを確認
            }
            break;
          }
          default:
            break;
        }
        receive_bytes.clear();
        decorded_data.clear();
      }
    }
  }
}
