/*
  Arudino course final project - Plant monitoring system
  Group 3
  January2023

  contributions and tutorials, which were used in the scope of this work:
  measure moisture: https://forum.arduino.cc/t/x4-soil-moisture-dht11-with-wi-fi-2560/349434/9
  using the light resistor: https://www.childs.be/blog/post/how-to-connect-a-photoresistor-or-light-dependant-resistor-to-an-esp8266-12e
  e-mail notifications: https://randomnerdtutorials.com/esp32-send-email-smtp-server-arduino-ide/
  thingspeak: https://learn.inside.dtu.dk/d2l/le/content/142961/viewContent/538144/View
  local webserver: https://learn.inside.dtu.dk/d2l/le/content/142961/viewContent/538144/View
*/
//included libraries
#include "DHT.h"
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <ESP_Mail_Client.h>
#include <ThingSpeak.h>

//define pins
#define DHTPIN D4
ThreeWire myWire(D7, D6, D8);  // DAT, CLK, RST

RtcDS1302<ThreeWire> Rtc(myWire);

//define variables for water/ moisture
int soil = 0;
DHT dht(DHTPIN, DHT11);

//define variables for time
int last_notification = 0;

//define email variables
#define WIFI_SSID "Basecamp Resident 2E"
#define WIFI_PASSWORD "4WriteSuffixDried144"
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
/* The sign in credentials */
#define AUTHOR_EMAIL "esp34338@gmail.com"
#define AUTHOR_PASSWORD "wvwxurwsfnjxjngr"
/* Recipient's email*/
#define RECIPIENT_EMAIL "s220154@dtu.dk"

//define thingspeak variables
const char* ssid = "Basecamp Resident 2E";
const char* pass = "4WriteSuffixDried144";
WiFiClient client;
unsigned long channelID = 2009564; //your TS channal
const char * APIKey = "N5P1LHC1UOBMWD5I"; //your TS API
const char* thingspeak_server = "api.thingspeak.com";

//button<<
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>   // Include the Wi-Fi-Multi library
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <ESP8266mDNS.h>        // Include the mDNS library
ESP8266WiFiMulti wifiMulti;
// Create an instance of the server
ESP8266WebServer server(80);

void handleRoot();
void handleLED();
void handleNotFound();

//define variables for RGB LED
int LEDred = D0;
int LEDgreen = D1;
int LEDblue = D2;
int brightness = 150; //0-255

//<<

//------------------------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  Serial.println("Plant Monitoring System");
  dht.begin();

  //clock preparation
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  //check clock for errors
  if (!Rtc.IsDateTimeValid()) {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }
  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }
  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  } else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  //thingspeak
  WiFi.begin(ssid, pass);

  //button<<
  // Connect to WiFi network
  Serial.println();
  Serial.println("WiFi connected to ");
  Serial.println(WiFi.SSID());
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("iot")) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/LED", HTTP_POST, handleLED);
  server.onNotFound(handleNotFound);

  // Start the server
  server.begin();
  Serial.println("Server started");

  //set LEDs as output
  pinMode(LEDblue, OUTPUT);
  pinMode(LEDgreen, OUTPUT);
  pinMode(LEDred, OUTPUT);
}

void loop() {
  //LEDs off from last loop
  analogWrite(LEDgreen, 0);
  analogWrite(LEDblue, 0);
  analogWrite(LEDred, 0);
  
  //get time
  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);
  Serial.println();
  int hour = returnHour(now);
  int day = returnDay(now);
  if ((hour > 15) || (hour < 10)) {
    Serial.println("It's probably dark outside.");
  } else {
    Serial.println("It's daytime and natural light should be provided.");
  }

  //---------------------------------------------------------------------------------------------
  //get moisture level
  float h = dht.readHumidity();         //humiditiy
  float t = dht.readTemperature();      //temp as celsius (default)
  float f = dht.readTemperature(true);  //temp as fahrenheit

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C\t\t ");

  //reaction to too little water
  if ((h < 20) && (last_notification != day)) {
    //send mail notification for too little water here!
    Serial.println("Water notification shall be send.");
    last_notification = day;
    send_mail(0);
  }

  //------------------------------------------------------------------------------------------------
  //get light level
  int sensorValue_light = analogRead(A0);                    // read the input on analog pin 0
  float light_voltage = sensorValue_light * (5.0 / 1023.0);  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V)
  Serial.print("Light lvl [0-5]: ");
  Serial.println(light_voltage);  // print out the value you read (between 1-5)

  //reaction to too little light
  if ((light_voltage < 1) && (hour < 16) && (hour > 10) && (last_notification != day)) {
    //send mail notification for too little light here!!!
    Serial.println("Light notification shall be send.");
    last_notification = day;
    send_mail(1);
  }

  //------------------------------------------------------------------------------------------------
  //thingspeak/ website
  light_voltage = light_voltage * 20; //from a value from 0-5 to a percentage between 0-100
  ThingSpeak.begin(client);
  client.connect(thingspeak_server, 80); //connect(URL, Port)
  ThingSpeak.setField(1, h); //set data on the humidity graph
  ThingSpeak.setField(2, light_voltage); //set data on the lighting graph
  ThingSpeak.writeFields(channelID, APIKey);//post everything to TS
  client.stop();

  //delay added for stability between thingspeak and button handling
  delay(500);

  // Check if a client has connected
  server.handleClient();

  delay(10000);  // delay between each loop iteration
  Serial.println();
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------

