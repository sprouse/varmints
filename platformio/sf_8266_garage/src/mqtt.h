WiFiClient espClient;
PubSubClient client(espClient);


////////////// MQTT Routines //////////////

const char* mqtt_server = "192.168.2.163";

long lastMsg = 0;
char msg[50];
int value = 0;

#define MQTT_RETRY_INTERVAL 60
unsigned long last_attempt = 0;
bool reconnect() {

  if ((millis() - last_attempt) < MQTT_RETRY_INTERVAL * 1000L){
    return false;
  }
  // Loop until we're reconnected
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" try again in ");
      Serial.print(MQTT_RETRY_INTERVAL);
      Serial.println(" seconds." );
      last_attempt = millis();
      return false;
    }
  return true;
}

void mqtt_publish_topic(const char *topic, const char *status){
  if (!client.connected()) {
    Serial.println("Connect to MQTT");
    if (!reconnect()) {
        return;
    }
  }
  client.loop();

  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(status);
  client.publish(topic, status, true);
}

void mqtt_setup(){
  client.setServer(mqtt_server, 1883);
}

void mqtt_run(){
  if (!client.connected()) {
    if (!reconnect()) {
        return;
    }
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

