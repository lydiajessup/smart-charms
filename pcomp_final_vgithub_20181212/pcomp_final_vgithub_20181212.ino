/*
 * Title: PComp Final 2018
 * Author: Lydia Jessup
 * Date: December 11, 2018
 * 
 * Description: User text code for pcomp final project 
 * 
 * Twilio text
 * 
 * Plus LED light up
 * 
 * PLUS Accelerometer activation
 */

//wifi
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "twilio.hpp"

//accelerometer
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

//timer
#include <Chrono.h>

/* Set the delay between fresh samples */
#define BNO055_SAMPLERATE_DELAY_MS (100)

// Use software serial for debugging?
#define USE_SOFTWARE_SERIAL 1

// Your network SSID and password
const char* itpwifi = "[REDACTED]";
const char* itppassword = "[REDACTED]";
const char* homewifi = "[REDACTED]";
const char* homepassword = "[REDACTED]";

const char* ssid = itpwifi;
const char* password = itppassword;

// Find the api.twilio.com SHA1 fingerprint, this one was valid as 
// of July 2018.
const char* fingerprint = "[REDACTED]";

// Twilio account specific details, from https://twilio.com/console
// Please see the article: 
// https://www.twilio.com/docs/guides/receive-and-reply-sms-and-mms-messages-esp8266-c-and-ngrok

// If this device is deployed in the field you should only deploy a revocable
// key. This code is only suitable for prototyping or if you retain physical
// control of the installation.
const char* account_sid = "[REDACTED]";
const char* auth_token = "[REDACTED]";

// Details for the SMS we'll send with Twilio.  Should be a number you own 
// (check the console, link above).
String to_number    = "[REDACTED]";
String from_number = "[REDACTED]";
//String message_body    = "Fall detected! Do you need assistance?";

// The 'authorized number' to text the ESP8266 for our example
String master_number    = "[REDACTED]"";

// Optional - a url to an image.  See 'MediaUrl' here: 
// https://www.twilio.com/docs/api/rest/sending-messages
String media_url = "";

// Global twilio objects
Twilio *twilio;
ESP8266WebServer twilio_server(8000);

//  Optional software serial debugging
#if USE_SOFTWARE_SERIAL == 1
#include <SoftwareSerial.h>
// You'll need to set pin numbers to match your setup if you
// do use Software Serial
//extern SoftwareSerial swSer(14, 4, false, 256);
#endif

///////////////////Buttons!//////////////////////

//set pin numbers
int help = 14;
int light = 12;
int buzz = 13;
int button = 15;

//set other vars
int helpState;
int buttonState;

//timer
int chronoON = 0;
Chrono myChrono;

String message;
String helpmessage = "Help button activated! Do you need assistance?";
String fallmessage = "Fall detected! Do you need assistance?"; 

//////////////Accelerometer////////////////////
//declare variables
Adafruit_BNO055 bno = Adafruit_BNO055();

/*
 * Setup function for ESP8266 Twilio Example.
 * 
 * Here we connect to a friendly wireless network, instantiate our twilio 
 * object, optionally set up software serial, then send a SMS or MMS message.
 */
void setup() {
 
////////Buttons!///////////////////
          //set up button as input
  pinMode(help, INPUT); // help button input
  pinMode(button, INPUT); // second button in put

  //set up led and buzzer as output
  pinMode(light, OUTPUT); //LED output
  pinMode(buzz, OUTPUT); //buzzer

////////////Wifi//////////////////////
  WiFi.begin(ssid, password);
  twilio = new Twilio(account_sid, auth_token, fingerprint);

////////////Accelerometer//////////////
  /* Initialise the sensor */
  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    //Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  delay(1000);

  /* Display the current temperature */
  //  int8_t temp = bno.getTemp();
  //  Serial.print("Current Temperature: ");
  //  Serial.print(temp);
  //  Serial.println(" C");
  //  Serial.println("");

  bno.setExtCrystalUse(true);

  //Serial.println("Calibration status values: 0=uncalibrated, 3=fully calibrated");

     
}


/* 
 *  In our main loop, we listen for connections from Twilio in handleClient().
 */
