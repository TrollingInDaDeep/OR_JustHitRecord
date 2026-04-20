/// opens SD card connection and performs IO Test
/// returns true if test successfull, otherwise false
bool sd_setup(){
  bool blnSuccess = false;
  if(!SD.begin(chipSelect)){
    //Serial.println("Card Mount Failed");
    error_blink(0);
    return blnSuccess;
  }
  uint8_t cardType = SD.card()->type();

  if(cardType == CARD_NONE){
    //Serial.println("No SD card attached");
    error_blink(1);
    return blnSuccess;
  }

  Serial.print("SD Card Type: ");
  Serial.println(cardType);

  //writeFile(SD, "/test.txt", "Test File Working"); //create a test file if it does not exist
  //testFileIO(SD, "/test.txt");

  // Card size (raw)
  uint64_t cardSizeMB = (sd.card()->sectorCount() * 512) / (1024 * 1024);

  // Filesystem usage
  uint64_t totalBytes = sd.vol()->clusterCount() * sd.vol()->bytesPerCluster();
  uint64_t freeBytes  = sd.vol()->freeClusterCount() * sd.vol()->bytesPerCluster();
  uint64_t usedBytes  = totalBytes - freeBytes;

  Serial.printf("Card size:   %llu MB\n", cardSizeMB);
  Serial.printf("Total space: %llu MB\n", totalBytes / (1024 * 1024));
  Serial.printf("Used space:  %llu MB\n", usedBytes / (1024 * 1024));
  return blnSuccess;
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