#define countof(a) (sizeof(a) / sizeof(a[0]))

//function, which prints complete timestamp
void printDateTime(const RtcDateTime& dt) {
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u.%02u"),
             dt.Day(),
             dt.Month(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  Serial.print(datestring);
}

//function which textwraps the current hour and returns it
int returnHour(const RtcDateTime& dt) {
  int hour;
  char datestring[3];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u"),
             dt.Hour());
  hour = atoi(datestring);
  return hour;
}

//function which textwraps the current day and returns it
int returnDay(const RtcDateTime& dt) {
  int day;
  char datestring[3];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u"),
             dt.Day());
  day = atoi(datestring);
  return day;
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
  /* The SMTP Session object used for Email sending */
  SMTPSession smtp;

  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()) {
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");
  }
}

//function responsible for the whole process of sending an email notification
void send_mail(int msg) {
  /* The SMTP Session object used for Email sending */
  SMTPSession smtp;

  /* Callback function to get the Email sending status */
  void smtpCallback(SMTP_Status status);

  if ((msg == 0) || (msg == 1)) {
    Serial.println("Connecting to AP");
    //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(200);
    }
    /** Enable the debug via Serial port
      none debug or 0
      basic debug or 1
    */
    smtp.debug(0);

    /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);

    /* Declare the session config data */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";

    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = "PMS";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "Plant alert!";
    message.addRecipient("User", RECIPIENT_EMAIL);

    /*Send HTML message*/
    String htmlMsg;
    if (msg == 0) {
      htmlMsg = "<div style=\"color:#2f4468;\"><h1>Water your plant, it is drying out!</h1><p>- Sent via Plant monitoring system</p></div>";
    } else if (msg == 1) {
      htmlMsg = "<div style=\"color:#2f4468;\"><h1>Your plant is not getting enough light! Change its location!</h1><p>- Sent via Plant monitoring system</p></div>";
    } else {
      Serial.println("Error! No valid message selected!");
    }
    message.html.content = htmlMsg.c_str();
    message.html.content = htmlMsg.c_str();
    message.text.charSet = "us-ascii";
    message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    /* Connect to server with the session config */
    if (!smtp.connect(&session))
      return;

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
      Serial.println("Error sending Email, " + smtp.errorReason());
  }
}

//creates the local sever website (=button), which will be embedded 
void handleRoot() {                         // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", "<html><title>Plant Monitoring System</title><meta charset=\"utf-8\" \/> \
      </head><body><form action=\"/LED\" method=\"POST\" ><input type=\"submit\" value=\"Show battery status\" style=\"width:300px; height:100px; font-size:24px; background-color:#90ee90;\"></form> \
      </body></html>");
}

// If a POST request is made to URI /LED -> this function describes what happens if the button is pressed on the dashboard
void handleLED() {                          
  Serial.println("Button pressed!");
  //calculate how long the program is running
  unsigned long myTime;
  myTime = millis(); //milliseconds passed since the arduino board began running
  myTime = myTime / (1000*60*60); //hours since arduino board started
  //powerbank has 3350mAh, Arduino might use around 10mAh (barely sleeping) -> 335h battery
  Serial.print("Program started ");
  Serial.print(myTime);
  Serial.println(" hours ago. Battery capacity: 335 hours.");
  if (myTime < 100) {
    //green for full battery
    analogWrite(LEDgreen, brightness);
  } else if (myTime < 220) {
    //yellow for partly filled battery
    analogWrite(LEDgreen, brightness);
    analogWrite(LEDred, brightness);
  } else {
    //red for nearly empty battery
    analogWrite(LEDred, brightness);
  }
  server.sendHeader("Location", "/");       // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

// Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
void handleNotFound() {
  server.send(404, "text/plain", "404: Not found"); 
}

