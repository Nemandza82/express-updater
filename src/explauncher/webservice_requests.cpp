#include "webservice_requests.h"
#include <fstream>
#include "include/cef_parser.h"
#include <iostream>
#include "tinysha1/TinySHA1.h"
#include "include/cef_base.h"
#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"

namespace expupd
{
    namespace
    {
        std::vector<FileItem> parseItemList(const std::string& jsonStr)
        {
            std::vector<FileItem> items;
            auto json = CefParseJSON(jsonStr, JSON_PARSER_ALLOW_TRAILING_COMMAS);

            if (!json)
                return items;

            auto list = json->GetList();

            if (!list)
                return items;

            for (size_t i = 0; i < list->GetSize(); i++)
            {
                auto item = list->GetDictionary(i);

                if (!item)
                    return items;

                auto name = item->GetString("name");
                auto hash = item->GetString("hash");
                auto size = item->GetInt("size");

                items.push_back({ name,  hash, size_t(size) });
            }

            return items;
        }

        std::string errCodeToString(CefURLRequest::ErrorCode errCode)
        {
            return std::string("Server unreachable (Err code: " + std::to_string(errCode) + ")");
        }

        class FileListRequestClient : public CefURLRequestClient
        {
            IMPLEMENT_REFCOUNTING(FileListRequestClient);

            uint64 _upload_total;
            uint64 _download_total;

            std::stringstream _download_data;
            FileItemListCallback _onCompleted;
            ErrorCallback _onError;

        public:
            FileListRequestClient(FileItemListCallback&& onCompleted, ErrorCallback&& onError)
                : _onCompleted(std::move(onCompleted))
                , _onError(std::move(onError))
            {
                _upload_total = 0;
                _download_total = 0;
            }

            // Notifies the client that the request has completed. Use the
            // CefURLRequest::GetRequestStatus method to determine if the request was
            // successful or not.
            void OnRequestComplete(CefRefPtr<CefURLRequest> request) override
            {
                CefURLRequest::Status status = request->GetRequestStatus();
                CefURLRequest::ErrorCode errorCode = request->GetRequestError();
                CefRefPtr<CefResponse> response = request->GetResponse();

                if (response->GetStatus() == 404)
                {
                    _onError(std::string("Page not found (404). ") + _download_data.str());
                }
                else if (status != CefURLRequest::Status::UR_SUCCESS)
                {
                    _onError(errCodeToString(errorCode));
                }
                else
                {
                    _onCompleted(parseItemList(_download_data.str()));
                }
            }

            // Notifies the client of upload progress. |current| denotes the number of
            // bytes sent so far and |total| is the total size of uploading data (or -1 if
            // chunked upload is enabled). This method will only be called if the
            // UR_FLAG_REPORT_UPLOAD_PROGRESS flag is set on the request.
            void OnUploadProgress(CefRefPtr<CefURLRequest> request, int64 current, int64 total) override
            {
                _upload_total = total;
            }

            // Notifies the client of download progress. |current| denotes the number of
            // bytes received up to the call and |total| is the expected total size of the
            // response (or -1 if not determined).
            void OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64 current, int64 total) override
            {
                _download_total = total;
            }

            // Called when some part of the response is read. |data| contains the current
            // bytes received since the last call. This method will not be called if the
            // UR_FLAG_NO_DOWNLOAD_DATA flag is set on the request.
            void OnDownloadData(CefRefPtr<CefURLRequest> request, const void* data, size_t data_length) override
            {
                _download_data << std::string(static_cast<const char*>(data), data_length);
            }

            // Called on the IO thread when the browser needs credentials from the user.
            // |isProxy| indicates whether the host is a proxy server. |host| contains the
            // hostname and |port| contains the port number. Return true to continue the
            // request and call CefAuthCallback::Continue() when the authentication
            // information is available. Return false to cancel the request. This method
            // will only be called for requests initiated from the browser process.
            bool GetAuthCredentials(bool isProxy, const CefString& host, int port, const CefString& realm, const CefString& scheme, CefRefPtr<CefAuthCallback> callback) override
            {
                return true;
            }
        };

        class FileDownloadRequestClient : public CefURLRequestClient
        {
            IMPLEMENT_REFCOUNTING(FileDownloadRequestClient);

            uint64 _upload_total;
            uint64 _download_total;
            fs::path _path;
            std::ofstream _outFile;
            DownloadFileCallback _onCompleted;
            DownloadFileCallback _onProgress;
            ErrorCallback _onError;

            int _lastReportProgress;

        public:
            FileDownloadRequestClient(const fs::path& path, DownloadFileCallback&& onCompleted, DownloadFileCallback&& onProgress, ErrorCallback&& onError)
                : _path(path)
                , _onCompleted(std::move(onCompleted))
                , _onProgress(onProgress)
                , _onError(std::move(onError))
            {
                _lastReportProgress = 0;
                _upload_total = 0;
                _download_total = 0;
                _outFile.open(path, std::ios::out | std::ios::binary);
            }

            void OnRequestComplete(CefRefPtr<CefURLRequest> request) override
            {
                CefURLRequest::Status status = request->GetRequestStatus();
                CefURLRequest::ErrorCode errorCode = request->GetRequestError();
                CefRefPtr<CefResponse> response = request->GetResponse();

                if (response->GetStatus() == 404)
                {
                    _onError(std::string("Page not found (404). ") + response->GetStatusText().ToString());
                }
                else  if (!_outFile.is_open())
                {
                    _onError(std::string("Failed to save updates to software. Check your write permissions. ") + _path.string());
                }
                else if (status != CefURLRequest::Status::UR_SUCCESS)
                {
                    _onError(errCodeToString(errorCode));
                }
                else
                {
                    _outFile.close();
                    _onCompleted(_download_total);
                }
            }

