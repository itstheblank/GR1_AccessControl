#include <Arduino.h>
#include <ETH.h>          // Thư viện mạng LAN (có sẵn trong core ESP32)
#include <WiFi.h>
#include <PubSubClient.h> // Thư viện MQTT
#include <SPI.h>
#include <MFRC522.h> // Thư viện đọc thẻ RFID

// ================= CẤU HÌNH CHÂN (PINS) =================
#define RST_PIN 2    // Chân Reset cho module RFID
#define SS_PIN 4     // Chân CS cho module RFID
#define SCK_PIN 14   // Chân SCK cho module RFID
#define MISO_PIN 35  // Chân MISO cho module RFID
#define MOSI_PIN 12  // Chân MOSI cho module RFID
#define RELAY_PIN 15 // Chân điều khiển khóa cửa (Relay)
#define OPTO_PIN 32   // Chân đọc trạng thái cửa (Opto) (labeled as CFG on the WT32-ETH01 board)

int lastDoorState = -1; // Biến lưu trạng thái cửa
unsigned long doorOpenTime = 0; // Lưu thời điểm mở cửa
bool isDoorOpen = false;        // Cờ đánh dấu trạng thái cửa

unsigned long lastCardReadTime = 0; // Lưu thời điểm đọc thẻ gần nhất
const unsigned long cardCooldown = 1000; // Thời gian "đóng băng" đầu đọc (1000ms = 1 giây)

// ================= KHỞI TẠO ĐỐI TƯỢNG =================
MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClient ethClient; // Dùng WiFiClient nhưng chạy trên nền cáp mạng Ethernet
PubSubClient mqtt(ethClient);

// Thông tin MQTT Broker (EMQX trên máy tính của bạn)
const char *mqtt_server = "192.168.137.1"; // Thay bằng IP máy tính của bạn
const int mqtt_port = 1883;

// ================= CÁC HÀM XỬ LÝ =================

// Hàm kết nối MQTT
void connectMQTT()
{
  while (!mqtt.connected())
  {
    Serial.print("Đang kết nối MQTT...");
    // Tạo Client ID ngẫu nhiên
    String clientId = "ESP32_GR1_";
    clientId += String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str()))
    {
      Serial.println("Thành công!");
      // Đăng ký nhận tin nhắn (Subscribe) từ Backend để mở cửa
      mqtt.subscribe("gr1/esp32/control");
    }
    else
    {
      Serial.print("Lỗi, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Thử lại sau 5 giây...");
      delay(5000);
    }
  }
}

// Hàm nhận tin nhắn điều khiển từ MQTT (khi Backend bảo mở cửa)
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Nhận lệnh từ topic: ");
  Serial.println(topic);

  // Chuyển payload thành chuỗi (String)
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  // Xử lý lệnh mở cửa
  if (String(topic) == "gr1/esp32/control" && message == "open_door") {
    Serial.println("Nhận lệnh: MỞ KHÓA CỬA!");
    
    mqtt.publish("gr1/esp32/door_status", "{\"action\": \"door_opened\"}");

    digitalWrite(RELAY_PIN, LOW);  // Rơ le nhảy "Tạch" - Cửa mở
    isDoorOpen = true;             // Bật cờ trạng thái
    doorOpenTime = millis();       // Bấm giờ ngay lúc này
  }
}

// ================= HÀM SETUP (Chạy 1 lần) =================
void setup()
{
  Serial.begin(115200);

  // 1. Khởi tạo chân ngoại vi
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);   // Mặc định khóa cửa
  pinMode(OPTO_PIN, INPUT_PULLUP); // Đọc trạng thái cửa (đóng/mở)

  // 2. Khởi tạo mạng dây (Cấu hình chuẩn cho mạch WT32-ETH01)
  // Các thông số này là bắt buộc để chip LAN8720 trên mạch hoạt động
  ETH.begin(ETH_PHY_LAN8720, 1, 23, 18, 16, ETH_CLOCK_GPIO0_IN);
  Serial.println("Đang nhận IP từ cáp mạng...");
  delay(2000);
  Serial.print("IP của ESP32: ");
  Serial.println(ETH.localIP());

  // 3. Khởi tạo RFID
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  rfid.PCD_DumpVersionToSerial(); // check version

  Serial.println("Hệ thống đã sẵn sàng quẹt thẻ!");

  // 4. Khởi tạo MQTT
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
}

// ================= HÀM LOOP (Chạy lặp lại) =================
void loop()
{
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop(); 

  // --- CƠ CHẾ TỰ ĐỘNG ĐÓNG CỬA SAU 3 GIÂY ---
  if (isDoorOpen && (millis() - doorOpenTime >= 3000)) {
    digitalWrite(RELAY_PIN, HIGH); // Rơ le nhả ra "Tạch"
    isDoorOpen = false;            // Tắt cờ
    Serial.println("Đã khóa cửa lại.");
    mqtt.publish("gr1/esp32/door_status", "{\"action\": \"door_closed\"}");
  }

  // --- ĐỌC TRẠNG THÁI OPTO NHƯ CŨ ---
  int currentDoorState = digitalRead(OPTO_PIN);
  if (currentDoorState != lastDoorState)
  {
    if (currentDoorState == LOW)
    {
      Serial.println("CẢNH BÁO: Cửa đang MỞ!");
      mqtt.publish("gr1/esp32/door_status", "{\"status\": \"open\"}");
    }
    else
    {
      Serial.println("AN TOÀN: Cửa đã ĐÓNG!");
      mqtt.publish("gr1/esp32/door_status", "{\"status\": \"closed\"}");
    }
    lastDoorState = currentDoorState;
    delay(50); // Chống dội
  }
  // ------------------------------------------

  // 2. Quét thẻ RFID liên tục (Non-blocking)
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
  {
    return; // Nếu không có thẻ thì bỏ qua, vòng lặp chạy tiếp ngay lập tức
  }

  // 3. Nếu có thẻ -> Kiểm tra xem đã hết thời gian Cooldown chưa?
  if (millis() - lastCardReadTime >= cardCooldown)
  {
    // Đọc mã UID
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++)
    {
      uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    Serial.print("Đã quét thẻ: ");
    Serial.println(uid);

    // 4. Gửi mã thẻ lên EMQX Broker
    String jsonPayload = "{\"uid\": \"" + uid + "\"}";
    mqtt.publish("gr1/esp32/rfid", jsonPayload.c_str());

    // Cập nhật lại thời điểm vừa đọc thẻ
    lastCardReadTime = millis();
  }

  // Lệnh bắt buộc: Tạm dừng thẻ hiện tại để tránh đọc lặp lại liên tục
  rfid.PICC_HaltA(); 
}