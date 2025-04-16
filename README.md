# Position-Feedback-System-for-Pressalit-Kitchen-Controller
4th year project of Xinpu Li

This project is for providing positioning function for Pressalit Kitchen Controller in Heriot-Watt University's Assistive Living Laboratory.

.yaml file and .h file is for configuration of microcontroller.
These two files should be transferred to the microcontroller via the ESPHome in Home Assistant.

It makes microcontroller read the digitial caliper through GPIO interfaces, and keep caliper running without shut down while no-operated for long time.
In the meantime, it will send the data it catched to MQTT Broker through topic esphome/caliper/values.
