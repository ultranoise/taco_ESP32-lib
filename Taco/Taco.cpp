
/////////////////////////////////////////////////////////////////////////
/// ESP32 Tangible Core  . OSC Client Generic Solution                 //
///                                                                    //
/// This sends OSC data to a host                                      //
/// configured as access point or connected to a wifi network          //
//                                                                     //
//  It uses pin 15 (LOW) to hardreset the board in access point mode   //
//  so you need to connect pin 15 to HIGH to make it work.             //
/// enrique.tomas@ufg.at may 2019                                      //
/////////////////////////////////////////////////////////////////////////

#include "Arduino.h"
#include "Taco.h"



Taco::Taco(int ledPin, int hardResetPin) {
  _ledPin = ledPin;

  _hardResetPin = hardResetPin;

  APssid = "taco-AP1"; //default name

  WiFiUDP udp;

  mode_clean = false;
  mode_test = true;

  WebServer _server(80);

}


Taco::Taco(int ledPin, int hardResetPin, const char *AP_name) {
  _ledPin = ledPin;

  _hardResetPin = hardResetPin;

  APssid = AP_name; //use given name

  WiFiUDP udp;

  mode_clean = false;
  mode_test = false;

  WebServer _server(80);

}



bool Taco::begin(int udpPort) {
  Serial.println();
  Serial.println("/////////////////////////////////////////////////////");
  Serial.println("Generic TACO for ESP32 solution");
  Serial.println("Tangible Music Lab 2019-2020 (enrique.tomas@ufg.at)");
  Serial.println("/////////////////////////////////////////////////////");

  _udpPort = udpPort;

  //set led pin modes and init it
  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, LOW);

  pinMode(_hardResetPin, INPUT_PULLUP); //hardreset pin

  ///eeprom init
  EEPROM.begin(EEPROM_SIZE);

  //update Access Point name ssid and password if user changed them
  writeStringMem(130, APssid);
  if(new_ssid) {
    writeStringMem(20, network);
    Serial.print("user ssid: ");
    Serial.println(network);
    writeStringMem(86, password);
    Serial.print("user passw: ");
    Serial.println(password);
  }




  // First check hardreset pin
  // A hardreset pin at pin 15 will save us when a network configuration fails!!
  // It resets the board to access point, stores this mode in the eeprom and restarts
  Serial.println("reading HARD_RESET pin");
  if(digitalRead(_hardResetPin) == LOW || mode_clean){
    Serial.println("RESET pin is low OR Mode clean is active");
    if(!mode_test){  //FLAG TO DEBUG AND TEST WITHOUT HARDRESET PIN, USUALLY THIS IS FALSE
      digitalWrite(_ledPin, LOW);
      Serial.println();
      Serial.println("HARD_RESET pin is low!! (CLEAR CONF & REBOOT)");

      resetBoard();   // reset board, fix Access Point mode and save to EEPROM

      Serial.println("Rebooting in 2 secs...");
      delay(2000);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      ESP.restart();
    }
  }
  else {
    Serial.println();
    Serial.println("HARD_RESET pin is high!!");
  }

  //read configuration from eeprom in both AP (access point) or STA (Station network) modes
  confSettings();

  //decide how to connect
  if(accesspoint) {
    boolean APconnected = false;

    Serial.println();
    Serial.println("Configuring access point...");

    createAccessPoint();
  } else {
    int devices_found = 0;
    network = read_String(20);
    password = read_String(86);
    Serial.print("user ssid: ");
    Serial.println(network);
    Serial.print("user passw: ");
    Serial.println(password);
    devices_found = connectToWiFi(network, password); //Connect to existing WLAN
    if (oled) {
      SSD1306_writeInt(120, 0, devices_found);    //inform about found devices on display
    }
  }
  return true;
}


void Taco::configureWifi(String net, String pass){
  new_ssid = true; //bool to write new values in eeprom during taco.begin
  network = net;
  password = pass;
}

//////////////////////////////////////////////////////////////////////////
//
//   PROJECT DEFINITION
//////////////////////////////////////////////////////////////////////////

//DIGITAL PIN DEFINITION
void Taco::def_digital_pins(int digital_pins[], int n_pins){

  nr_d_pins = n_pins;

  Serial.print("Digital pins defined: ");

  for(int i=0;i<=n_pins-1;i++){
    Serial.print(digital_pins[i], DEC);
    Serial.print(" ");
    d_pins.push_back(digital_pins[i]);
    d_values.push_back(-1);
  }
  Serial.println(" ");
}

