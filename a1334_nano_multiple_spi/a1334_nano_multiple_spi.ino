// -------------------------------------------------------------------------------------------------------- //
// Robert's Smorgasbord 2021                                                                                //
// https://robertssmorgasbord.net                                                                           //
// https://www.youtube.com/channel/UCGtReyiNPrY4RhyjClLifBA                                                 //
// Hall Effect Angle Sensors (Arduino, Allegro A1334 and SPI) – The Basics (2) https://youtu.be/E0t62Gfjvhk //
// -------------------------------------------------------------------------------------------------------- //

#include <SPI.h>

// A1334 Pins 
const byte a1334_num                = 1;      // number of a1334s
const byte a1334_cs_pins[a1334_num] = { 53 }; // a1334s' chip select pins

// A1334 Register Read/Write Commands   RSRRRRRRDDDDDDDD
//                                      WY54321076543210
const unsigned int a1334_ANG_read   = 0b0000000000000000; // Register 0x00: Angle output
const unsigned int a1334_ERR_read   = 0b0000010000000000; // Register 0x04: Error
const unsigned int a1334_CTRL_write = 0b1000100000000000; // Register 0x08: Control

// A1334 Register Read Bits            EU1110000000000P   
//                                     FF2109876543210B     Register
const unsigned int a1334_data_EF   = 0b1000000000000000; // All:   General Error Flag
const unsigned int a1334_data_UF   = 0b0100000000000000; // All:   Undervoltage Warning Flag
const unsigned int a1334_data_P    = 0b0000000000000001; // All:   Parity Bit
const unsigned int a1334_data_bits = 0b0011111111111110; // All:   Data bits
const unsigned int a1334_ERR_EEP2  = 0b0000000010000000; // Error: EEPROM Error Flag 2
const unsigned int a1334_ERR_EEP1  = 0b0000000001000000; // Error: EEPROM Error Flag 1
const unsigned int a1334_ERR_TMP   = 0b0000000000100000; // Error: Temperature Out of Range
const unsigned int a1334_ERR_IERR  = 0b0000000000001000; // Error: Internal Error
const unsigned int a1334_ERR_MAGM  = 0b0000000000000100; // Error: Target Magnet Lost
const unsigned int a1334_ERR_BATD  = 0b0000000000000010; // Error: Low Power Mode Supply Lost (Must be reset!) 

// A1334 Register Write Bits           RSRRRRRRDDDDDDDD
//                                     WY54321076543210
const unsigned int a1334_CTRL_RPM  = 0b0000000000000100; // RPM Operating Mode (Set: Low RPM; Otherwise: High RPM)
const unsigned int a1334_CTRL_ERST = 0b0000000000000001; // Error Flags Reset (Set: Reset; Otherwise: Do nothing)

// A1334 Data Position
const byte         a1334_data_shift_right = 1; 

// SPI Mode | Clock Idle | Capture Edge
// ---------|------------|-------------  
// 0        | low        | rising
// 1        | low        | falling
// 2        | high       | falling 
// 3        | high       | rising

SPISettings a1334_spi_settings(100000,     // 0.1Mhz SPI clock
                               MSBFIRST,   
                               SPI_MODE3); // SPI_MODE0 - clock idle low, capture rising edge
  
void setup() 
{
   int i;

   for (i = 0; i < a1334_num; i++)
   {
      pinMode(a1334_cs_pins[i], OUTPUT);
      digitalWrite(a1334_cs_pins[i], HIGH);
   }
   
   SPI.begin();
   Serial.begin(1000000); // 1Mbit/s
}
 
void loop()
{
   byte i;
   
   delay(250);

   for (i = 0; i < a1334_num; i++)
   { 
      Serial.print(a1334ReadAngle(a1334_cs_pins[i], true));
      Serial.print("\t");
   }

   Serial.println("");
}

float a1334ReadAngle(byte    cs_pin,
                     boolean verbose)
{
   unsigned int data;
   boolean      general_error;
   boolean      undervoltage_warning;

   if (a1334ReadRegister(cs_pin,
                         a1334_ANG_read,
                         &data,
                         &general_error,
                         &undervoltage_warning))
   {
      if (!general_error && !undervoltage_warning)
      {
         return a1334DecodeAngleRegister(data);
      }
      else
      {
         if (verbose) printDataErrors(general_error, undervoltage_warning);

         a1334ReadAndResetErrors(cs_pin, verbose);

         return NAN;
      }
   }
   else
   {
      if (verbose) Serial.println("Parity Error!");

      return NAN;
   }
}

