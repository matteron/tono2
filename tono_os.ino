// This version of the os uses the cartridge itself to act
// as a power switch of sorts.  It simply closes the circuit.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Bounce.h>  // For the Buttons

// Requires Arduino-Teensy-Codec-lib
// https://github.com/FrankBoesing/Arduino-Teensy-Codec-lib
#include <play_sd_flac.h> // flac decoder
#include <play_sd_mp3.h> // mp3 decoder

// Button Pins
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

// Cartridge Slot Pins

#define SLOT0 33 // card pin 1
#define SLOT1 34 // card pin 2
#define SLOT2 35 // card pin 3
#define SLOT3 36 // card pin 4
#define SLOT4 37 // card pin 5
#define SLOT5 38 // card pin 6
#define SLOT6 39 // card pin 7
#define SLOT7 25 // card pin 8
#define SLOT8 24 // card pin 9

#define PARITY 15 // Number of validations to perform on slot.

int slotPins[] = {SLOT0, SLOT1, SLOT2, SLOT3, SLOT4, SLOT5, SLOT6, SLOT7, SLOT8};
int pinAmount = sizeof(slotPins)/sizeof(int);

int album;
int track;
int tracknum;
String filepath;

bool trackext[255]; // false = mp3, true = flac
String tracklist[255];
char playthis[15];
File root;

boolean trackChange;
boolean paused;

AudioPlaySdMp3    sourceMP3;
AudioPlaySdFlac   sourceFLAC;
AudioOutputI2S    audioOutput;

AudioMixer4       mixLeft;
AudioMixer4       mixRight;

// mp3 Connection
AudioConnection   patch1(sourceMP3, 0, mixLeft, 0);
AudioConnection   patch2(sourceMP3, 1, mixRight, 0);

// flac Connection
AudioConnection   patch3(sourceFLAC, 0, mixLeft, 1);
AudioConnection   patch4(sourceFLAC, 1, mixRight, 1);

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
  for(int n = 0; n < pinAmount; n++){
    pinMode(slotPins[n], INPUT_PULLDOWN);
  }
  
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
  loadCartridge();
}

void loadCartridge(){
  album = validateCartridge();
  
  Serial.print("Cartridge number ");
  Serial.print(album);
  Serial.println(" loaded.");
  Serial.println("Setting up file paths...");
  loadAlbum();
}

// Loops through and keeps trying to read cartridge until all checks
//  are satisfied.
int validateCartridge() {
  bool albumInvalid = true;
  int validators[PARITY];
  int v0;
  
  while(albumInvalid){
    for(int n = 0; n < PARITY; n++){
      validators[n] = readCartridge();
    }
    v0 = validators[0];
    for(int n = 1; n < PARITY; n++){
      albumInvalid = !(v0 == validators[n]);
      if(albumInvalid){
        break;
      }
    }
  }
  return v0;
}

// Simple reads all the pins and returns the result.
int readCartridge() {
  int i = 0;
  for(int n = 0; n < pinAmount; n++){
    i = i | (digitalRead(slotPins[n]) << n);
  }
  return i;
}

// Loads the album and prepares the track extension array.
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
    int f = curfile.lastIndexOf(".FLA");

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
  filepath = String(album)+ "/" + tracklist[track];
  filepath.toCharArray(playthis, sizeof(tracklist[track]));

  Serial.print("Album number ");
  Serial.print(album);
  Serial.println(" fully loaded.");
  Serial.println("Setup Complete!");
}

// Plays the file supplied through either the FLAC or MP3 channel
void play(const char *filename){
  trackChange = true;

  Serial.print("Playing: [");
  Serial.print(track);
  Serial.print("] ");
  Serial.println(filename);

  if(trackext[track]){
    sourceFLAC.play(filename);
    while(sourceFLAC.isPlaying()) {
      controls();
    }
  } else { // MP3
    sourceMP3.play(filename);
    while(sourceMP3.isPlaying()) {
      controls();
    }
  }
}

// Checks the buttons for activity and acts upon it.
void controls() {
  bouncer_fwd.update();
  bouncer_pau.update();
  bouncer_bck.update();

  if(bouncer_fwd.fallingEdge()){
    changeTrack(false);
  }
  if(bouncer_pau.fallingEdge()){
    pauseTrack();
  }
  if(bouncer_bck.fallingEdge()){
    changeTrack(true);
  }
}

// Changes the track.  Next track is default behavior.
//  If prev is true, it goes to previous track.
void changeTrack(boolean prev){
  
  trackChange = false;  // Turning off auto change to prevent double skips.
  sourceMP3.stop();
  sourceFLAC.stop();
  
  if(prev){ // Change to previous track
    
    Serial.println("Previous track...");
    track--;
    if(track < 0){
      track = tracknum-1;
    }
    
  } else {  // Change to next track
    
    Serial.println("Next track...");
    track++;
    if(track >= tracknum){
      track = 0;
    }
  
  }
  filepath = String(album)+ "/" + tracklist[track];
  filepath.toCharArray(playthis, sizeof(tracklist[track]));
}

// Pause/Play Function
void pauseTrack(){
  if(paused){
    Serial.println("Resuming Playback...");
  } else {
    Serial.println("Pausing Playback...");
  }
  if(trackext[track]){
    paused = sourceFLAC.pause(!paused);
  } else {
    paused = sourceMP3.pause(!paused);
  }
  
  //might need sourceFLAC to pause too.
}

void loop() {
  Serial.println();
  Serial.print("Album Number: ");
  Serial.println(album);
  Serial.print("Track ");
  Serial.print(track);
  Serial.print(" / ");
  Serial.println(tracknum);

  play(playthis);

  if(trackChange){ // Track has finished from previous code block
    changeTrack(false);
  }
  delay(100);
}
