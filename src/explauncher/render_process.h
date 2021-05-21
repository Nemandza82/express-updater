#pragma once
#include "include/cef_app.h"
#include "filesystem_helper.h"


namespace expupd
{
    // Implements application-level callbacks for the render process and browser process.
    class RenderProcess : public CefApp,
        public CefBrowserProcessHandler,
        public CefRenderProcessHandler
    {
        IMPLEMENT_REFCOUNTING(RenderProcess);

        fs::path _sha1CachePath;
        fs::path _eulaCheckPath;

    public:
        RenderProcess();

        const fs::path& sha1CachePath() const;
        const fs::path& eulaCheckPath() const;

        void acceptEula();

        // CefApp methods:
        CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
        {
            return this;
        }

        CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
        {
            return this;
        }

        // CefRenderProcessHandler methods:
        void OnWebKitInitialized() override;
        bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

        // CefBrowserProcessHandler methods:
        void OnContextInitialized() override;
    };
}
