#define TINY_GSM_MODEM_SIM808

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <Wire.h>

#define startSlaveAddress 1
#define endSlaveAddress 5

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[] = "mobitel";
const char user[] = "";
const char pass[] = "";

// Use Hardware Serial on Mega, Leonardo, Micro
//#define SerialAT Serial1

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>

SoftwareSerial SerialAT(10, 11); // RX, TX

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

//mqtt broker, username and password
const char *broker = "peraride.tk";
const char *username = "dock1";
const char *passw = "peraridedock1";
const char *dockName = "DOCK1";
const int port = 1883;

const char *dockUnlockTopic = "PeraRide/unlock/dock2";
const char *redockTopic = "PeraRide/redock/lock";

long lastReconnectAttempt = 0;

void setup() {
    Wire.begin();        // join i2c bus

    // Set console baud rate
    Serial.begin(9600);
    delay(10);
    Serial.println("Master Arduino");


    // Set GSM module baud rate
    SerialAT.begin(9600);
    delay(3000);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    Serial.println("Initializing modem...");
    modem.init();

    String modemInfo = modem.getModemInfo();
    Serial.print("Modem: ");
    Serial.println(modemInfo);

    Serial.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
        Serial.println(" fail");
        while (true);
    }
    Serial.println(" OK");

    Serial.print("Connecting to ");
    Serial.print(apn);
    if (!modem.gprsConnect(apn, user, pass)) {
        Serial.println(" fail");
        while (true);
    }
    Serial.println(" OK");

    // MQTT Broker setup
    mqtt.setServer(broker, port);
    mqtt.setCallback(mqttCallback);

}

boolean mqttConnect() {
    Serial.print("Connecting to ");
    Serial.print(broker);
    if (!mqtt.connect(dockName, username, passw)) {
        Serial.println(" fail");
        return false;
    }
    Serial.println(" OK");
    mqtt.subscribe(dockUnlockTopic);
    return mqtt.connected();
}

void loop() {
    

    if (mqtt.connected()) {

      int node;
    for (node = startSlaveAddress; node <= endSlaveAddress; node++) {
        Wire.requestFrom(node, 11, false);
        int i = 0;
        char rec[11];
        while (Wire.available()) { // slave may send less than requested
            rec[i++] = Wire.read(); // receive a byte as character
        }
        rec[i] = '\0';

        //if a valid rfid recieves
        if (strcmp(rec, "nullnullnul") && strcmp(rec, "")) {
            char pubTopic[30] = "";
            sprintf(pubTopic,"%s%d",redockTopic,node);
            mqtt.publish(pubTopic, rec);
            Serial.println(pubTopic);
        }
        delay(250);
    }
      
        mqtt.loop();
    } else {
        // Reconnect every 10 seconds
        unsigned long t = millis();
        if (t - lastReconnectAttempt > 10000L) {
            lastReconnectAttempt = t;
            if (mqttConnect()) {
                lastReconnectAttempt = 0;
            }
        }
    }

}

void mqttCallback(char *topic, byte *payload, unsigned int len) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.write(payload, len);
    Serial.println();

    // unlock correspond dock
    if (String(topic) == dockUnlockTopic) {
      int lockNum = byteArrayToInt(payload, len);
      if (lockNum >= startSlaveAddress && lockNum <= endSlaveAddress){
        unlockSlave(lockNum);
      }
    }
}

void unlockSlave(int slaveNum) {
    Wire.beginTransmission(slaveNum); // transmit to device #8
    Wire.write("rel");        // sends five bytes
    Wire.endTransmission();
}

//convert byte array to int
int byteArrayToInt(byte *arr, unsigned int length) {
    arr[length] = '\0';
    String s = String((char *) arr);
    return s.toInt();
}
