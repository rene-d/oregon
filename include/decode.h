// decode.h
// rene-d 01/2019

// internal clock, set from OS sensor
struct
{
    byte year, month, day;
    byte hour, minute, second;
    unsigned long now;
} sensor_clock;

//
// get a nibble (a half byte)
//
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

//
// calculate the packet checksum (sum of all nibbles)
//
bool checksum(const byte *osdata, byte last, byte check)
{
    byte calc = 0;

    for (byte i = 1; i <= last; i++)
    {
        calc += nibble(osdata, i);
    }

    return (check == calc);
}

//
// print a buffer in nibble order, with nibbles packed by field
//
void print_nibbles(const byte *osdata, size_t len, const char *def)
{
    static const char digits[] = "0123456789ABCDEF";
    char hexa[128];
    size_t i = 0;
    size_t j = 0;
    size_t k = 0;
    char c, n;

    n = def[0];
    if (n)
        n -= '0';
    c = n;

    while (i < len * 2 && j < sizeof(hexa) - 3)
    {
        hexa[j++] = digits[nibble(osdata, i++)];
        if (c > 0)
        {
            c--;
            if (c == 0)
            {
                // reverse last def[k] chars
                if (n > 1)
                {
                    for (char z = 0; z < n / 2; ++z)
                    {
                        c = hexa[j - 1 - z];
                        hexa[j - 1 - z] = hexa[j - n + z];
                        hexa[j - n + z] = c;
                    }
                }
                hexa[j++] = ' ';
                n = def[++k];
                if (n)
                    n -= '0';
                c = n;
            }
        }
    }
    hexa[j] = 0;

    Serial.print("data: ");
    Serial.println(hexa);
}

//
// message 3EA8 or 3EC8 : clock
//
void decode_date_time(const byte *osdata, size_t len)
{
    if (len < 12)
        return;

    byte crc = osdata[11];
    bool ok = checksum(osdata, 21, crc);

    byte channel = (osdata[2] >> 4);
    byte rolling_code = osdata[3];

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

    if (ok && (nibble(osdata, 8) & 2) != 0)
    {
        // update the sensor clock
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
    snprintf(buf, sizeof(buf), " date: 20%02d/%02d/%02d", year, month, day);
    Serial.println(buf);
    snprintf(buf, sizeof(buf), " time: %02d:%02d:%02d", hour, minute, second);
    Serial.println(buf);
#else
    print_nibbles(osdata, len, "14121222211212");

    char buf[80];
    snprintf(buf, sizeof(buf), "channel=%d crc=$%02X %s id=%d channel=%d state=%d clock=20%02d/%02d/%02d %02d:%02d:%02d",
             channel, crc, ok ? "OK" : "KO", rolling_code,
             channel, nibble(osdata, 8),
             year, month, day, hour, minute, second);
    Serial.println(buf);

    static const char *label[] = {
        "alwaysA",        // 0    b0    A=1010 - not part of the message
        "id_msg",         // 1    b0
        "id_msg",         // 2    b1
        "id_msg",         // 3    b1
        "id_msg",         // 4    b2
        "channel",        // 5    b2
        "rolling code",   // 6    b3
        "rolling code",   // 7    b3
        "clock state",    // 8    b4    0,4,8: date is invalid, 2 or 6: date is valid
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
// message 3CCx or 02D1: temperature humidity
//
void decode_temp_hygro(const byte *osdata, size_t len)
{
    if (len < 9)
        return;

    byte crc = osdata[8];
    bool ok = checksum(osdata, 15, crc); // checksum = nibbles 1-15, result is nibbles 17..16

    byte channel = (osdata[2] >> 4);
    byte rolling_code = osdata[3];

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
    if ((bat & 4) != 0)
    {
        Serial.println(" low");
    }
    else
    {
        Serial.println(" ok");
    }
#else
    print_nibbles(osdata, len, "141214212");
    char buf[80];
    snprintf(buf, sizeof(buf), "channel=%d crc=$%02X %s id=%d temp=%.1lf°C hum=%d%% bat=%d",
             channel, crc, ok ? "OK" : "KO", rolling_code,
             temp / 10., hum, bat);
    Serial.println(buf);

    static const char *label[] = {
        "alwaysA",              // 0    b0 lsb  always A=1010
        "id_msg",               // 1    b0 msb
        "id_msg",               // 2    b1 lsb
        "id_msg",               // 3    b1 msb
        "id_msg",               // 4    b2 lsb
        "channel",              // 5    b2 msb
        "rolling code",         // 6    b3 lsb
        "rolling code",         // 7    b3 msb
        "battery",              // 8    b4 lsb  bit 3=1 => low?
        "temperature (tenths)", // 9    b4 msb
        "temperature (units)",  // 10   b5 lsb
        "temperature (tens)",   // 11   b5 msb
        "temperature (sign)",   // 12   b6 lsb
        "humidity (units)",     // 13   b6 msb
        "humidity (tens)",      // 14   b7 lsb
        "comfort",              // 15   b7 msb  comfort ??? (according to RFLink) 0: normal, 4: comfortable, 8: dry, C: wet
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

void oregon_decode(const byte *osdata, size_t len)
{
    // we need 2 bytes at least
    //  - the preamble A
    //  - the ID on four nibbles
    if (len < 3)
        return;

    uint16_t id = (((uint16_t)nibble(osdata, 4)) << 12) +
                  (((uint16_t)nibble(osdata, 3)) << 8) +
                  (((uint16_t)nibble(osdata, 2)) << 4) +
                  (((uint16_t)nibble(osdata, 1)));

#ifndef ARDUINO
    char buf[32];
    snprintf(buf, 32, "message: %04X len=%zu", id, len);
    Serial.println(buf);
#endif

    if ((id & 0xFFF0) == 0x3CC0 || id == 0x02D1)
    {
        decode_temp_hygro(osdata, len);
    }
    else if (id == 0x3EA8 || id == 0x3EC8)
    {
        decode_date_time(osdata, len);
    }
    else
    {
#ifndef ARDUINO
        print_nibbles(osdata, len, "1412");
        Serial.println("UNKNOWN");
#endif
    }
}
