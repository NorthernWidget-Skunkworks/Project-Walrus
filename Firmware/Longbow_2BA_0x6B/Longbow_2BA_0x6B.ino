// #include <MS5803.h>
// #define I2C_HARDWARE 1
#include <SlowSoftWire.h> 
// #include <SlowSoftI2CMaster.h>
// #include <Wire.h>
#include <EEPROM.h>
// #include <math.h>
//#define SDA_PORT PORTA
//#define SDA_PIN 6 // = A4
//#define SCL_PORT PORTA
//#define SCL_PIN 4 // = A5

// #define GUT 
SlowSoftWire Wire = SlowSoftWire(4, 6);
// SlowSoftI2CMaster si = SlowSoftI2CMaster(4, 6, true);  //Initialize software I2C

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

// #define ADR_Print 13

#define SOF '['
#define EOF ']'

// Pins for ATTiny1634
#define SN_DE 13
#define SN_RE 14
#define REF_EN 4

uint16_t coefficient[8];// Coefficients;
// static uint16_t ConvCoef[] = {18, 5, 17, 7, 10000, 3, 33, 3, 3, 7, 3, 0, 3, 0, 0, 0}; //Coefficients for data conversion

int32_t _temperature_actual;
int32_t _pressure_actual;
// MS5803 Pres;

// uint8_t PresADR = 0x77;
// uint8_t ADC_ADR = 0x68;

float Pressure = 0; //Global pressure value
float Temp0 = 0; //Global tempurature from themristor
float Temp1 = 0; //Global tempurature from on board temp

uint8_t StatusReg = 0; //Register to be used to display the status of the sub modules , Bits 0~1 used for status of ADC, bits 2~3 used for MS5803 status
// char Val = 0; //FIX!

// uint8_t DecFigs = 0; //Used to specify print formating of numbers

uint8_t ADR_Print = 13; //FIX make this less bad!
static const char GenADR[2] = {'9', '9'}; 
char ADR[2] = {'1', '3'}; //Used to read in and check ADR

char Format = 'f';
uint8_t Baud = 4;  //Baud rate multiplier x*1200

void setup() {
//	 pinMode(10, OUTPUT); //DEBUG!
     DDRA = DDRA | 0x01; //Set pin A0 as OUTPUT
     PORTA = PORTA | 0x01; //DEBUG! Set A0 port, set A0 HIGH 
	// digitalWrite(10, HIGH); //DEBUG!
	Wire.begin();
    // si.i2c_init(); //Begin I2C master
	// Pres.begin(0x76, 2);  //Inialize pressure sensor
	// Pres.reset();

	InitADC();

	InitPressure();

	GetValues(); //Update on power on
	Serial.begin(4800); //Start serial comunication over RS-485, start in "slow mode" to allow for reset

	// For ATTiny 1634 -- aka Walrus
	pinMode(SN_DE, OUTPUT);
	pinMode(SN_RE, OUTPUT);
	pinMode(REF_EN, OUTPUT);

	digitalWrite(SN_DE, HIGH);
	digitalWrite(SN_RE, LOW);
	digitalWrite(REF_EN, HIGH);


    if(EEPROM.read(0x00) != 'F') {  //If EEPROM is not initialized, like on initial firmware load
        LoadEEPROM();
    }
    else {  //Normally, read address from EEPROM on startup
        ADR[0] = EEPROM.read(0x01);
        ADR[1] = EEPROM.read(0x02);
        // Serial.println(EEPROM.read(0x01));
        // Serial.println(EEPROM.read(0x02));
        Baud = EEPROM.read(0x03);
        ADR_Print = CharToInt(ADR); 
    }
    
    long LocalTime = millis();
    while(millis() - LocalTime < 1000) {
        if(Serial.read() == '!') {  //If 'ESCAPE' message is sent, reset to defaults!
            Reset();
            break;
        }
    }
    // while(Serial.available() < 1) {
    //     delay(1);
    // }
    // if(Serial.read())
    

    Serial.end(); //End previous serial instance
    Serial.begin(Baud*1200);  //Start again with general baud rate
//	Serial.println("Vals = ");
//	Serial.println(Pressure);
	// delay(1000); //DEBUG!
	// digitalWrite(10, LOW); //DEBUG!
  PORTA = PORTA & 0xFE; //DEBUG! Clear A0 port, set A0 LOW 
}

