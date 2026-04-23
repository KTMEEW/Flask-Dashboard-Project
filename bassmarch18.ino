/*Information
 * The code is written by Curious Scientist
 * https://curiousscientist.tech
 * 
 * Playlist for more ADS1256-related videos
 * https://www.youtube.com/playlist?list=PLaeIi4Gbl1T-RpVNM8uKdiV1G_3t5jCIu
 * 
 * If you use the code, please subscribe to my channel
 * https://www.youtube.com/c/CuriousScientist?sub_confirmation=1
 * 
 * I also accept donations
 * https://www.paypal.com/donate/?hosted_button_id=53CNKJLRHAYGQ
 * 
 * The code belongs to the following video
 * https://youtu.be/rsi9o5PQzwM
 * 
 */
//--------------------------------------------------------------------------------
/*
The green board has a RESET pin and the blue does not.
Therefore, for the blue board, the reset function is disabled.

BUFEN should be enabled for precise measurement, but in that case, the max input voltage
cannot be higher than AVDD-2 V. 
*/
//--------------------------------------------------------------------------------

// Pin configuration - 

/*
 SPI default pins on the STM32F103 and Uno/Nano ...

 ----------------------------------
 MOSI: Master OUT Slave IN -> DIN
 MISO: Master IN Slave OUT -> DOUT
 ----------------------------------
                    Arduino         
         STM32F103  Uno/Nano    ADS1256  
         --------   --------    ---------
 MOSI  - PA7        11          DIN
 MISO  - PA6        12          DOUT
 SCK   - PA5        13          SCLK
 SS    - PA4        10          CS


 Other pins - You can assign them to any pins
 DRDY  -  PA2    // this is an interrupt pin to indicate data ready
 RESET -  PA3 
 PDWN  - +3.3 V
 PDWN  -  PA1 (Alternatively, if you want to switch it)
*/

//--------------------------------------------------------------------------------
//Clock rate
/*
  f_CLKIN = 7.68 MHz
  tau = 130.2 ns
*/
//--------------------------------------------------------------------------------
//REGISTERS
/*
REG   VAL     USE
0     54      Status Register, Everything Is Default, Except ACAL and BUFEN
1     1       Multiplexer Register, AIN0 POS, AIN1 NEG
2     0       ADCON, Everything is OFF, PGA = 1
3     99      DataRate = 50 SPS
4     225     GPIO, Everything Is Default
*/
//--------------------------------------------------------------------------------
#include <SPI.h>

// The setup() function runs once each time the micro-controller starts
void setup()
{
  Serial.begin(115200);

  delay(1000);
  
  initialize_ADS1256();
 
  delay(1000);
 
  reset_ADS1256();
  userDefaultRegisters();
  printInstructions();
}

// -------------------------------------------------------------------------------
// Variables
double VREF    = 2.50;
double voltage = 0;
int    CS_Value;

// -------------------------------------------------------------------------------
// Pin Assignments

// NODEMCU-32S pin assignment
const byte DRDY_pin   = 15;   // goes to DRDY on ADS1256
const byte CS_pin     =  5;   // goes to CS   on ADS1256  
const byte PDWN_PIN   =  2;   // goes to the PDWN/SYNC/RESET pin 
const byte RESET_pin  =  4;   // goes to RST  on ADS1256 

// -------------------------------------------------------------------------------
// Values for registers
uint8_t registerAddress;
uint8_t registerValueR;
uint8_t registerValueW;
int32_t registerData;
uint8_t directCommand;
String  ConversionResults;
String  PrintMessage;

void loop()
{
  if (Serial.available() > 0) 
  {
    char commandCharacter = Serial.read();

    switch (commandCharacter)
    {
      case 'r':
      Serial.println("*Which register to read?");
      registerAddress = Serial.parseInt();
      while (!Serial.available());
      PrintMessage = "*Value of register " + String(registerAddress) + " is " + String(readRegister(registerAddress));
      Serial.println(PrintMessage);
      PrintMessage = "";
      break;

      case 'w':
      Serial.println("*Which Register to write?");
      while (!Serial.available());
      registerAddress = Serial.parseInt();

      Serial.println("*Which Value to write?");
      while (!Serial.available());
      registerValueW = Serial.parseInt();

      writeRegister(registerAddress, registerValueW);
      delay(500);
      PrintMessage = "*The value of the register now is: " + String(readRegister(registerAddress));          
      Serial.println(PrintMessage);
      PrintMessage = "";
      break;
    
      case 't':
      Serial.println("*Test message triggered by serial command");
      break;

      case 'O':
      readSingle();
      break;

      case 'R':
      reset_ADS1256();
      break;

      case 's':
      SPI.transfer(B00001111); // SDATAC
      break;

      case 'p':
      printInstructions();
      break;

      case 'C':
      cycleSingleEnded();
      break;

      case 'D':
      cycleDifferential();
      break;

      case 'H':
      cycleDifferential_HS();
      break;

      case 'd':
      while (!Serial.available());
      directCommand = Serial.parseInt();
      sendDirectCommand(directCommand);
      Serial.println("*Direct command performed!");
      break;

      case 'A':
      readSingleContinuous();
      break;

      case 'U':
      userDefaultRegisters();
      break;
    }
  }
}