void a1334ReadAndResetErrors(byte    cs_pin,
                             boolean verbose)
{
   unsigned int data;
   boolean      general_error;
   boolean      undervoltage_warning;

   if (a1334ReadRegister(cs_pin,
                         a1334_ERR_read,
                         &data,
                         &general_error,
                         &undervoltage_warning))
   {
      if (general_error || undervoltage_warning)
      {
         if (verbose) printDataErrors(general_error, undervoltage_warning);
      }

      if (verbose) a1334PrintErrorRegister(data);
   }
   else
   {
      if (verbose) Serial.println("Parity Error!");
   }

   if (a1334WriteRegister(cs_pin,
                          a1334_CTRL_write | a1334_CTRL_ERST,
                          &general_error,
                          &undervoltage_warning))
   {
      if (general_error || undervoltage_warning)
      {
         if (verbose) printDataErrors(general_error, undervoltage_warning);
      }                                   
   }
   else
   {
      if (verbose) Serial.println("Parity Error!");
   }
}

void printDataErrors(boolean general_error,
                     boolean undervoltage_warning)
{
   Serial.print("Data Errors:");
   Serial.print(general_error        ? " General Error |" : "               |");
   Serial.print(undervoltage_warning ? " Undervoltage  |" : "               |");
   Serial.println("");
}
                     
float a1334DecodeAngleRegister(unsigned int data)
{
   return   360.0 
          * ((data & a1334_data_bits) >> a1334_data_shift_right) 
          / ((a1334_data_bits >> a1334_data_shift_right) + 1);
}

void a1334PrintErrorRegister(unsigned int data)
{
   Serial.print("Error Register:");
   Serial.print(data & a1334_ERR_BATD ? " Supply Lost |" : "             |");
   Serial.print(data & a1334_ERR_MAGM ? " Magnet Lost |" : "             |");
   Serial.print(data & a1334_ERR_IERR ? "  Internal   |" : "             |");
   Serial.print(data & a1334_ERR_TMP  ? " Temperature |" : "             |");
   Serial.print(data & a1334_ERR_EEP1 ? "  EEPROM 1   |" : "             |");
   Serial.print(data & a1334_ERR_EEP2 ? "  EEPROM 2   |" : "             |");
   Serial.println();
}  

boolean a1334WriteRegister(byte          cs_pin,
                           unsigned int  command,
                           boolean*      general_error,
                           boolean*      undervoltage_warning)
{
   unsigned int returned_data;
   
   return a1334SPITransaction(cs_pin,
                              command,
                              &returned_data,
                              general_error,
                              undervoltage_warning);   
}

boolean a1334ReadRegister(byte          cs_pin,
                          unsigned int  command,
                          unsigned int* returned_data,
                          boolean*      general_error,
                          boolean*      undervoltage_warning)
{
   a1334SPITransaction(cs_pin,
                       command,
                       returned_data,
                       general_error,
                       undervoltage_warning);
   return a1334SPITransaction(cs_pin,
                              command,
                              returned_data,
                              general_error,
                              undervoltage_warning);   
}

boolean a1334SPITransaction(byte          cs_pin,
                            unsigned int  command,
                            unsigned int* returned_data,
                            boolean*      general_error,
                            boolean*      undervoltage_warning)
{
   SPI.beginTransaction(a1334_spi_settings);
   digitalWrite(cs_pin, LOW);

   *returned_data = SPI.transfer16(command);

   digitalWrite(cs_pin, HIGH);
   SPI.endTransaction();

   *general_error        = *returned_data & a1334_data_EF; 
   *undervoltage_warning = *returned_data & a1334_data_UF;

   return a1334ParityOK(&returned_data);
}

boolean a1334ParityOK(unsigned int data)
{
   byte bit_counted;
   byte bits_set;     

   bits_set = 0;
   
   for (bit_counted = 0; bit_counted < 16; bit_counted++)
   {
      if ((0b0000000000000001 << bit_counted) & (data & ~a1334_data_P)) bits_set++;
   }

   // uneven number of set bits -> parity bit low
   // even number of set bits -> parity bit high
   
   return (bits_set % 2 == 1) == (data & a1334_data_P == a1334_data_P);
}
