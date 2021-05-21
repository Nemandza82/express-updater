#include "render_process.h"
#include <string>
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "browser_process.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include "msg_helpers.h"
#include "webservice_requests.h"
#include <memory>
#include <cstdlib>

#ifdef OS_WIN
#include <Shlobj.h> // For APP DATA path
#endif

using namespace std::experimental;


namespace expupd
{
    enum class InstallDstType
    {
        UpdaterRelative, // Where the updater is places
        UserAppData
    };

    static const int WIDTH = 640;
    static const int HEIGHT = 240;

    // Url of the webservice providing the app.
    //static const std::string SERVICE_URL = "http://localhost:3000/";
    static const std::string SERVICE_URL = "https://arcane-lowlands-75138.herokuapp.com/";

    // Api key for the service...
    static const std::string API_KEY = "bb5014d3d821b5dc88dcdc225884e161cfa6c6ce";

    // Where is the install destination...
    static const InstallDstType INSTALL_DST = InstallDstType::UserAppData;

#if defined(OS_WIN)
    // Folder where to place the app. It appends to InstallDst.
    static fs::path APP_FOLDER = "SmartLab";
    static const std::string OS_KEY = "win";
#elif defined(OS_LINUX)
    static fs::path APP_FOLDER = ".smartlab";
    static const std::string OS_KEY = "lin";
#elif defined(OS_MAC)
    static fs::path APP_FOLDER = ".smartlab";
    static const std::string OS_KEY = "mac";
#endif

    // Exe which to start when updating is finished.
    static const std::string BINARY_TO_START = "slab_cef.exe";
    static const std::string EULA_FILENAME = "eula_and_terms.md";
    static wchar_t* COMMAND_LINE_PARAMS = L"launcher";

    // Title of the app
    static const std::string APP_TITLE = "Smartlab Launcher";

    // Initial text in the header...
    static const std::string HEADER_TEXT = "Updating Smartlab, please wait...";
    static const std::string FIRST_TIME_HEADER_TEXT = "Downloading Smartlab for the first time...";

    namespace
    {
        // When using the Views framework this object provides the delegate implementation for the CefWindow that hosts the Views-based browser.
        class SimpleWindowDelegate : public CefWindowDelegate
        {
            CefRefPtr<CefBrowserView> browser_view_;
            IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
            DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);

        public:
            explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
                : browser_view_(browser_view)
            {
            }

            void OnWindowCreated(CefRefPtr<CefWindow> window) override
            {
                // Add the browser view and show the window.
                window->AddChildView(browser_view_);
                window->Show();

                // Give keyboard focus to the browser view.
                browser_view_->RequestFocus();
            }

            void OnWindowDestroyed(CefRefPtr<CefWindow> window) override
            {
                browser_view_ = nullptr;
            }

