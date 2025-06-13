#include <SPI.h>
#include <RF24.h>

#define CE_PIN  9
#define CSN_PIN 10

RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "00001";  // Must match on receiver side

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);           // Power level: LOW, HIGH, MAX, MIN
  radio.setDataRate(RF24_1MBPS);           // Optional: set data rate
  radio.setChannel(108);                   // Choose a clear channel
  radio.openWritingPipe(address);
  radio.stopListening();                   // Set module as transmitter
}

void loop() {
  const char text[] = "Beacon Signal";     // Can be any data
  bool sent = radio.write(&text, sizeof(text));

  if (sent) {
    Serial.println("Message sent: Beacon Signal");
  } else {
    Serial.println("Failed to send");
  }

  delay(1000); // Send every second
}
