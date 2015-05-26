/*
Social Water: Arduino Code for Water Level/Temperature sensor using
the SIM900 type gsm sheild and 10k thermistor.
@author Matthew McGovern contact at mgmcgove@buffalo.edu

This code is pre-de-messed!

*/

#include <SoftwareSerial.h>



//These need to be set up before the station is deployed.
const String NUMBER_TO_TEXT = "\"+17162180282\"";
const String STATIONID = "NY20002";
const int SENSOR_IS_MOUNTED_AT_HEIGHT = 30; //inches



const int PWR = A5;
const int IN = A0;
int reading;
float resistance, temp;
const float RES = 10000;
const float BETACOEF = 3950;

//sonic rangefinder stuff
const int PING = A1; //ping output pin
const int ECHO = A2;
int tempReading;
volatile long high, low;
long measurement, numSamples;


void wait_one_minute(){
 int i = 0; 
 while (i < 10){
  delay(6000); //delay 6 seconds.
  i++; 
 }
}

void wait_n_minutes(int n){
 int i = 0;
  while (i < n){
    wait_one_minute();
    i++;
  }
}

void sim900PowerButton(){
   pinMode(9,OUTPUT);
   digitalWrite(9,low);
   delay(2000);
   digitalWrite(9,HIGH);
   delay(2000);
   digitalWrite(9,low); 
   delay(7000);
}

String assembleMessage(long distance, float temperature ){
  String message = "";
  message = message + "IMAROBOT,"; //our "magic number" of sorts.
  //There will be some logic on the other end to tell the system.
  //That this message will be strictly formatted and 
  //will contain two kinds of data to be processed.
  message = message + STATIONID;
  message = message + ",";
  message = message + (int) distance;
  message+= ",";
  char tempString[6];
  dtostrf(temperature, 4, 2, tempString);
  message =  message + tempString;
  return message;
  
}

void testCall(){
  SoftwareSerial phoneCom(7,8);
  phoneCom.begin(19200);
  //This can be used to test your board, replace *HERE* with your number!
  //phoneCom.println("ATD + + *HERE*  ;");
  phoneCom.println();
  delay(30000);
  
  
}

void sendText(long distance, float temp ){
  //I used some example code from an excellent site:
  //http://tronixstuff.com/2014/01/08/tutorial-arduino-and-sim900-gsm-modules/
  // Though there may be an even simpler way to post data for this system using
  // gprs' ftp capability. That will have to wait for next year (or later this summer).
 SoftwareSerial phoneCom(7,8);
 phoneCom.begin(19200);
 phoneCom.print("AT+CMGF=1\r");
 delay(100);

 phoneCom.println( String("AT + CMGS = ") + NUMBER_TO_TEXT );
 delay(100);
 phoneCom.println( assembleMessage( distance, temp ) );
 delay(100);
 phoneCom.println((char)26);
 delay(100);
 phoneCom.println();
 delay(7000);
 
 
}

long msToIn(long ms){
    return ms / 74/ 2;
}

void setup(){
 Serial.begin(9600);
 pinMode(PWR, OUTPUT);
 pinMode(IN, INPUT);
 analogWrite(PWR,0);
 delay(10000);
 //sim900PowerButton();
}

void loop(){
  analogWrite(PWR, 1028);
  delay(200);
  reading = analogRead(IN);
  resistance = RES/( (1028.0/reading) -1 );
  Serial.println(reading);
  Serial.println(resistance);
  
  float temp = log(resistance/RES);
  temp /= BETACOEF;
  temp += 1.0/ (25 + 273.15);
  temp = 1.0/temp;
  temp -= 273.15;
  
  Serial.print("temp: ");
  Serial.println( temp );
  
  delay(500);
  
 
  
  long flyTime, latestMeasurement;
  //Ultrasonic Rangefinder Section
  pinMode(PING, OUTPUT);
  digitalWrite(PING, LOW);
  delayMicroseconds(2);
  digitalWrite(PING, HIGH);
  delayMicroseconds(5);
  digitalWrite(PING, LOW);
  pinMode(ECHO, INPUT); //this uses the same pin for in and out.
  flyTime = pulseIn(ECHO, HIGH);
  latestMeasurement = msToIn(flyTime);
  //we have the distance from the sensor to the top of the water.
  //now we calculate the water level with that measurement.
  latestMeasurement = SENSOR_IS_MOUNTED_AT_HEIGHT - latestMeasurement;
  Serial.print("inches: ");
  Serial.println(latestMeasurement);
  delay(1000);
  
  
  Serial.println(assembleMessage(latestMeasurement,temp) );
  ///**********************************************
  //Turn on the phone (which sucks up a lot of power!)
  //and send off the text with the data to the base station.
  //***********************************************
  
  sim900PowerButton();
  //send a message;
  Serial.println("Sim900 is on");
  delay(10000); 
  sendText(latestMeasurement,temp);
  delay(2000);
  ////testCall(); //works!
  Serial.println("Turning sim900 off");
  sim900PowerButton();
  
  wait_n_minutes(30);
  
}