            bool CanClose(CefRefPtr<CefWindow> window) override
            {
                // Allow the window to close if the browser says it's OK.
                CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();

                if (browser)
                    return browser->GetHost()->TryCloseBrowser();

                return true;
            }
        };
    }  // namespace

    RenderProcess::RenderProcess()
    {
    }

    void RenderProcess::OnContextInitialized()
    {
        CEF_REQUIRE_UI_THREAD();

        auto command_line = CefCommandLine::GetGlobalCommandLine();

#if defined(OS_WIN) || defined(OS_LINUX)
        // Create the browser using the Views framework if "--use-views" is specified
        // via the command-line. Otherwise, create the browser using the native
        // platform framework. The Views framework is currently only supported on Windows and Linux.
        bool use_views = command_line->HasSwitch("use-views");
#else
        bool use_views = false;
#endif

        // SimpleHandler implements browser-level callbacks.
        CefRefPtr<BrowserProcess> browserHandler(new BrowserProcess(use_views));

        // Get start path so can read licences, third party etc...
        auto startPath = filesystem::current_path();

        std::wstring url = std::wstring(L"file://") + startPath.generic_wstring() + L"/index.html";

        // Specify CEF browser settings here.
        CefBrowserSettings browserSettings;

        if (use_views)
        {
            // Create the BrowserView.
            CefRefPtr<CefBrowserView> browserView = CefBrowserView::CreateBrowserView(browserHandler, url, browserSettings, nullptr, nullptr);

            // Create the Window. It will show itself after creation.
            CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browserView));
        }
        else
        {
            // Information used when creating the native window.
            CefWindowInfo window_info;

#if defined(OS_WIN)
            // On Windows we need to specify certain flags that will be passed to CreateWindowEx().
            window_info.SetAsPopup(nullptr, APP_TITLE.c_str());
#endif

            window_info.height = HEIGHT;
            window_info.width = WIDTH;

            // Create the first browser window.
            CefBrowserHost::CreateBrowser(window_info, browserHandler, url, browserSettings, nullptr); 
        }
    }

    namespace
    {
        // Adds quotes around string
        std::string stringify(const std::string& str)
        {
            return std::string("'") + str + "'";
        }

        fs::path binaryPath()
        {
            return APP_FOLDER / BINARY_TO_START;
        }

        bool binaryExists()
        {
            std::error_code ec;
            return fs::exists(binaryPath(), ec);
        }

        // Starts the app after the download is finished
        void startTheApp(CefRefPtr<CefBrowser> browser)
        {
#if defined(OS_WIN)
            STARTUPINFO info = { sizeof(info) };
            PROCESS_INFORMATION processInfo;

            auto bp = binaryPath().generic_wstring();
            auto cl = bp + L" " + COMMAND_LINE_PARAMS;

            if (CreateProcess(bp.c_str(), (wchar_t*)cl.c_str(), nullptr, nullptr, TRUE, 0, nullptr,
                APP_FOLDER.generic_wstring().c_str(), &info, &processInfo))
            {
                sendMessageToProcess<PID_BROWSER>(browser, "Exit");
            }
#else
            // Change folder to app folder
            fs::current_path(APP_FOLDER);

            // Send browser process message to exit the launcher
            sendMessageToProcess<PID_BROWSER>(browser, "Exit");

            // Start the app
            auto command = std::string("start \"\" ") + BINARY_TO_START;
            std::system(command.c_str());
#endif
        }

        void downloadFilesRecursive(std::shared_ptr<fs::path> appPath, std::shared_ptr<Sha1Cache> cache, std::shared_ptr<std::vector<FileItem>> items,
            int ind, CefRefPtr<CefBrowser> browser)
        {
            if (ind >= items->size())
            {
                execJs(browser, "onCompleted();");
                cache->write();
                startTheApp(browser);
                return;
            }

            auto item = (*items)[ind];
            auto itemName = (*items)[ind].name;
            fs::path itemPath = (*appPath) / itemName;

            calculateHashFromFile(itemPath, cache, [=](std::string&& hash)
            {
                if (hash == item.hash)
                {
                    // If hash is the same don't download, report to GUI
                    execJsMethod(browser, "onSameHash", ind + 1, stringify(itemName), stringify(hash));

                    // Download next file...
                    downloadFilesRecursive(appPath, cache, items, ind + 1, browser);
                }
                else
                {
                    try
                    {
                        // Create the directory if missing
                        fs::create_directories(itemPath.parent_path());
                    }
                    catch (...)
                    {
                        // Directory creation may throw an exception...
                        execJsMethod(browser, "onError", stringify("Unable to create folders for application update. Check your permissions."), binaryExists());
                        return;
                    }

                    execJsMethod(browser, "onStartDownload", ind + 1, stringify(itemName));

                    // Download the item
                    downloadFileItem(SERVICE_URL, API_KEY, OS_KEY, itemPath, itemName, [=](uint64 totalBytes)
                    {
                        // Report to GUI that file is downloaded...
                        execJsMethod(browser, "onFileDownloaded", ind + 1, stringify(itemName), totalBytes);

                        // Continue the recursion
                        downloadFilesRecursive(appPath, cache, items, ind + 1, browser);
                    }, [=](uint64 totalBytes)
                    {
                        execJsMethod(browser, "onDownloadProgress", ind + 1, totalBytes, item.size);

                        int x = 1;
                        x = 0;

                    }, [=](std::string&& err)
                    {
                        execJsMethod(browser, "onError", stringify(err), binaryExists());
                    });
                }
            });
        }

        fs::path getAppDataPath()
        {
            fs::path path;

#if defined(OS_WIN)
            TCHAR tcharPath[MAX_PATH];

            if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, tcharPath)))
                path = tcharPath;

#elif defined(OS_LINUX) || defined(OS_MAC)
            path = std::getenv("HOME") ? std::getenv("HOME") : "";
