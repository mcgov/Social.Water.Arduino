/*
Social Water: Arduino Code for Water Level/Temperature sensor using
the SIM900 type gsm sheild and 10k thermistor.
@author Matthew McGovern contact at mgmcgove@buffalo.edu

This code is somewhat-de-messed!

*/

#include <SoftwareSerial.h>



//Social.Water globals, these need to be checked and changed before the
//station is deployed.
const String NUMBER_TO_TEXT = "\"+17162180282\"";
const String STATIONID = "NY20002";
const int SENSOR_HEIGHT_FROM_GROUND_IN_INCHES = 30;
//height from the bottom of the lake/stream/swamp/etc


const int SIM900_POWER_PIN = 9; //software pin to turn on the gsm shield.
//requires a jumper to be soldered.
const int SIM900_SERIAL_BAUD_RATE = 19200;

//thermistor globals
const int THERMISTOR_POWER_PIN = A5;
const int THERMISTOR_INPUT_PIN = A0;
const float CIRCUIT_SERIAL_RESISTOR_IN_OHMS = 10000;
const float BETA_COEFFICIENT = 3950;

int thermistor_adc_reading;
float calculated_resistance_of_thermistor, calculated_temperature;


//sonic rangefinder globals
const int PING_OUTPUT_PIN = A1;
const int PING_PULSE_INPUT_PIN = A2;
int temperature_initial_reading;
long measurement;


long milliseconds_to_inches_conversion(long ms)
{	//convert the ping reading to inches using the speed of sound.
    return ms / 74 / 2;
}



void wait_one_minute()
{
	 int i = 0;
	 while (i < 10)
	 {
		delay(6000); //delay 6 seconds.
		i++;
	 }
}

void wait_n_minutes(int n)
{
	int i = 0;
	while (i < n)
	{
		wait_one_minute();
		i++;
	}
}

void press_sim900_power_button()
{
		// sim900 type boards can be powered on using a pulse
		// longer than a second to the power pin
		// as long as the jumper bumps have been soldered.
	   pinMode(SIM900_POWER_PIN,OUTPUT);
	   digitalWrite(SIM900_POWER_PIN,LOW);
	   delay(2000);
	   digitalWrite(SIM900_POWER_PIN,HIGH);
	   delay(2000);
	   digitalWrite(SIM900_POWER_PIN,LOW);
	   delay(10000); //give it some time to turn on.
}

String assemble_sms_section_of_at_command(long distance, float temperature )
{
	  String message = "";
	  message = message + "IMAROBOT,"; //our "magic number" of sorts.
	  //There will be some logic on the Social.Water end to tell the system.
	  //that this message will be strictly formatted and
	  //will contain two kinds of data to be processed.
	  message = message + STATIONID;
	  message = message + ","; //arduino's are touchy about appending
	  message = message + (int) distance; //different data types for some reason.
	  message+= ",";

	  char tempString[6]; //arduino's don't have sprintf
	  dtostrf(temperature, 4, 2, tempString); //so we use this nonsense <-
	  message =  message + tempString;
	  return message;

}

void test_call()
{
	  SoftwareSerial phoneCom(7,8); //initialize communication with the board.
	  phoneCom.begin(SIM900_SERIAL_BAUD_RATE);

	  //This can be used to test your board
	  //decomment the line below and replace *HERE* with your number!
	  //phoneCom.println("ATD + + *HERE*  ;");
	  phoneCom.println();
	  delay(30000); //wait for the call to complete.

}

void send_sms(long distance, float temp )
{
	  //I looked over some example code from an excellent site:
	  //http://tronixstuff.com/2014/01/08/tutorial-arduino-and-sim900-gsm-modules/
	  // and this is very similar. Though there may be an even simpler way to post data for this system using
	  // gprs' ftp capability. That will have to wait for next year (or later this summer).
	 SoftwareSerial phoneCom(7,8);
	 phoneCom.begin(SIM900_SERIAL_BAUD_RATE);
	 phoneCom.print("AT+CMGF=1\r");
	 delay(100);
	 phoneCom.println( String("AT + CMGS = ") + NUMBER_TO_TEXT ); //AT command to send sms.
	 delay(100);
	 phoneCom.println( assemble_sms_section_of_at_command( distance, temp ) );
	 delay(100);
	 phoneCom.println((char)26);
	 delay(100);
	 phoneCom.println();
	 delay(10000); //give it a few seconds to send the sms out.

}

int get_thermistor_reading_from_adc() {
	analogWrite(THERMISTOR_POWER_PIN, 1028);
	delay(200);
	return analogRead(THERMISTOR_INPUT_PIN);
}

float calculate_temperature_from_thermistor_reading() {
	//https://learn.adafruit.com/thermistor/using-a-thermistor
	// almost exactly as described in tutorial above.
	calculated_resistance_of_thermistor = CIRCUIT_SERIAL_RESISTOR_IN_OHMS
			/ ((1028.0 / thermistor_adc_reading) - 1);
	float temp = log( calculated_resistance_of_thermistor
					/ CIRCUIT_SERIAL_RESISTOR_IN_OHMS  );
	temp /= BETA_COEFFICIENT;
	temp += 1.0 / (25 + 273.15);
	temp = 1.0 / temp;
	temp -= 273.15;
	return temp;
}

float get_calculated_temperature_from_thermistor() {
	thermistor_adc_reading = get_thermistor_reading_from_adc();
	//https://learn.adafruit.com/thermistor/using-a-thermistor
	// thanks adafruit!
	float temp = calculate_temperature_from_thermistor_reading();
	Serial.print("temp: ");
	Serial.println(temp);
	delay(500);
	return temp;
}

long measure_distance_with_ping() {
	//Ultrasonic Rangefinder Section
	pinMode(PING_OUTPUT_PIN, OUTPUT);
	digitalWrite(PING_OUTPUT_PIN, LOW);
	delayMicroseconds(2);
	digitalWrite(PING_OUTPUT_PIN, HIGH);
	delayMicroseconds(5);
	digitalWrite(PING_OUTPUT_PIN, LOW);
	pinMode(PING_PULSE_INPUT_PIN, INPUT); //this uses the same pin for in and out.
	return pulseIn(PING_PULSE_INPUT_PIN, HIGH);
}

long read_distance_from_ping_sensor() {
	long fly_time, distance_to_water_in_inches;
	//Ultrasonic Rangefinder Section
	fly_time = measure_distance_with_ping();
	distance_to_water_in_inches = milliseconds_to_inches_conversion(fly_time);
	//we have the distance from the sensor to the top of the water.
	//now we calculate the water level with that measurement.
	distance_to_water_in_inches = SENSOR_HEIGHT_FROM_GROUND_IN_INCHES - distance_to_water_in_inches;
	return distance_to_water_in_inches;
}



//======================================================
// Arduino setup/loop code!
//=====================================================

void setup()
{
	 Serial.begin(9600); //Arduino serial start (for printing debugging/status)
	 pinMode(THERMISTOR_POWER_PIN, OUTPUT);
	 pinMode(THERMISTOR_INPUT_PIN, INPUT);
	 analogWrite(THERMISTOR_POWER_PIN,0);
	 delay(10000);
}



void loop()
{
  //First we take our measurements.
          float temp = get_calculated_temperature_from_thermistor();
	  long latestMeasurement = read_distance_from_ping_sensor();

 //  then turn on the phone (which sucks up a lot of power!)
 //  and send off the text with the data to the base station.

	  press_sim900_power_button();
	  send_sms(latestMeasurement,temp);
	  press_sim900_power_button();

	  wait_n_minutes(60);

}