// -------------------------------------------------------------------------------
// Functions

unsigned long readRegister(uint8_t registerAddress)
{
  while (digitalRead(DRDY_pin)) { /* wait */ }
  
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));

  digitalWrite(CS_pin, LOW);
  
  SPI.transfer(0x10 | registerAddress);
  SPI.transfer(0x00);

  delayMicroseconds(5);

  registerValueR = SPI.transfer(0xFF);

  digitalWrite(CS_pin, HIGH);
  SPI.endTransaction();

  return registerValueR;
}

void writeRegister(uint8_t registerAddress, uint8_t registerValueW)
{  
  while (digitalRead(DRDY_pin)) { /* wait */ }
  
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));

  digitalWrite(CS_pin, LOW);
  
  delayMicroseconds(5);
  
  SPI.transfer(0x50 | registerAddress);
  SPI.transfer(0x00);

  SPI.transfer(registerValueW);
  
  digitalWrite(CS_pin, HIGH);
  SPI.endTransaction();
}

void reset_ADS1256()
{
  Serial.println("* ADS1256 Reset .....");

  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));

  digitalWrite(CS_pin, LOW);

  delayMicroseconds(10);

  SPI.transfer(0xFE);

  delay(2);

  SPI.transfer(0x0F);

  delayMicroseconds(100);

  digitalWrite(CS_pin, HIGH);

  SPI.endTransaction();

  Serial.println("* ADS1256 Reset complete!");
}

void initialize_ADS1256()
{
  Serial.println("* ADS1256 Initialization .....");

  pinMode(CS_pin, OUTPUT);
  digitalWrite(CS_pin, LOW);

  SPI.begin();

  CS_Value = CS_pin;

//DRDY
  pinMode(DRDY_pin, INPUT);
  pinMode(RESET_pin, OUTPUT);
  digitalWrite(RESET_pin, LOW);

  delay(500);

  digitalWrite(RESET_pin, HIGH);
 
  delay(500);

  Serial.println("* ADS1256 Initialization complete!");
}

void readSingle()
{
  registerData = 0;
  
  while (digitalRead(DRDY_pin)) { /* wait */ }

  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
  digitalWrite(CS_pin, LOW);
    
  SPI.transfer(B00000001); // RDATA

  delayMicroseconds(10);

  registerData |= SPI.transfer(0x0F);
  registerData <<= 8;
  registerData |= SPI.transfer(0x0F);
  registerData <<= 8;
  registerData |= SPI.transfer(0x0F);

  Serial.println(registerData);
  convertToVoltage(registerData);
  
  digitalWrite(CS_pin, HIGH);
  SPI.endTransaction();
}

void readSingleContinuous()
{
  registerData = 0;
  
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
  digitalWrite(CS_pin, LOW);
  
  SPI.transfer(B00000011); // RDATAC
  delayMicroseconds(5);
  
  while (Serial.read() != 's')
  {    
    while (digitalRead(DRDY_pin)) { /* wait */ }
    delayMicroseconds(5);
    
    registerData |= SPI.transfer(0x0F);
    registerData <<= 8;
    registerData |= SPI.transfer(0x0F);
    registerData <<= 8;
    registerData |= SPI.transfer(0x0F);

    Serial.println(registerData);
    
    registerData = 0;
  }
 
  digitalWrite(CS_pin, HIGH);
  SPI.endTransaction();  
}