#endif

            return path;
        }

        // Gets the file item list from the webservice and updates the app on the local filesystem...
        void updateAll(CefRefPtr<CefBrowser> browser, const fs::path& sha1CachePath)
        {
            auto sha1Cache = std::make_shared<Sha1Cache>(sha1CachePath);

            getFileItemList(SERVICE_URL, API_KEY, OS_KEY, [=](std::vector<FileItem>&& items)
            {
                execJsMethod(browser, "setTotalItems", items.size());
                downloadFilesRecursive(std::make_shared<fs::path>(APP_FOLDER), sha1Cache, std::make_shared<std::vector<FileItem>>(items), 0, browser);
            }, [=](std::string&& err)
            {
                execJsMethod(browser, "onError", stringify(err), binaryExists()); 
            });
        }

        std::string readWholeFile(const fs::path& path)
        {
            std::ifstream infile(path);
            std::string str((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
            return str;
        }

        void downloadEulaAndShow(CefRefPtr<CefBrowser> browser)
        {
            auto eulaPath = APP_FOLDER / EULA_FILENAME;

            try
            {
                // Create the directory if missing
                fs::create_directories(eulaPath.parent_path());
            }
            catch (...)
            {
                // Directory creation may throw an exception...
                execJsMethod(browser, "onError", stringify("Unable to create folders for application update. Check your permissions."), binaryExists());
                return;
            }

            downloadFileItem(SERVICE_URL, API_KEY, OS_KEY, eulaPath, EULA_FILENAME, [=](uint64 totalBytes)
            {
                auto eulaText = readWholeFile(eulaPath);
                execJsMethod(browser, "showEula", std::string("`") + eulaText + "`");
            }, [=](uint64 totalBytes)
            {
            }, [=](std::string&& err) 
            {
                execJsMethod(browser, "onError", stringify(err), binaryExists());
            });
        }

        void onStarted(CefRefPtr<CefBrowser> browser, const fs::path& sha1CachePath, const fs::path& eulaCheckPath)
        {
            execJsMethod(browser, "setTitle", stringify(APP_TITLE));
            std::error_code ec;

            if (!fs::exists(APP_FOLDER, ec))
                execJsMethod(browser, "setHeaderText", stringify(FIRST_TIME_HEADER_TEXT));
            else
                execJsMethod(browser, "setHeaderText", stringify(HEADER_TEXT));

            if (!fs::exists(eulaCheckPath, ec))
            {
                downloadEulaAndShow(browser);
            }
            else
            {
                updateAll(browser, sha1CachePath);
            }
        }

        class ClientV8ExtensionHandler : public CefV8Handler
        {
            RenderProcess* _app;
            IMPLEMENT_REFCOUNTING(ClientV8ExtensionHandler);

        public:
            ClientV8ExtensionHandler(RenderProcess* app)
            {
                _app = app;
            }

            bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& args, CefRefPtr<CefV8Value>& retval, CefString& exception) override
            {
                auto browser = CefV8Context::GetCurrentContext()->GetBrowser();

                if (name == "started" && args.size() == 0)
                {
                    onStarted(browser, _app->sha1CachePath(), _app->eulaCheckPath());
                    return true;
                }
                else if (name == "eulaAccepted" && args.size() == 0)
                {
                    _app->acceptEula();
                    updateAll(browser, _app->sha1CachePath());
                }
                else if (name == "eulaDeclined" && args.size() == 0)
                {
                    sendMessageToProcess<PID_BROWSER>(browser, "Exit"); 
                    return true;
                }
                else if (name == "runapp" && args.size() == 0) 
                {
                    startTheApp(browser);
                    return true;
                }
                else if (name == "exit" && args.size() == 0)
                {
                    sendMessageToProcess<PID_BROWSER>(browser, "Exit");
                    return true;
                }

                return false;
            }
        };
    } // namespace

    void RenderProcess::OnWebKitInitialized()
    {
        std::string app_code =
            "var app;"
            ""
            "if (!app)"
            "    app = {};"
            ""
            "(function() {"
            ""
            "    app.exit = function() { native function exit(); return exit(); };"
            "    app.started = function() { native function started(); return started(); };"
            "    app.eulaAccepted = function() { native function eulaAccepted(); return eulaAccepted(); };"
            "    app.eulaDeclined = function() { native function eulaDeclined(); return eulaDeclined(); };"
            "    app.runapp = function() { native function runapp(); return runapp(); };"
            ""
            "})();";

        CefRegisterExtension("v8/app", app_code, new ClientV8ExtensionHandler(this));

        // Setup folders
        fs::path rootFolder = "";

        switch (INSTALL_DST)
        {
        case InstallDstType::UpdaterRelative:
            rootFolder = APP_FOLDER;
            break;
        case InstallDstType::UserAppData:
            rootFolder = getAppDataPath() / APP_FOLDER;
            break;
        }

        _sha1CachePath = rootFolder / ".sha1";
        APP_FOLDER = rootFolder / "app";
        _eulaCheckPath = rootFolder / ".eula";
    }

    // Called when message is recieved from browser proccess
    bool RenderProcess::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
    {
        const std::string& messageName = message->GetName();

        if (messageName == "ExecuteJavaScript")
        {
            // Browser process is sending message to execute the javascript.
            auto script = message->GetArgumentList()->GetString(0);
            auto frame = browser->GetMainFrame();
            auto context = frame->GetV8Context();

            context->Enter();
            frame->ExecuteJavaScript(script, frame->GetURL(), 0);
            context->Exit();
        }

        return false;
    }

    const fs::path& RenderProcess::sha1CachePath() const
    {
        return _sha1CachePath;
    }

    const fs::path& RenderProcess::eulaCheckPath() const
    {
        return _eulaCheckPath;
    }

    void RenderProcess::acceptEula()
    {
        // Create the file...
        std::ofstream file(_eulaCheckPath);
        file.close();
    }
}
