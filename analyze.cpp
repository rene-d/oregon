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
        Serial.print(b[i] >> 4, HEX);
        Serial.print(b[i] & 0xf, HEX);
    }
    Serial.println();
}

void oregon(const byte *osdata, size_t len)
{
    if (len < 2)
        return;

    uint16_t id = (osdata[0] << 8) + osdata[1];

    Serial.println();
    Serial.print("message: ");
    Serial.print(id, HEX);
    Serial.print(" len=");
    Serial.println(len);

    print_hexa(osdata, len);

    if ((id & 0x0fff) == 0xACC)
    {
        decode_ACC(osdata, len);
    }
    else if (id == 0x1A2D)
    {
        decode_ACC(osdata, len);
    }
    else if ((id & 0x0fff) == 0xAEA || (id & 0x0fff) == 0xAEC)
    {
        decode_AEA(osdata, len);
    }
    else
    {
        Serial.println("UNKNOWN");
    }
}

void dump(const char *filename)
{
    FILE *f;
    char buf[128];

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

            byte osdata[128];
            size_t len = fromhex(osdata, sizeof(osdata), buf);

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
