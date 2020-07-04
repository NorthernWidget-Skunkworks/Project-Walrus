#include <SlowSoftWire.h>
#include <Wire.h>
#include <EEPROM.h>

#define MODEL 0x1201
#define GROUPID 0x0871
#define INDID 0x06
#define FIRMWAREID 0x00

#define CMD_RESET 0x1E // reset command
#define CMD_ADC_READ 0x00 // ADC read command
#define CMD_ADC_CONV 0x40 // ADC conversion command

#define CMD_PROM 0xA0 // Coefficient location

#define COEF0 17
#define COEF1 6
#define COEF2 16
#define COEF3 7
#define COEF4 10000
#define COEF5 1
#define COEF6 31
#define COEF7 61
#define COEF8 4
#define COEF9 2
#define COEF10 0
#define COEF11 20
#define COEF12 12
#define COEF13 0
#define COEF14 0
#define COEF15 0

#define PresADR 0x77
#define ADC_ADR 0x6B

#define SOF '['
#define EOF ']'

// Pins for ATTiny1634
#define SN_DE 13
#define SN_RE 14
#define REF_EN 4

uint16_t coefficient[8];// Coefficients;

int32_t _temperature_actual;
int32_t _pressure_actual;

float Pressure = 0; // MS5803 pressure
float Temp0 = 0; // Global tempurature from thermistor
float Temp1 = 0; // Global tempurature MS5803

// Register to be used to display the status of the sub modules
// Bits 0~1 used for status of ADC, bits 2~3 used for MS5803 status
uint8_t StatusReg = 0;

uint8_t ADR_Print = 13; //FIX make this less bad!
static const char GenADR[2] = {'9', '9'};
char ADR[2] = {'1', '3'}; //Used to read in and check ADR

char Format = 'f';
uint8_t Baud_Multiplier = 4;  // Baud = Baud_Multiplier * 1200

void setup() {
     DDRA = DDRA | 0x01; //Set pin A0 as OUTPUT
     PORTA = PORTA | 0x01; //DEBUG! Set A0 port, set A0 HIGH

    // Start communicating wtih ADC to thermistor and MS5803
    Wire.begin();
  	initADC();
  	initMS5803();

  	getValues(); // Update on power on

    // Start serial comunication over RS-485, start in "slow mode" to allow
    // for reset
  	Serial.begin(4800);

  	// For ATTiny 1634 -- aka Walrus
  	pinMode(SN_DE, OUTPUT);
  	pinMode(SN_RE, OUTPUT);
  	pinMode(REF_EN, OUTPUT);

    // Begin in minion mode by default, recieve on, transmit off
  	digitalWrite(SN_DE, LOW);
  	digitalWrite(SN_RE, LOW);
  	digitalWrite(REF_EN, HIGH);

    // If EEPROM is not initialized, like on initial firmware load,
    // then write addresses and BAUD_MULTIPLIER to it
    if(EEPROM.read(0x00) != 'F') {
        writeEEPROM();
    }
    // Otherwise, read address from EEPROM on startup
    else {
        ADR[0] = EEPROM.read(0x01);
        ADR[1] = EEPROM.read(0x02);
        Baud_Multiplier = EEPROM.read(0x03);
        ADR_Print = charToInt(ADR);
    }

    long LocalTime = millis();
    while(millis() - LocalTime < 1000) {
        // If 'ESCAPE' message is sent, reset to defaults!
        if(Serial.read() == '!') {
            reset();
            break;
        }
    }
    Serial.end(); //End previous serial instance

    // Restart Serial with general baud rate
    Serial.begin(Baud_Multiplier*1200);

    PORTA = PORTA & 0xFE; //DEBUG! Clear A0 port, set A0 LOW
}

