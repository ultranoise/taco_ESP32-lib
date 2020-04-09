# taco_ESP32-lib
A first attempt to produce an Arduino library for ESP32 boards helping you out to transmit OSC data from sensors

Features (version 0.1):

* It transmits OSC data to all connected hosts

* It can be configured as Access Point (AP) or connect to an existing Wifi (STA)

* It incorporates an html server to configure it (try it with a browser)

* It saves configuration information to eeprom

* It deals with add-ons like displays and I2C sensors (take a look at the templates)-

Dependencies:

* <WiFi.h>

* <OSCMessage.h>

* <ESPmDNS.h>

* <WebServer.h>

* <Adafruit_GFX.h>

* <Adafruit_SSD1306.h>

Download version 0.1: download or clone the contents of these repo.


Install: Move the folder “Taco” and all its contents to your Arduino libraries folder. Check the included examples.


Documentation (check the rest of Taco.h):

  * /* Basic Constructor with onboard led pin and hardreset pin */
  
  Taco(int ledPin, int hardResetPin);
  

  * /* Constructor with onboard led pin, hardreset pin and access point name */
  
  Taco(int ledPin, int hardResetPin, const char *AP_name);
  

  * /* Begin all necessary stuff (eeprom, hardreset checks, saved informations, network) */
  
  bool begin(int udpPort);
  

  * /* Update board status */
  
  void update();
  

  /* Callback function to manage and capture changes in the network (connection status, ips, connected devices, etc).
  
  It has to be called from setup prior to Begin inside of the function WiFi.onEvent(WiFiEvent); */
  
  Example:
  
  void setup() {
  
    ....
    
    //First register for wifi events
    
    WiFi.onEvent(WiFiEvent);
    
    ...
    
    //INIT NETWORK AND ADDONS
    
    taco.begin(4444);
    
    ...
    
  }
  
  void loop(){...}
  
  ...
  
  //Receive event from the network. We manage it with taco.
  
  void WiFiEvent(WiFiEvent_t event) {
  
    Serial.printf("**** kike: [WiFi-event] event: %d\n", event);
    
    taco.manageWiFiEvent(event);
    
  } 
  
  void manageWiFiEvent(WiFiEvent_t event);
  

  * /* Define SSID (Network name) and Password of the Wifi you want to connect.
  It has to be called before Begin */
  
  void configureWifi(String net, String pass);
  

  * /* Transmit OSC data - a simple float value */
  
  void send(OSCMessage& msg, float value);
  

  * /* Transmit OSC data - an array of float values. You need to specify its size */
  
  void send(OSCMessage& msg, float *arr, int size);
  

  * /* Define a list of digital pins to read. It is necessary to define the size if the array with n_pins.*/
  
  Example:
  
    int digital_pins[] = {16, 18, 20, 22};
    
    ... setup() {
    
    ...   taco.def_digital_pins(digital_pins, 4);
    
    ....} 
  
  void def_digital_pins(int digital_pins[], int n_pins);
  

  * /* Define a list of analog pins to read. It is necessary to define the size if the array with n_pins.*/
  
  Example:
  
  int analog_pins[] = {35, 34, 33};
  
    ... setup() {
    
    ...   taco.def_analog_pins(analog_pins, 3);
    
    ....}     
  
  void def_analog_pins(int analog_pins[], int n_pins);
  

  * /* Read both lists of analog and digital pins. You should program something inside */
  
  void readPins();
  

  * //OLED display functions
  
  /*Constructor needs to get a reference of the actual display*/
  
  void createSSD1306(Adafruit_SSD1306& ssd1306);
  

  * /*Write a text at x,y coordinates in pixels*/
  
  void SSD1306_write(int x, int y, const char* text);
  

  * /*Write an int at x,y coordinates in pixels*/
  
  void SSD1306_writeInt(int x, int y, int value);
  

  * /*Clear Display*/
  
  void SSD1306_clear();
  

  * /*Show a small animation of pixels*/
  
  void SSD1306_stars();
  

  * /*Display the value of an analog pin on the display*/
  
  void SSD1306_analog_monitor(int pin);
  

  * /*Display the value of digital pin on the display*/
  
  void SSD1306_digital_monitor(int pin);
  

  * /*Inform if the user has created an OLED object or not*/
  
  bool hasOled();
  

  * //SERVER FUNCTIONS
  
  /*Begin a server to configure the board. Receive a reference*/
  
  void beginServer(WebServer& s);
  

  * /*Callback function to deal with clients asking the server*/
  
  Example:
  
    WebServer server(80);
    
    void setup(){
    
      ...
      
      server.on("/", handleRoot);
      
      taco.beginServer(server);
      
      ...
      
    }
    
    void handleRoot() {
    
      taco.handleRoot(server);
      
    }
    
  
 
  void handleRoot(WebServer& s);



