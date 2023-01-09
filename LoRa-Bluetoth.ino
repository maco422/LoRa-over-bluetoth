#include <SPI.h>
#include <LoRa.h>
#include <EEPROM.h>
#include "BluetoothSerial.h"
#include <Button2.h>
#include "Cipher.h"


#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    923E6

#define NAME_SIZE 20
#define CIPHER_SIZE 17
#define BUTTON_A_PIN  38


// Bluetoth
BluetoothSerial SerialBT;

// Button
Button2 buttonA = Button2(BUTTON_A_PIN);

// Encryption
Cipher * cipher = new Cipher();
char * PUBLIC_KEY = (char*)"jXn2r3u8x!A%D*G-";

String HELP = "[ INF ] Commands to use:\n\n--set_name:<YOUR NAME> (for name change)\n--get_name (for current name)\n--get_code (for current Encryption code)\n--set_code:<CODE> (For Encryption code change:15 chars)\n--set_public (set public configuration)";
bool PUBLIC_MSG = false;




String read_name() {
    String name;
    for(int i = 0; i < NAME_SIZE; ++i) {
        char val = EEPROM.read(i);
        if (val == 255) {
            break;
        }
        else {
            name += val;
        }
    }
    return name;
}



void write_name(String name) {
    for (int i = 0; i < NAME_SIZE+3; ++i) {
        EEPROM.write(i, 255);
    }
    EEPROM.commit();

    int str_len = name.length();
    for (int i = 0; i < str_len; ++i) {
            if (name.charAt(i) != 255)  {
                EEPROM.write(i, name.charAt(i));
            }
    }
    EEPROM.commit();
}



String read_cihper() {
    String code;
    for(int i = 0; i < CIPHER_SIZE; ++i) {
        char val = EEPROM.read(150+i);
        if (val == 255) {
            break;
        }
        else {
            code += val;
        }
    }
    return code;
}



void write_cipher(String code) {
    for (int i = 0; i < CIPHER_SIZE+3; ++i) {
        EEPROM.write(150+i, 255);
    }

    int str_len = code.length();
    for (int i = 0; i < str_len; ++i) {
            if (code.charAt(i) != 255)  {
                EEPROM.write(150+i, code.charAt(i));
            }
    }
    EEPROM.commit();
}



void Recieve(int packetSize) {
    String encrypted = "";
    String recived_message = "";

    for (int i = 0; i < packetSize; ++i) {
        encrypted += (char) LoRa.read();
    }

    if (PUBLIC_MSG == false) {
        String packet = cipher->decryptString(encrypted);
        String sender_username = packet.substring(packet.indexOf("<strt>")+6, packet.length());
        sender_username = sender_username.substring(0, sender_username.length()-2);
        packet = packet.substring(0, packet.indexOf("<strt>"));
        String reciever_name = packet.substring(2, packet.indexOf(":"));
        recived_message = "(" + String(LoRa.packetRssi(), DEC) + ") FROM: " + sender_username + "\nMSG: " + packet.substring(packet.indexOf(":")+1, packet.length());
    } else {
        recived_message = String(LoRa.packetRssi(), DEC) + ": " + encrypted;
    }

    SerialBT.print(recived_message.c_str());
    SerialBT.print("\n");
}



