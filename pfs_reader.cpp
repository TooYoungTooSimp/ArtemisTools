#define _CRT_SECURE_NO_WARNINGS
#include "io.h"
#include <cryptopp/sha.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <ranges>

#ifndef __cpp_lib_format
#define FMT_HEADER_ONLY
#include <fmt/format.h>
using namespace fmt;
#endif

using namespace std;
#define byte unsigned char

struct file_hdr
{
    size_t offset;
    size_t size;
    string name;
};

bool parse_pfs(const char *pth)
{
    /*
    pfs="pf8"[index_size:4][index:index_size]
    index=[count:4][file_hdr*count][offset_table]
    file_hdr=[name_length:4][name:name_length][empty:4][offset:4][size:4]
    offset_table=[entry_count:4][8*count][table_eof:8][table_pos:4]
    */
    FILE *fp = fopen(pth, "rb");
    FIO fio{fp};
    static byte buf[1 << 10];
    fio.read(buf, 2);
    if (memcmp(buf, "pf", 2))
        return false;
    auto pfsver = fio.read<char>() - '0';
    cout << format("pfsver={}", pfsver) << endl;
    auto index_size = fio.read<int32_t>();
    cout << format("index_size={}", index_size) << endl;

    auto index = new byte[index_size];
    fio.read(index, index_size);
    MIO idx_reader{index};

    auto count = idx_reader.read<int32_t>();
    cout << count << endl;
    auto offset_tbl_size = sizeof(int32_t) * (1 + 2 * count + 2 + 1);
    cout << format("offset_tbl_size={}", offset_tbl_size) << endl;
    auto dir = vector<file_hdr>();
    for (int i = 0; i < count; i++)
    {
        int name_length = idx_reader.read<int32_t>();
        string name(name_length, 0);
        idx_reader.read((byte *)name.data(), name_length);
        idx_reader.read<int32_t>();
        file_hdr f;
        f.offset = idx_reader.read<int32_t>();
        f.size = idx_reader.read<int32_t>();
        f.name = move(name);
        dir.push_back(move(f));
    }
    auto entry_count = idx_reader.read<int32_t>();
    if (entry_count != count + 1)
        return false;
    cout << format("index_offset={}", idx_reader.offset) << endl;
    sort(dir.begin(), dir.end(),
         [](auto lhs, auto rhs)
         { return lhs.offset < rhs.offset; });

    static byte index_hash[CryptoPP::SHA1::DIGESTSIZE];
    CryptoPP::SHA1 hash;
    hash.CalculateDigest(index_hash, index, index_size);

    for (auto &hdr : dir)
        cout << format("{} offset:{} sz:{}", hdr.name, hdr.offset, hdr.size)
             << endl;
    delete[] index;
    return true;
}

int main()
{
    return 0;
}
