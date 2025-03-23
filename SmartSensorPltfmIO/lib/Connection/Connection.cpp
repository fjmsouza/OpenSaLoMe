#include "Connection.h"
#include "Credentials.h" // comment this line

// #include "YourCredentials.h"// uncomment this line

// Based on:
// https://iotdesignpro.com/articles/esp32-data-logging-to-google-sheets-with-google-scripts
// https://electropeak.com/learn/sending-data-from-esp32-or-esp8266-to-google-sheets-2-methods/
// https://how2electronics.com/how-to-send-esp32-cam-captured-image-to-google-drive/#google_vignette
// https://www.niraltek.com/blog/how-to-take-photos-and-upload-it-to-google-drive-using-esp32-cam/
// https://www.makerhero.com/blog/tire-fotos-com-esp32-cam-e-armazene-no-google-drive/

RTC_DATA_ATTR int fail_counter = 0;
ConnectionHandler Connection;

bool ConnectionHandler::setup()
{
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Waiting connection to WiFi..");
  begin = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    // reset the esp after 10 seconds
    if ((millis() - begin) > timeout)
    {
      // ESP.restart();
      fail_counter++;
      return connection_status = false;
    }
  }
  Serial.println("Connected!");
  return connection_status = true;
}

void ConnectionHandler::sendData(float moisture1, bool valve_state, float moisture2)
{
  Serial.println("Connecting to " + String(host));
  WiFiClientSecure client;
  client.setInsecure();
  if (client.connect(host, httpsPort))
  {

    Serial.println("Connection succeeded!");
    String string_humidity1 = String(moisture1, 1);
    String string_humidity2 = String(moisture2, 1);
    String string_valve_state = "";
    if (valve_state)
    {
      string_valve_state = "100";
    }
    else
    {
      string_valve_state = "0";
    }

    Serial.println("Sending data to Google Sheets.");
    String url = "https://script.google.com/macros/s/" + GOOGLE_SHEETS_SCRIPT_ID + "/exec?" + "moisture1=" + string_humidity1 + "&valvestate=" + string_valve_state + +"&moisture2=" + string_humidity2;
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

void ConnectionHandler::sendImage(float moisture, camera_fb_t *fb)
{

  // URL com parâmetro 'moisture' na query
  Serial.println("Sending image to Google Drive.");
  String string_humidity = String(moisture, 1);
  String url = "https://script.google.com/macros/s/" + GOOGLE_DRIVE_SCRIPT_ID + "/exec?" + "moisture=" + string_humidity;

  const int maxRetries = 5;
  int retryCount = 0;
  bool success = false;

  // Verificação básica do JPEG
  if (fb->len < 64 || fb->buf[0] != 0xFF || fb->buf[1] != 0xD8) {
    Serial.println("Erro: JPEG inválido ou corrompido!");
    esp_camera_fb_return(fb);
    return;
  }

  while (retryCount < maxRetries && !success) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(60);

    HTTPClient http;
    http.setReuse(true);
    http.setTimeout(60000);

    if (http.begin(client, url)) {
      http.addHeader("Content-Type", "image/jpeg");
      
      // Envia a imagem
      int httpCode = http.POST(fb->buf, fb->len);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_FOUND) {
        Serial.println("Imagem enviada com sucesso!");
        success = true;
      } else {
        Serial.printf("Tentativa %d: HTTP %d - %s\n", retryCount + 1, httpCode, http.errorToString(httpCode).c_str());
        retryCount++;
        delay(10000 * retryCount);
      }
      http.end();
    } else {
      Serial.println("Falha ao iniciar conexão HTTP");
      retryCount++;
    }
    client.stop();
    delay(1000);
  }

  if (!success) {
    Serial.println("Falha crítica: não foi possível enviar após tentativas");
    // ESP.restart();
    fail_counter++;
  }
  esp_camera_fb_return(fb);
}

String ConnectionHandler::receiveData()
{

  HTTPClient http;

  Serial.println("Read data from Google Sheets.");
  String url = "https://script.google.com/macros/s/" + GOOGLE_SHEETS_SCRIPT_ID + "/exec?read";
  Serial.print("requesting URL: ");
  Serial.println(url);

  http.begin(url.c_str());
  //-----------------------------------------------------------------------------------
  // Removes the error "302 Moved Temporarily Error"
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  //-----------------------------------------------------------------------------------
  // Get the returning HTTP status code
  int httpCode = http.GET();
  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);
  //-----------------------------------------------------------------------------------
  if (httpCode <= 0)
  {
    Serial.println("Error on HTTP request");
    http.end();
    return "0,0";
  }
  //-----------------------------------------------------------------------------------
  // reading data comming from Google Sheet
  String payload = http.getString();
  Serial.println("Payload: " + payload);
  //-----------------------------------------------------------------------------------
  if (httpCode == 200)
  {
    http.end();
    return payload;
  }
  else
  {
    http.end();
    // ESP.restart();
    fail_counter++;
    return "00,00";
  }
}

void ConnectionHandler::close()
{
  WiFi.disconnect();
}