void Send() {
    String dataToSend = SerialBT.readString();
    dataToSend.replace("\n", "");
    dataToSend.replace("�", "");
    String name_stripped = read_name();

    // Set Name
    if (dataToSend.startsWith("--set_name:")) {
        String new_name = dataToSend.substring(11, dataToSend.length());
        if (new_name.length() <= NAME_SIZE +1) {
            write_name(new_name);
            String notify = "[ INF ] Recieved command to change Bluetooth name to: " + new_name;
            SerialBT.println(notify.c_str());
        }

        else {
            String notify = "[ ERR ] Recieved command to change Bluetooth name but name was too long! Max." + String(NAME_SIZE) + " characters" + "\n";
            SerialBT.print(notify.c_str());
        }
    }

    // Get Name
    else if (dataToSend.startsWith("--get_name")) {
        String myName = "[ INF ] Your name is: " + name_stripped + "\n";
        SerialBT.print(myName.c_str());
    }

    // Get Help
    else if (dataToSend.startsWith("--help")) {
        SerialBT.println("\n**************************");
        SerialBT.print(HELP.c_str());
        SerialBT.println("\n**************************");
    }


    else if (dataToSend.startsWith("--set_code:")) {
        String new_code = dataToSend.substring(11, dataToSend.length());
        String notify;
        
        char chars[new_code.length() + 1];
        new_code.toCharArray(chars, sizeof(chars));

        bool notASCII = false;
        for (int i = 0; i < sizeof(chars); i++) {
            if (chars[i] < 0 || chars[i] > 127) {
                notASCII = true;
            }
        }

        if (notASCII == true) {
            notify = "[ WARM ] Recieved command must contains only ASCII characters";
        } else {
            if (new_code.length() != 16) {
                notify = "[ WARM ] Recieved command to change Encryption code, but the length of code must be 15 ASCII characters";
            } else {
                notify = "[ INF ] Recieved command to change Encryption Code to: " + new_code + "\nCode will change after device restart.";
                write_cipher(new_code);
            }
        }
        SerialBT.println(notify.c_str());
    } else if (dataToSend.startsWith("--get_code")) {
        String current_code = read_cihper();
        String notify = "[ INF ] Encryption Code is: " + current_code + "\n";
        SerialBT.print(notify.c_str());
    } else if (dataToSend.startsWith("--set_public")) {
        PUBLIC_MSG = true;
        SerialBT.println("[ INF ] Set configuration to public LoRa. To set back to private, please restart the device.");
    }

    // Send Msg
    else {
        LoRa.beginPacket();
        if (PUBLIC_MSG == false) {
            dataToSend += "<strt>" + name_stripped;
            String cipherString = cipher->encryptString(dataToSend);
            LoRa.print(cipherString);
        } else {
            LoRa.print(dataToSend);
        }
        LoRa.endPacket();
    }
}



void buttonEvent(Button2& btn) {
    SerialBT.println("\n**************************");
    SerialBT.print(HELP.c_str());
    SerialBT.println("\n**************************");
}



void setup() {
    // Start Serial
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n");

    // Start EEPROM
    EEPROM.begin(210);
    delay(1000);


    // Set Button Handler
    buttonA.setPressedHandler(buttonEvent);

    // Set name of bluetoth
    String bluetothName = read_name();
    if (bluetothName.length() < 1) {
        bluetothName += "ESP32-" + String(random(1, 100));
        Serial.println("[ INF ] Bluetoth device name not set. Setting to:");
        Serial.println(bluetothName);
        write_name(bluetothName);
    }

    String current_cihper = read_cihper();
    if (current_cihper.length() < 1) {
        Serial.println("[ INF ] Security Code is not set. Setting to:");
        Serial.println(PUBLIC_KEY);
        write_cipher(PUBLIC_KEY);
        // Cipher key
        Serial.println("[ INF ] Setting Encryption key ...");
        cipher->setKey(PUBLIC_KEY);
    } else {
        // Cipher key
        Serial.println("[ INF ] Setting Encryption key ...");
        char* char_array = new char[current_cihper.length() + 1];
        strcpy(char_array, current_cihper.c_str());
        Serial.println(char_array);
        cipher->setKey(char_array);
    }

    // Start Bluetoth
    SerialBT.begin(bluetothName);
    delay(200);
    while (!SerialBT) {
        delay(1000);
        Serial.println("[ INF ] Waiting for Bluetoth to start...");
    }

    Serial.println("[ SUCC ] Bluetoth initialized successfully!");

    // Start Lora
    SPI.begin(SCK, MISO, MOSI, SS);
    LoRa.setPins(SS, RST, DI0);
    delay(500);
    while (!LoRa.begin(BAND)) {
        Serial.println("[ INF ] Waiting for Lora to start ...");
        delay(1000);
    }

    Serial.println("[ SUCC ] LoRa initialized successfully!");
    LoRa.receive();
    delay(1000);
}



void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        Recieve(packetSize);
    }

    if (SerialBT.available()) {
        Send();
    }
    buttonA.loop();
}