serial_managerのマイコン側の簡単な使用例

SerialMsgクラス、もしくはstd::vector<uint8_t>であれば送れる

send_log(std::string)でログが送られる
send_msg(SerialMsg) or send_msg(std::vector<uint8_t>)で送れる

受け取ったデータはreceived_msg変数に収納される