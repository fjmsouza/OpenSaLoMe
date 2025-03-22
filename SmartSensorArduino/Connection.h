#ifndef Connection_h
#define Connection_h

bool connectionSetup();
void sendData(float moisture, bool valve_state, int drop_counter);
String urlencode(String str);
void sendImage(float moisture, camera_fb_t * fb);
String receiveData();
void connectionClose();

#endif