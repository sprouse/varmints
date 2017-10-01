PubSubClient ps_client(client);

////////////// MQTT Routines //////////////

const char* mqtt_server = "192.168.2.163";

long lastMsg = 0;
char msg[50];
int value = 0;

void reconnect() {
  // Loop until we're reconnected
  while (!ps_client.connected()) {
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
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void mqtt_publish_topic(const char *topic, const char *status){
  if (!ps_client.connected()) {
    Serial.println("Connect to MQTT");
    reconnect();
  }
  ps_client.loop();

  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(status);
  ps_client.publish(topic, status);
}

void mqtt_setup(){
  ps_client.setServer(mqtt_server, 1883);
  ps_client.setCallback(mqtt_sub_callback);
}

void mqtt_run(){
  if (!ps_client.connected()) {
    reconnect();
  }
  ps_client.loop();
}

