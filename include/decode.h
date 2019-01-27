// decode.h
//
#ifndef decode_h
#define decode_h

// internal clock, set from OS sensor
struct
{
    byte year, month, day;
    byte hour, minute, second;
    unsigned long now;
} sensor_clock;

// calculate a packet checksum by performing a
bool checksum(const byte *osdata, byte type, byte count, byte check)
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

byte nibble(const byte *osdata, byte n)
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

// message AEA or AEC
void decode_AEA(const byte *osdata, size_t len)
{
    if (len < 12)
        return;

    byte crc = osdata[11];
    bool ok = checksum(osdata, 1, 11, crc);

    uint8_t channel = (osdata[2] >> 4);
    uint16_t rolling_code = osdata[3];

    int year = (osdata[9] >> 4) + 10 * (osdata[10] & 0xf);
    int month = (osdata[8] >> 4);
    int day = (osdata[7] >> 4) + 10 * (osdata[8] & 0xf);
    int hour = (osdata[6] >> 4) + 10 * (osdata[7] & 0xf);
    int minute = (osdata[5] >> 4) + 10 * (osdata[6] & 0xf);
    int second = (osdata[4] >> 4) + 10 * (osdata[5] & 0xf);

    channel = nibble(osdata, 5);

#ifdef ARDUINO
    if (!ok)
        Serial.println(" bad crc");

    if (ok && nibble(osdata, 8) == 6)
    {
        // update our internal clock
        sensor_clock.now = millis();
        sensor_clock.year = year;
        sensor_clock.month = month;
        sensor_clock.day = day;
        sensor_clock.hour = hour;
        sensor_clock.minute = minute;
        sensor_clock.second = second;
    }

    Serial.print(" id: ");
    Serial.println(rolling_code);
    Serial.print(" channel: ");
    Serial.println(channel);

    char buf[100];
    snprintf(buf, sizeof(buf), " date: %02d/%02d/20%02d", day, month, year);
    Serial.println(buf);
    snprintf(buf, sizeof(buf), " time: %02d:%02d:%02d", hour, minute, second);
    Serial.println(buf);
#else
    char buf[80];
    snprintf(buf, sizeof(buf), "channel=%d crc=%02X %s id=%02X date=%02d/%02d/20%02d %02d:%02d:%02d",
             channel, crc, ok ? "OK" : "KO", rolling_code,
             day, month, year, hour, minute, second);
    Serial.println(buf);

    static const char *label[] = {
        "id_msg",         // 0    b0
        "?",              // 1    b0
        "id_msg",         // 2    b1
        "id_msg",         // 3    b1
        "?",              // 4    b2
        "channel",        // 5    b2
        "rolling code",   // 6    b3
        "rolling code",   // 7    b3
        "?",              // 8    b4        0,4,8: date invalide, 6: date valide
        "second (units)", // 9    b4
        "second (tens)",  // 10   b5
        "minute (unit)",  // 11   b5
        "minute (tens)",  // 12   b6
        "hour (units)",   // 13   b6
        "hour (tens)",    // 14   b7
        "day (units)",    // 15   b7
        "day (tens)",     // 16   b8
        "month",          // 17   b8
        "day of week",    // 18   b9
        "year (units)",   // 19   b9
        "year (tens)",    // 20   b10
        "?",              // 21   b10
        "crc",            // 22   b11
        "crc",            // 23   b11
    };

    for (int i = 0; i < 24; ++i)
    {
        snprintf(buf, sizeof(buf), "  nibble %2d : %X  %s", i, nibble(osdata, i), label[i]);
        Serial.println(buf);
    }
#endif
}

//
void decode_ACC(const byte *osdata, size_t len)
{
    if (len < 9)
        return;

    uint8_t crc = osdata[8];
    bool ok = checksum(osdata, 1, 8, crc); // checksum = all nibbles 0-15 results is nibbles 16.17

    uint8_t channel = (osdata[2] >> 4);
    uint16_t rolling_code = osdata[3];

    int temp = ((osdata[5] >> 4) * 100) + ((osdata[5] & 0x0F) * 10) + ((osdata[4] >> 4));
    if (osdata[6] & 0x08)
        temp = -temp;

    byte hum = ((osdata[7] & 0x0F) * 10) + (osdata[6] >> 4);

    byte bat = osdata[4] & 0x0F;

#ifdef ARDUINO
    if (!ok)
        Serial.println(" bad crc");
    Serial.print(" id: ");
    Serial.println(rolling_code);
    Serial.print(" channel: ");
    Serial.println(channel);

    Serial.print(" temperature: ");
    Serial.println(temp / 10.);
    Serial.print(" humidity: ");
    Serial.println(hum);

    Serial.print(" bat: ");
    Serial.print(bat);
    if (bat >= 4)
    {
        Serial.println(" low");
    }
    else
    {
        Serial.println(" ok");
    }
#else
    char buf[80];
    snprintf(buf, sizeof(buf), "channel=%d crc=%02X %s id=%02X temp=%.1lf°C hum=%d%% bat=%d",
             channel, crc, ok ? "OK" : "KO", rolling_code,
             temp / 10., hum, bat);
    Serial.println(buf);

    static const char *label[] = {
        "id_msg",               // 0    b0 lsb
        "?",                    // 1    b0 msb
        "id_msg",               // 2    b1 lsb
        "id_msg",               // 3    b1 msb
        "?",                    // 4    b2 lsb
        "channel",              // 5    b2 msb
        "rolling code",         // 6    b3 lsb
        "rolling code",         // 7    b3 msb
        "battery",              // 8    b4 lsb
        "temperature (tenths)", // 9    b4 msb
        "temperature (units)",  // 10   b5 lsb
        "temperature (tens)",   // 11   b5 msb
        "temperature (sign)",   // 12   b6 lsb
        "humidity (units)",     // 13   b6 msb
        "humidity (tens)",      // 14   b7 lsb
        "?",                    // 15   b7 msb
        "crc",                  // 16   b8 lsb
        "crc",                  // 17   b8 msb
    };

    for (int i = 0; i < 18; ++i)
    {
        snprintf(buf, sizeof(buf), "  nibble %2d : %X  %s", i, nibble(osdata, i), label[i]);
        Serial.println(buf);
    }
#endif
}

#endif
