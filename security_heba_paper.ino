#include <SPI.h>
#include <SD.h>

///////////////////////
// SD Logging Config //
///////////////////////
#define ENABLE_SD_LOGGING true // Default SD logging (can be changed via serial menu)
#define LOG_FILE_INDEX_MAX 999 // Max number of "logXXX.txt" files
#define LOG_FILE_PREFIX "log"  // Prefix name for log files
#define LOG_FILE_SUFFIX "txt"  // Suffix name for log files
#define SD_MAX_FILE_SIZE 5000000 // 5MB max file size, increment to next file before surpassing
#define SD_LOG_WRITE_BUFFER_SIZE 1024 // Experimentally tested to produce 100Hz logs
#define SD_CHIP_SELECT_PIN 8
/////////////////////
// SD Card Globals //
/////////////////////
bool sdCardPresent = false; // Keeps track of if SD card is plugged in
String logFileName; // Active logging file
String logFileBuffer; // Buffer for logged data. Max is set in config
/////////////////////////////
// variable initialization //
/////////////////////////////
#define vdd 6 // pin for supply voltage
#define vgs 3 // pin for gate source voltage of the transistor
int V1p =  A0; // reading V1 pin
int V2p =  A1; // reading V2 pin
int V3p = A2; // reading V3 pin
float Icom = 0.7E-6; // maximum current defined by the gate voltage
float Icom95 = Icom * 0.95;
float I = 0.0;
float R = 100E3; // the fixed resistor connected with the memristor
float Ron = 0; // ON resistance
float Vs = 0; // current voltage supply value
float VR = 0;
float VM = 0;

void setup() {
  Serial.begin(9600);
  pinMode(vdd, OUTPUT); // define supply voltage pin
  pinMode(vgs, OUTPUT); // define gate voltage pin
  digitalWrite(vdd, LOW);
  analogWrite(vgs, 69); // write 1.5 V to the gate of the transistor to limit the current to Icomp

  // SD card initlization
  if ( initSD() )
  {
    sdCardPresent = true;
    // Get the next, available log file name
    logFileName = nextLogFile();
  }

}

void loop() {
  int V1D = 0; // V1 digital value
  int V2D = 0; // V2 digital value
  int V3D = 0; // V3 digital value
  float V1A = 0; // V1 analog value
  float V2A = 0; // V2 analog value
  float V3A = 0; // V3 analog value

  Vs = Vs + 2; // increment source voltage by 0.04 V
  analogWrite(vdd, Vs); // write value to supply pin
  delay(500);
  V1D = analogRead(V1p); // reading the digital value of V1
  V2D = analogRead(V2p); // reading the digital value of V2
  V3D = analogRead(V3p); // reading the digital value of V3
  V1A =  V1D / 204.80; // conver the digital reading value to analog voltage value;
  V2A =  V2D / 204.80; // conver the digital reading value to analog voltage value;
  V3A =  V3D / 204.80; // conver the digital reading value to analog voltage value;
  VR = V1A - V2A; // voltage across the resistor
  VM = V2A - V3A; // voltage across the memristor
  I = (VR / R); // calculate the current in the circuit

  if (I >= Icom)
  {
    Serial.print("Memristor is ON");
    Serial.println();
    delay(500);

    do {
      Vs = Vs - 1; // decrement the power supply
      analogWrite(vdd, Vs); // write value to supply pin
      delay (500);
      V1D = analogRead(V1p); // reading the digital value of V1
      V2D = analogRead(V2p); // reading the digital value of V2
      V3D = analogRead(V3p); // reading the digital value of V3
      V1A =  V1D / 204.80; // conver the digital reading value to analog voltage value;
      V2A =  V2D / 204.80; // conver the digital reading value to analog voltage value;
      V3A =  V3D / 204.80; // conver the digital reading value to analog voltage value;
      VR = V1A - V2A; // voltage across the resistor
      VM = V2A - V3A; // voltage across the memristor
      I = VR / R; // calculate the current in the circuit
      if (Vs == 0)
      {
        break;
      }

    } while (I >= Icom95);
    Ron = (VM) / I; //calculate Ron


    Vs = 0; // reset the supply voltage for next cycle
    analogWrite(vdd, Vs); // write value to supply pin

    // log  Ron and printed to the serial monitor
    Serial.print("Ron = ");
    Serial.print(long(Ron));
    Serial.println();
    if (sdCardPresent)
      logIMUData(); // Log new data

    Ron = 0;
    //        ArrayCounter = 0;

    delay(1000);
  }
}

// function that log RON
void logIMUData(void)
{
  String imuLog = "";
  imuLog += String(Ron) + ", ";

  // Remove last comma/space:
  imuLog.remove(imuLog.length() - 2, 2);
  imuLog += "\r\n"; // Add a new line

  logFileBuffer = ""; // Clear SD log buffer

  // Add new line to SD log buffer
  logFileBuffer += imuLog;

  sdLogString(logFileBuffer); // Log SD buffer
}

// Log a string to the SD card
bool sdLogString(String toLog)
{
  // Open the current file name:
  File logFile = SD.open(logFileName, FILE_WRITE);

  // If the file will get too big with this new string, create
  // a new one, and open it.
  if (logFile.size() > (SD_MAX_FILE_SIZE - toLog.length()))
  {
    logFileName = nextLogFile();
    logFile = SD.open(logFileName, FILE_WRITE);
  }

  // If the log file opened properly, add the string to it.
  if (logFile)
  {
    logFile.print(toLog);
    logFile.close();
    return true; // Return success
  }

  return false; // Return fail
}

bool initSD(void)
{
  // SD.begin should return true if a valid SD card is present
  if ( !SD.begin(SD_CHIP_SELECT_PIN) )
  {
    return false;
  }

  return true;
}

String nextLogFile(void)
{
  String filename;
  int logIndex = 0;

  for (int i = 0; i < LOG_FILE_INDEX_MAX; i++)
  {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String(LOG_FILE_PREFIX);
    filename += String(logIndex);
    filename += ".";
    filename += String(LOG_FILE_SUFFIX);
    // If the file name doesn't exist, return it
    if (!SD.exists(filename))
    {
      return filename;
    }
    // Otherwise increment the index, and try again
    logIndex++;
  }

  return "";
}
