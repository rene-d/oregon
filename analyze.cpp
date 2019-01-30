// analyze.cpp
// rene-d 01/2019

#include <cstring>
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

//
// dump Oregon Scientific messages from a text file
//
void dump(const char *filename)
{
    FILE *f;
    char buf[128];
    byte osdata[128];
    size_t len;
    char *c;

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
        if (*buf == '!')
        {
            // decode message in nibble order
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

            printf("\n");
            oregon_decode(osdata, len);
        }
        else
        {
            // look for line with hexadecimal digits only
            for (c = buf; *c; ++c)
            {
                if (*c == '\r' || *c == '\n') // end of line
                {
                    *c = 0;
                    break;
                }
                if (fromhex(*c) == 0xFF) // not a digit
                    break;
            }

            if (*c == 0 && c > buf + 1)
            {
                // ok, found
                *c = 0;

                len = fromhex(osdata, sizeof(osdata), buf);

                printf("\n");
                oregon_decode(osdata, len);
            }
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
