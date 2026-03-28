#include <BleConnectionStatus.h>
#include <BleMouse.h>

//ignoring the detector so the ESP32 doesn't keep restarting
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// NEW: Library to control CPU speed
#include "esp32-hal-cpu.h" 

BleMouse bleMouse("ESP32 G502 Mouse");

//using hiletgo to connect to the esp32
#include <Arduino.h>
#include <HardwareSerial.h>

//pin definitions

//joystick pins
const int PIN_X = 34; //horizaontal axis
const int PIN_Y = 35; //vertical movement

//the buttons orientation of joystick being at the top
const int PIN_BTN_C = 27; //left green btn (left click)
const int PIN_BTN_A = 13; //right green button (right click)
const int PIN_BTN_B = 14; //bottom yellow button (MB4/back)
const int PIN_BTN_D = 26; //top yel btn (MB5/forward)

//small buttons I will use as home and map ig
const int PIN_BTN_E = 25; //bottom button ()
const int PIN_BTN_F = 33; //top button (scroll mode toggle)

//joystick btn
const int PIN_BTN_K = 32; //joystick button (middle click)

//NEW: Status LED Pin (Usually GPIO 2)
const int STATUS_LED = 2;

//mouse code

//config for the "cursor"
const int DEADZONE = 150; //Ignores minor drift
const int CENTERVAL = 1930; //joystick center
const int SENSITIVITY = 60; //Higher sens = slower cursor
const int SCROLLSPEED = 500; //higher = slower scroll

//state variables
bool scrollMode = false; //default non scroll mode
bool lastToggleState = HIGH; //helper to make sure the scroll toggle just happens without any issues

//void setup cause this is C and void means no data type assignment is needed
void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector

  // NEW: LOWER CPU FREQUENCY TO SAVE POWER
  // Default is 240MHz. We drop to 80MHz to reduce current draw.
  // This might prevent the voltage form dropping too low.
  setCpuFrequencyMhz(80);

  //setup blue LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW); //starts off
  
  //start communication to check for the data on the serial monitor
  Serial.begin(115200);
  Serial.println("Starting the ESP32 Air mouse G502 edition complete with MB4 and MB5");

  // --- DIAGNOSTIC BLINK ---
  // We do this BEFORE bleMouse.begin() to see if the board is alive.
  // If you see these blinks, the code is running!
  for(int i=0; i<5; i++){
      digitalWrite(STATUS_LED, HIGH);
      delay(200);
      digitalWrite(STATUS_LED, LOW);
      delay(200);
      Serial.print("Blink "); Serial.println(i+1);
  }

  Serial.println("Attempting to start Bluetooth...");
  
  bleMouse.begin();
  
  // NEW: Wait for radio to stabilize power draw
  delay(1000); 

  Serial.println("Bluetooth Started!");

  //config the buttons
  //using INPUT_PULLUP because the buttons connect to gnd when pressed.0
  //ESP32 will hold pin HIGH (3.3V) by default.
  //when button is pressed, it connects to LOW or more specifically gnd
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);
  pinMode(PIN_BTN_C, INPUT_PULLUP);
  pinMode(PIN_BTN_D, INPUT_PULLUP);
  pinMode(PIN_BTN_E, INPUT_PULLUP);
  pinMode(PIN_BTN_F, INPUT_PULLUP);
  pinMode(PIN_BTN_K, INPUT_PULLUP);

  //config joystick axes
  //for now it is purely analog so just using INPUT
  pinMode(PIN_X, INPUT);
  pinMode(PIN_Y, INPUT);
}

//buttons tester set of code... (skipped as it was commented out in original)

//void loop time for the ESP32 to continually check the inputs
void loop(){

  // --- STATUS LED LOGIC ---
  // This runs regardless of connection status
  if(bleMouse.isConnected()){
    digitalWrite(STATUS_LED, HIGH); // Solid Blue = Connected
  } else {
    // Slow Blink = Searching
    if ((millis() / 500) % 2 == 0) {
        digitalWrite(STATUS_LED, HIGH);
    } else {
        digitalWrite(STATUS_LED, LOW);
    }
  }

  //checking if the mouse is connected
  if(bleMouse.isConnected()){

    //read the analog joystick
    //esp32 analog values go from 0 to 4095
    //center is roughly 2048 might vary though
    int xRead = analogRead(PIN_X);
    int yRead = analogRead(PIN_Y);
    int xVal = 0;
    int yVal = 0;

    //adjusting readings for drift
    if(abs(xRead-CENTERVAL)>DEADZONE){
      xVal = xRead - CENTERVAL;
    }
    if(abs(yRead - CENTERVAL)>DEADZONE){
      yVal = yRead - CENTERVAL;
    }

    //scroll mode activation
    int currentToggleState = digitalRead(PIN_BTN_F);
    if(lastToggleState == HIGH && currentToggleState == LOW){
      scrollMode = !scrollMode; // effeciently changes the scroll mode state
      Serial.println(scrollMode?"SCROLL MODE: ":"CURSOR MODE");
    }
    lastToggleState = currentToggleState;

    //movement logic on scroll mode
    if(scrollMode){
      //scrolling
      if(yVal != 0){
        int scrollAmount = -(yVal/SCROLLSPEED);
        if(scrollAmount != 0){
          bleMouse.move(0, 0, scrollAmount);
          delay(100);
        }
      }
    }
    //cursor mode
    else{
      //if mouse moves wrong way, add negative: -xMove or -yMove
      int xMove = xVal / SENSITIVITY;
      int yMove = yVal / SENSITIVITY;

      if(xMove != 0 || yMove != 0){
        bleMouse.move(xMove, yMove);
      }
    }

    //clicking logic
    //left click or the C button i guess
    if(digitalRead(PIN_BTN_C) == LOW){
      if(!bleMouse.isPressed(MOUSE_LEFT)) bleMouse.press(MOUSE_LEFT);
    }
    else{
      if(bleMouse.isPressed(MOUSE_LEFT)) bleMouse.release(MOUSE_LEFT);
    }

    //right click or the A button
    if(digitalRead(PIN_BTN_A) == LOW){
      if(!bleMouse.isPressed(MOUSE_RIGHT)) bleMouse.press(MOUSE_RIGHT);
    }
    else{
      if(bleMouse.isPressed(MOUSE_RIGHT)) bleMouse.release(MOUSE_RIGHT);
    }

    //Middle click or the K button
    if(digitalRead(PIN_BTN_K) == LOW){
      if(!bleMouse.isPressed(MOUSE_MIDDLE)) bleMouse.press(MOUSE_MIDDLE);
    }
    else{
      if(bleMouse.isPressed(MOUSE_MIDDLE)) bleMouse.release(MOUSE_MIDDLE);
    }

    //MB4 OR BTN B
    if(digitalRead(PIN_BTN_B) == LOW){
      if(!bleMouse.isPressed(MOUSE_BACK)) bleMouse.press(MOUSE_BACK);
    }
    else{
      if(bleMouse.isPressed(MOUSE_BACK)) bleMouse.release(MOUSE_BACK);
    }

    //MB5 OR BTN D
    if(digitalRead(PIN_BTN_D) == LOW){
      if(!bleMouse.isPressed(MOUSE_FORWARD)) bleMouse.press(MOUSE_FORWARD);
    }
    else{
      if(bleMouse.isPressed(MOUSE_FORWARD)) bleMouse.release(MOUSE_FORWARD);
    }
    
    //delay for stability
    delay(10);
  }
  //end if connected
}