void cycleSingleEnded()
{
   static int muxReg[8] = {
     B00001000, B00011000, B00101000, B00111000,
     B01001000, B01011000, B01101000, B01111000
   };

  int cycle = 1;  
  registerData = 0;
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));

  while (Serial.read() != 's')
  {
    for (cycle = 0; cycle < 8; cycle++)
    {
      while (digitalRead(DRDY_pin)) { /* wait */ }
      
      digitalWrite(CS_pin, LOW);
      SPI.transfer(0x50 | 1);
      SPI.transfer(0x00); 

      switch (cycle) 
      {
        case 0: SPI.transfer(muxReg[1]); break;
        case 1: SPI.transfer(muxReg[2]); break;
        case 2: SPI.transfer(muxReg[3]); break;
        case 3: SPI.transfer(muxReg[4]); break;
        case 4: SPI.transfer(muxReg[5]); break;
        case 5: SPI.transfer(muxReg[6]); break;
        case 6: SPI.transfer(muxReg[7]); break;
        case 7: SPI.transfer(muxReg[0]); break;
      }

      SPI.transfer(B11111100);  // SYNC
      delayMicroseconds(4);
      SPI.transfer(B11111111);  // WAKEUP

      SPI.transfer(B00000001);  // RDATA
      delayMicroseconds(5);

      registerData = 0;
      registerData |= SPI.transfer(0x0F);
      registerData <<= 8;
      registerData |= SPI.transfer(0x0F);
      registerData <<= 8;
      registerData |= SPI.transfer(0x0F);

      digitalWrite(CS_pin, HIGH);
          
      ConversionResults = ConversionResults + registerData;
      ConversionResults = ConversionResults + "\t";     
      registerData = 0;
    }
    Serial.println(ConversionResults);   
    ConversionResults="";
  } 
  SPI.endTransaction(); 
}

void cycleDifferential() 
{
  int cycle = 1;  
  registerData = 0;
  
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
  
  digitalWrite(CS_pin, LOW);
  SPI.transfer(0x50 | 1);
  SPI.transfer(0x00); 
  SPI.transfer(B00000001);  // AIN0-AIN1
  digitalWrite(CS_pin, HIGH);
  SPI.endTransaction();
  
  while (Serial.read() != 's')
  {   
    for (cycle = 1; cycle < 5; cycle++)
    {
      while (digitalRead(DRDY_pin)) {} 
      
      switch (cycle)
      {
      case 1:
        digitalWrite(CS_pin, LOW);
        SPI.transfer(0x50 | 1);
        SPI.transfer(0x00); 
        SPI.transfer(B00100011);   // AIN2-AIN3
        break;

      case 2:
        digitalWrite(CS_pin, LOW);
        SPI.transfer(0x50 | 1);
        SPI.transfer(0x00); 
        SPI.transfer(B01000101);   // AIN4-AIN5
        break;

      case 3:
        digitalWrite(CS_pin, LOW);
        SPI.transfer(0x50 | 1);
        SPI.transfer(0x00); 
        SPI.transfer(B01100111);   // AIN6-AIN7
        break;      

      case 4:
        digitalWrite(CS_pin, LOW);
        SPI.transfer(0x50 | 1);
        SPI.transfer(0x00); 
        SPI.transfer(B00000001);   // AIN0-AIN1
        break;
      }            
      
      SPI.transfer(B11111100);  // SYNC
      delayMicroseconds(4); 
      SPI.transfer(B11111111);  // WAKEUP

      SPI.transfer(B00000001);  // RDATA
      delayMicroseconds(5);

      registerData |= SPI.transfer(0x0F);
      registerData <<= 8;
      registerData |= SPI.transfer(0x0F);
      registerData <<= 8;
      registerData |= SPI.transfer(0x0F);
 
      ConversionResults = ConversionResults + registerData;
      ConversionResults = ConversionResults + "\t";     

      registerData = 0;
      digitalWrite(CS_pin, HIGH);
    }
    Serial.println(ConversionResults);
    ConversionResults = "";
  }
  SPI.endTransaction();
}

