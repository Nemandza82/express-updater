#pragma once
#include "include/cef_client.h"
#include <list>


namespace expupd
{
    class BrowserProcess : public CefClient,
        public CefDisplayHandler,
        public CefLifeSpanHandler,
        public CefLoadHandler,
        public CefContextMenuHandler
    {
        // Include the default reference counting implementation.
        IMPLEMENT_REFCOUNTING(BrowserProcess);

        // Platform-specific implementation.
        void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);

        // True if the application is using the Views framework.
        const bool _use_views;

        // List of existing browser windows. Only accessed on the CEF UI thread.
        typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
        BrowserList _browser_list;
        bool _is_closing;

    public:
        explicit BrowserProcess(bool use_views);
        ~BrowserProcess();

        // Provide access to the single global instance of this object.
        static BrowserProcess* GetInstance();

        // CefClient methods
        CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
        CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
        CefRefPtr<CefLoadHandler> GetLoadHandler() override;
        CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override;

        bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

        // CefDisplayHandler methods
        void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;

        // CefLifeSpanHandler methods
        void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
        bool DoClose(CefRefPtr<CefBrowser> browser) override;
        void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

        // CefLoadHandler methods
        void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override;

        // CefContextMenuHandler
        void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) override;

        void closeAllBrowsers(bool force_close);
        bool isClosing() const;
    };
}
