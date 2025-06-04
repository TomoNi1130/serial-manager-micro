#include "serial_manager.hpp"

SerialManager::SerialManager(BufferedSerial& serial, uint8_t id) : men_serial(serial), serial_id(id), state_(STANBY), ShowIDPin(LED1), ChangeIDPin(BUTTON1) {
  send_msg_thread.start(callback(this, &SerialManager::serial_send));
  receive_msg_thread.start(callback(this, &SerialManager::serial_callback));
  heart_beat_thread.start(callback(this, &SerialManager::heart_beat));
  state_ = SETUP;
}

SerialManager::SerialManager(BufferedSerial& serial, uint8_t id, PinName show_id_pin, PinName change_id_pin) : men_serial(serial), serial_id(id), state_(STANBY), ShowIDPin(show_id_pin), ChangeIDPin(change_id_pin) {
  send_msg_thread.start(callback(this, &SerialManager::serial_send));
  receive_msg_thread.start(callback(this, &SerialManager::serial_callback));
  heart_beat_thread.start(callback(this, &SerialManager::heart_beat));
  show_id_thread.start(callback(this, &SerialManager::show_id));
  change_mode_thread.start(callback(this, &SerialManager::change_mode));
  state_ = SETUP;
  mode = SHOWID;
}

void SerialManager::send_log(const std::string& log_msg) {
  sending_log = log_msg;
  ThisThread::sleep_for(10ms);
}

int SerialManager::get_id() const {
  return serial_id;
}
bool SerialManager::is_connected() const {
  return state_ == CONNECT;
}

void SerialManager::show_id() {
  while (1) {
    while (mode == SHOWID) {
      for (int i = 0; i < serial_id; i++) {
        led = true;
        ThisThread::sleep_for(150ms);
        led = false;
        ThisThread::sleep_for(150ms);
      }
      ThisThread::sleep_for(1200ms);
    }
    ThisThread::sleep_for(1000ms);
  }
}

void SerialManager::change_mode() {
  DigitalIn userbutton(ChangeIDPin, PullUp);
  bool button_pushing;
  Timer id_set_timer;
  while (1) {
    if (!button_pushing && !userbutton) {  // ボタンが押されたら
      mode = SETID;
      button_pushing = true;
      id_set_timer.reset();
      id_set_timer.start();
      uint8_t buf_id = 0;
      while (mode == SETID) {
        if (!button_pushing && !userbutton) {
          printf("%d\n", buf_id);
          button_pushing = true;
          buf_id++;
          id_set_timer.reset();
          id_set_timer.start();
        }
        if (userbutton) {
          led = false;
          button_pushing = false;
        } else {
          led = true;
        }
        id_set_timer.stop();
        if (id_set_timer.read_ms() > 1500) {
          serial_id = buf_id;
          mode = SHOWID;
          state_ = SETUP;
          id_set_timer.reset();
          led = false;
        }
        id_set_timer.start();
        ThisThread::sleep_for(100ms);
      }
    } else {
      button_pushing = false;
    }
    ThisThread::sleep_for(100ms);
  }
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
              ThisThread::sleep_for(5ms);
            }
          } else {
            if (!sending_msg.numbers.empty()) {  // 小数を送信
              send_bytes = make_msg(sending_msg.numbers);
              men_serial.write(send_bytes.data(), send_bytes.size());
              sending_msg.numbers.clear();
              ThisThread::sleep_for(5ms);
            }
            if (!sending_msg.flags.empty()) {  // boolを送信
              booldata.assign(sending_msg.flags.begin(), sending_msg.flags.end());
              send_bytes = make_msg(booldata);
              men_serial.write(send_bytes.data(), send_bytes.size());
              sending_msg.flags.clear();
              ThisThread::sleep_for(5ms);
            }
            if (!sending_log.empty()) {  // ログメッセージを送信
              encoded_msg = make_msg(sending_log);
              men_serial.write(encoded_msg.data(), encoded_msg.size());
              sending_log.clear();
              sending_log = "";
              ThisThread::sleep_for(5ms);
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
            } else if (type_keeper == config::HEART_BEAT_HEADER) {
              if (decorded_data == config::HEARTBEAT_BYTES) {
                last_heart_beat_time = Kernel::Clock::now();
              }
            }
            break;
          }
          case STANBY: {
            if (std::equal(config::RECORL_BYTES.begin(), config::RECORL_BYTES.end(), decorded_data.begin()) && decorded_data.back() == serial_id) {
              state_ = CONNECT;  // PCが存在することを確認
              first_msg = true;
            } else if (decorded_data == config::HEARTBEAT_BYTES) {
              last_heart_beat_time = Kernel::Clock::now();
            }
            break;
          }
          case SETUP: {
            if (std::equal(config::INTRODUCTION_BYTES.begin(), config::INTRODUCTION_BYTES.end(), decorded_data.begin())) {
              state_ = STANBY;  // PCが存在することを確認
            } else if (decorded_data == config::HEARTBEAT_BYTES) {
              last_heart_beat_time = Kernel::Clock::now();
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

void SerialManager::heart_beat() {
  while (1) {
    while (state_ != CONNECT) {  // 接続されるまで待機
      ThisThread::sleep_for(100ms);
      last_heart_beat_time = Kernel::Clock::now();
    }
    auto now = Kernel::Clock::now();
    if (abs(now - last_heart_beat_time) > 500ms && state_ == CONNECT) {
      state_ = SETUP;
    }
    std::vector<uint8_t> heartbeat_msg;
    heartbeat_msg.push_back(config::HEART_BEAT_HEADER);
    std::vector<uint8_t> heartbeat_data = cobs_encode(config::HEARTBEAT_BYTES);
    heartbeat_msg.insert(heartbeat_msg.end(), heartbeat_data.begin(), heartbeat_data.end());
    men_serial.write(heartbeat_msg.data(), heartbeat_msg.size());
    ThisThread::sleep_for(200ms);
  }
}