void loop() {

    // Define variables
    char TempADR[2] = {0}; // Temporary address
    uint8_t RegID = 0; // Used to store int converted register value
    char Msg[10] = {0}; // 9 digits + null termanator?
    bool Neg = false; // Negative flag
    char Reg[2] = {0}; // Used to read in register ID

    while(Serial.read() != SOF); //Wait for SOF character //DEBUG!
    PORTA = PORTA | 0x01; //DEBUG! Set A0 port, set A0 HIGH

    while(Serial.available() < 4) { //FIX add timeout!  //DEBUG!
        // Must remain! I know it seems unessicary, but Scouts Honor, it's not!
        delay(1);
    };

    /////////////////////////////////////
    // Communication and data transfer //
    /////////////////////////////////////

    // Read temperary address that is provided
    // ADW: This looks to be some kind of digital handshake to make sure that
    // the sensor does not do anything unless it is supposed to
    TempADR[0] = Serial.read();
    TempADR[1] = Serial.read();

    // Proceed only if address matches unique, or general call
    if( (TempADR[0] == ADR[0] && TempADR[1] == ADR[1]) ||
        (TempADR[0] == GenADR[0] && TempADR[1] == GenADR[1]) )
        {
            // Set baud rate
            Reg[0] = Serial.read();
            Reg[1] = Serial.read();
            RegID = charToInt(Reg);
            if(RegID == 97) {
                //Wait for 1 byte FIX add timeout!
                while(Serial.available() < 1) {
                    delay(1);
                }
                // Must restart to take effect
                Baud_Multiplier = Serial.read() - 48;
                // Store baud value as integer
                if(Baud_Multiplier <= 96) EEPROM.write(0x03, Baud_Multiplier);
            }
            // Set address
            if(RegID == 99) {
                //Wait for 2 bytes FIX add timeout! DEBUG!
                while(Serial.available() < 2) {
                    delay(1);
                }
                ADR[0] = Serial.read();
                ADR[1] = Serial.read();
                // Convert into int for easier printing later FIX??
                ADR_Print = charToInt(ADR);
                if(ADR < 99) {
                // Write values to EEPROM for future use
                EEPROM.write(0x01, ADR[0]);
                EEPROM.write(0x02, ADR[1]);
            }
        }

        // Gets data of a given type based on the RegID provided
        float DataTemp = getData(RegID);
        // shift 3 decimal digits left to make it a whole number
        long Temp = long((DataTemp*float(1000.0)));
        if(Temp < 0) {
            // Convert to positive for now, will replace negative later
            // in conversion
            Temp = Temp*-1.0;
            // Set negative flag
            Neg = true;
        }

        // Get length of number to know when to stop.
        uint8_t Len = 1;
        // Check for 0 to prevent weird math/mem access issues
        if(Temp != 0) {
            Len = log10(Temp);
        }

        // Test if less than 1, but positive
        if(Temp < 1000 && Temp > 0) {
            // If so, add an additional place to generate a leading zero
            Len = Len + 1;
        }

        // Convert to ASCII
        for(int i = Len + 1; i > Len - 2; i--) {
            // Take lowest digit, convert to ASCII char
            Msg[i] = (Temp % 10) + 48;
            Temp /= 10; // Shift to the right one
        }
        Msg[Len - 2] = '.'; //Add decimal
        for(int p = Len - 3; p >= 0; p--) {
            // Take lowest digit, convert to ASCII char
            Msg[p] = (Temp % 10) + 48;
            Temp /= 10;
        }

        // If value is negative, shift values right and append '-' sign
        if(Neg) {
            for(int i = 9; i > 0; i--) {
                Msg[i] = Msg[i - 1];
            }
            Msg[0] = '-';
        }

        // Remove decimal and trailing zero for integer values
        if((Format == 'd' || Format == 'b') && Len > 2) {
            Len = Len - 2;
        }
        // Otherwise, add space for the values beyond the decimal
        if(Format == 'f') {
            Len = Len + 2;
        }

        // Transmit data
        digitalWrite(SN_RE, HIGH); // Disable recieve (to prevent loopback)
        digitalWrite(SN_DE, HIGH); // Enable transmit

        Serial.print(SOF);
        zeroPrepend(ADR_Print);
        zeroPrepend(RegID);
        for(int i = 0; i < Len; i++) {
            Serial.print(Msg[i]); // Print subset of Msg
        }
        Serial.print(Format);
        Serial.print(getCRC(Msg, Len));
        Serial.print(EOF);
        Serial.print("\n\r"); //Print formatter

        digitalWrite(SN_DE, LOW);  //Disable transmit
        digitalWrite(SN_RE, LOW);  //Enable recieve, return to normal operation
    }
    PORTA = PORTA & 0xFE; //DEBUG! Clear A0 port, set A0 LOW
}

///////////////
// FUNCTIONS //
///////////////

