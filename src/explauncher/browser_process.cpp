#include "browser_process.h"
#ifdef OS_WIN
#include <windows.h>
#endif
#include <sstream>
#include <iostream>
#include <string>
#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_browser.h"
#include "resource.h"
#include <filesystem>
#include "msg_helpers.h"

using namespace std::experimental;


namespace expupd
{
    namespace
    {
        BrowserProcess* g_instance = nullptr;

        // Sends message to RenderProccess, which executes given javascript!
        void executeJavaScript(CefRefPtr<CefBrowser> browser, const std::string& script)
        {
            sendMessageToProcess<PID_RENDERER>(browser, "ExecuteJavaScript", script);
        }
    } // namespace

    BrowserProcess::BrowserProcess(bool use_views)
        : _use_views(use_views)
        , _is_closing(false)
    {
        DCHECK(!g_instance);
        g_instance = this;
    }

    BrowserProcess::~BrowserProcess()
    {
        g_instance = nullptr;
    }

    // static
    BrowserProcess* BrowserProcess::GetInstance()
    {
        return g_instance;
    }

    // CefClient methods
    CefRefPtr<CefDisplayHandler> BrowserProcess::GetDisplayHandler()
    {
        return this;
    }

    CefRefPtr<CefLifeSpanHandler> BrowserProcess::GetLifeSpanHandler()
    {
        return this;
    }

    CefRefPtr<CefLoadHandler> BrowserProcess::GetLoadHandler()
    {
        return this;
    }

    CefRefPtr<CefContextMenuHandler> BrowserProcess::GetContextMenuHandler()
    {
        return this;
    }

    bool BrowserProcess::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
    {
        const std::string& messageName = message->GetName();
        auto args = message->GetArgumentList();

        if (messageName == "Exit")
        {
            this->closeAllBrowsers(false);
            return true;
        }

        return false;
    }

    // CefDisplayHandler methods
    void BrowserProcess::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
    {
        CEF_REQUIRE_UI_THREAD();

        if (_use_views)
        {
            // Set the title of the window using the Views framework.
            CefRefPtr<CefBrowserView> browser_view = CefBrowserView::GetForBrowser(browser);

            if (browser_view)
            {
                if (auto window = browser_view->GetWindow())
                    window->SetTitle(title);
            }
        }
        else
        {
            // Set the title of the window using platform APIs.
            PlatformTitleChange(browser, title);
        }
    }

    // CefLifeSpanHandler methods:
    void BrowserProcess::OnAfterCreated(CefRefPtr<CefBrowser> browser)
    {
        CEF_REQUIRE_UI_THREAD();

        // Add to the list of existing browsers.
        _browser_list.push_back(browser);

#ifdef OS_WIN
        auto hmodule = GetModuleHandle(L"SmartLab.exe");

        //int xSize = GetSystemMetrics(SM_CXICON);
        //int ySize = GetSystemMetrics(SM_CYSMICON);
        int xSize = 64;
        int ySize = 64;

        auto hIcon = LoadImage(hmodule, MAKEINTRESOURCE(IDI_ICON_MAIN), IMAGE_ICON, xSize, ySize, 0);

        if (hIcon)
        {
            auto hwnd = browser->GetHost()->GetWindowHandle();
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
#endif
    }

    bool BrowserProcess::DoClose(CefRefPtr<CefBrowser> browser)
    {
        CEF_REQUIRE_UI_THREAD();

        // Closing the main window requires special handling. See the DoClose()
        // documentation in the CEF header for a detailed destription of this process.
        if (_browser_list.size() == 1)
        {
            // Set a flag to indicate that the window close should be allowed.
            _is_closing = true;
        }

        // Allow the close. For windowed browsers this will result in the OS close event being sent.
        return false;
    }

    void BrowserProcess::OnBeforeClose(CefRefPtr<CefBrowser> browser)
    {
        CEF_REQUIRE_UI_THREAD();

        // Remove from the list of existing browsers.
        auto bit = _browser_list.begin();

        for (; bit != _browser_list.end(); ++bit)
        {
            if ((*bit)->IsSame(browser))
            {
                _browser_list.erase(bit);
                break;
            }
        }

        if (_browser_list.empty())
        {
            // All browser windows have closed. Quit the application message loop.
            CefQuitMessageLoop();
        }
    }

    // CefLoadHandler methods:
    void BrowserProcess::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
    {
        CEF_REQUIRE_UI_THREAD();

        // Don't display an error for downloaded files.
        if (errorCode == ERR_ABORTED)
            return;

        // Display a load error message.
        std::stringstream ss;

        ss << "<html><body bgcolor=\"white\">"
            "<h2>Failed to load URL "
            << std::string(failedUrl) << " with error " << std::string(errorText)
            << " (" << errorCode << ").</h2></body></html>";

        frame->LoadString(ss.str(), failedUrl);
    }

    void BrowserProcess::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model)
    {
        //auto bla1 = params->GetTitleText();
        //model->AddItem(10, "Blalalalla");
    }

    void BrowserProcess::closeAllBrowsers(bool force_close)
    {
        if (!CefCurrentlyOn(TID_UI))
        {
            // Execute on the UI thread.
            CefPostTask(TID_UI, base::Bind(&BrowserProcess::closeAllBrowsers, this, force_close));
            return;
        }

        for (auto& browser : _browser_list)
            browser->GetHost()->CloseBrowser(force_close);
    }

    bool BrowserProcess::isClosing() const
    {
        return _is_closing;
    }

#ifdef OS_WIN
    void BrowserProcess::PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
    {
        CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
        SetWindowText(hwnd, std::wstring(title).c_str());
    }
#endif
}
