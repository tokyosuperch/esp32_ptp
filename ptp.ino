#include "WiFi.h"
#include "AsyncUDP.h"

#include "ssidpw.h"
// "ssidpw.h"でconst charの以下二変数を記載してください。
extern const char * ssid;
extern const char * password;
#include "ptp_send.c"
extern char *ptpmsg();

unsigned char srcUuid[6];

AsyncUDP udp;
 
void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed");
        while(1) {
            delay(1000);
            Serial.println("WiFi Failed");
        }
    }
    if(udp.listenMulticast(IPAddress(224,0,1,129), 319)) {
        Serial.print("UDP Listening on IP: ");
        Serial.println(WiFi.localIP());
        udp.onPacket([](AsyncUDPPacket packet) {
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
            Serial.write(packet.data(), packet.length());
            Serial.println();
            //reply to the client
            packet.printf("Got %u bytes of data", packet.length());
        });
        //Send multicast
        udp.print("Hello!");
    }
}
 
void loop()
{
    delay(1000);
    char* temp = ptpmsg();
    //Send multicast
    udp.write((const uint8_t *) temp, 124);
    Serial.write((const uint8_t *) temp, 124);
}

void printer(char* str) {
  Serial.print("Captured: ");
  Serial.println(str);
}