//ANALOG PIN DEFINITION
void Taco::def_analog_pins(int analog_pins[], int n_pins){

  nr_a_pins = n_pins;

  Serial.print("analog pins defined: ");

  for(int i=0;i<=n_pins-1;i++){
    Serial.print(analog_pins[i], DEC);
    Serial.print(" ");
    a_pins.push_back(analog_pins[i]);
    a_values.push_back(-1);
  }
  Serial.println(" ");
}




////////////////////////////////////////////////////////////
//
// update
//
////////////////////////////////////////////////////////////

void Taco::update(){
  //if connected and ALL ok the LED should be ON
  if(ok) {
    digitalWrite(_ledPin, HIGH);
  } else {
    digitalWrite(_ledPin, LOW);
  }

  //after a change of mode we should reboot the board
  if(shouldReboot){
    Serial.println("Rebooting...");
    shouldReboot = false;
    delay(100);
    ESP.restart();
  }

}

//Function to read from a list of analog or digital a_pins
void Taco::readPins(){
  //Read Values from all inputs
  //DIGITAL
  list<int>::iterator it_d;  //iterator to read the list of digital pins
  d_values.clear();
  for(it_d=d_pins.begin();it_d!=d_pins.end();it_d++) {
        d_values.push_back(digitalRead(*it_d));
  }

  //Below some code to check now if the valuesin d_values are correctly stored
  /*
  list<int>::iterator itt;
  for(itt=d_values.begin();itt!=d_values.end();itt++) {
        Serial.print("Check pin ");
        Serial.print(*itt);
        Serial.println(" values ");
  }
  delay(500);
  */

  //ANALOG
  list<int>::iterator it_a;
  a_values.clear();
  for(it_a=a_pins.begin();it_a!=a_pins.end();it_a++) {
        a_values.push_back(analogRead(*it_a));
  }
}


// Funtion sending OSC messages to the network
void Taco::send(OSCMessage& msg, float value){

  // OSC data transmission of orientation sensor
  if(connected || APconnected){ //only send OSC data when connected

    if(accesspoint){   // ESP32 AS ACCESS POINT: send to all connected clients

        //a control flag
        ok = false; //set to false

        for(int i = 0; i < numClients; i++) {
          ok = true;
          msg.add(value);  // some dummy data (milliseconds running this code)
          udp.beginPacket(clientsAddress[i], _udpPort);
          msg.send(udp);
          udp.endPacket();
          msg.empty();
        }
    }
    if(!accesspoint){ ///CONNECTED TO STA WIFI
     ok = false;

      //OSC send

      //Multicast Discovered Hosts
      for (int i = 0; i < nServices; ++i) {
        ok = true;
        //Ip address to transmit udp packages
        sta_clientAddress = MDNS.IP(i);

        //Debug.printf("input1: %s\n", String(inputVal).c_str()); // Note: if type is String need c_str()
        msg.add(value);  // some dummy data (milliseconds running this code)
        udp.beginPacket(sta_clientAddress, _udpPort);
        msg.send(udp);
        udp.endPacket();
        msg.empty();
      }

      //Extra hosts added with taco.addHost("host_name");
      for (int i = 0; i < nExtraHosts; ++i) {
        ok = true;
        //Ip address to transmit udp packages
        sta_clientAddress = extraHostAddress[i];

        msg.add(value);
        udp.beginPacket(sta_clientAddress, _udpPort);
        msg.send(udp);
        udp.endPacket();
        msg.empty();
      }

    }

  }
}

// SEND FOR array OF ARGUMENTS, size is the number of arguments
void Taco::send(OSCMessage& msg, float *arr, int size){

  // OSC data transmission of orientation sensor
  if(connected || APconnected){ //only send OSC data when connected

    if(accesspoint){   // ESP32 AS ACCESS POINT: send to all connected clients

      //a control flag
      ok = false; //set to false

      for(int i = 0; i < numClients; i++) {
        ok = true;
          for(int j=0; j<=size-1;j++){ //add array contents
            msg.add(arr[j]);  // some dummy data (milliseconds running this code)
          }
          udp.beginPacket(clientsAddress[i], _udpPort);
          msg.send(udp);
          udp.endPacket();
          msg.empty();
     }
    }
    if(!accesspoint){ ///CONNECTED TO STA WIFI
      ok=false;
      //OSC send
      for (int i = 0; i < nServices; ++i) {
        ok = true;

        //Ip address to transmit udp packages
        sta_clientAddress = MDNS.IP(i);

        //Debug.printf("input1: %s\n", String(inputVal).c_str()); // Note: if type is String need c_str()
        for(int j=0; j<=size-1;j++){ //add array contents
          msg.add(arr[j]);  // some dummy data (milliseconds running this code)
        }
        udp.beginPacket(sta_clientAddress, _udpPort);
        msg.send(udp);
        udp.endPacket();
        msg.empty();
      }

      //Extra hosts added with taco.addHost("host_name");
      for (int i = 0; i < nExtraHosts; ++i) {
        ok = true;
        //Ip address to transmit udp packages
        sta_clientAddress = extraHostAddress[i];

        for(int j=0; j<=size-1;j++){ //add array contents
          msg.add(arr[j]);  // some dummy data (milliseconds running this code)
        }
        udp.beginPacket(sta_clientAddress, _udpPort);
        msg.send(udp);
        udp.endPacket();
        msg.empty();
      }




    }
  }
}


