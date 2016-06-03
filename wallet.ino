// Display
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

#define DISPLAY_SCK 15
#define DISPLAY_MOSI 16
#define DISPLAY_SS A5

Adafruit_SharpMem display(DISPLAY_SCK, DISPLAY_MOSI, DISPLAY_SS);

#define BLACK 0
#define WHITE 1

// Accelerometer
#include <Wire.h> // Must include Wire library for I2C
#include <SparkFun_MMA8452Q.h> // Includes the SFE_MMA8452Q library

MMA8452Q accel;

// Bluetooth
#include <Arduino.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// App
enum WalletState {
  SLEEPING,
  LOADING,
  ACTIVE
};

int loadingIndicatorRadius = 1;

WalletState walletState = SLEEPING;

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setupDisplay() {
  display.begin();
  display.setRotation(3);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
}

void setupBluetooth(void) {
  updateDisplay("Initializing");
  
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if (!ble.begin(VERBOSE_MODE)) {
    updateDisplay("Test 3");
    updateDisplay("Bluetooth module error");
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  
  Serial.println(F("OK!"));
  updateDisplay("Done");

  if (FACTORYRESET_ENABLE) {
    Serial.println(F("Performing a factory reset: "));
    if (!ble.factoryReset()) {
      error(F("Couldn't factory reset"));
    }
  }
  
  ble.echo(false);
  ble.verbose(false);
  updateDisplay("Ready for connection!");
  Serial.println(F("Waiting for Bluetooth connection"));

  /* Wait for connection */
  while (!ble.isConnected()) {
      delay(500);
  }

  Serial.println(F("******************************"));
  
  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set module to DATA mode
  Serial.println(F("Switching to DATA mode!"));
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));

  updateDisplay("Connected");

  delay(500);
}

void setup(void) 
{
  Serial.begin(115200);

  // Disable bluetooth via SS
  pinMode(BLUEFRUIT_SPI_CS, OUTPUT);
  digitalWrite(BLUEFRUIT_SPI_CS, HIGH);

  // Disable display via SS
  pinMode(DISPLAY_SS, OUTPUT);
  digitalWrite(DISPLAY_SS, HIGH);
  
  setupDisplay();
  setupBluetooth();

  updateDisplay("Wallet 1.0");
  
  accel.init();
}

void loop(void) 
{
  switch (walletState) {
    case SLEEPING:
      checkAccelerometer();
      break;

    case LOADING:
      updateLoadingAnimation();
      receiveData();
      break;

    case ACTIVE:
      display.refresh();
      delay(500);
      break;
  }
}

void checkAccelerometer() {
  if (accel.available() && walletState == SLEEPING) {
    accel.read();

    byte orientation = accel.readPL();
    //Serial.print("Orientation: ");
    //Serial.println(orientation);

    if (orientation == LANDSCAPE_R) {
      Serial.println("Refreshing!");
      setupDisplay();
      loadingIndicatorRadius = 1;
      ble.println("REFRESH");
      walletState = LOADING;
    }
  }
  
  delay(100);
}

void updateLoadingAnimation() {
  display.fillCircle(display.width() / 2, display.height() / 2, ++loadingIndicatorRadius, BLACK);
  display.refresh();

  if (loadingIndicatorRadius > 20) {
    loadingIndicatorRadius = 0;
    display.clearDisplay();
  }
  
  delay(100);
}

void receiveData() {
  while (ble.available()) {
    int c = ble.read();

    Serial.print((char)c);
    
    Serial.print(" [0x");
    if (c <= 0xF) Serial.print(F("0"));
    Serial.print(c, HEX);
    Serial.print("] ");
  }
}

void updateDisplay(char *message) {
  //display.clearDisplay();
  display.println(message);
  display.refresh();
  
  delay(500);
}

