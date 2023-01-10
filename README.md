Bước 1: Cài đặt thêm phần mềm IDE Arduino: Link https://www.arduino.cc/en/software
---------------------------------
Bước 2: Add link cài đặt ESP8266 (ESP32) qua link: "http://arduino.esp8266.com/stable/package_esp8266com_index.json,https://dl.espressif.com/dl/package_esp32_index.json"
----- Cách Add:
              + Mở phần mềm IDE Arduino vừa cài đặt:
              + Click vào File->Preferences...->Tại "Additional boards manager URLs" thực hiện dán đường link trên vào và ấn "OK"
---------------------------------
Bước 3: Thực hiện cài các "Boards Manager" cần thiết (ESP8266) (Khi cài có nhiều version khác nhau, nếu cài xong chạy trương trình lỗi thì hãy kiểm tra lại phiên bản có phù hợp không)
---------------------------------
Bước 4: Tạo DataBase trên FireBase (Cách tạo và cách gửi, nhận tìm trên mạng)
=====================================================================================================================================
Đối với bài này, mình sử dụng:
- Boards Manager: ESP8266 ver 2.7.2
- Add thêm thư viện hỗ trợ thêm là: "Firebase-ESP8266" và "firebase-arduino" (2 thư viện này mình cũng để trên git này)
