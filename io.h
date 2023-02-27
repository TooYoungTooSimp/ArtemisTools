#pragma once
#ifndef _IO_H_
#define _IO_H_
#include <cstdint>
#include <cstdio>
#include <cstring>

struct FIO
{
    FILE *fp;
    template <typename T> T read(size_t pos = -1)
    {
        if (~pos)
            fseek(fp, pos, SEEK_SET);
        T t;
        fread(&t, sizeof(T), 1, fp);
        return t;
    }
    void read(uint8_t *buf, size_t len, size_t pos = -1)
    {
        if (~pos)
            fseek(fp, pos, SEEK_SET);
        fread(buf, 1, len, fp);
    }

    void seek(size_t pos) { fseek(fp, pos, SEEK_SET); }
};

struct MIO
{
    uint8_t *data;
    size_t offset;

    template <typename T> T read(size_t pos = -1)
    {
        if (~pos)
            offset = pos;
        T t;
        memcpy(&t, &data[offset], sizeof(T));
        offset += sizeof(T);
        return t;
    }

    void read(uint8_t *buf, size_t len, size_t pos = -1)
    {
        if (~pos)
            offset = pos;
        memcpy(buf, &data[offset], len);
        offset += len;
    }

    template <typename T> void write(T val)
    {
        memcpy(&data[offset], &val, sizeof(T));
        offset += sizeof(T);
    }
    void write(const char *buf, size_t len)
    {
        memcpy(&data[offset], buf, len);
        offset += len;
    }
    void seek(size_t pos) { offset = pos; }
};

#endif // !_IO_H_