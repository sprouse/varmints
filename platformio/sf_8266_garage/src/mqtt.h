WiFiClient espClient;
PubSubClient client(espClient);


////////////// MQTT Routines //////////////

const char* mqtt_server = "192.168.2.163";

long lastMsg = 0;
char msg[50];
int value = 0;

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void mqtt_publish_garage_status(const char *status){
  if (!client.connected()) {
    Serial.println("Connect to MQTT");
    reconnect();
  }
  client.loop();

  Serial.print("home/garage/door = ");
  Serial.println(status);
  client.publish("home/garage/door", status);
}

void mqtt_setup(){
  client.setServer(mqtt_server, 1883);
}

void mqtt_run(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

#if 0
  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
  }
#endif
}