void loop() {

	// static char Index = 0;
    char TempADR[2] = {0};
    uint8_t RegID = 0; //Used to store int converted register value
    char Msg[10] = {0}; // 9 digits + null termanator? 
    bool Neg = false; //Negative flag
    char Reg[2] = {0}; //Used to read in register ID
    // char Val;
    while(Serial.read() != SOF); //Wait for SOF character //DEBUG!
    PORTA = PORTA | 0x01; //DEBUG! Set A0 port, set A0 HIGH 
    // delay(10);
    // Serial.print(Serial.available()); //DEBUG!
    // if(Serial.read() == '!') Reset();
    while(Serial.available() < 4) { //FIX add timeout!  //DEBUG!
        delay(1); //Must remain! I know it seems unessicary, but Scouts Honor, it's not! 
    }; 
    TempADR[0] = Serial.read(); //Read in address
    TempADR[1] = Serial.read();
    // Serial.print(TempADR[0]); //DEBUG!
    // Serial.print(TempADR[1]); //DEBUG!
    if(TempADR[0] == ADR[0] && TempADR[1] == ADR[1] || (TempADR[0] == GenADR[0] && TempADR[1] == GenADR[1])) //Proceed only if address matches unique, or general call
    {
//        digitalWrite(10, HIGH); //DEBUG!
        // PORTA = PORTA | 0x01; //DEBUG! Set A0 port, set A0 HIGH 
        Reg[0] = Serial.read();
        Reg[1] = Serial.read();
        // RegID = String(Reg).toInt();
        RegID = CharToInt(Reg);
        // RegID = (Reg[0] - 48)*10 + (Reg[1] - 48); //Convert Reg to int because frak strings! 
        // sscanf(Reg, "%d", &RegID); //Convert the pair of chars to int
        // while(Serial.read() != EOF); //FIX add timeout! //DEBUG!
        if(RegID == 97) { //Set baud rate
            while(Serial.available() < 1) { //Wait for 1 byte FIX add timeout!
                delay(1);
            }
            Baud = Serial.read() - 48; //Must restart to take effect
            if(Baud <= 96) EEPROM.write(0x03, Baud);  //Store baud value as integer 
            // Serial.println(Baud); //DEBUG!
        }
        if(RegID == 99) {  //Set address
            while(Serial.available() < 2) { //Wait for 2 bytes FIX add timeout! DEBUG!
                delay(1);
            }
            ADR[0] = Serial.read();
            ADR[1] = Serial.read();
            ADR_Print = CharToInt(ADR); //Convert into int for easier printing later FIX??
            if(ADR < 99) {
              EEPROM.write(0x01, ADR[0]); //Write values to EEPROM for future use 
              EEPROM.write(0x02, ADR[1]); 
            }
        }
        float DataTemp = GetData(RegID);
        // String Msg = String(DataTemp);

        // sprintf(Msg, "%ld", 0x10);
        // dtostrf(DataTemp, 6, 2, Msg);
        // Msg.toCharArray(MsgAry, Msg.length());
        long Temp = long((DataTemp*float(1000.0))); //shift 3 digits left to make it a whole number
        if(Temp < 0) {
            Temp = Temp*-1.0; //Convert to positive for now, will replace negative later in conversion 
            Neg = true; //Set negative flag
        }
        uint8_t Len = 1;
        if(Temp != 0) Len = log10(Temp); //Get length of number to know when to stop, check for 0 to prevent weird math/mem access issues 
        if(Temp < 1000 && Temp > 0) {  //Test if less than 1, but positive
            Len = Len + 1;  //If so, add an additional place to generate a leading zero 
        }

        for(int i = Len + 1; i > Len - 2; i--) {
            Msg[i] = (Temp % 10) + 48; //Take lowest digit, convert to ASCII char
            Temp /= 10; //Shift to the right one
        } 
        Msg[Len - 2] = '.'; //Add decimal
        for(int p = Len - 3; p >= 0; p--) {
            Msg[p] = (Temp % 10) + 48; //Take lowest digit, convert to ASCII char
            Temp /= 10;
        }

        if(Neg) {  //If value is negative, shift values right and append '-' sign 
            // char TempChar = 0;
            for(int i = 9; i > 0; i--) {
                Msg[i] = Msg[i - 1];
            }
            Msg[0] = '-';
        }

        if((Format == 'd' || Format == 'b') && Len > 2) Len = Len - 2;  //Remove decimal and trailing zero for decimal values
        if(Format == 'f') Len = Len + 2;

        Serial.print(SOF);
        ZeroAppend(ADR_Print); 
        //if(ADR_Print < 10) Serial.print('0'); //Zero pad if needed
        // Serial.print(ADR_Print);
        // if(RegID < 10) Serial.print('0');  //Zero pad if needed
        // Serial.print(RegID);
        ZeroAppend(RegID);
        // Serial.print(DataTemp, DecFigs);
        for(int i = 0; i < Len; i++) {
            Serial.print(Msg[i]); //Print subset of Msg
        }
        // Serial.print(Msg); //DEBUG!
        Serial.print(Format);
        Serial.print(GetCRC(Msg, Len));
        Serial.print(EOF);
        Serial.print("\n\r"); //Print formatter
    }
	// if(Serial.available() > 0) { //Check buffer
	// 	Val = Serial.read(); //Read value in
	// }
// 	if(Val != '\r') {
// 		Index = Val;  //Copy desied index
// 	}
// 	if(Val == '\r') { //End of transmission 
// 	    switch(Index) {
// 	    	case '0':
// //          Serial.print("\rf");
// 	    		Serial.print(Pressure);
// //          Serial.print("f\r");
//           digitalWrite(10, HIGH); //DEBUG!
//           delay(200); //DEBUG!
//           digitalWrite(10, LOW); //DEBUG!
// 	    		break;

//     		case '1':
//     			Serial.println(Temp0);
//     			break;

// 			case '2':
// 				Serial.println(Temp1);
// 				break;

// 			case '5':
// 				//FIX! Print Self test
// 				GetValues();
// 				break;
// 	    }
//      Val = NULL; //clear value?? 
//   	}
//  digitalWrite(10, LOW); //DEBUG!
    PORTA = PORTA & 0xFE; //DEBUG! Clear A0 port, set A0 LOW 
}

