#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include "Settings.h"



const long sampleInterval = 1000;
unsigned long sampleIntervalLast = 0;

const long sendInterval = 60000;
unsigned long sendIntervalLast = 0;



int sampleNumber = 0;
int sensorValue = 0;        // value read from the pot
int outputValue = 0;        // value output to the PWM (analog out)



#define LED_PIN 5
#define ANALOG_PIN A0



Adafruit_NeoPixel pixels = Adafruit_NeoPixel(3, LED_PIN, NEO_GRB + NEO_KHZ400);

const uint32_t COLOR_START = pixels.Color(0, 0, 255); //Blue
const uint32_t COLOR_ERROR = pixels.Color(255, 0, 0); //Red
const uint32_t COLOR_WIFI_WAIT = pixels.Color(255, 165, 0); //Orange
const uint32_t COLOR_WIFI_SEND = pixels.Color(0, 255, 0); //Green

const uint32_t COLOR_RED = pixels.Color(255, 0, 0);
const uint32_t COLOR_ORANGE = pixels.Color(255, 165, 0);
const uint32_t COLOR_GREEN = pixels.Color(0, 255, 0);
const uint32_t COLOR_BLUE = pixels.Color(0, 0, 255);


void setup() {
	pixels.begin();
	pixels.setBrightness(25);
	pixels.setPixelColor(0, COLOR_START);
	pixels.setPixelColor(1, COLOR_START);
	pixels.setPixelColor(2, COLOR_START);
	pixels.show();
	delay(1000);

	pixels.setPixelColor(0, pixels.Color(0,0,0));
	pixels.setPixelColor(1, pixels.Color(0,0,0));
	pixels.setPixelColor(2, pixels.Color(0,0,0));
	pixels.show();

	Serial.begin(115200);
	delay(10);

	// We start by connecting to a WiFi network
	Serial.println();
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		pixels.setPixelColor(0, COLOR_WIFI_WAIT);
		pixels.show();
		delay(250);
		Serial.print(".");
		pixels.setPixelColor(0, pixels.Color(0,0,0));
		pixels.show();
		delay(250);
	}

	pixels.setPixelColor(0, COLOR_WIFI_SEND);
	pixels.show();
	delay(1000);

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}


void loop() {
	unsigned long currentMillis = millis();

	// Sample sensor data
	if(currentMillis - sampleIntervalLast >= sampleInterval) {
		sampleIntervalLast = currentMillis;
		readData();
	}

	// Send sensor data
	if(currentMillis - sendIntervalLast >= sendInterval) {
		sendIntervalLast = currentMillis;
		sendData();
	}
}


void readData(){
	// read the analog in value:
	sensorValue = analogRead(ANALOG_PIN);
	// map it to the range of the analog out:
	outputValue = map(sensorValue, 0, 1023, 0, 255);

	if (outputValue >= 0 and outputValue < 100) {
		// Red
		pixels.setPixelColor(2, COLOR_RED);
	} else if (outputValue > 100 and outputValue < 150) {
		// Orange
		pixels.setPixelColor(2, COLOR_ORANGE);
	} else if (outputValue > 150 and outputValue < 200) {
		// Green
		pixels.setPixelColor(2, COLOR_GREEN);
	} else {
		// Blue
		pixels.setPixelColor(2, COLOR_BLUE);
	}
	pixels.show();
	
	// print the results to the serial monitor:
	Serial.print("sensor = " );
	Serial.print(sensorValue);
	Serial.print("\t output = ");
	Serial.println(outputValue);
}

void sendData(){
	pixels.setPixelColor(0, COLOR_WIFI_SEND);
	pixels.show();

	Serial.print("connecting to ");
	Serial.println(host);

	// Use WiFiClient class to create TCP connections
	WiFiClient client;
	const int httpPort = 8080;
	if (!client.connect(host, httpPort)) {
		pixels.setPixelColor(0, COLOR_ERROR);
		pixels.setPixelColor(1, COLOR_ERROR);
		pixels.setPixelColor(2, COLOR_ERROR);
		pixels.show();
		Serial.println("connection failed");
		return;
	}


	// We now create a URI for the request
	String PostData = "data={";
	PostData += "  \"node\": {";
	PostData += "    \"identifier\": " + String(ESP.getChipId()) + ",";
	PostData += "    \"data\": {";
	PostData += "      \"moisture\": " + String(sensorValue) + ",";
	PostData += "      \"moisture2\": " + String(outputValue) + ",";
	PostData += "      \"uptime\": " + String(millis()/1000) + ",";
	PostData += "      \"freeHeap\": " + String(ESP.getFreeHeap()) + ",";
	PostData += "      \"vcc\": " + String(ESP.getVcc());
	PostData += "    }";
	PostData += "  }";
	PostData += "}";

	client.println("POST " + String(endpoint) + " HTTP/1.1");
	client.println("Host: " + String(host));
	client.println("User-Agent: Arduino/1.0");
	client.println("Content-Type: application/x-www-form-urlencoded");
	client.println("Connection: close");
	client.print("Content-Length: ");
	client.println(PostData.length());
	client.println();
	client.println(PostData);
	delay(50);

	// Read all the lines of the reply from server and print them to Serial
	while(client.available()){
		String line = client.readStringUntil('\r');
		Serial.print(line);
	}

	Serial.println();
	Serial.println("closing connection");
	pixels.setPixelColor(0, pixels.Color(0,0,0));
	pixels.show();
	Serial.println();
	Serial.println();
}
