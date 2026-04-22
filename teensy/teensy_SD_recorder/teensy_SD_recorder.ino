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
const int chipSelect = 10; //SD card on teensy audio board
int fileCount = 0; //how many files are created in current recording Session

const char* dirPrefix = "/Session_"; //prefix of the directory name
int dirCount = 0; //counts how many directories there are, to create next directory
char nextDirName[20]; //name of the next directory to be created

//Audio settings
const int teensy_audio_memory = 120; //increase if unstable. how much memory is given to the audio:
//50 – 200  → normal use
//200 – 400 → heavy audio processing
//400 – 600 → high-risk but sometimes OK

#define NBLOCKS 16 //can be increased to 32 if unstable. will increase buffer
#define SAMPLES_PER_BLOCK AUDIO_BLOCK_SAMPLES //use same sample per block number as in teensy audio library (128)

int16_t buffer[NBLOCKS * SAMPLES_PER_BLOCK * 2]; // stereo
uint32_t totalSamples = 0; //counts how many samples were recorded to measure total data file size

//information for WAV file header
uint32_t subChunk1Size = 16;
uint16_t audioFormat   = 1;      // PCM
uint16_t numChannels    = 2;      // stereo
uint32_t sampleRate    = 44100;
uint16_t bitsPerSample = 16;

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=882,406
AudioRecordQueue         queueR;         //xy=1168,489
AudioRecordQueue         queueL;         //xy=1182,356
AudioConnection          patchCord1(i2s1, 0, queueL, 0);
AudioConnection          patchCord2(i2s1, 1, queueR, 0);
// GUItool: end automatically generated code

//functions

/// writes wav file header
/// expects file size
/// AI generated and double checked with wikipedia and i can grasp it a bit but could never write this
/// and yes there are libraries for that but don't get me started
/// sorry :(
void writeWavHeader(uint32_t dataSize) {
  recFile.seek(0); //go to file start

  //write standardized expected data that is needed to make the file a WAV file
  recFile.write("RIFF", 4);
  uint32_t chunkSize = 36 + dataSize;
  recFile.write((uint8_t*)&chunkSize, 4);
  recFile.write("WAVE", 4);

  recFile.write("fmt ", 4);
  
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint16_t blockAlign = numChannels * bitsPerSample / 8;

  recFile.write((uint8_t*)&subChunk1Size, 4);
  recFile.write((uint8_t*)&audioFormat, 2);
  recFile.write((uint8_t*)&numChannels, 2);
  recFile.write((uint8_t*)&sampleRate, 4);
  recFile.write((uint8_t*)&byteRate, 4);
  recFile.write((uint8_t*)&blockAlign, 2);
  recFile.write((uint8_t*)&bitsPerSample, 2);

  recFile.write("data", 4);
  recFile.write((uint8_t*)&dataSize, 4);
}

/// starts SD card recording
void record_start(){
  if (setNextRecordingName()) { //set the recording file name
    //yaaaaa letzgoooo!
  } else {
    //we're fucked, but it'll be alright
  }

  //open output file in write mode
  recFile = SD.sdfs.open(nextRecFileName, O_WRITE | O_CREAT);

  // reserve WAV header. first 44 bytes will contain file information
  for (int i = 0; i < 44; i++) recFile.write((uint8_t)0);

  //start recording queues
  queueL.begin();
  queueR.begin();

  blnRecording = true;
  digitalWrite(ledPin, blnRecording);
  Serial.println("recording...");
}

/// stops SD card recording
void record_end(){

    queueL.end();
    queueR.end();

    //will not fetch the remaining blocks in the queue, so some milliseconds are lost at the end.

    //calculate data file size, needed to write the WAV header
    uint32_t dataSize = totalSamples * numChannels * (bitsPerSample / 8); //samples count, stereo(2 channels), 16bit(2 bytes)

    //write wav header
    writeWavHeader(dataSize);

    recFile.close();
    blnRecording = false;
    digitalWrite(ledPin, blnRecording);
    Serial.println("recording end");
}

///looped part of recording
///streams audio to opened file while recording is active
///right and left channel are interleaved (R L R L R L ...) 
void recordingLoop() {
  //crashes ---> //if (queueL.available() && queueR.available()) { // check both queues if they have data
  if (queueL.available() >= NBLOCKS && queueR.available() >= NBLOCKS) { //check both queues if they have required amount of blocks inside
    int idx = 0; //index which bit in the buffer we're at

    for (int b = 0; b < NBLOCKS; b++) { //loop through blocks fast to free up queue
      int16_t *l = (int16_t*)queueL.readBuffer(); //write block from left queue to l byte
      int16_t *r = (int16_t*)queueR.readBuffer(); //write block from right queue to r byte

      for (int i = 0; i < SAMPLES_PER_BLOCK; i++) { //loop through every sample in both l and r byte
        //this will write the bits alternating into the buffer
        buffer[idx++] = l[i]; //write l bit to buffer, jump one bit forward in buffer
        buffer[idx++] = r[i]; //write r bit to buffer, jump one bit forward in buffer
        totalSamples++; //increase total samples written. to count file size in the end. Will be multiplied by 2 as we have 2 channels
      }
      
      //buffer is freed, so new audio data can come in even if SD write is not done yet
      queueL.freeBuffer();
      queueR.freeBuffer();
    }

    // actually write buffer to SD
    // this might be slow, but the buffer can fill again as we've freed it before.
    // audio data is still being retreived
    recFile.write((uint8_t*)buffer, idx * 2);
  }
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

  AudioMemory(teensy_audio_memory); //give much memory
  lastBtnState = digitalRead(recBtnPin);
  
  //expose SD card to computer when connected via USB
  MTP.begin();
  MTP.addFilesystem(SD, "OR_JHR");
}

void loop() {
  currentMillis = millis(); //store current ms timestamp
  
  //write audio stream to SD card if recording is armed
  if (blnRecording) {
    recordingLoop();
  } else {
    //update file system access only when not recording
    MTP.loop();
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