void ZeroAppend(uint8_t Val) 
{
    if(Val < 10) Serial.print('0');
    Serial.print(Val);
}

uint8_t GetValues() 
{
	//Update global values
	// Pressure = Pres.getPressure(ADC_4096);
    getMeasurements();
	// Pressure = GetPressure();
    Pressure = _pressure_actual / (float(COEF4)/100.0);
	Temp0 = GetTemp();
	// Temp1 = GetTempDH(); 
    Temp1 = _temperature_actual / 100.0;
    return StatusReg; //FIX to give status indication!
    // return 3;
}

void InitADC() 
{
	Wire.beginTransmission(ADC_ADR);
	Wire.write(0x9C); //Set ADC to continuious conversion, 18 bits, 1V/V gain
	StatusReg = StatusReg | Wire.endTransmission();  //Lower 2 bits for ADC status
    // si.i2c_start((ADC_ADR << 1) | WRITE);
    // si.i2c_write(0x9C);
    // si.i2c_stop();
}

void InitPressure()
{
	uint8_t i;
    // uint8_t highByte = 0;
    // uint8_t lowByte = 0;
    uint8_t Data[2] = {0};

	for(i = 0; i <= 7; i++){
        sendCommand(CMD_PROM + (i * 2));
        // si.i2c_start((PresADR << 1) | READ);
        // Data[0] = si.i2c_read(false);
        // Data[1] = si.i2c_read(false);
        // si.i2c_stop();
        Wire.requestFrom(PresADR, 2);
        // highByte = Wire.read();
        // lowByte = Wire.read();
        Data[0] = Wire.read();
        Data[1] = Wire.read();
        // Wire.endTransmission(); //DEBUG!
        // coefficient[i] = (highByte << 8)|lowByte;
        coefficient[i] = (Data[0] << 8)|Data[1];
    }
	StatusReg = StatusReg | (sendCommand(CMD_RESET) << 2);  //Bits 2~3 show the status of the MS5803
	delay(3);
}