            void OnUploadProgress(CefRefPtr<CefURLRequest> request, int64 current, int64 total) override
            {
                _upload_total = current;
            }

            void OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64 current, int64 total) override
            {
            }

            void OnDownloadData(CefRefPtr<CefURLRequest> request, const void* data, size_t data_length) override
            {
                _download_total += data_length;
                _onProgress(_download_total);

                _outFile.write((char*)data, data_length);
            }

            bool GetAuthCredentials(bool isProxy, const CefString& host, int port, const CefString& realm, const CefString& scheme, CefRefPtr<CefAuthCallback> callback) override
            {
                return true;
            }
        };

        class FileSha1RequestClient : public CefURLRequestClient
        {
            IMPLEMENT_REFCOUNTING(FileSha1RequestClient);

            uint64 _upload_total;
            uint64 _download_total;
            fs::path _path;
            sha1::SHA1 _sha1;
            std::shared_ptr<Sha1Cache> _cache;
            HashCalcCallback _onCompleted;

        public:
            FileSha1RequestClient(const fs::path& path, std::shared_ptr<Sha1Cache> cache, HashCalcCallback&& onCompleted)
                : _path(path)
                , _cache(cache)
                , _onCompleted(std::move(onCompleted))
            {
                _upload_total = 0;
                _download_total = 0;
            }

            void OnRequestComplete(CefRefPtr<CefURLRequest> request) override
            {
                if (request->GetRequestStatus() == CefURLRequest::Status::UR_SUCCESS)
                {
                    uint32_t digest[5];
                    _sha1.getDigest(digest);

                    char tmpStr[48];
                    snprintf(tmpStr, 45, "%08x%08x%08x%08x%08x", digest[0], digest[1], digest[2], digest[3], digest[4]);
                    auto sha1Hash = std::string(tmpStr);

                    _cache->put(_path, sha1Hash);
                    _onCompleted(std::move(sha1Hash));
                }
                else
                {
                    // Could not find file, or something similar, return empty string...
                    _onCompleted("");
                }
            }

            void OnUploadProgress(CefRefPtr<CefURLRequest> request, int64 current, int64 total) override
            {
                _upload_total = current;
            }

            void OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64 current, int64 total) override
            {
                _download_total = current;
            }

            void OnDownloadData(CefRefPtr<CefURLRequest> request, const void* data, size_t data_length) override
            {
                _sha1.processBytes(data, data_length);
            }

            bool GetAuthCredentials(bool isProxy, const CefString& host, int port, const CefString& realm, const CefString& scheme, CefRefPtr<CefAuthCallback> callback) override
            {
                return true;
            }
        };

        // Convinience class for cef posting tasks...
        class RunCallback : public CefBaseRefCounted
        {
            IMPLEMENT_REFCOUNTING(RunCallback);
            HashCalcCallback _onCompleted;
        public:
            
            RunCallback(HashCalcCallback&& onCompleted)
                : _onCompleted(onCompleted)
            {
            }

            void run(std::string hash)
            {
                _onCompleted(std::move(hash));
            }
        };

    } // namespace


    void getFileItemList(const std::string& serviceUrl, const std::string& apiKey, const std::string& osKey, FileItemListCallback&& onCompleted, ErrorCallback&& onError)
    {
        CefRefPtr<CefRequest> request = CefRequest::Create();
        request->SetURL(serviceUrl + "api/list?key=" + apiKey + "&os=" + osKey);
        request->SetMethod("GET");
        request->SetFlags(UR_FLAG_SKIP_CACHE);
        CefURLRequest::Create(request, new FileListRequestClient(std::move(onCompleted), std::move(onError)), nullptr);
    }

    void downloadFileItem(const std::string& serviceUrl, const std::string& apiKey, const std::string& osKey, const fs::path& path, const std::string& itemName,
        DownloadFileCallback&& onCompleted, DownloadFileCallback&& onProgress, ErrorCallback&& onError)
    {
        CefRefPtr<CefRequest> request = CefRequest::Create();
        request->SetURL(serviceUrl + "api/download?item=" + itemName + "&key=" + apiKey + "&os=" + osKey);
        request->SetMethod("GET");
        request->SetFlags(UR_FLAG_SKIP_CACHE);
        auto req = CefURLRequest::Create(request, new FileDownloadRequestClient(path, std::move(onCompleted), std::move(onProgress), std::move(onError)), nullptr);
    }

    void calculateHashFromFile(const fs::path& path, std::shared_ptr<Sha1Cache> cache, HashCalcCallback&& onCompleted)
    {
        auto hashFromCache = cache->get(path);

        if (hashFromCache == "")
        {
            // No sha1 hash in cache -> calculate it...
            CefRefPtr<CefRequest> request = CefRequest::Create();
            request->SetURL(path.c_str());
            request->SetMethod("GET");
            request->SetFlags(UR_FLAG_SKIP_CACHE);
            auto req = CefURLRequest::Create(request, new FileSha1RequestClient(path, cache, std::move(onCompleted)), nullptr);
        }
        else
        {
            // Post callback on renderer thread... If we execute it emidiatelly we will overflow the stack...
            CefRefPtr<RunCallback> instance = new RunCallback(std::move(onCompleted));
            CefPostTask(TID_RENDERER, base::Bind(&RunCallback::run, instance, hashFromCache));
        }
    }
}