//////////////////////////////////////////////////////////////////////////////
//
// NETWORK METHODS
//
/////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////
/// PRIVATE NETWORK FUNCTIONS
///////////////////////////////////////////////

// basic Access Point creation
 void Taco::createAccessPoint(){

   //register to wifi event handler to manage wifi events
   //we try to make it in begin()

    WiFi.mode(WIFI_AP);
    Serial.print("---- APssid before creating AP: ");
    Serial.println(APssid);
    WiFi.softAP(APssid);
    delay(100);  //"Wait 100 ms for AP_START..."

    Serial.println("Set softAPConfig");
    IPAddress Ip(192, 168, 0, 1);       //We fix an IP easy to recover without serial monitor
    IPAddress NMask(255, 255, 255, 0);
    WiFi.softAPConfig(Ip, Ip, NMask);

    IPAddress myIP1 = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP1);
    APconnected = true;
}

///////////////////////////////////////////
//wifi basic STA connection function
///////////////////////////////////////////
int Taco::connectToWiFi(String ssid, String pwd){

  IPAddress staIP(192,168,0,129);         //Board static IP
  IPAddress staGateway(192,168,0,1);      //Gateway IP
  IPAddress staSubnet(255,255,255,0);     //Subnet range
  IPAddress primaryDNS(192, 168, 0, 1);   //optional
  IPAddress secondaryDNS(8, 8, 4, 4);     //optional

  //register event handler. This allows receiving messages of connection, disconnections, etc
/*
  Serial.println("Registering to Wifi Events");
  if(!registered){
    registered = true;
    WiFi.onEvent(WiFiEvent);
  }
*/
  // delete old config if already connected
  WiFi.disconnect(true);

  //we will simply use gateway as DNS address
  primaryDNS = staGateway;
  secondaryDNS = staGateway;


  Serial.print("connecting to: ");
  Serial.println(ssid);

  //connect to wifi
  WiFi.begin(ssid.c_str(), pwd.c_str());  //I use c_str() as we have to convert strings to arrays of characters
  //Comment below for DHCP
  WiFi.config(staIP, staGateway, staSubnet, primaryDNS,secondaryDNS);   //fix IP at network

  //while (WiFi.status() != WL_CONNECTED) { //wait until connected
  //      delay(500);
  //      Serial.print(".");
  //  }

  delay(1000);
  //Some info
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());

  //setup mDNS for collecting the IPs of other machines in the network, only in STA Mode
  discoverMDNShosts();

  //here is the place to find other hosts too
 //addHost(kike); or you make it at Arduino level with taco.addHost("KIKE-ULTRANOISE")


  return nServices;
}

/////////////////////////////////////////////////////////////////////////////////////////
///
/// WIFI management functions
///
////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////
// wifi event handler
// These are all the messages we receive from Wifi Manager

