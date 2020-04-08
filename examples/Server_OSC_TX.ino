#include <Taco.h>

//name of this board
const char *board_name = "taquito";  //give it a different name :)

//init Taco: (led pin, hardware reset Pin)
Taco taco(2, 15, board_name);

//OSC messages
OSCMessage msg_test("/osc/test");
OSCMessage msg_test2("/osc/test2");

//HTML server to configure the board
WebServer server(80);

void setup()
{
  Serial.begin(115200);     //if you want to receive updates in your serial console

  //NETWORK
  WiFi.onEvent(WiFiEvent);  //First register for wifi events
  taco.begin(4444);         //connect as access point and transmit to all connected devices using this port
                            //It is possible to connect it to a Wifi going to 192.168.0.1 in your browser.

  //WEBSERVER
  server.on("/", handleRoot);  //handleRoot is a function dealing with the actions on the server via browser
  taco.beginServer(server);
}

void loop(){
  
  taco.update();        //update board

  //handle wifi server clients connected and configuring via browser
  //In access point mode connect to 192.168.0.1
  //In Wifi Mode the board can be at 192.168.0.129, but it depends on your network settings
  server.handleClient();
  
  //send one value
  taco.send(msg_test, analogRead(35) / 4095.0);
  
  //send an array of values
  float a[] = {35.0, 34.0, 33.0};
  taco.send(msg_test2, a, 3);                   
}



//Receive event from the network. We manage it with taco.
void WiFiEvent(WiFiEvent_t event) {
  taco.manageWiFiEvent(event);
}

void handleRoot() {
  Serial.println("**en handle root de setup**");
  taco.handleRoot(server);
}
