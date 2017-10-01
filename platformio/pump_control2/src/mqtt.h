PubSubClient ps_client(client);

////////////// MQTT Routines //////////////

const char* mqtt_server = "192.168.2.163";

long lastMsg = 0;
char msg[50];
int value = 0;

unsigned long last_attempt = 0;
bool reconnect() {
  // Loop until we're reconnected
  if ((millis() - last_attempt) < 30000) {
      return false;
  }
//  while (!ps_client.connected()) {

    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (ps_client.connect("ESP8266_Pump")) {
      Serial.println("connected");
      send_last_status_to_mqtt();
      // ... and resubscribe
      ps_client.subscribe(MQTT_PUMP_CMD);
    } else {
      Serial.print("failed, rc=");
      Serial.print(ps_client.state());
      Serial.println(" try again in 30 seconds");
      // Wait 5 seconds before retrying
      last_attempt = millis();
      return false;
      // delay(5000);
    }
 // }
    return true;
}

void mqtt_publish_topic(const char *topic, const char *status){
  if (!ps_client.connected()) {
    Serial.println("Connect to MQTT");
    if (!reconnect()){
        return;
    }
  }
  ps_client.loop();

  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(status);
  ps_client.publish(topic, status, true);
}

void mqtt_setup(){
  ps_client.setServer(mqtt_server, 1883);
  ps_client.setCallback(mqtt_sub_callback);
}

void mqtt_run(){
  if (!ps_client.connected()) {
    if (!reconnect()) {
      return;
    }
  }
  ps_client.loop();
}

