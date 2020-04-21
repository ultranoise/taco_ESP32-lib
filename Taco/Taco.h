#ifndef Morse_h
#define Morse_h


/////////////////////////////////////////////////////////////////////////
/// ESP32 Tangible Core  . OSC Client Generic Solution                 //
///                                                                    //
/// This class helps you to send OSC data to a host with your ESP32.   //
/// It can be configured (with your internet browser) as an access     //
/// point or STA (connect the board to an existing wifi network).      //
///                                                                    //
/// Once the board is running with this code, connect your computer to //
/// the access point "taquito" and browse to 192.168.0,1               //
///                                                                    //
///  This code uses a pin(if LOW) to hardreset the board to access     //
///  point mode so you need to connect that pin to HIGH to run         //
///  the rest of the code (if not it will just reboot).                //
///                                                                    //
/// enrique.tomas@ufg.at april 2020                                    //
/////////////////////////////////////////////////////////////////////////

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "esp_wifi.h"
#include "EEPROM.h"
#include <Wire.h>

// ADDONS includes:
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

////C++ includes
#include <list>
#include <string>

using namespace std;


////////////////////////////
// define section
////////////////////////////


//EEPROM global vars
#define EEPROM_SIZE 256
#define INTERVAL_UPDATE_OLED 250


class Taco
{
  public:
    /* Basic Constructor with onboard led pin and hardreset pin */
    Taco(int ledPin, int hardResetPin);

    /* Constructor with onboard led pin, hardreset pin and access point name */
    Taco(int ledPin, int hardResetPin, const char *AP_name);

    /* Begin all necessary stuff (eeprom, hardreset checks, saved informations, network) */
    bool begin(int udpPort);

    /* Update board status */
    void update();

    /* Callback function to manage and capture changes in the network (connection status, ips, connected devices, etc).
    It has to be called from setup prior to Begin inside of the function WiFi.onEvent(WiFiEvent);
    Example:
    void setup() {
      ....
      //First register for wifi events
      WiFi.onEvent(WiFiEvent);

      //INIT NETWORK AND ADDONS
      taco.begin(4444);
      ...
    }
    void loop(){...}




    //Receive event from the network. We manage it with taco.
    void WiFiEvent(WiFiEvent_t event) {
      Serial.printf("**** kike: [WiFi-event] event: %d\n", event);
      taco.manageWiFiEvent(event);
    } */
    void manageWiFiEvent(WiFiEvent_t event);

    void resetBoard();                          //reset board to access point mode

    /* Define SSID (Network name) and Password of the Wifi you want to connect.
    It has to be called before Begin */
    void configureWifi(String net, String pass);

    /* Transmit OSC data - a simple float value */
    void send(OSCMessage& msg, float value);

    /* Transmit OSC data - an array of float values. You need to specify its size */
    void send(OSCMessage& msg, float *arr, int size);

    /* Define a list of digital pins to read. It is necessary to define the size if the array with n_pins.
    Example:
      int digital_pins[] = {16, 18, 20, 22};
      ... setup() {
      ...   taco.def_digital_pins(digital_pins, 4);
      ....} */
    void def_digital_pins(int digital_pins[], int n_pins);

    /* Define a list of digital pins to read. It is necessary to define the size if the array with n_pins.
    Example:
    int analog_pins[] = {35, 34, 33};
      ... setup() {
      ...   taco.def_analog_pins(analog_pins, 3);
      ....} */    void def_analog_pins(int analog_pins[], int n_pins);

    /* Read both lists of analog and digital pins. You should program something inside */
    void readPins();

    //OLED display functions
    /*Constructor needs to get a reference of the actual display*/
    void createSSD1306(Adafruit_SSD1306& ssd1306);

    /*Write a text at x,y coordinates in pixels*/
    void SSD1306_write(int x, int y, const char* text);

    /*Write an int at x,y coordinates in pixels*/
    void SSD1306_writeInt(int x, int y, int value);

    /*Clear Display*/
    void SSD1306_clear();

    /*Show a small animation of pixels*/
    void SSD1306_stars();

    /*Display the value of an analog pin on the display*/
    void SSD1306_analog_monitor(int pin);

    /*Display the value of digital pin on the display*/
    void SSD1306_digital_monitor(int pin);

    /*Inform if the user has created an OLED object or not*/
    bool hasOled();

    //SERVER FUNCTIONS
    /*Begin a server to configure the board. Receive a reference*/
    void beginServer(WebServer& s);

