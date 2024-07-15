#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include "Base64.h"
#include "Arduino.h"
#include "Credentials.h"


// Dados para conexão com o Google Drive
const char* host = "script.google.com";
const int httpsPort = 443;

//Dados para o arquivo de imagem
String file_name = "filename=ESP32-CAM.jpg";//"filename=ESP32-CAM.jpg";
String mime_type = "&mimetype=image/jpeg";//"&mimetype=image/jpeg";
String image2string = "&data=";
int begin;
const int time_2_reset = 10000; //10"

bool connectionSetup()
{
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Waiting connection to WiFi..");
  begin = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    // reset the esp after 10 seconds
    if((millis() - begin) > time_2_reset){
      // ESP.restart();  
      return false;
    }
  }  
  Serial.println("Connected!");
  return true;
}

String urlencode(String str)  //Função de codificação
{
  String encodedString="";
  char c;
  char code0;
  char code1;
  char code2;
    
  for (int i =0; i < str.length(); i++){
    c=str.charAt(i);
    if (c == ' '){
      encodedString+= '+';
    } else if (isalnum(c)){
      encodedString+=c;
    } else{
      code1=(c & 0xf)+'0';
      if ((c & 0xf) >9){
          code1=(c & 0xf) - 10 + 'A';
      }
      c=(c>>4)&0xf;
      code0=c+'0';
      if (c > 9){
          code0=c - 10 + 'A';
      }
      code2='\0';
      encodedString+='%';
      encodedString+=code0;
      encodedString+=code1;
    }
    yield();
  }
  return encodedString;
}

void sendData(float moisture, bool valve_state, int drop_counter)
{
  Serial.println("Connecting to " + String(host));
  WiFiClientSecure client;
  client.setInsecure();
  if (client.connect(host, httpsPort)) {

    Serial.println("Connection succeeded!");
    String string_humidity = String(moisture,1);
    String string_valve_state = "";
	  String string_drop_counter = String(drop_counter,1);													
    if (valve_state)
    {
      string_valve_state = "100";
    }
    else
    {
      string_valve_state = "0";
    }

    Serial.println("Sending data to Google Sheets.");
    String url = "https://script.google.com/macros/s/"+ GOOGLE_SHEETS_SCRIPT_ID +"/exec?"+"moisture=" + string_humidity +"&valvestate=" + string_valve_state + "&dropcounter=" + String(drop_counter);
    Serial.print("requesting URL: ");
    Serial.println(url);

    client.println(String("GET ") + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("User-Agent: BuildFailureDetectorESP32 ");
    client.println("Connection: close");
    client.println();
  }
  client.stop();
}

void sendImage(float moisture, camera_fb_t * fb) {
   
  Serial.println("Connecting to " + String(host));
  WiFiClientSecure client;
  client.setInsecure();
  if (client.connect(host, httpsPort)) { //Conectando no Google
      
    Serial.println("Connection succeeded!");

    char *input = (char *)fb->buf;
    char output[base64_enc_len(3)];
    String imageFile = "";
     
    for (int i=0;i<fb->len;i++) {
      base64_encode(output, (input++), 3);
      if (i%3==0) imageFile += urlencode(String(output));
    }
     
    String Data = file_name + mime_type + image2string;
    esp_camera_fb_return(fb); 
    Serial.println("Sending image to Google Drive.");
    String string_humidity = String(moisture,1);
    String url = "https://script.google.com/macros/s/"+ GOOGLE_DRIVE_SCRIPT_ID +"/exec?"+"moisture=" + string_humidity;
    Serial.print("requesting URL: ");
    Serial.println(url);

    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Content-Length: " + String(Data.length()+imageFile.length()));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();
    client.print(Data);
     
    int Index;
     
    for (Index = 0; Index < imageFile.length(); Index = Index+1000) {
      client.print(imageFile.substring(Index, Index+1000));
    }
     
    Serial.println("Waiting response."); 

    begin = millis();
    while (!client.available()) { //Aguarda resposta do envio da imagem
      Serial.print(".");
      // reset the esp after 10 seconds
      if((millis() - begin) > time_2_reset){
        ESP.restart();  
      }
    }   
     
    Serial.println();   

    begin = millis();
    while (client.available()) { //Aguarda resposta do envio da imagem
      Serial.print(char(client.read())); //Mostra na tela a resposta      
      // reset the esp after 10 seconds
      if((millis() - begin) > time_2_reset){
        ESP.restart();  
      }
    }        
  
  } else {         
    Serial.println("Connection to " + String(host) + " failed.");
  }
  client.stop();
}


String receiveData() {

  HTTPClient http;

  Serial.println("Read data from Google Sheets.");
  String url="https://script.google.com/macros/s/" + GOOGLE_SHEETS_SCRIPT_ID + "/exec?read";
  Serial.print("requesting URL: ");
  Serial.println(url);

  http.begin(url.c_str());
  //-----------------------------------------------------------------------------------
  //Removes the error "302 Moved Temporarily Error"
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  //-----------------------------------------------------------------------------------
  //Get the returning HTTP status code
  int httpCode = http.GET();
  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);
  //-----------------------------------------------------------------------------------
  if(httpCode <= 0){Serial.println("Error on HTTP request"); http.end(); return "0,0";}
  //-----------------------------------------------------------------------------------
  //reading data comming from Google Sheet
  String payload = http.getString();
  Serial.println("Payload: "+payload);
  //-----------------------------------------------------------------------------------
  if(httpCode == 200){
    http.end();
    return payload;
  }
  else{
    http.end();
    return "0,0";
  }
  
}

void connectionClose(){
  WiFi.disconnect();
}

