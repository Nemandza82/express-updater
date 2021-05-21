#include "msg_helpers.h"
#include "include/cef_app.h"


namespace expupd
{
    void setArg(CefRefPtr<CefListValue> args, size_t index, const CefString& argVal)
    {
        args->SetString(index, argVal);
    }

    void setArg(CefRefPtr<CefListValue> args, size_t index, int argVal)
    {
        args->SetInt(index, argVal);
    }

    void setArg(CefRefPtr<CefListValue> args, size_t index, bool argVal)
    {
        args->SetBool(index, argVal);
    }

    // Executes js to update the GUI
    void execJs(CefRefPtr<CefBrowser> browser, const std::string& str)
    {
        auto frame = browser->GetMainFrame();
        auto context = frame->GetV8Context();

        context->Enter();
        frame->ExecuteJavaScript(str, frame->GetURL(), 0);
        context->Exit();
    }
}