    /*Callback function to deal with clients asking the server
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
    */
    void handleRoot(WebServer& s);


  private:
    void createAccessPoint();                   //creates the actual AP
    int connectToWiFi(String ssid, String pwd); //connect to wifi with ssid and passw
    void updateStations();                      //update the connected devices in this network
    void writeStringMem(char add,String data);  //write string in eeprom with add as the address in eeprom
    String read_String(char add);               //read string from eeprom with add as the address in eeprom
    void confSettings();                        //read the board configuration from eeprom
    void discoverMDNShosts();                   //discover hosts connect to this network
    void browseService(const char * service, const char * proto);  //find devices browsing network services (ftp, samba, etc)

    //SSD1306 OLED display
    void hSlider(int x, int y, int w, int h, int value);    //show a horizontal slider with a value at x,y coordinates with weight w and hight h.

    //Server
    String SendHTML();                          //function to send the html code of the server
    void handle_APchange();                     //handle function to deal with user changing to Access Point mode with the server
    void handleSsid(WebServer& s);              //handle function to deal with user changing to Wifi mode with the server
    void returnFail(WebServer& s, String msg);  //basic html response
    void returnOK(WebServer& s);                //basic html response

    //Aux utils from to IP address to String
    IPAddress string2IP(String strIP);
    String IpAddress2String(const IPAddress& ipAddress);

    //Wifi objects
    WiFiUDP udp;  //Using Wifi UDP

    //SERVER
    WebServer _server;

    //OLED
    Adafruit_SSD1306 display;

    //list of pins
    int nr_d_pins;      //nr of digital pins used
    list<int> d_pins;   //list of digital pins
    list<int> d_values; //list of digital pin values
    int nr_a_pins;      //nr of analog pins used
    list<int> a_pins;   //list of analog pins
    list<int> a_values; //list of analog pin values

    //hardcoding flags for debugging
    bool mode_clean;    //if true, code cleans the eeprom at Begin
    bool mode_test;     //if true, code does not check HARD_RESET pin

    // void wifiEventCaller();

    int _ledPin;        //onboard led pin
    int _udpPort;       //tx udp port to transmit
    int _hardResetPin;  //pin to hardreset the board network

    // Name of your default access point.
    const char *APssid;
    const char *APssid_read_from_eeprom;

    //A few flags to know the status of our connection
    boolean connected = false;     //wifi connection
    bool accesspoint = true;       //acccess point mode
    boolean APconnected;           //access point connected
    boolean ok;                    // a general flag about network
    bool shouldReboot = false;     //flag to use from web update to reboot the ESP
    bool user_AP = false;          //reserved, not necessary
    bool user_STA = false;         //reserved, not necessary
    bool new_ssid = false;         //if user changed ssid information

    // WiFi network name and password:
    String network;
    String password;

    //Clients connected in Access Point mode
    IPAddress clientsAddress[10];       //ten clients can be connected in access point mode
    int numClients = 0;

    //client ip address in STA-MODE
    IPAddress sta_clientAddress;
    int nServices = 0;            //mDNS services count: number of devices connected in Wifi Mode

    //TIME control
    unsigned long time_1 = 0;

    //to know if there are oleds
    bool oled = false;

    /* html Style for adding to server actual HTML code */
    String style =
    "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
    "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
    "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
    "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
    "form{background:#fff;max-width:358px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
    ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

    /* Login page */
    String loginIndex =
    "<form name=loginForm>"
    "<h1>ESP32 Login</h1>"
    "<input name=userid placeholder='User ID'> "
    "<input name=pwd placeholder=Password type=Password> "
    "<input type=submit onclick=check(this.form) class=btn value=Login></form>"
    "<script>"
    "function check(form) {"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{window.open('/serverIndex')}"
    "else"
    "{alert('Error Password or Username')}"
    "}"
    "</script>" + style;

    /* Server Index Page */
     String updateform =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
    "<label id='file-input' for='file'>   Choose file...</label>"
    "<input type='submit' class=btn value='Update'>"
    "<br><br>"
    "<div id='prg'></div>"
    "<br><div id='prgbar'><div id='bar'></div></div><br></form>"
    "<script>"
    "function sub(obj){"
    "var fileName = obj.value.split('\\\\');"
    "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
    "};"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    "$.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "$('#bar').css('width',Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!') "
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>" + style;


};


#endif