void loop() {

//declare buttons
 helpState = digitalRead(help);
 buttonState = digitalRead(button);

//set linear acceleration
  imu::Vector<3> linearaccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

//calculate average and send fall signal if fall is detected
  int avg_accel = (linearaccel.x() + linearaccel.y() + linearaccel.z())/3;

///////////////Do things/////////////////////

//check accelerometer
//if the average change is above or below a threshold
//or if button is pushed
  if (avg_accel > 4 || avg_accel < -4){
    //Serial.print("Fall!     ");

    ///turn on light
     callHelp();
    
    //send text
    callForHelp(fallmessage);
    
  }

//////// Help Button //////////////
  if (helpState == HIGH){
      
    ///turn on light
      callHelp();
    ///send text
      callForHelp(helpmessage);

  }   

////////// Timer Button //////////////

  if (buttonState == HIGH){
      slowBlink();
      chronoON = 1;
      Serial.print(chronoON);
      myChrono.restart();
  }

  //timer done
   if (chronoON == 1 && myChrono.hasPassed(3000)){
      timerDone();
      chronoON = 0;
   }

  else{
    digitalWrite(light, LOW);
    digitalWrite(buzz, LOW);
  }
        
        //twilio_server.handleClient();


/* Display calibration status for each sensor. */
  uint8_t system, gyro, accel, mag = 0;
  bno.getCalibration(&system, &gyro, &accel, &mag);
  //Serial.print("CALIBRATION: Sys=");
  //Serial.print(system, DEC);
  //Serial.print(" Gyro=");
  //Serial.print(gyro, DEC);
  //Serial.print(" Accel=");
  //Serial.print(accel, DEC);
  //Serial.print(" Mag=");
  //Serial.println(mag, DEC);

  delay(BNO055_SAMPLERATE_DELAY_MS); 
}

////////////////////Functions/////////////////////

//pass in string for message
void callForHelp (String message){

  String message_body = message;

////////send text
      #if USE_SOFTWARE_SERIAL == 1
        Serial.begin(115200);
        while (WiFi.status() != WL_CONNECTED) {
                delay(1000);
                Serial.print(".");
        }
        Serial.println("");
        Serial.println("Connected to WiFi, IP address: ");
        Serial.println(WiFi.localIP());
        #else
        while (WiFi.status() != WL_CONNECTED) delay(1000);
        #endif

        // Response will be filled with connection info and Twilio API responses
        // from this initial SMS send.
        String response;
        bool success = twilio->send_message(
                to_number,
                from_number,
                message_body,
                response,
                media_url
                );

        // Set up a route to /message which will be the webhook url
       // twilio_server.on("/message", handle_message);
        twilio_server.begin();

        // Use LED_BUILTIN to find the LED pin and set the GPIO to output
        pinMode(LED_BUILTIN, OUTPUT);

        #if USE_SOFTWARE_SERIAL == 1
        Serial.println(response);
        #endif

//could return true/false if connection works/doesn't

}


//////////////Functions////////////

void slowBlink(){

    int pulse = 700;


    for(int i = 0; i < 3; i++){
      digitalWrite(light, HIGH);
      delay(pulse);
      digitalWrite(light, LOW);
      delay(pulse);
    }
}


void timerDone() {

   int pulse = 200;
    
   for(int i = 0; i < 10; i++){
    digitalWrite(light, HIGH);
    tone(buzz, 500, 500);
    delay(pulse);
    digitalWrite(light, LOW);
    tone(buzz, 500, 500);
    delay(pulse);
    //add in tone  
   } 
}


void callHelp() {

  //SOS pattern
  int dot = 200;
  int dash = 800;

  for (int i = 0; i < 3; i++){
  
      for (int j = 0; j < 3; j++){
          digitalWrite(light, HIGH);
          tone(buzz, 250, 500);
          delay(dot);
          digitalWrite(light, LOW);
          delay(dot);       
      }
  
      for (int j = 0; j < 3; j++){
          digitalWrite(light, HIGH);
          tone(buzz, 250, 500);
          delay(dash);
          digitalWrite(light, LOW);
          delay(dash);       
      }  
  } 
}
