# Focuser
Uses:
1. Adafruit Feather ESP8266
2. Adafruit Feather Motorcontroller
3. Current sensor INA219

Uses current sensor to detect when focuser has hit it's limit.  Measure relative distance based on time.  Web interface provided via AP "Focuser" at 192.168.4.1.  Typical use, "init" first to set zero at focuser all the way out.
