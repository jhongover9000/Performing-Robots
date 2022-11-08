// Music Maker using nRF24L01 and potentiometer
// Joseph Hong
// Description: Code to transmit a potentiometer signal to change music tracks. Using base code.
// NOTE: Need to rename MP3 files to "0.mp3", "1.mp3", etc.

#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include <string.h>

// define the pins used
//#define CLK 13       // SPI Clock, shared with SD card
//#define MISO 12      // Input data, from VS1053/SD card
//#define MOSI 11      // Output data, to VS1053/SD card
// Connect CLK, MISO and MOSI to hardware SPI pins. 
// See http://arduino.cc/en/Reference/SPI "Connections"

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin
 
int pastData = 0;
int currentTrack = 0;
int totalTrackCount = 7;
int currentTrackData = 0;
bool startFresh = true;

// nRF24L01 uses SPI which is fixed on pins 11, 12, and 13.
// It also requires two other signals
// (CE = Chip Enable, CSN = Chip Select Not)
// Which can be any pins:
const int CEPIN = 9;
const int CSNPIN = 10;
// In summary,
// nRF 24L01 pin    Arduino pin   name
//          1                     GND
//          2                     3.3V
//          3             9       CE
//          4             10      CSN
//          5             13      SCLK
//          6             11      MOSI/COPI
//          7             12      MISO/CIPO
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(CEPIN, CSNPIN);                // CE, CSN
// Byte of array representing the address.
// This is the address where we will send the data.
// This should be same on the receiving side.
const byte address[6] = "1100";
Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

  void setup() {
    Serial.begin(9600);
    Serial.println("Adafruit VS1053 Simple Test");

    if (! musicPlayer.begin()) { // initialise the music player
      Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
      while (1);
    }
    Serial.println(F("VS1053 found"));
    
    if (!SD.begin(CARDCS)) {
      Serial.println(F("SD failed, or not present"));
      while (1);  // don't do anything more
    }

    // list files
    printDirectory(SD.open("/"), 0);
    
    // Set volume for left, right channels. lower numbers == louder volume!
    musicPlayer.setVolume(20,20);

    // Timer interrupts are not suggested, better to use DREQ interrupt!
    //musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int

    // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
    // audio playing
    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

    // RF24 setup
    if (!radio.begin()) {
      Serial.println("radio  initialization failed");
      while (1)
        ;
    } else {
      Serial.println("radio successfully initialized");
    }
    char* trackTitle = '/' + char(currentTrack) + '.mp3';
    // Play one file, don't return until complete
    Serial.println(F("Playing track 001"));
    musicPlayer.startPlayingFile("/1.mp3");
    // Play another file in the background, REQUIRES interrupts!
    Serial.println(F("Playing track 002"));
    // musicPlayer.startPlayingFile("/1.mp3");

    radio.openReadingPipe(0, address);  //destination addres
    radio.setPALevel(RF24_PA_MIN);   // min or max
    radio.startListening();           //This sets the module as transmitter
    
    Serial.println(F("Setup Success"));
  }

  void loop() {
    uint8_t pipeNum;
    if (radio.available(&pipeNum))  //Looking for the data.
    {
      int data;
      radio.read(&data, sizeof(data));  //Reading the data
      // Serial.print("data = ");
      // if(pastData!=data){Serial.println(data);}
      // if no buttons are pressed turn off lights
      if(startFresh){
        pastData = data;
        currentTrackData = data;
        startFresh = false;
      }
      // if potentiometer is turned, take the value and map music track
      // if((pastData-100) < data && data < (pastData+100)){
        // check if track is the same. if not, update
        int track = int(map(data, 0, 10234, 0, totalTrackCount));
        Serial.println(track);
        Serial.println("Track: " + String(track)+"  Cur. Track Data: "+String(currentTrackData)+"  Data: "+String(data));
        // ((currentTrackData-500 < data)&&(data < currentTrackData+500))
        if (track != currentTrack){
          // Serial.println("New Track Detected: " + String(track));
          musicPlayer.stopPlaying();
          char trackTitle[50];
          sprintf(trackTitle, "/%d.mp3", track); 
          Serial.println("Now Playing" + String(trackTitle));       
          musicPlayer.startPlayingFile(trackTitle);
          currentTrack = track;
          currentTrackData = data;
        }
      // }
      pastData = data;
    }
  
  }


/// File listing helper
void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}