void Taco::manageWiFiEvent(WiFiEvent_t event){

    Serial.printf("[WiFi-event] event: %d\n", event);
    int result;

    switch(event) {
       case SYSTEM_EVENT_STA_GOT_IP:    //WE HAVE GOT AN IP!!
          //When connected set
          Serial.print("WiFi connected! IP address: ");
          result = 1;
          //resultEvent(1);
             udp.begin(WiFi.localIP(),_udpPort);
             connected = true;
             ok = true;
          break;

       case SYSTEM_EVENT_STA_DISCONNECTED:  //try to reconnect if disconnected
          //Serial.println("WiFi lost connection");
          Serial.println("Disconnected from WiFi access point");
          connected = false;

            WiFi.disconnect(true);
            ok = false;
            //Initiate connection
            connectToWiFi(network, password);
            delay(1000);
          break;

        case SYSTEM_EVENT_WIFI_READY:
            Serial.println("WiFi interface ready");
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi client started");
            break;
        case SYSTEM_EVENT_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("Connected to STA");
            break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
            Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case SYSTEM_EVENT_AP_START:
            Serial.println("WiFi access point started");
            APconnected = true;
            connected = true;
            updateStations();

            break;
        case SYSTEM_EVENT_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("Client connected");
            connected = true;

            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            updateStations();
            break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            updateStations();
            connected = true;
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case SYSTEM_EVENT_GOT_IP6:
            Serial.println("IPv6 is preferred");
            break;
        case SYSTEM_EVENT_ETH_START:
            Serial.println("Ethernet started");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            Serial.println("Ethernet stopped");
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            Serial.println("Ethernet connected");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet disconnected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            Serial.println("Obtained IP address");
        break;
    }

}


///////////////////////////////////////////////////////////////////////
// Handling information about connected clients in access point mode
// For sending OSC data to clients we need to know their IPs and this
// function makes the work.
void Taco::updateStations() {

  wifi_sta_list_t stationList;
  esp_wifi_ap_get_sta_list(&stationList);

  //update number of clients (stations connected)
  numClients = stationList.num;

  Serial.println("-----------------");
  Serial.print("Number of connected stations: ");
  Serial.println(stationList.num);
  Serial.println("-----------------");
  if(oled) {
    display.fillRect(120, 0, 25, 10, SSD1306_BLACK);
    SSD1306_writeInt(120, 0, stationList.num);
  }


  //Now obtain their Mac addresses and IP addresses
  tcpip_adapter_sta_list_t adapter_sta_list;
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

  ESP_ERROR_CHECK(tcpip_adapter_get_sta_list(&stationList, &adapter_sta_list));

  //we have to convert station information to IPAdress type of arduino
  for(int i = 0; i < adapter_sta_list.num; i++) {
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
    printf("%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP: %s\n", i + 1,
    station.mac[0], station.mac[1], station.mac[2],
    station.mac[3], station.mac[4], station.mac[5],
    ip4addr_ntoa(&(station.ip)));

    String ipi = ip4addr_ntoa(&(station.ip));
    String strIP = ipi;

    int Parts[4] = {0,0,0,0};
    int Part = 0;
    for ( int i=0; i<strIP.length(); i++ ) {
      char c = strIP[i];
      if ( c == '.' ) {
        Part++;
        continue;
      }
      Parts[Part] *= 10;
      Parts[Part] += c - '0';
    }

    //finally this is the array of clients ip addresses
    IPAddress ipo( Parts[0], Parts[1], Parts[2], Parts[3] );
    clientsAddress[i]= ipo;

  }

}


/////////////////////////////////////////////////////////////
//   //setup mDNS for collecting the IPs of other machines in the network,
///only in STA Mode
////////////////////////////////////////////////////////////////////

void Taco::discoverMDNShosts(){

  Serial.println("discovering hosts");

  if(accesspoint == false || user_STA){
    delay(2000);
    if (!MDNS.begin("taco")) {  //dummy name
          Serial.println("Error setting up MDNS responder!");
         while(1){
              delay(1000);
          }
      }


    //browse samba services to discover available devices
    browseService("smb", "tcp");
    delay(100);
    browseService("http", "tcp");
    delay(100);
    browseService("workstation", "tcp");
    delay(100);
    browseService("upnp", "tcp");
    delay(100);
    browseService("ssdp", "tcp");
    delay(100);
    browseService("ftp", "tcp");
    delay(100);
    browseService("uuid", "tcp");
    delay(100);
    browseService("printer", "tcp");

  }
}

void Taco::addHost(String host_name){
  /// NEW STUFF
  Serial.println("Browsing for host");
  IPAddress serverIp = MDNS.queryHost(host_name);
  while (serverIp.toString() == "0.0.0.0") {
    Serial.println("Trying again to resolve mDNS");
    delay(250);
    serverIp = MDNS.queryHost(host_name);
  }
  Serial.print("IP address of server: ");
  Serial.println(serverIp.toString());
  Serial.println("Done finding the host...");
  extraHostAddress[nExtraHosts] = serverIp;
  nExtraHosts = nExtraHosts + 1;
  //return serverIp.toString();

}

