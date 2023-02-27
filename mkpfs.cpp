#define _CRT_SECURE_NO_WARNINGS
#include "io.h"
#include <cryptopp/sha.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <ranges>
using namespace std;
#define byte uint8_t
struct pfs_archive
{
    struct file_hdr
    {
        size_t offset;
        size_t size;
        size_t hdr_pos;
        string name;
        filesystem::path pth;
    };
    vector<file_hdr> file_list;
    byte *hdr = nullptr;
    size_t hdr_size;
    byte index_hash[CryptoPP::SHA1::DIGESTSIZE];
    void add_file(filesystem::directory_entry pth, const string &name)
    {
        cout << format("add file {}", name) << endl;
        file_list.push_back({0, pth.file_size(), 0, name, pth});
    }

    /*
    pfs="pf8"[index_size:4][index:index_size]
    index=[count:4][file_hdr*count][offset_table]
    file_hdr=[name_length:4][name:name_length][empty:4][offset:4][size:4]
    offset_table=[entry_count:4][8*count][table_eof:8][table_pos:4]
    */
    void make_index()
    {
        auto file_hdr_sz =
            accumulate(file_list.begin(), file_list.end(), 4ll,
                       [](size_t acc, file_hdr &cur)
                       { return acc + 4 + cur.name.size() + 4 + 4 + 4; });
        auto offset_tbl_sz = 4 + 8 * file_list.size() + 8 + 4;
        hdr_size = 3 + 4 + file_hdr_sz + offset_tbl_sz;
        hdr = new byte[hdr_size];
        MIO hdrio{hdr};
        hdrio.write("pf8", 3);
        hdrio.write<int32_t>(hdr_size - 7);
        hdrio.write<int32_t>(file_list.size());
        int file_offset = hdr_size;
        for (auto &f : file_list)
        {
            f.offset = file_offset;
            file_offset += f.size;

            hdrio.write<int32_t>(f.name.size());
            hdrio.write(f.name.data(), f.name.size());
            f.hdr_pos = hdrio.offset;
            hdrio.write<int32_t>(0);
            hdrio.write<int32_t>(f.offset);
            hdrio.write<int32_t>(f.size);
        }
        auto pos_offset_tbl = hdrio.offset;
        hdrio.write<int32_t>(file_list.size() + 1);
        for (auto &f : file_list)
        {
            hdrio.write<int32_t>(f.hdr_pos - 7);
            hdrio.write<int32_t>(0);
        }
        hdrio.write<int64_t>(0);
        hdrio.write<int32_t>(pos_offset_tbl - 7);
        CryptoPP::SHA1 sha1;
        sha1.CalculateDigest(index_hash, hdr + 7, hdr_size - 7);
    }

    void write_pfs(const char *pth)
    {
        alignas(64) static byte buf[128 << 20];
        alignas(64) static byte key[sizeof(index_hash) << 1];

        FILE *fp = fopen(pth, "wb");
        fwrite(hdr, 1, hdr_size, fp);
        memcpy(key, index_hash, sizeof(index_hash));
        memcpy(key + sizeof(index_hash), index_hash, sizeof(index_hash));
        for (auto &f : file_list)
        {
            FILE *f2 = fopen(f.pth.string().data(), "rb");

            size_t bytes_read;
            while (bytes_read = fread(buf, 1, 128 << 20, f2))
            {
                size_t i = 0;
                for (; i < bytes_read; i += 40)
                {
                    ((uint64_t *)&buf[i])[0] ^= ((uint64_t *)key)[0];
                    ((uint64_t *)&buf[i])[1] ^= ((uint64_t *)key)[1];
                    ((uint64_t *)&buf[i])[2] ^= ((uint64_t *)key)[2];
                    ((uint64_t *)&buf[i])[3] ^= ((uint64_t *)key)[3];
                    ((uint64_t *)&buf[i])[4] ^= ((uint64_t *)key)[4];
                }
                fwrite(buf, 1, bytes_read, fp);
            }
        }
        fclose(fp);
    }
    ~pfs_archive()
    {
        if (hdr)
            delete[] hdr;
    }
};

int main(int argc, const char *argv[])
{
    if (argc < 3)
    {
        cerr << format("usage: {} target_file [...args]",
                       filesystem::path(argv[0]).filename().string())
             << endl;
        return 1;
    }
    pfs_archive archive;
    bool flag = true;
    for (auto arg : span(argv, argc) | views::drop(2))
    {
        if (filesystem::is_directory(arg))
        {
            cout << format("traversing directory {}", arg) << endl;
            auto base_path = filesystem::path(arg).parent_path();
            for (auto &subfile :
                 filesystem::recursive_directory_iterator(arg) |
                     views::filter([](auto &n)
                                   { return n.path().extension() == ".ast"; }))
            {
                auto rel_path = filesystem::relative(subfile.path(), base_path);
                archive.add_file(subfile, rel_path.string());
            }
        }
        else if (filesystem::is_regular_file(arg))
        {
            if (auto pth = filesystem::path(arg); pth.extension() == ".ast")
                archive.add_file(filesystem::directory_entry(pth),
                                 pth.filename().generic_string());
        }
        else
        {
            cerr << format("invalid path {}", arg) << endl;
        }
    }
    archive.make_index();
    archive.write_pfs(argv[1]);
    return !flag;
}