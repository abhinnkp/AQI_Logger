struct SensorValidFlags {
    bool bme680;
    bool s300e;
    bool sds011;
    bool ltr390;
    bool veml7700;
    bool gas;
};
extern SensorValidFlags sensor_valid;
#include "debug_serial.h"
#include "gas_sensor.h"
#include "esp_task_wdt.h"


extern weather_station_global_structure_t weather_data;

// ---------------- SERIAL ----------------
static HardwareSerial Gas_Sensor_Serial(2);

// ---------------- REQUEST FRAME ----------------
//static uint8_t cmdRequest[9] =  { 0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};     // Request frame (same for all sensors)
static uint8_t noiseCmd[8]   =  { 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};           // Request frame for Noise Sensor

// ---------------- MUX SELECTION FOR NOISE SENSOR ----------------
// Select Channel 6 on dual 74HC4052 MUX IC for Noise Sensor
static void selectNoiseSensor(void)
{
    digitalWrite(EN1, HIGH);
    digitalWrite(EN2, HIGH);
    delay(100);

    digitalWrite(EN2, LOW);
    digitalWrite(S0, LOW);
    digitalWrite(S1, HIGH);

    delay(200);
}


// // ---------------- MUX MAP ----------------
// // MUX IC function logic for Selection (S0, S1) according to datasheet
// static const bool S0_SELECT[7] = {0,1,0,1,0,1,0};//S0
// static const bool S1_SELECT[7] = {0,0,1,1,0,0,1};//S1

bool readNoiseFrame(void)
{
    uint8_t buffer[10] = {0};
    uint8_t index = 0;

    // Flush any pending data in UART buffer
    while(Gas_Sensor_Serial.available())
    {
        Gas_Sensor_Serial.read();
    }

    Gas_Sensor_Serial.write(noiseCmd, sizeof(noiseCmd));

    uint32_t start = millis();

    while((millis() - start) < RESPONSE_TIMEOUT)
    {
        if(Gas_Sensor_Serial.available())
        {
            buffer[index++] = Gas_Sensor_Serial.read();

            if(index >= 7)
            {
                break;
            }
        }
    }

    USBSerial.print("Noise Raw = ");

    for(uint8_t i=0;i<index;i++)
    {
        USBSerial.printf("%02X ",buffer[i]);
    }

    USBSerial.println();

    if(index < 7)
    {
        return false;
    }

    if(buffer[0] != 0x01 ||
       buffer[1] != 0x03 ||
       buffer[2] != 0x02)
    {
        return false;
    }

    uint16_t rawNoise =
        ((uint16_t)buffer[3] << 8) |
         buffer[4];

    weather_data.NS_db = rawNoise / 10.0f;
    USBSerial.printf("Noise = %.1f dB\n", weather_data.NS_db);
    sensor_valid.gas = true;

    return true;
}


// ---------------- INIT ----------------
void gas_sensor_init(void)
{
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(EN1, OUTPUT);
    pinMode(EN2, OUTPUT);

    // digitalWrite(EN1, HIGH);
    // digitalWrite(EN2, HIGH);

    selectNoiseSensor();

    Gas_Sensor_Serial.begin(9600, SERIAL_8N1, GAS_RX_PIN, GAS_TX_PIN);

    USBSerial.println("\nNoise System Ready\n");
}

// ---------------- TASK (loop logic) ----------------
void gas_sensor_task(void)
{
    selectNoiseSensor();

    if(!readNoiseFrame())
    {
        USBSerial.println("Noise Sensor -> BAD FRAME");
        sensor_valid.gas = false;
    }
}

// ---------------- PRINT ----------------
void gas_sensor_print(void)
{
    USBSerial.println("==============================================================================================================");
    USBSerial.printf("NOISE: %.1f dB\n", weather_data.NS_db);
    USBSerial.println("==============================================================================================================\n");
}
