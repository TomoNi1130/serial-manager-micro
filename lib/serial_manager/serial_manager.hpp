#ifndef SERIAL_MANAGER_HPP
#define SERIAL_MANAGER_HPP

#include <string>
#include <thread>
#include <vector>

#include "mbed.h"
#include "stm32f4xx_hal.h"

namespace config {
const uint8_t FLOAT_HEADER = 0x01;
const uint8_t BOOL_HAEDER = 0x02;
const uint8_t LOG_HEADER = 0x09;
const uint8_t HEART_BEAT_HEADER = 0x77;
const std::vector<uint8_t> START_COM_BYTES = {0xff};                 // マイコンとの通信開始信号
const std::vector<uint8_t> INTRODUCTION_BYTES = {0x12, 0x00, 0x56};  // 自己紹介用データセット
const std::vector<uint8_t> RECORL_BYTES = {0x56, 0x34, 0x12};        // 返信用データセット
const std::vector<uint8_t> HEARTBEAT_BYTES = {0xaa, 0xbb, 0xcc};     // ハートビート用のバイト列
}  // namespace config

struct SerialMsg {
 public:
  std::vector<float> numbers;
  std::vector<bool> flags;
  SerialMsg() = default;
  template <typename... Args>
  SerialMsg(const Args&... args) {
    (assign(args), ...);  // 各引数に assign を適用
  }
  void clear() {
    numbers.clear();
    flags.clear();
  }

 private:
  template <typename T>
  void assign(const std::vector<T>& v);
  template <typename T, std::size_t S>
  void assign(const std::array<T, S>& v);
};

class SerialManager {
 public:
  SerialManager(BufferedSerial& serial, uint8_t id);
  SerialManager(BufferedSerial& serial, PinName id_show_id, PinName change_id_pin);
  SerialManager(BufferedSerial& serial, uint8_t id, PinName id_show_id, PinName change_id_pin);

  void send_msg(const SerialMsg& send_msg) {
    sending_msg = send_msg;
    ThisThread::sleep_for(5ms);
  };
  template <typename T>
  void send_msg(const std::vector<T>& send_data);
  template <typename T, std::size_t S>
  void send_msg(const std::array<T, S>& send_data);

  void send_log(const std::string& log_msg);

  int get_id() const;
  bool is_connected() const;

  std::vector<float> received_nums;
  std::vector<bool> received_flags;

 private:
  void serial_callback();
  void serial_send();
  void heart_beat();
  void show_id();
  void change_mode();

  static void save_id_to_backup(uint8_t id);
  static uint8_t load_id_from_backup();

  template <typename T>
  std::vector<uint8_t> make_msg(const std::vector<T>& input);
  std::vector<uint8_t> make_msg(const std::string& input);
  template <typename T>
  std::vector<uint8_t> cobs_encode(const std::vector<T>& input);

  enum State {
    SETUP,    // 初期化中、pcからの信号待ち
    STANBY,   // 接続待機状態、マイコンIDを送信する
    CONNECT,  // 通信状態、通信可能
  };

  enum MODE {
    SETID,
    SHOWID,
  };

  BufferedSerial& men_serial;
  static uint8_t serial_id;

  SerialMsg sending_msg;
  std::string sending_log;

  State state_;  // 状態管理
  MODE mode;
  const double WaitTimePerByte_ = 0.1;  // ms

  Thread send_msg_thread;
  Thread receive_msg_thread;
  Thread heart_beat_thread;
  Thread show_id_thread;
  Thread change_mode_thread;

  const PinName ShowIDPin;
  const PinName ChangeIDPin;
  DigitalOut led{ShowIDPin};

  Kernel::Clock::time_point last_heart_beat_time;

  bool first_msg = true;
  bool msg_sending = false;

  const uint8_t float_keeper = 0x01;  // 小数の識別子
  const uint8_t bool_keeper = 0x02;   // bool型の識別子
  const uint8_t log_keeper = 0x09;    // ログメッセージの識別子
};

