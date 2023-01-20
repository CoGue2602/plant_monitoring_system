# plant_monitoring_system
ESP8266 C-code to monitor the needs (water, light) of your plants and notify you by mail if human interaction is necessary

I. How do I use the code?
Connect the hardware (seen below) to the ESP and change the WIFI data in line 40/41, 51/52 as well as the thingspeak channel and API key to your own data. Open the .html file and edit the IP of your localhost to the ESP's IP in your LAN. 
After setting it up, the code will monitor lighting and soil moisture and forward it to your .html dashboard. You can set the thresholds accordingly. If the plant monitored lacks any of the two, you will be notified by e-mail. If you press the button on the dashboard, the RGB LED will light up dependant on the loading status of your power bank (green = full, yellow = approx. 200 hrs (or less) left, red = approx. 100 hours (or less) left).

II. Hardware
used components:
- ESP8266
- DS1302 realtime clock
- DHT11 temperature & humidity sensor
- GL5516 photocell
- RGB LED
- 3350mAh powerbank

wiring:
![image](https://user-images.githubusercontent.com/123165331/213775981-fe4e292f-9e3f-4e58-a893-1abb7bc93e50.png)

III. Code implementation
simplified structure of the code:
![image](https://user-images.githubusercontent.com/123165331/213776392-f3744f0e-f12a-40a5-a208-ef296fe193cb.png)
