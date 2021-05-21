#pragma once
#include <functional>
#include <string>
#include <vector>
#include "include/cef_urlrequest.h"
#include "filesystem_helper.h"
#include "sha1_cache.h"


namespace expupd
{
    struct FileItem
    {
        std::string name;
        std::string hash;
        size_t size;
    };

    using ErrorCallback = std::function<void(std::string&& err)>;
    using FileItemListCallback = std::function<void(std::vector<FileItem>&& items)>;
    using DownloadFileCallback = std::function<void(uint64 totalBytes)>;
    using HashCalcCallback = std::function<void(std::string&& hash)>;

    // Gets the file item list from the webservice endpoint, with the hashes...
    void getFileItemList(const std::string& serviceUrl, const std::string& apiKey, const std::string& osKey, FileItemListCallback&& onCompleted, ErrorCallback&& onError);

    // Downloads a file from a webservice and saves it to path...
    void downloadFileItem(const std::string& serviceUrl, const std::string& apiKey, const std::string& osKey, const fs::path& path, const std::string& itemName,
        DownloadFileCallback&& onCompleted, DownloadFileCallback&& onProgress, ErrorCallback&& onError);

    // Async calc sha1 from file
    void calculateHashFromFile(const fs::path& path, std::shared_ptr<Sha1Cache> cache, HashCalcCallback&& onCompleted);
}