/////////////////////////////////////////////////////////////
//   browse computer services to discover devices
////////////////////////////////////////////////////////////////////
void Taco::browseService(const char * service, const char * proto){
    Serial.println(" ");
    Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
    nServices = MDNS.queryService(service, proto);
    if (nServices == 0) {
        Serial.println("no services found");
    } else {
        Serial.print(nServices);
        Serial.println(" service(s) found");
        for (int i = 0; i < nServices; ++i) {
            // Print details for each service found
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(MDNS.hostname(i));
            Serial.print(" (");
            Serial.print(MDNS.IP(i));
            Serial.print(":");
            Serial.print(MDNS.port(i));
            Serial.println(")");
        }
    }
    Serial.println();

}

/////////////////////////////////////////////////////////
///
/// EEPROM
/////////////////////////////////////////////////////////

//Configuration Settings loaded from eeprom
void Taco::confSettings(){
  Serial.println();
  Serial.println("* Reading EEPROM:");

  String d;
  int address;

  // the first byte is a flag (0 or 1) informing about wifi modes
  address = 0;
  d = read_String(address);
  Serial.print("- AccesPoint flag (0= AP, 1=STA):");
  Serial.println(d);
  if(d.toInt()==0){
    accesspoint = true;
  } else {
    accesspoint = false;
  }

  address = 2; //we know the flag is always 2 bytes long
  d = read_String(2);
  //Serial.print("- SSID Length:");
  //Serial.println(d);

  d = read_String(20);  //reading ssid, address (20) is arbitrary, the following are too.
  Serial.print("- SSID:");
  Serial.println(d);
  network = d;

  d = read_String(86);
  Serial.print("- Password:");
  Serial.println(d);
  password = d;

  d = read_String(130);
  Serial.print("- Access Point Name: ");
  Serial.println(d);
  APssid_read_from_eeprom = d.c_str();

  /*
  d = read_String(170);
  Serial.print("- ip at new network:");
  Serial.println(d);
  if(!accesspoint ) staIP = string2IP(d);
  */

  d = read_String(200);
  Serial.print("- OSC port:");
  Serial.println(d);
  if(!accesspoint ) _udpPort = d.toInt();  //cause it can be empty at the beginning

  /*
  d = read_String(216);
  Serial.print("- Gateway ip:");
  Serial.println(d);
  if(!accesspoint ) staGateway = string2IP(d);
  */

}



// reset board to Access Point mode and clear eeprom
void Taco::resetBoard(){
  Serial.println();
  Serial.println("I will reset this ESP32");

  //LOAD INFORMATION IN EEPROM
  Serial.println();
  Serial.println("writing original data in eeprom memory");

    //clear eeprom
    for (int i = 0 ; i < EEPROM_SIZE ; i++) {
      EEPROM.write(i, 0);
    }

    int address = 0;
    writeStringMem(0, "0");                   //flag for setting access point OR NOT (0=AP, 1=STA)

    Serial.println("ESP32 should be in AP mode, rebooting...");
    delay(300);
    //shouldReboot = true;

}


// eeprom basic read/write functions
void Taco::writeStringMem(char add,String data){
  int _size = data.length();
  int i;
  for(i=0;i<_size;i++)
  {
    EEPROM.write(add+i,data[i]);
  }
  EEPROM.write(add+_size,'\0');   //Add termination null character for String Data
  EEPROM.commit();
}


