#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <Bounce.h>

#ifdef __AVR__
#define FsFile File32
#endif

// VARS
int ledPin = 14;
int recBtnPin = 16;
bool blnRecording = false;
bool stateChanged = false; //if the button state already changed
bool btnState = false;
bool lastBtnState = false;
float currentMillis; //current ms timestamp
float btnPressMillis; //ms Timestamp when btn was pressed
int debounceMillis = 50; //button debounce time in ms

//SDCard Stuff

const char *session_root_dir = "/JHR"; //root directory where the files are stored
const char *file_prefix = "/aufnahme_"; //prefix of rec file
const char *file_suffix = ".wav"; //suffix of rec file
char nextRecFileName[64]; //full name of next file to be recorded
FsFile recFile; //Stream that will be saved to SD Card
const int chipSelect = 10; //teensy audio board
int fileCount = 0; //how many files are created in current recording Session

const char* dirPrefix = "/Session_"; //prefix of the directory name
int dirCount = 0; //counts how many directories there are, to create next directory
char nextDirName[20]; //name of the next directory to be created

//SD Write Buffering
#define NBLOCKS 16
#define SAMPLES_PER_BLOCK AUDIO_BLOCK_SAMPLES

int16_t buffer[NBLOCKS * SAMPLES_PER_BLOCK * 2]; // stereo
int bufIndex = 0;

uint32_t totalSamples = 0;

//Audio I2s Teensy Audio Shield stuff

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=882,406
AudioRecordQueue         queueR;         //xy=1168,489
AudioRecordQueue         queueL;         //xy=1182,356
AudioConnection          patchCord1(i2s1, 0, queueL, 0);
AudioConnection          patchCord2(i2s1, 1, queueR, 0);
// GUItool: end automatically generated code

//functions

/// writes wav file header
/// expects file and size
// void writeWavHeader(File &f, uint32_t dataSize) {
//   f.seek(0);

//   f.write("RIFF", 4);
//   uint32_t chunkSize = 36 + dataSize;
//   f.write((uint8_t*)&chunkSize, 4);
//   f.write("WAVE", 4);

//   f.write("fmt ", 4);
  
//   uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
//   uint16_t blockAlign = numChannels * bitsPerSample / 8;

//   f.write((uint8_t*)&subChunk1Size, 4);
//   f.write((uint8_t*)&audioFormat, 2);
//   f.write((uint8_t*)&numChannels, 2);
//   f.write((uint8_t*)&sampleRate, 4);
//   f.write((uint8_t*)&byteRate, 4);
//   f.write((uint8_t*)&blockAlign, 2);
//   f.write((uint8_t*)&bitsPerSample, 2);

//   f.write("data", 4);
//   f.write((uint8_t*)&dataSize, 4);
// }

/// starts SD card recording
void record_start(){
  if (setNextRecordingName()) { //set the recording file name
    //yaaaaa letzgoooo!
  } else {
    //we're fucked, but it'll be alright
  }
  
  //open output file in Write mode
  recFile = SD.sdfs.open(nextRecFileName, O_WRITE | O_CREAT);
  //audioStreamWAVToFile.begin(out, i2sStream);
  blnRecording = true;
  digitalWrite(ledPin, blnRecording);
  Serial.println("recording...");
}

/// stops SD card recording
void record_end(){
    recFile.close();
    blnRecording = false;
    digitalWrite(ledPin, blnRecording);
    Serial.println("recording end");
}


/// flashes the LED with a pattern
/// expects integer with the error number
/// code 0 = Card Mount Failed
/// code 1 = No SD card attached
/// code 2 = Failed to open directory
/// code 3 = Not a directory
/// code 4 = mkdir failed
/// code 5 = rmdir failed
/// code 6 = Failed to open file for reading
/// code 7 = Failed to open file for writing
/// code 8 = Write failed
/// code 9 = Failed to open file for appending
/// code 10 = Append failed
/// code 11 = Rename failed
/// code 12 = Delete failed
/// code 69 = Success!!
/// code 99 = test error

void error_blink(int errorCode){
  Serial.print("Error ");
  Serial.print(errorCode);
  Serial.print(": ");

  switch (errorCode) {
    case 0:
      Serial.println("Card Mount Failed");
    break;

    case 1:
      Serial.println("No SD card attached");
    break;

    case 2:
      Serial.println("Failed to open directory");
    break;

    case 3:
      Serial.println("Not a directory");
    break;

    case 4:
      Serial.println("mkdir failed");
    break;

    case 5:
      Serial.println("rmdir failed");
    break;

    case 6:
      Serial.println("Failed to open file for reading");
    break;

    case 7:
      Serial.println("Failed to open file for writing");
    break;

    case 8:
      Serial.println("Write failed");
    break;

    case 9:
      Serial.println("Failed to open file for appending");
    break;

    case 10:
      Serial.println("Append failed");
    break;

    case 11:
      Serial.println("Rename failed");
    break;

    case 12:
      Serial.println("Delete failed");
    break;

    case 69:
      Serial.println("SD Card INIT Success!!");
    break;

    case 99:
      Serial.println("Test error");
    break;

    default:
     Serial.println("Unknown error!");
    break;
  }
}

/// reads if button was pressed
/// triggers needed actions
void readBtn() {
  btnState = digitalRead(recBtnPin);
  
  if (btnState != lastBtnState){
    if((millis() - btnPressMillis) > debounceMillis){
      //Serial.println("changed");
      btnPressMillis = millis();

      if (btnState && !lastBtnState) {
        //Serial.println("Press");
        //stateChanged = true;
      }

      if (!btnState && lastBtnState) {
        Serial.println("Release");
        stateChanged = true;
      }
      lastBtnState = btnState;
    }
  }
}
void setup() {
  //LED Pin Modes
  pinMode(ledPin, OUTPUT);
  pinMode(recBtnPin, INPUT_PULLUP);

  Serial.begin(115200);

  while(!Serial); // wait for serial to be ready

  //Open SD drive
  if (sd_setup()){
    error_blink(69); //show success message!!
  } else {
    //todo, what now?
    SD.sdfs.ls();
  }

  AudioMemory(100); //give much memory
  lastBtnState = digitalRead(recBtnPin);
}

void loop() {
  currentMillis = millis(); //store current ms timestamp
  
  //write audio stream to SD card if recording is armed
  if (blnRecording) {
    //audioStreamWAVToFile.copy();
  }

  //read if a button has been pressed
  //only read button after debounce time has passed
  readBtn();



  // if button state changed start or stop recording
  if (stateChanged) {
    if (!blnRecording){ //inverted as we want to start when unarmed and stop when armed
      record_start();
    } else {
      record_end();
    }
    
    stateChanged = false;
  }
}