// vector用
template <typename T>
void SerialMsg::assign(const std::vector<T>& v) {
  if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
    numbers.assign(v.begin(), v.end());
  } else if constexpr (std::is_same_v<T, bool>) {
    flags.assign(v.begin(), v.end());
  }
}

// array 用
template <typename T, std::size_t N>
void SerialMsg::assign(const std::array<T, N>& a) {
  if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
    numbers.assign(a.begin(), a.end());
  } else if constexpr (std::is_same_v<T, bool>) {
    flags.assign(a.begin(), a.end());
  }
}

template <typename T>
void SerialManager::send_msg(const std::vector<T>& send_data) {  // send_msg vector用
  if (!msg_sending) {
    if constexpr (std::is_same_v<T, float>)
      sending_msg.numbers = send_data;
    else if constexpr (std::is_same_v<T, double>)
      sending_msg.numbers = std::vector<float>(send_data.begin(), send_data.end());
    else if constexpr (std::is_same_v<T, bool>)
      sending_msg.flags = send_data;
    else if constexpr (std::is_same_v<T, uint8_t>)
      sending_msg.flags = std::vector<bool>(send_data.begin(), send_data.end());
    else
      return;  // サポートされていない型の場合は何もしない
  }
  ThisThread::sleep_for(1ms);
}

template <typename T, std::size_t S>
void SerialManager::send_msg(const std::array<T, S>& send_data) {  // send_msg array用
  if constexpr (std::is_same_v<T, float>)
    sending_msg.numbers = std::vector<float>(send_data.begin(), send_data.end());
  else if constexpr (std::is_same_v<T, double>)
    sending_msg.numbers = std::vector<float>(send_data.begin(), send_data.end());
  else if constexpr (std::is_same_v<T, bool>)
    sending_msg.flags = std::vector<bool>(send_data.begin(), send_data.end());
  else if constexpr (std::is_same_v<T, uint8_t>)
    sending_msg.flags = std::vector<bool>(send_data.begin(), send_data.end());
  else
    return;  // サポートされていない型の場合は何もしない
}
template <typename T>
std::vector<uint8_t> SerialManager::make_msg(const std::vector<T>& input) {
  std::vector<uint8_t> encoded;
  if constexpr (std::is_same_v<T, float>)
    encoded.push_back(config::FLOAT_HEADER);
  else if constexpr (std::is_same_v<T, double>)
    input = std::vector<float>(input.begin(), input.end()), encoded.push_back(config::FLOAT_HEADER);
  else if constexpr (std::is_same_v<T, uint8_t>)
    encoded.push_back(config::BOOL_HAEDER);
  else if constexpr (std::is_same_v<T, bool>)
    input = std::vector<uint8_t>(input.begin(), input.end()), encoded.push_back(config::BOOL_HAEDER);
  else
    encoded.push_back(0xff);
  std::vector<uint8_t> encoded_data = cobs_encode(input);
  encoded.insert(encoded.end(), encoded_data.begin(), encoded_data.end());
  return encoded;
}

template <typename T>
std::vector<uint8_t> SerialManager::cobs_encode(const std::vector<T>& input) {
  std::vector<uint8_t> encoded;
  encoded.push_back(0x00);  // プレースホルダ
  size_t mark = 0;
  uint8_t count = 1;  // code byteは1から始まる

  for (size_t i = 0; i < input.size(); ++i) {
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(&input[i]);

    for (size_t j = 0; j < sizeof(T); ++j) {
      if (raw[j] == 0x00) {
        encoded[mark] = count;
        mark = encoded.size();
        encoded.push_back(0x00);  // 新しいブロックのプレースホルダ
        count = 1;
      } else {
        encoded.push_back(raw[j]);
        ++count;

        if (count == 0xFF) {
          encoded[mark] = count;
          mark = encoded.size();
          encoded.push_back(0x00);  // 新しいブロック
          count = 1;
        }
      }
    }
  }

  // 最後のブロック処理
  encoded[mark] = count;
  encoded.push_back(0x00);  // 終端用

  return encoded;
}

#endif  // SERIAL_MANAGER_HPP