// eeprom functions
String Taco::read_String(char add){
  int i;
  char data[100]; //Max 100 Bytes
  int len=0;
  unsigned char k;
  k=EEPROM.read(add);
  while(k != '\0' && len<500)   //Read until null character
  {
    k=EEPROM.read(add+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}


/////////////////////////////////////////////////
//
// SSDD1306 display functions
//
//////////////////////////////////////////////////

struct my_monitor
{
   Adafruit_SSD1306 d;
   int pin;
};

void Taco::createSSD1306(Adafruit_SSD1306& ssd1306){

  display = ssd1306;
  //ssd1306.begin();

  Serial.println("INIT OLED");
  //delay(20);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  Serial.println("OLED ok");

  oled = true;
  // Clear the buffer
  display.clearDisplay();

  //init a few things of the clearDisplaydisplay.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text

  //print a few things
  SSD1306_write(20, 10, "hola, I'm taco");
  delay(1000);
  display.clearDisplay();
  display.display();

  delay(500);

}

//returns if there is oled
bool Taco::hasOled(){
  return oled;
}

void Taco::SSD1306_clear(){
  // Clear the buffer
  display.clearDisplay();
  display.display();
}

void Taco::SSD1306_write(int x, int y, const char* text){
  display.setTextSize(1);
  display.setCursor(x,y);             // Start at top-left corner
  display.println(F(text));
  display.display();
}

void Taco::SSD1306_writeInt(int x, int y, int value){
  display.setTextSize(1);
  display.setCursor(x,y);             // Start at top-left corner
  display.println(value);
  display.display();
}

void Taco::SSD1306_analog_monitor(int pin){

  if(millis() >= time_1 + INTERVAL_UPDATE_OLED){
          time_1 +=INTERVAL_UPDATE_OLED;

          display.setCursor(0,10);
          display.println("pin");
          display.setCursor(25,10);
          display.println(pin);
          display.setCursor(45,10);
          display.println(":");

          //clear lines of the value
          display.fillRect(65, 10, 25, 10, SSD1306_BLACK);

          display.setCursor(65,10);
          display.println(analogRead(pin));
          display.display();
  }

}

void Taco::SSD1306_digital_monitor(int pin){
  if(millis() >= time_1 + INTERVAL_UPDATE_OLED){
          time_1 +=INTERVAL_UPDATE_OLED;

          display.setCursor(0,10);
          display.println("pin");
          display.setCursor(25,10);
          display.println(pin);
          display.setCursor(45,10);
          display.println(":");

          //clear lines of the value
          display.fillRect(65, 10, 25, 10, SSD1306_BLACK);

          display.setCursor(65,10);
          display.println(digitalRead(pin));
          display.display();
  }
}

void Taco::hSlider(int x, int y, int w, int h, int value){
  //altura slider es h
  //border
  display.drawFastHLine(x,y, w, SSD1306_WHITE);
  //display.display(); // Update screen with each newly-drawn line
  display.drawFastHLine(x,y+h, w, SSD1306_WHITE);
  //display.display(); // Update screen with each newly-drawn line
  display.drawFastVLine(x,y, h, SSD1306_WHITE);
  //display.display();
  display.drawFastVLine(x+w,y, h, SSD1306_WHITE);
  //display.display();

  //value
  for(int i=0; i<value; i+=1) {
    display.drawFastVLine(x+i,y, h, SSD1306_WHITE);

  }
 display.display();
}


void Taco::SSD1306_stars(){
  // Draw a single pixel in white
  //
  // Draw a single pixel in white
  for(int i= 0;i<50;i++){
    display.drawPixel(random(127), random(32), SSD1306_WHITE);
    display.display();
    delay(50);
  }
}






/////////////////////////////////////////////////////////////////////////////////////////
//
// server
//
/////////////////////////////////////////////////////////////////////////////////////////

void Taco::beginServer(WebServer& s) {

  //Server start
  s.begin();
  Serial.println("Web Server started");
}


void Taco::handleRoot(WebServer& s){
  Serial.println("*** he llegado aqui tras /");

  //Browse information received
  for(int i=0; i<s.args(); i++) {
    Serial.println(s.arg(i));
  }

  s.send(200, "text/html", SendHTML()); //refresh the website or the browser will show an empty page

  //ESTO DE ABAJO PARA CUANDO TODO FUNCIONE

  if (s.hasArg("mode_ap") && s.args()==1) {     //when it is STA mode and we want to change to access point
      handle_APchange();
      //send a response
      s.send(200, "text/html", SendHTML());
  }
  else{
    if (s.hasArg("ssid_name") && s.args()==3) { //when it is access point and we want to change to STA
      handleSsid(s);
      //send a response
      s.send(200, "text/html", SendHTML());
    }
    else {
      s.send(200, "text/html", SendHTML()); //refresh the website or the browser will show an empty page
    }
 }

}


//funtion for changing from STA to access point mode in the HMTL server
void Taco::handle_APchange() {

  Serial.println();
  Serial.println("Configuring as Access Point");

  //LOAD INFORMATION IN EEPROM
  Serial.println();
  Serial.println("writing data in eeprom memory");

  Serial.print("Writing Data:");
    //writing strings at memory. First arg sets a byte address, second the data to store

    //clear eeprom first
    for (int i = 0 ; i < EEPROM_SIZE ; i++) {
      EEPROM.write(i, 0);
    }

    //then set flag to Access Point, the rest was cleared
    int address = 0;
    writeStringMem(0, "0");                   //flag for setting access point OR NOT (0=AP, 1=STA)

    delay(3000);
    shouldReboot = true;
}


//function for changing from Access Point mode to STA mode  with the HTML Server
void Taco::handleSsid(WebServer& s)
{
  //some aux strings to store information typed by user at the webserver
  String new_ssid;
  String new_passw;
  String new_fixed_ip;
  String new_host_ip;
  String new_host_port;
  String new_gateway_ip;

  //check data and load wifi data
  if (!s.hasArg("ssid_name")) return returnFail(s, "BAD ARGS");
  new_ssid = s.arg("ssid_name");
  Serial.println("New SSID to connect: " + new_ssid);

  if (!s.hasArg("password_name")) return returnFail(s, "BAD ARGS");
  new_passw = s.arg("password_name");
  Serial.println("New passw to connect " + new_passw);


  /*
  if (!server.hasArg("fixed_IP")) return returnFail("BAD ARGS");
  new_fixed_ip = server.arg("fixed_IP");
  Serial.println("IP at new network  " + new_fixed_ip);

  if (!server.hasArg("host_IP")) return returnFail("BAD ARGS");
  new_host_ip = server.arg("host_IP");
  Serial.println("IP of host at new network  " + new_host_ip);

  if (!server.hasArg("gateway_IP")) return returnFail("BAD ARGS");
  new_gateway_ip = server.arg("gateway_IP");
  Serial.println("Gateway IP  " + new_gateway_ip);
  */
  if (!s.hasArg("host_Port")) return returnFail(s, "BAD ARGS");
  new_host_port = s.arg("host_Port");
  Serial.println("OSC port to change  " + new_host_port);

  //LOAD INFORMATION IN EEPROM
  Serial.println();
  Serial.println("writing data in eeprom memory");

  String data = new_ssid;


  //writing strings in flash memory. First arg sets a byte address, second the data to store
  Serial.print("Writing Data:");
  Serial.println(data);


    //clear eeprom first
    for (int i = 0 ; i < EEPROM_SIZE ; i++) {
      EEPROM.write(i, 0);
    }

    int address = 0;
    writeStringMem(0, "1");                   //flag for setting access point OR NOT (0=AP, 1=STA)

    address = 2; // as the flag "1" is two bytes long
    writeStringMem(2, String(new_ssid.length()));           //SSID string length

    address = 20;       //20 is arbitrary, to leave some space for other flags
    writeStringMem(address, new_ssid);

    address = 86;       //SSIDs can be 32 character long (65 bytes)
    writeStringMem(address, new_passw);

    address = 130;       //passw max is 20 char long (41 bytes)
    writeStringMem(address, APssid);                 //IP is 16 bythes long

    /*
    address = 170;       //an IP is max 15 char long (31 bytes)
    writeStringMem(address, new_fixed_ip);
    */
    address = 200;       //an IP is max 15 char long (31 bytes)
    writeStringMem(address, new_host_port);
    /*
    address = 216;       //an IP is max 15 char long (31 bytes)
    writeStringMem(address, new_gateway_ip);
    */

  //We should reboot the esp32 now
  Serial.println();
  Serial.println("data written in eeprom memory");
  delay(3000);
  shouldReboot = true;
}



// other necessary handling functions
void Taco::returnFail(WebServer& s, String msg){

  s.sendHeader("Connection", "close");
  s.sendHeader("Access-Control-Allow-Origin", "*");
  s.send(500, "text/plain", msg + "\r\n");

}

void Taco::returnOK(WebServer& s){

  s.sendHeader("Connection", "close");
  s.sendHeader("Access-Control-Allow-Origin", "*");
  s.send(200, "text/plain", "OK\r\n");

}


String Taco::SendHTML(){
  String ptr = "<!DOCTYPE HTML>";
ptr +="<html>";
ptr +="<head>";
ptr +="<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">";
ptr +="<title>ESP32 - Tangible Core</title>";
ptr +="<style>";
ptr +="\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\"";
ptr +="</style>";
ptr +="</head>";
ptr +="<body>";
ptr +="<h1>ESP32 - Tangible Core</h1>";
ptr +="<FORM action=\"/\" method=\"post\">";
ptr +="<P>";

if(accesspoint){                              //html code when access point. it allows connecting to a network using a form
  ptr +="<P>";
  ptr +="ESP32 Configured as Access Point";
  ptr +="<P>";
  ptr +="OSC Port: ";
  ptr +=String(_udpPort);
  ptr +="<P>";
  ptr +="Number of connected Devices: ";
  ptr +=String(numClients);
  ptr +="<P>";
  if (numClients > 0) {
    ptr +="<P>";
    ptr +="Clients IPs: ";

    for(int i = 0; i < numClients; i++) {
      ptr +=" ";
      ptr +=clientsAddress[i].toString();;
    }
  }
  ptr +="<br>";
  // now create a few input fields for entering information about the network to connect
  ptr +="</P>";
  ptr +="To change Access Point mode and connect to a Wifi: <br>";
  ptr +="</P>";
  ptr +="Wifi name<br>";
  ptr +="<INPUT type=\"text\" name=\"ssid_name\"<BR>";

  ptr +="</P>";
  ptr +="Password<br>";
  ptr +="<INPUT type=\"text\" name=\"password_name\"<BR>";
  /*
  ptr +="</P>";
  ptr +="IP of the computer you will send OSC<br>";
  ptr +="<INPUT type=\"text\" name=\"host_IP\"<BR>";

  ptr +="</P>";
  ptr +="Your IP number in the new Wifi Network<br>";
  ptr +="<INPUT type=\"text\" name=\"fixed_IP\"<BR>";

  ptr +="</P>";
  ptr +="Gateway IP <br>";
  ptr +="<INPUT type=\"text\" name=\"gateway_IP\"<BR>";
  */
  ptr +="</P>";
  ptr +="OSC Port<br>";
  ptr +="<INPUT type=\"text\" name=\"host_Port\"<BR>";

  ptr +="</P>";
  ptr +="Activate Wifi Now clicking on Send<br>";

  ptr +="<INPUT type=\"submit\" value=\"Send\">";  //the button to submit information
  ptr +="</P>";



} else{                             //html code when STA mode. it only allows setting as access point
  ptr +="<P>";
  ptr +="ESP32 Connected to WIFI: " + String(network);
  ptr +="<P>";
  ptr +="my IP is: ";
  ptr += IpAddress2String(WiFi.localIP());
  ptr +="<P>";
  ptr +="OSC Port: ";
  ptr +=String(_udpPort);
  ptr +="<P>";
  /*
  if(hostSelected){
    ptr +="Transmitting to device: ";
    ptr += hostSelectedName;
    ptr +="<P>";
  } else{
    ptr +="No device selected to transmit";
    ptr +="<P>";
  }
  */
  ptr +="Available devices to transmit OSC: ";
  ptr +="<P>";
  for (int i = 0; i < nServices; ++i) {
      // Print details for each service found
      ptr +=MDNS.hostname(i) + " (" +  IpAddress2String(MDNS.IP(i)) +") ";
      ptr +=" ";
      ptr +="<INPUT type=\"checkbox\"";
      ptr += " name=\"host" + String(i) + "\"<BR>host to transmit<br>";
      ptr +="</P>";


      /*
      Serial.print("  ");
      Serial.print(": ");
      Serial.print(MDNS.hostname(i));
      Serial.print(" (");
      Serial.print(MDNS.IP(i));
      Serial.print(":");
      Serial.print(MDNS.port(i));
      Serial.println(")");
      */
  }

  ptr +="Transmit OSC to: ";

  //ptr +="Sending OSC to IP: ";
  //ptr +=IpAddress2String(udpAddress);
  ptr +="<P>";
  ptr +="<br>";

  ptr +="Configuration:";
  ptr +="</P>";
  ptr +="Configure ESP32 board as Access Point: ";
  ptr +="</P>";
  ptr +="<INPUT type=\"checkbox\" name=\"mode_ap\"<BR>Activate Access Point<br>";
  ptr +="</P>";

  ptr +="<INPUT type=\"submit\" value=\"Send\">";   //a button to reset to acccess point
  ptr +="</P>";
  ptr +="</P>";
  ptr +="</P>";
}

ptr +="</FORM>";
ptr +="</body>";
ptr +="</html>";
ptr +=style;
return ptr;
}






//////////////////////////////////////////////
// Aux Functions (converters, parsers, smoothing, filters, etc)
//
/////////////////////////////////////////////

//function to convert a string to an IPAdress type
IPAddress Taco::string2IP(String strIP){

    int Parts[4] = {0,0,0,0};
    int Part = 0;
    for ( int i=0; i<strIP.length(); i++ ) {
      char c = strIP[i];
      if ( c == '.' ) {
        Part++;
        continue;
      }
      Parts[Part] *= 10;
      Parts[Part] += c - '0';
    }

    IPAddress ipo( Parts[0], Parts[1], Parts[2], Parts[3] );
    return ipo;
}

//function to convert IPAdress type to string
String Taco::IpAddress2String(const IPAddress& ipAddress){
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}
