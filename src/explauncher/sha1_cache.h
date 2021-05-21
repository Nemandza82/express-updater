#pragma once
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include "filesystem_helper.h"


namespace expupd
{
    class Sha1Cache
    {
        struct Sha1CacheItem
        {
            long long lastWriteTime;
            std::string hash;
        };

        fs::path _path;
        std::unordered_map<std::string, Sha1CacheItem> _cache;

    public:

        // Reads the cache from the file. If unsuccesfull constructs empty cache.
        Sha1Cache(const fs::path& cacheFilePath) : _path(cacheFilePath)
        {
            std::ifstream inFile(cacheFilePath);

            if (!inFile.is_open())
                return;

            while (!inFile.eof())
            {
                std::string itemName;
                long long time = 0;
                std::string hash;

                inFile >> itemName >> time >> hash;

                if (itemName != "")
                    _cache[itemName] = { time, hash };
            }

            inFile.close();
        }

        // Writes cache to the file.
        void write()
        {
            std::ofstream outFile(_path);

            if (outFile.is_open())
            {
                for (const auto& item : _cache)
                    outFile << item.first << " " << item.second.lastWriteTime << " " << item.second.hash << "\n";

                outFile.close();
            }
        }

        // Gets the sha1 from the cache, return empty string if no sha1 in the cache...
        std::string get(const fs::path& itemPath)
        {
            auto it = _cache.find(itemPath.generic_string());

            // No item in cache return empty string....
            if (it == _cache.end())
                return "";

            try
            {
                auto time = fs::last_write_time(itemPath);

                if (it->second.lastWriteTime == time.time_since_epoch().count())
                    return it->second.hash;
            }
            catch (...)
            {
            }

            return "";
        }

        // Puts the sha1 into the cache...
        void put(const fs::path& itemPath, const std::string& hash)
        {
            try
            {
                auto time = fs::last_write_time(itemPath);
                _cache[itemPath.generic_string()] = { time.time_since_epoch().count(), hash };
            }
            catch (...)
            {
            }
        }
    };
}