void zeroPrepend(uint8_t Val)
{
    if(Val < 10) {
        Serial.print('0');
    }
    Serial.print(Val);
}

uint8_t getValues()
{
    // Update global values from sensors
    getMeasurements();
    Pressure = _pressure_actual / (float(COEF4)/100.0);
  	Temp0 = getTempThermistor();
    Temp1 = _temperature_actual / 100.0;
    return StatusReg; // FIX to give status indication!
}

void initADC()
{
	Wire.beginTransmission(ADC_ADR);
	Wire.write(0x9C); // Set ADC to continuious conversion, 18 bits, 1V/V gain
	StatusReg = StatusReg | Wire.endTransmission(); // Lower 2 bits for ADC status
}

void initMS5803()
{
    uint8_t i;
    uint8_t Data[2] = {0};

    for(i = 0; i <= 7; i++){
        sendCommand(CMD_PROM + (i * 2));
        Wire.requestFrom(PresADR, 2);
        Data[0] = Wire.read();
        Data[1] = Wire.read();
        coefficient[i] = (Data[0] << 8)|Data[1];
    }
    //Bits 2~3 show the status of the MS5803
  	StatusReg = StatusReg | (sendCommand(CMD_RESET) << 2);
  	delay(3);
}

void getMeasurements()
// Gets resuts from ADC and stores them into internal variables
{
    //Retrieve ADC result
    int32_t temperature_raw = getADCconversionMS5803(0x10);
    int32_t pressure_raw = getADCconversionMS5803(0x00);

    //Create Variables for calculations
    int32_t temp_calc;
    int32_t pressure_calc;

    int32_t dT;

    //Now that we have a raw temperature, let's compute our actual.
    dT = temperature_raw - ((int32_t)coefficient[5] << 8);
    temp_calc = (((int64_t)dT * coefficient[6]) >> 23) + 2000;

    // TODO TESTING  _temperature_actual = temp_calc;

    //Now we have our first order Temperature, let's calculate the second order.
    int64_t T2, OFF2, SENS2, OFF, SENS; //working variables

    if (temp_calc < 2000)
    // If temp_calc is below 20.0C
        //LOW TEMP
    {
        T2 = COEF5* (((int64_t)dT * dT) >> COEF6);
        OFF2 = COEF7 * ((temp_calc - 2000) * (temp_calc - 2000)) / (pow(2,COEF8));
        SENS2 = COEF9 * ((temp_calc - 2000) * (temp_calc - 2000)) / (pow(2,COEF10));

       // if(temp_calc < -1500)  //Not needed for water???????
       // // If temp_calc is below -15.0C
       //     //VERY LOW TEMP
       // {
       //     OFF2 = OFF2 + COEF11 * ((temp_calc + 1500) * (temp_calc + 1500));
       //     SENS2 = SENS2 + COEF12 * ((temp_calc + 1500) * (temp_calc + 1500));
       // }
    }
    else
    // If temp_calc is above 20.0C
        //HIGH TEMP
    {
        T2 = COEF13 * ((uint64_t)dT * dT)/pow(2,COEF14);
        OFF2 = COEF15*((temp_calc - 2000) * (temp_calc - 2000)) / 16;
        SENS2 = 0;
    }

    // Now bring it all together to apply offsets
    OFF = ((int64_t)coefficient[2] << COEF0) + (((coefficient[4] * (int64_t)dT)) >> COEF1);    //05BA model!
    SENS = ((int64_t)coefficient[1] << COEF2) + (((coefficient[3] * (int64_t)dT)) >> COEF3); //05BA model!

    temp_calc = temp_calc - T2;
    OFF = OFF - OFF2;
    SENS = SENS - SENS2;

    // Now lets calculate the pressure
    pressure_calc = (((SENS * pressure_raw) / 2097152 ) - OFF) / 32768;

    _temperature_actual = temp_calc ;
    _pressure_actual = pressure_calc ; // 10;// pressure_calc;
}

