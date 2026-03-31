/* Open Source Mini Video Player Code - CYD (Cheap Yellow Display) Version
 * Modified for ESP32-2432S028R (2.8" CYD with ILI9341 display)
 * Original by: Alex - Super Make Something
 * CYD adaptation: 2026
 * 
 * Original license: Creative Commons - Attribution - Non-Commercial
 * Original links: https://youtu.be/67RFm2RMjC4
 */

/*
 * Required libraries:
 * https://github.com/moononournation/Arduino_GFX.git
 * https://github.com/earlephilhower/ESP8266Audio.git
 * https://github.com/bitbank2/JPEGDEC.git
 */

 //******NOTE:******* Functional with the following library and board versions: 
 //BOARD: ESP32 v3.3.5 or compatible
 // GFX_Library_for_Arduino: 1.6.4
 // ESP8266Audio: 2.0.0 (version 2.4.1 does NOT work - audio is corrupted)

//Tools - Partition - HUGE App

#define FPS 24 //was 24
// Increased buffer size for 320x240 display (adjust as needed for your video resolution)
#define MJPEG_BUFFER_SIZE (320 * 240 * 2 / 3)// was 2 / 4// 4/4 no audio, 2/2 no audio but video is gerat

#include <WiFi.h>
#include <SD.h>
#include <driver/i2s.h>

//Color names
#define BLACK 0x0000 
#define CYAN 0x07FF
#define PINK 0xF81F
#define YELLOW 0xFFE0
#define GREEN 0x07E0


/* ===== CYD (ESP32-2432S028R) Pin Definitions ===== */
// SD Card pins - CYD uses VSPI (shared with display)
// These are the default VSPI pins for ESP32
int SD_SCK = 18;   // VSPI SCK
int SD_MISO = 19;  // VSPI MISO
int SD_MOSI = 23;  // VSPI MOSI
int SD_CS = 5;     // SD Card CS

// LCD pins (ILI9341) - also uses VSPI but different CS
int LCD_SCK = 14;   // Shared VSPI SCK 18
int LCD_MISO = 12;  // Shared VSPI MISO 12
int LCD_MOSI = 13;  // Shared VSPI MOSI 13
int LCD_DC_A0 = 2;  // CYD DC pin
int LCD_RESET = -1; // CYD doesn't use reset pin (set to -1)
int LCD_CS = 15;    // CYD LCD CS
int LCD_BL = 21;    // CYD backlight pin

// Button pins - use CYD's available GPIO
// Note: CYD has limited free pins. Using some that are available:
int BTN_NEXT = 35;  // Input only pin on CYD (right side connector)
int BTN_PREV = 22;  // Input only pin on CYD (right side connector)
// Alternative: You could use touch screen instead of physical buttons

/* Arduino_GFX - Configured for CYD's ILI9341 display */
#include <Adafruit_GFX.h>
#include <Arduino_GFX_Library.h>

#include <Fonts/FreeSans9pt7b.h> // Include custom font
#include <Fonts/FreeSerif12pt7b.h> // Include another font

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC_A0 /* DC */, LCD_CS /* CS */, LCD_SCK /* SCK */, LCD_MOSI /* MOSI */, LCD_MISO /* MISO */, VSPI /* spi_num */);

/* ILI9341 Configuration for CYD (320x240 display) */
Arduino_GFX *gfx = new Arduino_ILI9341(bus, LCD_RESET /* RST */, 1 /* rotation */, true /* IPS */);

/* MP3 Audio - CYD uses SC8002B amplifier connected to Internal DAC (GPIO 26) */
#include <AudioFileSourceFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
static AudioGeneratorMP3 *mp3 = NULL;
static AudioFileSourceFS *aFile = NULL;
static AudioOutputI2S *out = NULL;

/* MJPEG Video */
#include "MjpegClass.h"
static MjpegClass mjpeg;

static unsigned long total_show_video = 0;

int noFiles = 0; // Number of media files on SD Card in root directory
String videoFilename;
String audioFilename;

