#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#undef DEBUG
#include <FirebaseESP8266.h>
FirebaseData FireBase;

char ssid[] = "";         //  your network SSID (name)
char pass[] = "";       // your network password

#define VERSIONNUMBER 28
#define SWARMSIZE 5 // Khích thước bầy đàn là '5' thiết bị
#define SWARMTOOOLD 30000 // Khoảng thời gian mà không kết nối thì làm mới lại

#define UPDATE_PACKET 0 // Chưa giá trị thông tin xác định cái nào là master và cái nào là slave
#define RESET_PACKET 1 // Tất cả các thiết bị yêu cầu Restert lại
#define CHANGE_PACKET 2 // Thay đổi vai trò master và slave
#define RESET_ME_PACKET 3 // Đặt lại địa chỉ IP trên 1 thiết bị cụ thể
#define DEFINE_SERVER_LOGGER_PACKET 4 // Địa chỉ mới của Raspberry Pi để "master mới" có thể gửi dữ liệu
#define LOG_TO_SERVER_PACKET 5 // Các gói tin được gửi dữ liệu từ thiết bị đến Raspberry Pi


//============================================================
boolean masterState = true;   // Đặt trạng thái thiết bị là master
int swarmClear[SWARMSIZE];
int swarmVersion[SWARMSIZE];
int swarmState[SWARMSIZE];
long swarmTimeStamp[SWARMSIZE];   // for aging
int mySwarmID = 0;
unsigned int localPort = 2910;      // Cổng để nhận các gói UDP
int swarmAddresses[SWARMSIZE];  // Địa chỉ bầy đàn
const int PACKET_SIZE = 14; // Light Update Packet
const int BUFFERSIZE = 1024;
int i;

IPAddress serverAddress = IPAddress(0, 0, 0, 0); // Địa chỉ Ip mặc định là không có ==> Lưu động
byte packetBuffer[BUFFERSIZE]; // Bộ đệm để lưu các gói gửi và nhận
WiFiUDP udp; // Một phiên bản cho phép gửi và nhận các gói dữ liệu qua UDP
IPAddress localIP;



// =============================== Hàm Setup=====================================
void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nKết nối WiFi thành công");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());
  Firebase.begin("link to FireBase Database (EX: k55KMT.firebaseio.com)", "your Secret");
  Serial.println("Bắt đầu chạy UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  for (i = 0; i < SWARMSIZE; i++)
  {
    swarmAddresses[i] = 0;
    swarmClear[i] = 0;
    swarmTimeStamp[i] = -1;
  }
  swarmClear[mySwarmID] = 0;
  swarmTimeStamp[mySwarmID] = 1;
  swarmVersion[mySwarmID] = VERSIONNUMBER;
  swarmState[mySwarmID] = masterState;
  // đặt SwarmID dựa trên địa chỉ IP
  localIP = WiFi.localIP();
  swarmAddresses[0] =  localIP[3];
  Serial.print("MySwarmID = ");
  Serial.println(mySwarmID);
}


