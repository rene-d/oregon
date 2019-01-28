// analyze.cpp
// rene-d 01/2019

#include <iostream>

using namespace std;

//#define ARDUINO
#include "Arduino_sim.h"
#include "decode.h"

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

size_t fromhex(byte *x, size_t max_len, const char *s)
{
    byte h, l;
    size_t count = 0;

    while (count < max_len)
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

void print_hexa(const byte *b, size_t l)
{
    Serial.print("data:");
    for (size_t i = 0; i < l; ++i)
    {
        Serial.print(" ");
        Serial.print(b[i] & 0xf, HEX);
        Serial.print(b[i] >> 4, HEX);
    }
    Serial.println();
}


void oregon(const byte *osdata, size_t len)
{
    if (len < 2)
        return;

    uint16_t id = (((uint16_t)nibble(osdata, 1)) << 12) +
                  (((uint16_t)nibble(osdata, 2)) << 8) +
                  (((uint16_t)nibble(osdata, 3)) << 4) +
                  nibble(osdata, 4);

    Serial.println();
    Serial.print("message: ");
    Serial.print(id, HEX);
    Serial.print(" len=");
    Serial.println(len);

    if ((id & 0x0fff) == 0xCC3 || id == 0x1D20)
    {
        decode_temp_hygro(osdata, len);
    }
    else if (id == 0x8AE3 || id == 0x8CE3)
    {
        decode_date_time(osdata, len);
    }
    else
    {
        print_nibbles(osdata, len, "1412");
        Serial.println("UNKNOWN");
    }
}

void dump(const char *filename)
{
    FILE *f;
    char buf[128];
    byte osdata[128];
    size_t len;

    if (strcmp(filename, "-") == 0)
    {
        f = stdin;
    }
    else
    {
        f = fopen(filename, "r");
        if (f == nullptr)
            return;
    }

    while (fgets(buf, 128, f))
    {
        char *c;

        // decode message in nibble order
        if (*buf == '!')
        {
            len = 1;
            osdata[0] = 0xA;

            for (c = buf; *c; ++c)
            {
                byte b = fromhex(*c);
                if (b != 0xFF)
                {
                    if (len & 1)
                        osdata[len / 2] |= b << 4;
                    else
                        osdata[len / 2] = b;
                    ++len;
                }
            }
            len /= 2;
            oregon(osdata, len);
            continue;
        }

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

            len = fromhex(osdata, sizeof(osdata), buf);

            oregon(osdata, len);
        }
    }
    if (f != stdin)
        fclose(f);
}

int main(int argc, char *argv[])
{
    if (argc == 1)
        dump("dump.txt");
    else
        for (int i = 1; i < argc; ++i)
            dump(argv[i]);
    return 0;
}