// float GetPressure()
// {
// 	// getMeasurements();
//     // Serial.println(_pressure_actual); //DEBUG!
//     // float pressure_reported;
//     // pressure_reported = _pressure_actual;
//     // pressure_reported = pressure_reported / (float(COEF4)/100.0); //05BA model!
//     // return pressure_reported;
//     return _pressure_actual / (float(COEF4)/100.0);
//     // return -100.01; //DEBUG!
// }

// #ifndef GUT
// float GetTempDH()
// {
//     // getMeasurements();
//     // float temperature_reported;
//     // temperature_reported = _temperature_actual / 100.0f;
//     return _temperature_actual / 100.0;
// }
// #endif

void getMeasurements()
// Gets resuts from ADC and stores them into internal variables
{
    //Retrieve ADC result
    int32_t temperature_raw = getADCconversion(0x10);
    int32_t pressure_raw = getADCconversion(0x00);

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
    // OFF = ((int64_t)coefficient[2] << 16) + (((coefficient[4] * (int64_t)dT)) >> 7);
    // SENS = ((int64_t)coefficient[1] << 15) + (((coefficient[3] * (int64_t)dT)) >> 8);

    temp_calc = temp_calc - T2;
    OFF = OFF - OFF2;
    SENS = SENS - SENS2;

    // Now lets calculate the pressure
    pressure_calc = (((SENS * pressure_raw) / 2097152 ) - OFF) / 32768;

    _temperature_actual = temp_calc ;
    _pressure_actual = pressure_calc ; // 10;// pressure_calc;
}

uint32_t getADCconversion(uint8_t _measurement)
// Retrieve ADC measurement from the device.
// Select measurement type and precision
// TODO:  Set up for asynchronous conversion? - ie, w/o waits
{
    // uint32_t result;
    // uint8_t highByte = 0;
    // uint8_t midByte = 0;
    // uint8_t lowByte = 0;

    uint8_t Data[3] = {0};
    sendCommand(CMD_ADC_CONV + _measurement + 0x08);
    // Wait for conversion to complete
    delay(10);

    StatusReg = StatusReg | (sendCommand(CMD_ADC_READ) << 2);

    // si.i2c_start((PresADR << 1) | READ);
    // Data[0] = si.i2c_read(false);  //Read in high and low bytes (big endian)
    // Data[1] = si.i2c_read(false);
    // Data[2] = si.i2c_read(false);
    // si.i2c_stop();

    Wire.requestFrom(PresADR, 3);

   while(Wire.available())  //REPLACE!
   {
        Data[0] = Wire.read();
        Data[1] = Wire.read();
        Data[2] = Wire.read();
   //  Wire.endTransmission(); //DEBUG!

        // highByte = Wire.read();
        // midByte = Wire.read();
        // lowByte = Wire.read();
   }

    // result = ((long)highByte << 16) + ((long)midByte << 8) + lowByte;
    // result = (long(Data[0]) << 16) + (long(Data[1]) << 8) + Data[0];

    // return result;
    return (long(Data[0]) << 16) + (long(Data[1]) << 8) + Data[0];
}

uint8_t sendCommand(uint8_t Command)
{
    Wire.beginTransmission(PresADR);
    Wire.write(Command);
    return Wire.endTransmission();
    // si.i2c_start((PresADR << 1) | WRITE);
    // bool Error = si.i2c_write(Command);
    // si.i2c_stop();
    // return 1; //DEBUG!
}