uint32_t getADCconversionMS5803(uint8_t _measurement)
// Retrieve ADC measurement from the MS5803 device.
// Select measurement type and precision
// TODO:  Set up for asynchronous conversion? - ie, w/o waits
{
    uint8_t Data[3] = {0};
    sendCommand(CMD_ADC_CONV + _measurement + 0x08);
    // Wait for conversion to complete
    delay(10);

    StatusReg = StatusReg | (sendCommand(CMD_ADC_READ) << 2);

    Wire.requestFrom(PresADR, 3);

    while(Wire.available())  //REPLACE!
    {
        Data[0] = Wire.read();
        Data[1] = Wire.read();
        Data[2] = Wire.read();
    }
    return (long(Data[0]) << 16) + (long(Data[1]) << 8) + Data[0];
}

uint8_t sendCommand(uint8_t Command)
{
    Wire.beginTransmission(PresADR);
    Wire.write(Command);
    return Wire.endTransmission();
}

float getTempThermistor()
{
    uint8_t Data[3];
    float ThermB = 3380; //Basic B value for thermistor
    // Coefficients for enhanced accuracy
    // float ThermCoefs[4] = \
    //        {0.003354016, 0.0003074038, 1.019153E-05, 9.093712E-07};
    float VRef = 1.8; //Voltage referance used for thermistor
    float R0 = 10000; //Series resistor value with thermistor
    float ThermR = 10000; //Nominal resistace value for the thermistor

  	Wire.requestFrom(ADC_ADR, 4);
    delay(5); //DEBUG!
    // Get 3 bytes of potential data
  	if(Wire.available() == 4)
  	{
  		  Data[0] = Wire.read();
    		Data[1] = Wire.read();
    		Data[2] = Wire.read(); //DEBUG!
  	}
    StatusReg = StatusReg | Wire.endTransmission(); //DEBUG!

    // Voltage divider output
    float Val = ( ( (long(Data[0]) & 0x03 ) << 16 )
                  + (long(Data[1]) << 8)
                  + Data[2] )
                  * 1.5625e-5;
    // Voltage is measured across thermistor, not relative to ground
    Val = VRef - Val;

    float Rt = (VRef/Val)*R0 - R0;
    float T = 1/((1/ThermB)*log(Rt/ThermR) + 1/298.15);
    T = T - 273.15; // Convert to C
    return T; //DEBUG!
}

unsigned char getCRC(const unsigned char * data, const unsigned int size)
{
    // Cyclic redundancy check?
    unsigned char crc = 0;
    for ( unsigned int i = 0; i < size; ++i )
    {
        unsigned char inbyte = data[i];
        for ( unsigned char j = 0; j < 8; ++j )
        {
            unsigned char mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if ( mix ) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

uint8_t charToInt(char *data)
{
    return (data[0] - 48)*10 + (data[1] - 48);
}

float getData(uint8_t RegID)
{
    Format = 'd'; // Default to integer ("decimal", not point!)
    switch(RegID) {
        case 0:
            Format = 'f';
            // DecFigs = 3;
            return Pressure;
            break;

        case 1:
            Format = 'f';
            // DecFigs = 3;
            return Temp0;
            break;

        case 2:
            Format = 'f';
            // DecFigs = 3;
            return Temp1;
            break;

        case 5:
            //FIX! Print Self test
            Format = 'b';
            // DecFigs = 0;
            return getValues();
            break;

        case 80:  //read baud rate
            // Format = 'd';
            // DecFigs = 0;
            return MODEL;
            break;

        case 81:
            // Format = 'd';
            return GROUPID;
            break;

        case 82:
            // Format = 'd';
            return INDID;
            break;

        case 83:
            // Format = 'd';
            return FIRMWAREID;
            break;

        case 96:  //read baud rate
            // Format = 'd';
            // DecFigs = 0;
            return Baud_Multiplier*1200;
            break;

        case 97:  //Set baud rate
            // Format = 'd';
            return Baud_Multiplier*1200;
            break;

        case 98: //Read address
            Format = 'b';
            return ADR_Print;
            break;

        case 99:  //Set Address
            Format = 'b';
            return ADR_Print;
            break;
    }
}

void reset()
{
    ADR[0] = '1';
    ADR[1] = '3';
    Baud_Multiplier = 4;

    writeEEPROM();

    Serial.println("[r]");
}

void writeEEPROM()
{
    EEPROM.write(0x01, ADR[0]);
    EEPROM.write(0x02, ADR[1]);
    EEPROM.write(0x03, Baud_Multiplier);
    EEPROM.write(0x00, 'L'); //Write value to adr 0x00 to mark EEPROM as written
}