// =============================== Hàm Loop=====================================
void loop()
{
  int secondsCount;
  int lastSecondsCount;
  lastSecondsCount = 0;
  #define LOGHOWOFTEN
  secondsCount = millis() / 100;
  delay(300); // Chờ phản hồi
  int cb = udp.parsePacket();
  if (!cb)
  {
    Serial.println("Chưa nhận được Packet nào");
    Serial.print(".");
  }
  else
  {
    // Nhận được Packet, đọc dữ liệu từ nó
    udp.read(packetBuffer, PACKET_SIZE); // Đọc Packet vào bộ nhớ đệm
    Serial.print("packetbuffer[1] =");
    Serial.println(packetBuffer[1]);
    if (packetBuffer[1] == UPDATE_PACKET)
    {
      Serial.print("Kết nối IP: #");
      Serial.println(packetBuffer[2]);
      setAndReturnMySwarmIndex(packetBuffer[2]);
      Serial.print("Gói LS nhận từ: #");
      Serial.print(packetBuffer[2]);
      Serial.print("\nTrạng thái:");
      if (packetBuffer[3] == 0)
        Serial.print("SLAVE");
      else
        Serial.print("MASTER");
      Serial.print("\nNhận PACKET gửi đến:\n");
      Serial.print(" CC:");
      packetBuffer[5] = 150;
      packetBuffer[6] = 200;
      Serial.println(packetBuffer[5]);
      Serial.print("RC: ");
      Serial.print(packetBuffer[6]);
      
      // Serial.print(" GC:");
      // Serial.print(packetBuffer[9] * 256 + packetBuffer[10]);
      // Serial.print(" BC:");
      // Serial.print(packetBuffer[11] * 256 + packetBuffer[12]);
      Serial.print(" Version = ");
      Serial.println(packetBuffer[4]);
      swarmClear[setAndReturnMySwarmIndex(packetBuffer[2])] = packetBuffer[5];
      swarmVersion[setAndReturnMySwarmIndex(packetBuffer[2])] = packetBuffer[4];
      swarmState[setAndReturnMySwarmIndex(packetBuffer[2])] = packetBuffer[3];
      swarmTimeStamp[setAndReturnMySwarmIndex(packetBuffer[2])] = millis();
      // Kiểm tra lại có phải là master hay k?
      checkAndSetIfMaster();
    }
    if (packetBuffer[1] == RESET_PACKET)
    {
      Serial.println(">>>>>>>>>Đã nhận thông tin yêu cầu Resert");
      masterState = true;
      Serial.println("Thiết bị đã trở thành Master");
      digitalWrite(0, LOW);
    }
    if (packetBuffer[1] == CHANGE_PACKET)
    {
      Serial.println(">>>>>>>>>Đã nhận yêu cầu thay đổi vai trò");
      Serial.println("Không được thực hiện");
      for (i = 0; i < PACKET_SIZE; i++)
      {
        if (i == 2)
        {
          Serial.print("LPS[");
          Serial.print(i);
          Serial.print("] = ");
          Serial.println(packetBuffer[i]);
        }
        else
        {
          Serial.print("LPS[");
          Serial.print(i);
          Serial.print("] = 0x");
          Serial.println(packetBuffer[i], HEX);
        }
      }
    }
    if (packetBuffer[1] == RESET_ME_PACKET)
    {
      Serial.println(">>>>>>>>>Nhận gói RESET_ME_PACKET");
      if (packetBuffer[2] == swarmAddresses[mySwarmID])
      {
        masterState = true;
        Serial.println("Reset Me:  I just BECAME Master");
        digitalWrite(0, LOW);
      }
      else
      {
        Serial.print("Wanted #");
        Serial.print(packetBuffer[2]);
        Serial.println(" Not me - reset ignored");
      }
    }
  }
  if (packetBuffer[1] ==  DEFINE_SERVER_LOGGER_PACKET)
  {
    Serial.println(">>>>>>>>>Nhận DEFINE_SERVER_LOGGER_PACKET");
    serverAddress = IPAddress(packetBuffer[4], packetBuffer[5], packetBuffer[6], packetBuffer[7]);
    Serial.print("Server address received: ");
    Serial.println(serverAddress);
  }
  Serial.print("MasterStatus:");
  if (masterState == true)
  {
    digitalWrite(0, LOW);
    Serial.print("MASTER");
  }
  else
  {
    digitalWrite(0, HIGH);
    Serial.print("SLAVE");
  }
  Serial.print("/cc=");
  // Serial.print(clearColor);
  Serial.print("/KS:");
  Serial.println(serverAddress);
  Serial.println("--------");
  for (i = 0; i < SWARMSIZE; i++)
  {
    Serial.print("swarmAddress[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(swarmAddresses[i]); 
  }
  Serial.println("--------");
  broadcastARandomUpdatePacket();
  sendLogToServer();
}

// Gửi yêu cầu Packet đến bầy tại địa chỉ đã cho
unsigned long sendLightUpdatePacket(IPAddress & address)
{
  memset(packetBuffer, 0, PACKET_SIZE); // Đặt tất cả các bytes trong bộ nhớ đệm thành 0
  packetBuffer[0] = 0xF0;   // StartByte
  packetBuffer[1] = UPDATE_PACKET;     // Packet Type
  packetBuffer[2] = localIP[3];     // Sending Swarm Number
  packetBuffer[3] = masterState;  // 0 = slave, 1 = master
  packetBuffer[4] = VERSIONNUMBER;  // Software Version
  packetBuffer[13] = 0x0F;  //End Byte
  udp.beginPacketMulticast(address,  localPort, WiFi.localIP());
  udp.write(packetBuffer, PACKET_SIZE);
  udp.endPacket();
}
#define MAXDELAY 500
void broadcastARandomUpdatePacket()
{
  int sendToLightSwarm = 255;
  Serial.print("Broadcast ToSwarm = ");
  Serial.print(sendToLightSwarm);
  Serial.print(" ");
  int randomDelay;
  randomDelay = random(0, MAXDELAY);
  Serial.print("Delay = ");
  Serial.print(randomDelay);
  Serial.print("ms : ");
  delay(randomDelay);
  IPAddress sendSwarmAddress(192, 168, 1, sendToLightSwarm);
  sendLightUpdatePacket(sendSwarmAddress);
}

void checkAndSetIfMaster()
{
  for (i = 0; i < SWARMSIZE; i++)
  {
    #ifdef DEBUG
    Serial.print("swarmClear[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.print(swarmClear[i]);
    Serial.print("  swarmTimeStamp[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(swarmTimeStamp[i]);
    #endif
    Serial.print("#");
    Serial.print(i);
    Serial.print("/");
    Serial.print(swarmState[i]);
    Serial.print("/");
    Serial.print(swarmVersion[i]);
    Serial.print(":");
    int howLongAgo = millis() - swarmTimeStamp[i];
    if (swarmTimeStamp[i] == 0)
    {
      Serial.print("TO ");
    }
    else if (swarmTimeStamp[i] == -1)
    {
      Serial.print("NP ");
    }
    else if (swarmTimeStamp[i] == 1)
    {
      Serial.print("ME ");
    }
    else if (howLongAgo > SWARMTOOOLD)
    {
      Serial.print("TO ");
      swarmTimeStamp[i] = 0;
      swarmClear[i] = 0;
    }
    else
    {
      Serial.print("PR ");
    }
  }
  Serial.println();
  boolean setMaster = true;
  for (i = 0; i < SWARMSIZE; i++)
  {
    if (swarmClear[mySwarmID] >= swarmClear[i])
    {
      // Trường hợp là master
      Firebase.setString(FireBase, "/Test/XacNhan", "Thực hiện gửi lên FireBase thành công");
      Firebase.setInt(FireBase, "/Test/packetBuffer_5", packetBuffer[5]);
      Firebase.setInt(FireBase, "/Test/packetBuffer_6", packetBuffer[6]);
      Firebase.setInt(FireBase, "/Test/Time", millis());
    }
    else
    {
      setMaster = false;
      break;
    }
  }
  if (setMaster == true)
  {
    if (masterState == false)
    {
      Serial.println("I just BECAME Master");
      digitalWrite(0, LOW);
    }
    masterState = true;
  }
  else
  {
    if (masterState == true)
    {
      Serial.println("I just LOST Master");
      digitalWrite(0, HIGH);
    }
    masterState = false;
  }
  swarmState[mySwarmID] = masterState;
}

int setAndReturnMySwarmIndex(int incomingID)
{
  for (i = 0; i< SWARMSIZE; i++)
  {
    if (swarmAddresses[i] == incomingID)
    {
      return i;
    } 
    else if (swarmAddresses[i] == 0)  // không có trong hệ thống, vì vậy hãy đặt nó vào
    {
    
      swarmAddresses[i] = incomingID;
      Serial.print("incomingID ");
      Serial.print(incomingID);
      Serial.print("  assigned #");
      Serial.println(i);
      return i;
    }
  }
  int oldSwarmID;
  long oldTime;
  oldTime = millis();
  for (i = 0;  i < SWARMSIZE; i++)
 {
  if (oldTime > swarmTimeStamp[i])
  {
    oldTime = swarmTimeStamp[i];
    oldSwarmID = i;
  }
 }
 swarmAddresses[oldSwarmID] = incomingID;
}

void sendLogToServer()
{
  char myBuildString[1000];
  myBuildString[0] = '\0';
  if (masterState == true)
  {
    if ((serverAddress[0] == 0) && (serverAddress[1] == 0))
    {
      return;
    }
    else
    {
      char swarmString[20];
      swarmString[0] = '\0';
      for (i = 0; i < SWARMSIZE; i++)
      {

        char stateString[5];
        stateString[0] = '\0';
        if (swarmTimeStamp[i] == 0)
        {
          strcat(stateString, "TO");
        }
        else if (swarmTimeStamp[i] == -1)
        {
          strcat(stateString, "NP");
        }
        else if (swarmTimeStamp[i] == 1)
        {
          strcat(stateString, "PR");
        }
        else
        {
          strcat(stateString, "PR");
        }
        sprintf(swarmString, " %i,%i,%i,%i,%s,%i ", i, swarmState[i], swarmVersion[i], swarmClear[i], stateString, swarmAddresses[i]);
        strcat(myBuildString, swarmString);
        if (i < SWARMSIZE - 1)
        {
          strcat(myBuildString, "|");
        }
      }
    }
    memset(packetBuffer, 0, BUFFERSIZE);
    packetBuffer[0] = 0xF0;
    packetBuffer[1] = LOG_TO_SERVER_PACKET;
    packetBuffer[2] = localIP[3];
    packetBuffer[3] = strlen(myBuildString);
    packetBuffer[4] = VERSIONNUMBER;
    for (i = 0; i < strlen(myBuildString); i++)
    {
      packetBuffer[i + 5] = myBuildString[i];
    }
    packetBuffer[i + 5] = 0x0F;
    Serial.print("Sending Log to Sever:");
    Serial.println(myBuildString);
    int packetLength;
    packetLength = i + 5 + 1;
    udp.beginPacket(serverAddress,  localPort);
    udp.write(packetBuffer, packetLength);
    udp.endPacket();
  }
}