float GetTemp()
{
	uint8_t Data[3];
    float ThermB = 3380; //Basic B value for thermistor 
    // float ThermCoefs[4] = {0.003354016, 0.0003074038, 1.019153E-05, 9.093712E-07}; //Coefficients for enhanced accuracy
    float VRef = 1.8; //Voltage referance used for thermistor
    float R0 = 10000; //Series resistor value with thermistor 
    float ThermR = 10000; //Nominal resistace value for the thermistor 

	Wire.requestFrom(ADC_ADR, 4);
    delay(5); //DEBUG!
	if(Wire.available() == 4) //Get data bytes, 3 bytes of potential data and configuration register  //REPLACE!
	{
		Data[0] = Wire.read();
		Data[1] = Wire.read();
		Data[2] = Wire.read(); //DEBUG!
		// Data[3] = Wire.read();
	}
    StatusReg = StatusReg | Wire.endTransmission(); //DEBUG!

    float Val = (((long(Data[0]) & 0x03) << 16) + (long(Data[1]) << 8) + Data[2])*(1.5625e-5); //Voltage divider output

    Val = VRef - Val; //Voltage is measure across thermistor, not relative to ground
    // Val = (VRef/Val)*R0 - R0;
    // Serial.println(Val);
    // // Serial.println(Rt/ThermR); //DEBUG!
    // // float LogRt = log(Rt/ThermR);
    // Val = log(Val/ThermR);
    // // float LogRt = 0;
    // // float T = 0;
    // float T = 1.0/(ThermCoefs[0] + ThermCoefs[1]*Val + ThermCoefs[2]*pow(Val, 2) + ThermCoefs[3]*pow(Val, 3));
    // T = T - 273.15; //Convert to C
    // return T;

    float Rt = (VRef/Val)*R0 - R0;
    float T = 1/((1/ThermB)*log(Rt/ThermR) + 1/298.15);
    T = T - 273.15; //Convert to C
    return T; //DEBUG!
    // return 0.5; //DEBUG!
    // return -15.01; //DEBUG!
}

// uint8_t GetCRC(char Data[], uint8_t Len)
// {
//     //FIX dummy function!
//     const unsigned char Poly = 0x8C;
//     unsigned char crc = 0;
//     // union {
//     //     float f;
//     //     uint32_t i;
//     // } Val;
//     // Val.f = Data;
//     // uint32_t Temp = Val.i; //Copy the "float" to a genaric 32 bit value

//     for(int i = 0; i < Len; i++) {
//         crc ^= Data[i];
//         for(int j = 0; j < 9; j++) {
//             if(crc & 1) crc ^= Poly;
//             crc >>= 1;
//         }
//     }
//     // return BitRead(Temp, 32) + BitRead(Temp, 26) + BitRead(Temp, 23) + BitRead(Temp, 22) + BitRead(Temp, 16) + BitRead(Temp, 12) + BitRead(Temp, 11) + BitRead(Temp, 10) \
//     // + BitRead(Temp, 8) + BitRead(Temp, 7) + BitRead(Temp, 5) + BitRead(Temp, 4) + BitRead(Temp, 2) + BitRead(Temp, 1) + 1; 
//     return crc; 
// }

unsigned char GetCRC(const unsigned char * data, const unsigned int size)
{
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

// byte GetCRC(const byte *data, byte len) {
//   byte crc = 0x00;
//   while (len--) {
//     byte extract = *data++;
//     for (byte tempI = 8; tempI; tempI--) {
//       byte sum = (crc ^ extract) & 0x01;
//       crc >>= 1;
//       if (sum) {
//         crc ^= 0x8C;  //DEBUG! Return to 0x8C
//       }
//       extract >>= 1;
//     }
//   }
//   return crc;
// }

// uint8_t BitRead(unsigned int Val, uint8_t Pos)  //macro for reading a bit at a position 
// {
//     return (Val >> Pos) & 1; 
// }

uint8_t CharToInt(char *Data) 
{
    return (Data[0] - 48)*10 + (Data[1] - 48);
}

// long CharToLong(char *Data, uint8_t Len) 
// {
//     unsigned long Temp = 0; //Fix to work with signed longs??
//     for(int i = 0; i < Len; i++) {
//         Temp = Temp + (Data[i] - 48)*pow(10, i);
//     }
//     return Temp;
// }

float GetData(uint8_t RegID) 
{
    Format = 'd'; //Default to decimal 
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
            return GetValues();
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
            return Baud*1200;
            break;

        case 97:  //Set baud rate
            // Format = 'd';
            return Baud*1200;
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

void Reset() 
{
    ADR[0] = '1';
    ADR[1] = '3';
    Baud = 4; 

    LoadEEPROM();

    Serial.println("[r]");
}

void LoadEEPROM() 
{
    EEPROM.write(0x01, ADR[0]);
    EEPROM.write(0x02, ADR[1]);
    EEPROM.write(0x03, Baud);
    EEPROM.write(0x00, 'L'); //Write value to adr 0x00 to mark EEPROM as written 
}
