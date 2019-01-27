// analyze.cpp
// rene-d 01/2019

#include <iostream>

using namespace std;

//#define ARDUINO

typedef unsigned char byte;

enum
{
    DEC,
    HEX,
    OCT
};

struct
{
    void println(const char *s) const
    {
        printf("%s\n", s);
    }
    void print(const char *s) const
    {
        printf("%s", s);
    }

    void println(float f) const
    {
        printf("%g\n", f);
    }
    void print(float f) const
    {
        printf("%g", f);
    }

    void println(double f) const
    {
        printf("%lg\n", f);
    }
    void print(double f) const
    {
        printf("%lg", f);
    }

    void println(int i, int fmt = DEC) const
    {
        if (fmt == HEX)
        {
            printf("%X\n", i);
        }
        else if (fmt == OCT)
        {
            printf("%o\n", i);
        }
        else
        {
            printf("%d\n", i);
        }
    }
    void print(int i, int fmt = DEC) const
    {
        if (fmt == HEX)
        {
            printf("%X", i);
        }
        else if (fmt == OCT)
        {
            printf("%o", i);
        }
        else
        {
            printf("%d", i);
        }
    }
} Serial;

byte fromhex(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return 0xFF;
}

size_t fromhex(byte *x, const char *s)
{
    byte h, l;
    size_t count = 0;

    while (true)
    {
        do
        {
            if (*s == 0)
                return count;
            h = fromhex(*s++);
        } while (h == 0xFF);
        do
        {
            if (*s == 0)
                return count;
            l = fromhex(*s++);
        } while (l == 0xFF);
        x[count++] = (h << 4) + l;
    }
    return count;
}

void prt(const byte *b, size_t l)
{
    for (size_t i = 0; i < l; ++i)
    {
        if (i != 0)
            printf(" ");
        printf("%X%X", b[i] >> 4, b[i] & 0xf);
    }
    printf("\n");
}

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
    Serial.print("  channel: ");
    Serial.println(channel);
    Serial.print("  id: ");
    Serial.println(id, HEX);

    char buf[80];
    snprintf(buf, 80, "  date: %02d/%02d/20%02d %02d:%02d:%02d",
             day, month, year, hour, minute, second);
    Serial.println(buf);

#else
    char buf[80];
    snprintf(buf, 80, "channel=%d crc=%02X %s id=%02X date=%02d/%02d/20%02d %02d:%02d:%02d",
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
        printf("  nibble %2d : %X  %s\n", i, nibble(osdata, i), label[i]);
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

    int hum = ((osdata[7] & 0x0F) * 10) + (osdata[6] >> 4);

    int bat = osdata[4] & 0x0F;

#ifdef ARDUINO
    Serial.print("  channel: ");
    Serial.println(channel);
    Serial.print("  id: ");
    Serial.println(rolling_code, HEX);
    Serial.print("  temperature: ");
    Serial.println(temp / 10.);
    Serial.print("  humidity: ");
    Serial.println(hum);
    Serial.print("  battery: ");
    Serial.print(bat);
    Serial.print(" ");
    Serial.println(bat >= 4 ? "low" : "ok");
#else
    char buf[80];
    snprintf(buf, 80, "channel=%d crc=%02X %s id=%02X temp=%.1lf°C hum=%d%% bat=%d",
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
        printf("  nibble %2d : %X  %s\n", i, nibble(osdata, i), label[i]);
    }
#endif
}

void oregon(const char *data)
{
    byte osdata[128];

    size_t len = fromhex(osdata, data);
    uint16_t id = ((osdata[0] & 0xf) << 8) + osdata[1];
    uint16_t id4 = (osdata[0] << 8) + osdata[1];

    if (id == 0xACC)
    {
        printf("\nmessage: %03X   len=%zu\ndata: ", id, len);
        prt(osdata, len);
        decode_ACC(osdata, len);
    }
    else if (id4 == 0x1A2D)
    {
        printf("\nmessage: %04X   len=%zu\ndata: ", id4, len);
        prt(osdata, len);
        decode_ACC(osdata, len);
    }
    else if (id == 0xAEA || id == 0xAEC)
    {
        printf("\nmessage: %03X   len=%zu\ndata: ", id, len);
        prt(osdata, len);
        decode_AEA(osdata, len);
    }
    else
    {
        printf("\nmessage: %04X   len=%zu UNKNOWN\ndata: ", id4, len);
        prt(osdata, len);
    }
}

int main()
{
    FILE *f = fopen("dump.txt", "r");
    char buf[128];
    while (fgets(buf, 128, f))
    {
        char *c;
        for (c = buf; *c; ++c)
        {
            if (*c == '\r' || *c == '\n')
            {
                *c = 0;
                break;
            }
            if (fromhex(*c) == 0xFF)
                break;
        }
        if (*c == 0 && c > buf + 1)
        {
            *c = 0;
            oregon(buf);
        }
    }
    fclose(f);
}
