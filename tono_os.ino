#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Bounce.h>  // For the Buttons

// Requires Arduino-Teensy-Codec-lib
// https://github.com/FrankBoesing/Arduino-Teensy-Codec-lib
#include <play_sd_flac.h> // flac decoder
#include <play_sd_mp3.h> // mp3 decoder

#define BUTTON_FWD 30 // Next Track
#define BUTTON_PAU 31 // Play Pause
#define BUTTON_BCK 32 // Prev Track

#define SDCARD_CS_PIN 10
#define SDCARD_MOSI_PIN 7
#define SDCARD_SCK_PIN 14

#define SLOT_NOTICE 24 // card pin 9
#define SLOT0 25 // card pin 1
#define SLOT1 39 // card pin 2
#define SLOT2 38 // card pin 3
#define SLOT3 37 // card pin 4
#define SLOT4 36 // card pin 5
#define SLOT5 35 // card pin 6
#define SLOT6 34 // card pin 7
#define SLOT7 33 // card pin 8

Bounce bouncer_fwd = Bounce(BUTTON_FWD, 50);
Bounce bouncer_pau = Bounce(BUTTON_PAU, 50);
Bounce bouncer_bck = Bounce(BUTTON_BCK, 50);

const int chipSelect = 10; //if using another pin for SD card CS.
int slotPins[8] = {SLOT0, SLOT1, SLOT2, SLOT3, SLOT4, SLOT5, SLOT6, SLOT7};
int album;
int track;
int tracknum;
int trackext[255]; // 0 = nothing, 1 = mp3, 2 = flac
String tracklist[255];
File root;
char playthis[15];
boolean trackChange;
boolean paused;
boolean slotEmpty;

AudioPlaySdMp3    playMp3;
AudioPlaySdFlac   playFlac;
AudioOutputI2S    audioOutput;

AudioMixer4       mixLeft;
AudioMixer4       mixRight;

// mp3 Connection
AudioConnection   patch1(playMp3, 0, mixLeft, 0);
AudioConnection   patch2(playMp3, 1, mixRight, 0);

// flac Connection
AudioConnection   patch3(playFlac, 0, mixLeft, 1);
AudioConnection   patch4(playFlac, 1, mixRight, 1);

// AudioBoard
AudioConnection   patch5(mixLeft, 0, audioOutput, 0);
AudioConnection   patch6(mixRight, 0, audioOutput, 1);
AudioControlSGTL5000 sgtl5000_1;


void setup() {
  Serial.begin(115200);
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);

  // Setup buttons with pullups
  pinMode(BUTTON_FWD, INPUT_PULLUP);
  pinMode(BUTTON_PAU, INPUT_PULLUP);
  pinMode(BUTTON_BCK, INPUT_PULLUP);

  // Setup slot loader pins and state
  pinMode(SLOT_NOTICE, INPUT);
  pinMode(SLOT0, INPUT);
  pinMode(SLOT1, INPUT);
  pinMode(SLOT2, INPUT);
  pinMode(SLOT3, INPUT);
  pinMode(SLOT4, INPUT);
  pinMode(SLOT5, INPUT);
  pinMode(SLOT6, INPUT);
  pinMode(SLOT7, INPUT);
  slotEmpty = true;
  
  // Audio Connection Memory and Gain
  // Mp3 file might clip with full gain
  AudioMemory(16);
  mixLeft.gain(0,0.9);
  mixRight.gain(0,0.9);
  
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  track = 0;
}

// Loops until cartridge is inserted
void loadCartridge(){
  while(slotEmpty){
    slotEmpty = digitalRead(SLOT_NOTICE);
  }
  Serial.println("Cartridge Get!");
  
  album = 0;
  for(int n = 0; n < 8; n++){
    album = album | (digitalRead(slotPins[n]) << n);
  }
  Serial.print("Cartridge number ");
  Serial.print(album);
  Serial.println(" loaded.");
  Serial.println("Setting up file paths...");
  loadAlbum();
}

void loadAlbum(){
  root = SD.open("/" + album);

  while(true) {
    File files = root.openNextFile();
    if(!files) {
      // Loaded all files
      break;
    }
    
    String curfile = files.name();
    // Check if file is MP3 or FLAC
    int m = curfile.lastIndexOf(".MP3");
    int f = curfile.lastIndexOf(".FLAC");

    if(m > 0 || f > 0){
      tracklist[tracknum] = files.name();
      if(m > 0) trackext[tracknum] = 1;
      if(f > 0) trackext[tracknum] = 2;

      Serial.print(trackext[tracknum]);
      Serial.print(" - ");
      Serial.print(tracklist[tracknum]);
      Serial.println(" loaded succesfully.");
      tracknum++;
    }
    files.close();
  }

  tracklist[track].toCharArray(playthis, sizeof(tracklist[track]));

  Serial.print("Album number ");
  Serial.print(album);
  Serial.println(" fully loaded.");
}

void playFileMP3(const char *filename){
  
}

void playFileFLAC(const char *filename){
  
}

void controls() {
  
}

void nextTrack(){
  
}

void prevTrack(){
  
}

void pauseTrack(){
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if(slotEmpty){
    loadCartridge();
  } else {
    
  }
}
