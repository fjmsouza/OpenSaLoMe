#include "Connection.h"
#include "Credentials.h" // comment this line

// #include "YourCredentials.h"// uncomment this line

// Based on:
// https://iotdesignpro.com/articles/esp32-data-logging-to-google-sheets-with-google-scripts
// https://electropeak.com/learn/sending-data-from-esp32-or-esp8266-to-google-sheets-2-methods/
// https://how2electronics.com/how-to-send-esp32-cam-captured-image-to-google-drive/#google_vignette
// https://www.niraltek.com/blog/how-to-take-photos-and-upload-it-to-google-drive-using-esp32-cam/
// https://www.makerhero.com/blog/tire-fotos-com-esp32-cam-e-armazene-no-google-drive/

RTC_DATA_ATTR int connection_fail_counter = 0;

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
      connection_fail_counter++;

      Serial.printf("Connection failed %d times!", connection_fail_counter);
      return connection_status = false;
    }
  }
  Serial.println("Connected!");
  return connection_status = true;
}

String ConnectionHandler::urlencode(String str) // Função de codificação
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;

  for (int i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);
    if (c == ' ')
    {
      encodedString += '+';
    }
    else if (isalnum(c))
    {
      encodedString += c;
    }
    else
    {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9)
      {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9)
      {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  return encodedString;
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
  }
  esp_camera_fb_return(fb);
}
  // Serial.println("Connecting to " + String(host));
  // WiFiClientSecure client;
  // client.setInsecure();
  // if (client.connect(host, httpsPort))
  // { // Conectando no Google

  //   Serial.println("Connection succeeded!");

  //   char *input = (char *)fb->buf;
  //   char output[base64_enc_len(3)];
  //   String imageFile = "";

  //   for (int i = 0; i < fb->len; i++)
  //   {
  //     base64_encode(output, (input++), 3);
  //     if (i % 3 == 0)
  //       imageFile += urlencode(String(output));
  //   }

  //   String Data = file_name + mime_type + image2string;
  //   // esp_camera_fb_return(fb);
  //   Serial.println("Sending image to Google Drive.");
  //   String string_humidity = String(moisture, 1);
  //   String url = "https://script.google.com/macros/s/" + GOOGLE_DRIVE_SCRIPT_ID + "/exec?" + "moisture=" + string_humidity;
  //   Serial.print("requesting URL: ");
  //   Serial.println(url);

  //   client.println("POST " + url + " HTTP/1.1");
  //   client.println("Host: " + String(host));
  //   client.println("Content-Length: " + String(Data.length() + imageFile.length()));
  //   client.println("Content-Type: application/x-www-form-urlencoded");
  //   client.println();
  //   client.print(Data);

  //   int Index;

  //   for (Index = 0; Index < imageFile.length(); Index = Index + 1000)
  //   {
  //     client.print(imageFile.substring(Index, Index + 1000));
  //   }

  //   Serial.println("Waiting response.");

  //   begin = millis();
  //   while (!client.available())
  //   { // Aguarda resposta do envio da imagem
  //     Serial.print(".");
  //     // reset the esp after 10 seconds
  //     if ((millis() - begin) > timeout)
  //     {
  //       // ESP.restart();
  //       connection_fail_counter++;
  //       break;
  //     }
  //   }

  //   Serial.println();

  //   begin = millis();
  //   while (client.available())
  //   {                                    // Aguarda resposta do envio da imagem
  //     Serial.print(char(client.read())); // Mostra na tela a resposta
  //     // reset the esp after 10 seconds
  //     if ((millis() - begin) > timeout)
  //     {
  //       // ESP.restart();
  //       connection_fail_counter++;
  //       break;
  //     }
  //   }
  // }
  // else
  // {
  //   Serial.println("Connection to " + String(host) + " failed.");
  //   // esp_camera_fb_return(fb);
  // }
  // client.stop();
  // esp_camera_fb_return(fb);
// }

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
    return "0,0";
  }
}

void ConnectionHandler::close()
{
  WiFi.disconnect();
}
