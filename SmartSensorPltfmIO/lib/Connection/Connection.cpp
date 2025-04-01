#include "Connection.h"

// Based on:
// https://iotdesignpro.com/articles/esp32-data-logging-to-google-sheets-with-google-scripts
// https://electropeak.com/learn/sending-data-from-esp32-or-esp8266-to-google-sheets-2-methods/
// https://how2electronics.com/how-to-send-esp32-cam-captured-image-to-google-drive/#google_vignette
// https://www.niraltek.com/blog/how-to-take-photos-and-upload-it-to-google-drive-using-esp32-cam/
// https://www.makerhero.com/blog/tire-fotos-com-esp32-cam-e-armazene-no-google-drive/

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
      fail_counter++;
      return connection_status = false;
    }
  }
  Serial.println("Connected!");
  return connection_status = true;
}

void ConnectionHandler::sendData(float moisture, bool valve_state)
{
  Serial.println("Connecting to " + String(host));
  String string_humidity = String(moisture, 1);
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
  String url = "https://script.google.com/macros/s/" + GOOGLE_SHEETS_SCRIPT_ID + "/exec?" + "moisture=" + string_humidity + "&valvestate=" + string_valve_state;
  Serial.print("requesting URL: ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    Serial.println("Data sent successfully!");
  }
  http.end();
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
  if (fb->len < 64 || fb->buf[0] != 0xFF || fb->buf[1] != 0xD8)
  {
    Serial.println("Error: Invalid or corrupt JPEG!");
    esp_camera_fb_return(fb);
    return;
  }

  while (retryCount < maxRetries && !success)
  {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(60);

    HTTPClient http;
    http.setReuse(true);
    http.setTimeout(60000);

    if (http.begin(client, url))
    {
      http.addHeader("Content-Type", "image/jpeg");

      // Envia a imagem
      int httpCode = http.POST(fb->buf, fb->len);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_FOUND)
      {
        Serial.println("Image sent successfully!");
        success = true;
      }
      else
      {
        Serial.printf("Attempt %d: HTTP %d - %s\n", retryCount + 1, httpCode, http.errorToString(httpCode).c_str());
        retryCount++;
        delay(10000 * retryCount);
      }
      http.end();
    }
    else
    {
      Serial.println("Failed to start HTTP connection");
      retryCount++;
    }
    client.stop();
    delay(1000);
  }

  if (!success)
  {
    Serial.println("Falha crítica: não foi possível enviar após tentativas");
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
    fail_counter++;
    return "no water";
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
    fail_counter++;
    return "no water";
  }
}

void ConnectionHandler::close()
{
  WiFi.disconnect();
}