void cycleDifferential_HS()
{  
  int cycle = 1;  
  registerData = 0;
  
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));

  digitalWrite(CS_pin, LOW);
  SPI.transfer(0x50 | 1);
  SPI.transfer(0x00); 
  SPI.transfer(B00000001);  // AIN0-AIN1
  digitalWrite(CS_pin, HIGH);
  
  SPI.endTransaction();
  
  while (Serial.read() != 's')
  {   
    for(int package = 0; package < 10; package++)
    {      
      for (cycle = 1; cycle < 5; cycle++)
      {
        while (digitalRead(DRDY_pin)) { /* wait */ }
        
        switch (cycle)
        {
        case 1:
          digitalWrite(CS_pin, LOW);
          SPI.transfer(0x50 | 1);
          SPI.transfer(0x00); 
          SPI.transfer(B00100011);
          break;
  
        case 2:
          digitalWrite(CS_pin, LOW);
          SPI.transfer(0x50 | 1);
          SPI.transfer(0x00); 
          SPI.transfer(B01000101);
          break;
  
        case 3:
          digitalWrite(CS_pin, LOW);
          SPI.transfer(0x50 | 1);
          SPI.transfer(0x00); 
          SPI.transfer(B01100111);
          break;      
  
        case 4:
          digitalWrite(CS_pin, LOW);
          SPI.transfer(0x50 | 1);
          SPI.transfer(0x00); 
          SPI.transfer(B00000001);
          break;
        }            
        
        SPI.transfer(B11111100);
        delayMicroseconds(4);
        SPI.transfer(B11111111);
        SPI.transfer(B00000001);
        delayMicroseconds(5);

        registerData |= SPI.transfer(0x0F);
        registerData <<= 8;
        registerData |= SPI.transfer(0x0F);
        registerData <<= 8;
        registerData |= SPI.transfer(0x0F);

        ConversionResults = ConversionResults + registerData;
        if(cycle < 4) {      
          ConversionResults = ConversionResults + "\t";     
        }

        registerData = 0;
        digitalWrite(CS_pin, HIGH);
      }
      ConversionResults = ConversionResults + '\n';
    }
    Serial.print(ConversionResults);
    ConversionResults = "";
  }
  SPI.endTransaction();
}

void sendDirectCommand(uint8_t directCommand)
{
  SPI.beginTransaction(SPISettings(1700000, MSBFIRST, SPI_MODE1));

  digitalWrite(CS_pin, LOW);

  delayMicroseconds(5);

  SPI.transfer(directCommand);

  delayMicroseconds(5);

  digitalWrite(CS_pin, HIGH);

  SPI.endTransaction();
}

void userDefaultRegisters()
{
  Serial.println("* ADS1256 User Default Registers .....  ");
  
  delay(500);
  
  writeRegister(0x00, B00110110);  // STATUS
  Serial.println("* ADS1256 STATUS set .....  ");
  delay(200);

  // FIXED: default to differential AIN0-AIN1 for G1 geophone
  writeRegister(0x01, B00000001);  // AIN0-AIN1
  Serial.println("* ADS1256 MUX set to AIN0-AIN1");
  delay(200);

  writeRegister(0x02, B00000001);  // PGA = 16 --- change line 633 when you change this
  delay(200);

  writeRegister(0x03, B01100011);  // DRATE = 50 SPS
  Serial.println("* ADS1256 DRATE set to 50 SPS ");
  delay(500);

  sendDirectCommand(B11110000);    // SELFCAL
  Serial.println("* ADS1256 SELFCAL complete ");
  Serial.println("* ADS1256 User Default Registers updated!");
}

void printInstructions()
{
 PrintMessage = "* Use the following letters and [CTRL]+[ENTER] to send a command to the ADS 1256:" + String("\n") 
  + "  r - Read a register. Example: 'r1' - reads the register 1" + String("\n") 
  + "  w - Write a register. Example: 'w1 1' - changes the value of the 1st register to 1 (AIN0-AIN1)." + String("\n") 
  + "  O - Single readout. Example: 'O' - Returns a single value from the ADS1256." + String("\n") 
  + "  A - Single, continuous reading with manual MUX setting." + String("\n") 
  + "  C - Cycling the ADS1256 Input multiplexer in single-ended mode (8 channels)." + String("\n") 
  + "  D - Cycling the ADS1256 Input multiplexer in differential mode (4 channels)." + String("\n")  
  + "  H - Cycling the ADS1256 Input multiplexer in differential mode (4 channels) at high-speed." + String("\n")  
  + "  R - Reset ADS1256. Example: 'R' - Resets the device, everything is set to default." + String("\n")  
  + "  s - SDATAC: Stop Read Data Continuously." + String("\n")  
  + "  U - User Default Registers."  + String("\n")
  + "  t - triggers a test message."  + String("\n")    
  + "  d - Send direct command.";

  Serial.println(PrintMessage);
  PrintMessage = "";
}

void convertToVoltage(int32_t registerData)
{
  // Sign-extend 24-bit ADC output to 32-bit signed integer
  if (registerData & 0x800000) {
    registerData -= 16777216;
  }

  voltage = ((2 * VREF) /(8388608.0)) * registerData;
    
  Serial.println(voltage, 8);
  voltage = 0;
}