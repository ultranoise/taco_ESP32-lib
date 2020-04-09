/*
 * Simply create an access point and transmit OSC data to all hosts connected.
 * 
 * Enrique Tomas for Tangible Music Lab, Kunstuniversität Linz
 * enrique.tomas@ufg.at
 */

#include <Taco.h>

//name of this board
const char *board_name = "taquito";  //give it a different name :)

//init Taco: (led pin, hardware reset Pin, access point name)
Taco taco(2, 15, board_name);

//OSC messages
OSCMessage msg_test("/osc/test");
OSCMessage msg_test2("/osc/test2");

void setup()
{
  Serial.begin(115200);     //if you want to receive updates in your serial console

  //NETWORK
  WiFi.onEvent(WiFiEvent);  //First register for wifi events
  taco.begin(4444);         //connect as access point and transmit to all connected devices using this port

}

void loop(){
  
  taco.update();        //update board

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