int fileNo = 1; // Variable for which (video, audio) filepair to play first
bool buttonPressed = false; // file change button pressed variable
bool fullPlaythrough = true;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200;








File root;

void IRAM_ATTR incrFileNo()
{
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    fileNo += 1;
    if (fileNo > noFiles) // Loop around
    {
      fileNo = 1;
    }
    buttonPressed = true;
    lastDebounceTime = millis();
    fullPlaythrough = false;
    Serial.print("Next Button Pressed on Pin");
    Serial.println(BTN_NEXT);
    Serial.println(" Current Debounce Time: ");
    Serial.println(lastDebounceTime);
  }
}

void IRAM_ATTR decrFileNo()
{
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    fileNo -= 1;
    if (fileNo < 1) // Loop around
    {
      fileNo = noFiles;
    }
    buttonPressed = true;
    lastDebounceTime = millis();
    fullPlaythrough = false;
    Serial.print("Prev Button Pressed on Pin");
    Serial.println(BTN_PREV);
    Serial.println(" Current Debounce Time: ");
    Serial.println(lastDebounceTime);
  }
}

// Code to display file name for 3 seconds
unsigned long textDisplayStartTime = 0;
bool isShowingFilename = false;
const long displayDuration = 3000;

void displayFilename(String filename) {
    //gfx->fillScreen(0x0000);
    gfx->setCursor(10, 10);
    gfx->setTextColor(CYAN);
    gfx->setTextSize(1);  // Larger text for bigger display
    gfx->print("Playing:");
    //gfx->println("");
   // gfx->setTextSize(1);
    gfx->println(videoFilename);
    textDisplayStartTime = millis();
    isShowingFilename = true;
}

void setup()
{
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  
  // Configure backlight
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH); // Turn on backlight





  // Setup buttons (if using physical buttons)
  pinMode(BTN_NEXT, INPUT_PULLUP);  // GPIO 35 is input only, no pullup available
  pinMode(BTN_PREV, INPUT_PULLUP);  // GPIO 22 is input only, no pullup available
  
  // Note: You may want to add external pulldown resistors for these pins
  // Or modify to use touch screen instead

  attachInterrupt(BTN_NEXT, incrFileNo, RISING);
  attachInterrupt(BTN_PREV, decrFileNo, RISING);

// Init audio for CYD's SC8002B amplifier (uses Internal DAC on GPIO 26)
out = new AudioOutputI2S(0, 1, 128);  // 0 = internal DAC mode, 1 = mono// added the 128 to increase buffer size?
out->SetOutputModeMono(true);
//out->SetGain(0.2);  // Adjust volume (0.0 to 1.0)

  mp3 = new AudioGeneratorMP3();
  aFile = new AudioFileSourceFS(SD);

  // Init Video FIRST (this initializes the VSPI bus)
  gfx->begin();
  gfx->fillScreen(0x0000);

  // Print title screen
  typewriterFX();
  gfx->setTextSize(1);
  delay(4000);
  gfx->fillScreen(0x0000);

  // Init SD Card - AFTER display is initialized
  // Display already initialized VSPI, so we just use SD.begin()
  Serial.println("Initializing SD card...");
  
 // SPIClass spi = SPIClass(VSPI);
  //spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  // Don't re-initialize SPI - display already did it
  // Just begin SD card with the CS pin
  if (!SD.begin(SD_CS, SPI, 40000000))  // Use default VSPI that display already initialized
  {
    Serial.println(F("ERROR: SD card mount failed!"));
    Serial.println(F("Troubleshooting:"));
    Serial.println(F("1. Card formatted as FAT32 (not exFAT)?"));
    Serial.println(F("2. Card inserted fully and correctly?"));
    Serial.println(F("3. Card 32GB or smaller?"));
    Serial.println(F("4. Try different/slower card"));
    Serial.println(F("5. Check if card works in computer"));
    
    gfx->setTextColor(0xF800); // Red
    gfx->setTextSize(1);
    gfx->setCursor(10, 60);
    gfx->println(F("SD CARD ERROR"));
    gfx->println();
    gfx->setTextColor(0xFFFF); // White
    gfx->println(F("Check:"));
    gfx->println(F("- FAT32 format"));
    gfx->println(F("- Inserted correctly"));
    gfx->println(F("- 32GB or smaller"));
    gfx->println(F("- Works in computer?"));
    while(1) delay(1000); // Stop here
  }
  else
  {
    Serial.println("SD card initialized successfully!");
    root = SD.open("/");
    noFiles = getNoFiles(root);
    Serial.print("Found ");
    Serial.print(noFiles);
    Serial.println(" file pairs in root directory!");
    Serial.println("Starting playback!");
  }
}

