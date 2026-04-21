//Many examples from https://github.com/PaulStoffregen/SD/blob/Juse_Use_SdFat/examples/SdFat_Usage/SdFat_Usage.ino

/// opens SD card connection and performs IO Test
/// returns true if test successfull, otherwise false
bool sd_setup(){
  bool blnSuccess = false;
  Serial.println("initializing SD Card...");
  if(!SD.sdfs.begin(SdSpiConfig(chipSelect, SHARED_SPI, SD_SCK_MHZ(24)))){ //fast, but can be faster, see  https://github.com/PaulStoffregen/SD/blob/Juse_Use_SdFat/examples/SdFat_Usage/SdFat_Usage.ino
    //Serial.println("Card Mount Failed");
    error_blink(0);
    return blnSuccess;
  }
 
  // if(???){
  //   //Serial.println("No SD card attached");
  //   error_blink(1);
  //   return blnSuccess;
  // }
  if(!createSessionDir()){
    Serial.println("No SD card attached");
    error_blink(1);
    return blnSuccess;
  } else {
    blnSuccess = true;
    return blnSuccess;
  }
  
  // Serial.print("SD Card Data: ");
  // Serial.print(SD.exists());
  // Serial.print(" | ");
  // Serial.print(SD.position());
  // Serial.print(" | ");
  // Serial.print(SD.size());
  // Serial.print(" | ");
  // Serial.print(SD.mediaPresent());

  //writeFile(SD, "/test.txt", "Test File Working"); //create a test file if it does not exist
  //testFileIO(SD, "/test.txt");
}

void sd_loop() {
  Serial.println("running");
  //delay(5000);
  //listDir(SD, "/", 0);
  //createDir(SD, "/mydir");
  //listDir(SD, "/", 0);
  //removeDir(SD, "/mydir");
  //listDir(SD, "/", 2);
  //writeFile(SD, "/hello.txt", "Hello ");
  //appendFile(SD, "/hello.txt", "World!\n");
  //readFile(SD, "/hello.txt");
  //deleteFile(SD, "/foo.txt");
  //renameFile(SD, "/hello.txt", "/foo.txt");
  //readFile(SD, "/foo.txt");
  //testFileIO(SD, "/test.txt");
  //Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  //Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

/// creates directory for current session
/// session means one boot. at next reboot, new dir is created
/// called once at setup
bool createSessionDir(){
  SD.sdfs.mkdir(session_root_dir);
  FsFile root = SD.sdfs.open(session_root_dir, O_RDONLY);
  dirCount = 0;
  if (!root){
    //Serial.println("Failed to open directory");
    error_blink(2);
    return false;
  }

  if(!root.isDirectory()){
    //Serial.println("Not a directory");
    error_blink(3);
    return false;
  }

  root.rewindDirectory();
  //loop through all files in root dir to count directories
  while (true) {
    FsFile entry = root.openNextFile();
    if (!entry) break; //no more files
    if (entry.isDirectory()) {
      dirCount++; //count directory count up
    }
    entry.close();
  }

  Serial.print("counted directories: ");
  Serial.println(dirCount);
  Serial.print("creating next directory: ");
  dirCount++; //increment by one as I want to create next dir
  snprintf(nextDirName, sizeof(nextDirName), "%s%s%d", session_root_dir, dirPrefix, dirCount);
  Serial.println(nextDirName);

  if(SD.sdfs.mkdir(nextDirName)){
    Serial.println("Dir created");
    return true;
    root.close();
  } else {
    Serial.println("mkdir failed");
    error_blink(4);
    return false;
  }
}

bool setNextRecordingName(){
  SD.sdfs.mkdir(nextDirName);
  FsFile session = SD.sdfs.open(nextDirName, O_RDONLY);

  if(!session){
    //Serial.println("Failed to open directory");
    error_blink(2);
    return false;
  }
  if(!session.isDirectory()){
    //Serial.println("Not a directory");
    error_blink(3);
    return false;
  }


  fileCount=0; //reset before counting
  session.rewindDirectory();
  //loop through all files in root dir to count directories
  while (true) {
    FsFile file = session.openNextFile();
    if (!file) break; //no more files
    fileCount++; //count file count up
    file.close();
  }

  fileCount++; //increment by one as I want to create next file
  //%s = char, %d = int
  snprintf(nextRecFileName, sizeof(nextRecFileName), "%s%s%d%s", nextDirName, file_prefix, fileCount, file_suffix);
  Serial.print("next file will be: ");
  Serial.println(nextRecFileName);
  return true;
}

