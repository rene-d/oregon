// oregon.ino
// rene-d 01/2019

#include "Oregon.h"
#include "decode.h"

// Define pin where is 433Mhz receiver (here, pin 2)
#define MHZ_RECEIVER_PIN 2

void setup()
{
    Serial.begin(115200);
    Serial.println("Setup started");

    memset(&sensor_clock, 0, sizeof(sensor_clock));
    sensor_clock.day = 1;
    sensor_clock.month = 1;
    pinMode(LED_BUILTIN, OUTPUT);

    // Setup received data
    attachInterrupt(digitalPinToInterrupt(MHZ_RECEIVER_PIN), ext_int_1, CHANGE);

    Serial.println("Setup completed");
}

void loop()
{
    //------------------------------------------
    // Start process new data from Oregon sensors
    //------------------------------------------
    noInterrupts(); // Disable interrupts
    word p = pulse;
    pulse = 0;
    interrupts(); // Enable interrupts

    if (p != 0)
    {
        if (orscV2.nextPulse(p))
        {

            // Decode Hex Data once
            byte length;
            const byte *osdata = DataToDecoder(orscV2, length);

            print_hexa(osdata, length);

            oregon_decode(osdata, length);
        }
    }

    update_clock();

    if (Serial.available() > 0)
    {
        // read the incoming byte:
        byte incomingByte = Serial.read();

        if (incomingByte == 't')
        {
            // print the current time
            print_hexa(&incomingByte, 0);
        }
    }
}

int days_in_month(byte month, byte year)
{
    static byte days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2)
    {
        // we will never reach 2100
        if (year % 4 == 0)
            return 29;
    }
    return days[month];
}

void update_clock()
{
    unsigned long now = millis();
    unsigned long o = now - sensor_clock.now;
    if (o >= 1000)
    {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

        sensor_clock.now = now;

        while (o >= 1000)
        {
            // advance the clock by one second

            o -= 1000;

            // dirty code done dirt cheap...
            sensor_clock.second++;
            if (sensor_clock.second >= 60)
            {
                sensor_clock.second = 0;
                sensor_clock.minute++;
                if (sensor_clock.minute >= 60)
                {
                    sensor_clock.minute = 0;
                    sensor_clock.hour++;
                    if (sensor_clock.hour >= 24)
                    {
                        sensor_clock.hour = 0;
                        sensor_clock.day++;
                        if (sensor_clock.day > days_in_month(sensor_clock.month, sensor_clock.year))
                        {
                            sensor_clock.day = 1;
                            sensor_clock.month++;
                            if (sensor_clock.month > 12)
                            {
                                sensor_clock.month = 1;
                                sensor_clock.year++;
                            }
                        }
                    }
                }
            }
        }
    }
}

void print_hexa(const byte *data, byte length)
{
    Serial.println();

    // print the sensor/board clock
    char buf[32];
    snprintf(buf, 32, "[%04u/%02u/%02u %02u:%02u:%02u.%03lu]",
             2000 + sensor_clock.year, sensor_clock.month, sensor_clock.day,
             sensor_clock.hour, sensor_clock.minute, sensor_clock.second,
             millis() - sensor_clock.now);
    Serial.println(buf);

    // print data in byte order, that's NOT the nibble order
    for (byte i = 0; i < length; ++i)
    {
        Serial.print(data[i] >> 4, HEX);
        Serial.print(data[i] & 0x0F, HEX);
    }
    Serial.println();
}
