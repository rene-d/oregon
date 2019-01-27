// oregon.ino
// rene-d 01/2019

#include <Oregon.h>

// Define pin where is 433Mhz receiver (here, pin 2)
#define MHZ_RECEIVER_PIN 2

struct datetime
{
    byte year, month, day;
    byte hour, minute, second;
    unsigned long now;
};

datetime current_time;

void decode_AEA(const byte *osdata, size_t len);
void decode_ACC(const byte *osdata, size_t len);

void setup()
{
    Serial.begin(115200);
    Serial.println("Setup started");

    memset(&current_time, 0, sizeof(current_time));
    current_time.day = 1;
    current_time.month = 1;
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

    unsigned long now = millis();
    unsigned long o = now - current_time.now;
    if (o >= 1000)
    {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

        current_time.now = now;

        while (o >= 1000)
        {
            o -= 1000;

            current_time.second++;
            if (current_time.second >= 60)
            {
                current_time.second = 0;
                current_time.minute++;
                if (current_time.minute >= 60)
                {
                    current_time.minute = 0;
                    current_time.hour++;
                    if (current_time.hour >= 24)
                    {
                        current_time.hour = 0;
                        current_time.day++;
                        if (current_time.day > days_in_month(current_time.month, current_time.year))
                        {
                            current_time.day = 1;
                            current_time.month++;
                            if (current_time.month > 12)
                            {
                                current_time.month = 1;
                                current_time.year++;
                            }
                        }
                    }
                }
            }
        }
    }

    if (Serial.available() > 0)
    {
        // read the incoming byte:
        byte incomingByte = Serial.read();

        // say what you got:
        //Serial.print("I received: ");
        //Serial.println(incomingByte, DEC);

        if (incomingByte == 't')
        {
            print_hexa(&incomingByte, 0);
        }
    }

    if (p != 0)
    {
        if (orscV2.nextPulse(p))
        {
            Serial.println();

            // Decode Hex Data once
            byte length;
            const byte *osdata = DataToDecoder(orscV2, length);

            print_hexa(osdata, length);

            uint16_t msg_id = ((osdata[0] & 0xf) << 8) + osdata[1];

            if (msg_id == 0xACC || msg_id == 0xA2D)
                decode_ACC(osdata, length);
            else if (msg_id == 0xAEA || msg_id == 0xAEC)
                decode_AEA(osdata, length);
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

void print_hexa(const byte *data, byte length)
{
    char buf[32];

    snprintf(buf, 32, "[%04u/%02u/%02u %02u:%02u:%02u.%03lu]",
             2000 + current_time.year, current_time.month, current_time.day,
             current_time.hour, current_time.minute, current_time.second,
             millis() - current_time.now);
    Serial.println(buf);

    for (byte i = 0; i < length; ++i)
    {
        Serial.print(data[i] >> 4, HEX);
        Serial.print(data[i] & 0x0F, HEX);
    }
    Serial.println();
}

// calculate a packet checksum by performing a
bool checksum(const byte *osdata, byte type, int count, byte check)
{
    byte calc = 0;

    if (type == 1) // type 1, add all nibbles, deduct 10
    {
        for (byte i = 0; i < count; i++)
        {
            calc += (osdata[i] & 0xF0) >> 4;
            calc += (osdata[i] & 0xF);
        }
        calc = calc - 10;
    }
    else if (type == 2) // type 2, add all nibbles up to count, add the 13th nibble, deduct 10
    {
        for (byte i = 0; i < count; i++)
        {
            calc += (osdata[i] & 0xF0) >> 4;
            calc += (osdata[i] & 0xF);
        }
        calc += (osdata[6] & 0xF);
        calc = calc - 10;
    }
    else if (type == 3) // type 3, add all nibbles up to count, subtract 10 only use the low 4 bits for the compare
    {
        for (byte i = 0; i < count; i++)
        {
            calc += (osdata[i] & 0xF0) >> 4;
            calc += (osdata[i] & 0xF);
        }
        calc = calc - 10;
        calc = (calc & 0x0f);
    }
    else if (type == 4)
    {
        for (byte i = 0; i < count; i++)
        {
            calc += (osdata[i] & 0xF0) >> 4;
            calc += (osdata[i] & 0xF);
        }
        calc = calc - 10;
    }
    return (check == calc);
}

byte nibble(const byte *osdata, int n)
{
    if (n & 1)
    {
        return osdata[n / 2] >> 4;
    }
    else
    {
        return osdata[n / 2] & 0xf;
    }
}

void decode_AEA(const byte *osdata, size_t len)
{
    //bool ok = checksum(osdata, 1, 11, osdata[11]);

    int channel = (osdata[2] >> 4);

    int year = (osdata[9] >> 4) + 10 * (osdata[10] & 0xf);
    int month = (osdata[8] >> 4);
    int day = (osdata[7] >> 4) + 10 * (osdata[8] & 0xf);
    int hour = (osdata[6] >> 4) + 10 * (osdata[7] & 0xf);
    int minute = (osdata[5] >> 4) + 10 * (osdata[6] & 0xf);
    int second = (osdata[4] >> 4) + 10 * (osdata[5] & 0xf);

    if (nibble(osdata, 8) == 6)
    {
        current_time.now = millis();
        current_time.year = year;
        current_time.month = month;
        current_time.day = day;
        current_time.hour = hour;
        current_time.minute = minute;
        current_time.second = second;
    }

    Serial.print(" id: ");
    Serial.println(osdata[3]);
    Serial.print(" channel: ");
    Serial.println(channel);

    char buf[100];
    sprintf(buf, " date: %02d/%02d/20%02d %02d:%02d:%02d", day, month, year, hour, minute, second);
    Serial.println(buf);
}

void decode_ACC(const byte *osdata, size_t len)
{
    int temp, hum;

    //bool ok = checksum(osdata, 1, 8, osdata[8]);

    int channel = (osdata[2] >> 4);

    temp = ((osdata[5] >> 4) * 100) + ((osdata[5] & 0x0F) * 10) + ((osdata[4] >> 4));
    if ((osdata[6] & 0x0F) >= 8)
        temp = -temp;

    hum = ((osdata[7] & 0x0F) * 10) + (osdata[6] >> 4);

    Serial.print(" id: ");
    Serial.println(osdata[3]);
    Serial.print(" channel: ");
    Serial.println(channel);

    Serial.print(" temperature: ");
    Serial.println(temp / 10.);
    Serial.print(" humidity: ");
    Serial.println(hum);

    Serial.print(" bat: ");
    Serial.print(osdata[4] & 0x0F);
    if ((osdata[4] & 0x0F) >= 4)
    {
        Serial.println(" low");
    }
    else
    {
        Serial.println(" ok");
    }
}
