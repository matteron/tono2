#include <Wire.h>
#include <SPI.h>
#include <Bounce.h>  // For the Buttons
#include <SD.h>

#define BUTTON_FWD 30 // Next Track
#define BUTTON_PAU 31 // Play Pause
#define BUTTON_BCK 32 // Prev Track

Bounce bouncer_fwd = Bounce(BUTTON_FWD, 50);
Bounce bouncer_pau = Bounce(BUTTON_PAU, 50);
Bounce bouncer_bck = Bounce(BUTTON_BCK, 50);

// SD Card Pins Definition
#define SDCARD_CS_PIN 10
#define SDCARD_MOSI_PIN 7
#define SDCARD_SCK_PIN 14

#define SLOT0 33 // card pin 1
#define SLOT1 34 // card pin 2
#define SLOT2 35 // card pin 3
#define SLOT3 36 // card pin 4
#define SLOT4 37 // card pin 5
#define SLOT5 38 // card pin 6
#define SLOT6 39 // card pin 7
#define SLOT7 25 // card pin 8
#define SLOT8 24 // card pin 9

#define SLOT_HIGH 1

int slotPins[] = {SLOT0, SLOT1, SLOT2, SLOT3, SLOT4, SLOT5, SLOT6, SLOT7, SLOT8};
int pinAmount = sizeof(slotPins)/sizeof(int);

int album;
bool albumInvalid;
int track;
int tracknum;

bool trackext[255]; // false = mp3, true = flac
String tracklist[255];
char playthis[15];
File root;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  
  // Setup buttons with pullups
  pinMode(BUTTON_FWD, INPUT_PULLUP);
  pinMode(BUTTON_PAU, INPUT_PULLUP);
  pinMode(BUTTON_BCK, INPUT_PULLUP);

  // Setup slot loader pins and state
  for(int n = 0; n < pinAmount; n++){
    pinMode(slotPins[n], INPUT_PULLDOWN);
  }
  
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  track = 0;
  loadCartridge();
  loadAlbum();
}

// Loops until cartridge is inserted
void loadCartridge(){

  album = validateCartridge(15);
  
  Serial.print("Cartridge number ");
  Serial.print(album);
  Serial.println(" loaded.");

  int bitCount = 0;
  for(int n = (pinAmount-1); n >= 0; n--){
    bitCount += digitalRead(slotPins[n]);
    Serial.print(digitalRead(slotPins[n]));
  }
  Serial.println();
  Serial.print(bitCount);
  Serial.println(" bits read");
}

int readCartridge() {
  int i = 0;
  for(int n = 0; n < pinAmount; n++){
    i = i | (digitalRead(slotPins[n]) << n);
  }
  return i;
}

int validateCartridge(int parity) {
  albumInvalid = true;
  int validators[parity];
  int v0;
  while(albumInvalid){

    for(int n = 0; n < parity; n++){
      validators[n] = readCartridge();
    }
    v0 = validators[0];
    for(int n = 1; n < parity; n++){
      albumInvalid = !(v0 == validators[n]);
      if(albumInvalid){
        break;
      }
    }
  }
  return v0;
}

void loadAlbum(){
  Serial.println("/" + String(album));
  root = SD.open("/3");

  while(true) {
    
    File files = root.openNextFile();
    if(!files) {
      Serial.println("end of files");
      // Loaded all files
      break;
    }
    
    String curfile = files.name();
    Serial.println(curfile);
    // Check if file is MP3 or FLAC
    int m = curfile.lastIndexOf(".MP3");
    int f = curfile.lastIndexOf(".FLAC");

    if(m > 0 || f > 0){
      tracklist[tracknum] = files.name();
      if(m > 0) trackext[tracknum] = false;
      if(f > 0) trackext[tracknum] = true;

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
  Serial.println("Setup Complete!");
}

void controls() {
  bouncer_fwd.update();
  bouncer_pau.update();
  bouncer_bck.update();

  if(bouncer_fwd.fallingEdge()){
    Serial.println("NEXT TRACK!");
  }
  if(bouncer_pau.fallingEdge()){
    Serial.println("PLAY/PAUSE TRACK!");
  }
  if(bouncer_bck.fallingEdge()){
    Serial.println("PREV TRACK!");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  controls();
}