bool isAudioVideoPair(File const &entry, File dir)
{
  #if ESP_IDF_VERSION_MAJOR > 4 || (ESP_IDF_VERSION_MAJOR == 4 && ESP_IDF_VERSION_MINOR >= 4)
  String name = entry.path();
  #else
  String name = entry.name();
  #endif
  
  if (!name.endsWith(".mjpeg"))
    return false;
  
  if (SD.exists(name.substring(0, name.length() - 6) + ".mp3"))
    return true;

  return false;
}

int getNoFiles(File dir)
{
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry)
    {
      // no more files
      break;
    }
    if (entry.isDirectory() || !isAudioVideoPair(entry, dir))
    {
      // Skip file if in subfolder
      entry.close(); // Close folder entry
    }
    else
    {
      entry.close();
      ++noFiles;
    }
  }
  return noFiles;
}

void getFilenames(File dir, int fileNo)
{
  int fileCounter = 0;
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry)
    {
      // no more files
      break;
    }
    if (entry.isDirectory() || !isAudioVideoPair(entry, dir))
    {
      // Skip file if in subfolder
      entry.close(); // Close folder entry
    }
    else // Valid audio/video-pair
    {
      ++fileCounter;
      if (fileCounter == fileNo)
      {
        #if ESP_IDF_VERSION_MAJOR > 4 || (ESP_IDF_VERSION_MAJOR == 4 && ESP_IDF_VERSION_MINOR >= 4)
        videoFilename = entry.path();
        #else
        videoFilename = entry.name();
        #endif
        audioFilename = videoFilename.substring(0, videoFilename.length() - 6) + ".mp3";
        Serial.print("Loading video: ");
        Serial.println(videoFilename);
        Serial.print("Loading audio: ");
        Serial.println(audioFilename);
        
        // Adjusted for larger display
        //gfx->fillRect(10, 90, 300, 60, BLACK);
        //gfx->setTextColor(PINK);
        //gfx->setTextSize(2);
        //gfx->setCursor(30, 100);
        //gfx->println("We'll be right back...");
        //gfx->setCursor(100, 130);
        //gfx->println("At ya!");
      }
      entry.close();
    }
  }
}

