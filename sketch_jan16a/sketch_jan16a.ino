#include <SPI.h>

#define SS_PIN 53
#define ANGLE_REG 0x20 // Angle register for A1335.

// Function declarations
uint16_t PrimaryRead(uint16_t cs, uint16_t address, uint16_t& value);
bool CalculateParity(uint16_t input);

void setup(void) {
    Serial.begin(9600);
    SPI.begin();
    pinMode(SS_PIN, OUTPUT);
    digitalWrite(SS_PIN, HIGH); // Ensure SS is high to begin with
}

void loop(void) {
    uint16_t angle;
    uint16_t error = PrimaryRead(SS_PIN, ANGLE_REG, angle);

    if (error == 0) { // Check if no error
        if (CalculateParity(angle)) {
            float angleInDegrees = (float)(angle & 0x0FFF) * 360.0 / 4096.0;
            Serial.print("Angle = ");
            Serial.print(angleInDegrees);
            Serial.println(" Degrees");
        } else {
            Serial.println("Parity error on Angle read");
        }
    } else {
        Serial.print("Error reading angle, code: ");
        Serial.println(error);
    }

    delay(1000); // Read every 1 second
}

uint16_t PrimaryRead(uint16_t cs, uint16_t address, uint16_t& value) {
    uint16_t command;
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
    // Combine the register address and the command into one byte
    command = ((address & 0x3F) | 0x00) << 8;
    // take the chip select low to select the device
    digitalWrite(cs, LOW);
    // send the device the register you want to read
    SPI.transfer16(command);
    digitalWrite(cs, HIGH);
    digitalWrite(cs, LOW);
    // send the command again to read the contents
    value = SPI.transfer16(command);
    // take the chip select high to de-select
    digitalWrite(cs, HIGH);
    SPI.endTransaction();
    // Check the parity
    if (CalculateParity(value)) {
        return 0; // kNOERROR
    } else {
        return 5; // kCRCERROR
    }
}

bool CalculateParity(uint16_t input) {
    uint16_t count = 0;
    for (int index = 0; index < 16; ++index) {
        if ((input & 1) == 1) {
            ++count;
        }
        input >>= 1;
    }
    return (count & 1) != 0;
}