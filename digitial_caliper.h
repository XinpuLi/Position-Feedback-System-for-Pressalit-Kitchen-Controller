#include "esphome.h"
#include "esphome/components/mqtt/mqtt_client.h"

#define DATA_PIN 4              // Digital caliper's DATA signal pin
#define CLOCK_PIN 5             // Digital caliper's CLOCK signal pin
#define POWER_TRIGGER_PIN 27    // GPIO for sending wake-up pulses to caliper

/**
 * This sensor reads 24-bit serial data from a digital caliper, processes
 * the measurements, and publishes them via MQTT. It also handles periodic
 * wake-up pulses to maintain communication with battery-powered calipers.
 */
 
class MyCustomSensor : public Component, public Sensor {
public:
    MyCustomSensor() = default;

    void setup() override {
        // Configure pin modes
        pinMode(DATA_PIN, INPUT);
        pinMode(CLOCK_PIN, INPUT);
        pinMode(POWER_TRIGGER_PIN, OUTPUT);
        
        // Ensure initial state is LOW to prevent accidental activation
        digitalWrite(POWER_TRIGGER_PIN, LOW);
    }

   void loop() override {
        static uint32_t last_read_time = 0;
        static uint32_t last_trigger_time = 0;
        uint32_t current_time = millis();

        /* Data Reading Interval (1 second) */
        if (current_time - last_read_time >= 1000) {
            last_read_time = current_time;
            
            // Read and process raw 24-bit data
            uint32_t raw_data = read_data();
            float result = process_data(raw_data);
            
            // Update sensor state and trigger MQTT publish
            publish_state(result); 
        }

        // Wake up pulses are sent every 1 minute
        if (current_time - last_trigger_time >= 100000) { // 100000ms = 1 minute
            last_trigger_time = current_time;
            
            ESP_LOGI("CALIPER", "Sending wake-up pulse to GPIO27");
            digitalWrite(POWER_TRIGGER_PIN, HIGH);  // Pull up level
            delay(200);                             // Pulse duration is adjusted to 200ms
            digitalWrite(POWER_TRIGGER_PIN, LOW);   // Restore low level
        }
    }
        
    

private:
    // Read 24-bit data
    uint32_t read_data() {
        uint32_t data = 0;

        for (int i = 0; i < 24; i++) {
            // wait CLK go up
            while (digitalRead(CLOCK_PIN) == LOW);
            while (digitalRead(CLOCK_PIN) == HIGH);

            // read DATA
            if (digitalRead(DATA_PIN) == LOW) {
                data |= (1UL << i);  // set corresponding bit to 1
            }
        }

        return data;
    }

    // convert data to binary string
    std::string format_binary(uint32_t value, int bits) {
        std::string binary_str;
        for (int i = bits - 1; i >= 0; i--) {
            binary_str += (value & (1UL << i)) ? '1' : '0';
        }
        return binary_str;
    }

    // Process the read binary data and convert it to the actual value
    float process_data(uint32_t raw_data) {
        // Extract values (first 20)
        int32_t value = raw_data & 0xFFFFF;
        // Extract symbol bit (21st bit, counting from 0)
        bool is_negative = (raw_data >> 20) & 0x01;
        // Extract the unit bit (24th bit, corresponding to bit 23)
        bool is_inch = (raw_data >> 23) & 0x01;

        // process symbol
        if (is_negative) {
            value = -value;
        }

        // Convert values by unit (eventually unified to millimeters)
        if (is_inch) {
            // Inch mode: 1 inch = 2000 counts, Convert to mm (1 inch = 25.4 mm)
            float value_inch = value / 2000.0f;
            float value_mm = value_inch * 25.4f;
            ESP_LOGD("CALIPER", "Original value (inches): %.3f inch -> after converted: %.3f mm", value_inch, value_mm);
            return value_mm;
        } else {
            // Millimeter mode: 1 mm = 100 counts
            float value_mm = value / 100.0f;
            ESP_LOGD("CALIPER", "Original value (mm): %.3f mm", value_mm);
            return value_mm;
        }
    }

};
