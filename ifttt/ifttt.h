#define IFTTT_SERVER "maker.ifttt.com"

char ifttt_buff[150];

boolean ifttt_trigger(char* event, char* key, char* ing1, char* ing2, char* ing3) {
  boolean isSuccessful = false;
  Serial.println("connecting...");
  if (esp8266_connectTCP(F(IFTTT_SERVER), 80)) {

    Serial.println("connected");
    String url = "/trigger/";
    url += event;
    url += "/with/key/";
    url += key;

    boolean params = false;
    if (ing1[0] != NULL) {
      url += "?value1=";
      url += ing1;
      params = true;
    }
    if (ing2[0] != NULL) {
      if (!params) {
        url += "?";
      }
      else {
        url += "&";
      }
      url += "value2=";
      url += ing2;
      params = true;
    }
    if (ing3[0] != NULL) {
      if (!params) {
        url += "?";
      }
      else {
        url += "&";
      }
      url += "value3=";
      url += ing3;
      params = true;
    }

    Serial.println(url);
    url.toCharArray(ifttt_buff, sizeof(ifttt_buff));
	
    if (esp8266_requestURL(ifttt_buff, "POST")) {
      esp8266_find();
	  isSuccessful = true;
    }
  }
	
  // this is not mendatory at all
  if (!isSuccessful) {
    Serial.println("connection failed");
  }

  esp8266_closeTCP();
  return isSuccessful;
}

boolean ifttt_trigger(char* event, char* key) {
  return ifttt_trigger(event, key, NULL, NULL, NULL);
}