void playVideo(String videoFilename, String audioFilename)
{
  int next_frame = 0;
  int skipped_frames = 0;
  unsigned long total_play_audio = 0;
  unsigned long total_read_video = 0;
  unsigned long total_decode_video = 0;
  unsigned long start_ms, curr_ms, next_frame_ms;

  Serial.println("In playVideo() loop!");

  if (mp3 && mp3->isRunning())
    mp3->stop();
  if (!aFile->open(audioFilename.c_str()))
    Serial.println(F("Failed to open audio file"));   
  Serial.println("Created aFile!");

  File vFile = SD.open(videoFilename);
  Serial.println("Created vFile!");

  uint8_t *mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE);
  if (!mjpeg_buf)
  {
    Serial.println(F("mjpeg_buf malloc failed!"));
  }
  else
  {
    // init Video
    mjpeg.setup(&vFile, mjpeg_buf, drawMCU, false, true);
    
    // Debug: Check video dimensions
    Serial.print("Video dimensions: ");
    Serial.print(mjpeg.getWidth());
    Serial.print(" x ");
    Serial.println(mjpeg.getHeight());
  }

  if (!vFile || vFile.isDirectory())
  {
    Serial.println(("ERROR: Failed to open " + videoFilename + ".mjpeg file for reading"));
    gfx->println(("ERROR: Failed to open " + videoFilename + ".mjpeg file for reading"));
  }
  else
  {
    // init audio
    if (!mp3->begin(aFile, out))
      Serial.println(F("Failed to start audio!"));
    Serial.print("MP3 running: ");
    Serial.println(mp3->isRunning());

    start_ms = millis();
    curr_ms = start_ms;
    next_frame_ms = start_ms + (++next_frame * 1000 / FPS);

    while (vFile.available() && buttonPressed == false)
    {
      // Read video
      mjpeg.readMjpegBuf();
      total_read_video += millis() - curr_ms;
      curr_ms = millis();

      if (millis() < next_frame_ms) // check show frame or skip frame
      {
        // Play video
        mjpeg.drawJpg();
        total_decode_video += millis() - curr_ms;
      }
      else
      {
        ++skipped_frames;
        Serial.println(F("Skip frame"));
      }
      curr_ms = millis();
      
      // Play audio
      if (mp3->isRunning() && !mp3->loop())
      {
        mp3->stop();
      }

      total_play_audio += millis() - curr_ms;
      while (millis() < next_frame_ms)
      {
        vTaskDelay(1);
      }
      curr_ms = millis();
      next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
    }
    
    if (fullPlaythrough == false)
    {
      mp3->stop();
    }
    buttonPressed = false; // reset buttonPressed boolean
    int time_used = millis() - start_ms;
    int total_frames = next_frame - 1;
    Serial.println(F("MP3 audio MJPEG video end"));
    vFile.close();
    aFile->close();
  }
  if (mjpeg_buf)
  {
    free(mjpeg_buf);
  }
}

// pixel drawing callback
static int drawMCU(JPEGDRAW *pDraw)
{
  unsigned long s = millis();
  
  // Debug: Print first frame info
  static bool first_frame = true;
  if (first_frame) {
    Serial.print("Drawing frame at x=");
    Serial.print(pDraw->x);
    Serial.print(", y=");
    Serial.print(pDraw->y);
    Serial.print(", width=");
    Serial.print(pDraw->iWidth);
    Serial.print(", height=");
    Serial.println(pDraw->iHeight);
    first_frame = false;
  }
  
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  total_show_video += millis() - s;
  return 1;
}

void loop()
{





  root = SD.open("/");
  Serial.print("fileNo: ");
  Serial.println(fileNo);
  getFilenames(root, fileNo);

  // Show file name
  gfx->fillRect(0, 0, 320, 50, BLACK);
  displayFilename(videoFilename); 
  delay(1000);

  playVideo(videoFilename, audioFilename);

  if (!mp3->isRunning())
    Serial.println("MP3 stopped!");

  if (fullPlaythrough == true) // Check fullPlaythrough boolean to avoid double increment of fileNo
  {
    fileNo = fileNo + 1; // Increment fileNo to play next video
    if (fileNo > noFiles) // If exceeded number of files, reset counter
    {
      fileNo = 1;
    }
  }
  else // Reset fullPlaythrough boolean
  {
    fullPlaythrough = true;
  }
}
// Function for title screen - adjusted for 320x240 display
void typewriterFX() {
  gfx->setTextColor(GREEN, BLACK);
//gfx->setFont(&FreeSans9pt7b);
  gfx->setTextSize(1);
  gfx->setCursor(60, 60);
  String message = "Welcome to Liam's";

  for (int i = 0; i < message.length(); i++) {
    gfx->print(message[i]);
    delay(75);
  }


  gfx->setTextSize(1);
 //gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setCursor(0, 100);
  String messageb = "Super Movie Brick";

  for (int i = 0; i < messageb.length(); i++) {
    gfx->print(messageb[i]);
    delay(75);
  }

  gfx->setCursor(120, 150);
  String messagec = "Story Machine";

  for (int i = 0; i < messagec.length(); i++) {
    gfx->print(messagec[i]);
    delay(75);
    
  } gfx->setTextSize(1);
}
