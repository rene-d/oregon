#ifndef Arduino_sim_h
#define Arduino_sim_h

typedef unsigned char byte;

enum
{
    DEC,
    HEX,
    OCT
};

// emulate Arduino Serial class
struct
{
    void println()
    {
        printf("\n");
    }

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

    void println(size_t f) const
    {
        printf("%zu\n", f);
    }
    void print(size_t f) const
    {
        printf("%zu", f);
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

unsigned long millis()
{
    return 0;
}

#endif
