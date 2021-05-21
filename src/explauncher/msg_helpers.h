#pragma once
#include "include/cef_values.h"
#include "include/cef_browser.h"


namespace expupd
{
    void setArg(CefRefPtr<CefListValue> args, size_t index, const CefString& argVal);
    void setArg(CefRefPtr<CefListValue> args, size_t index, int argVal);
    void setArg(CefRefPtr<CefListValue> args, size_t index, bool argVal);

    // Sends the message to the process...
    template<CefProcessId PID>
    void sendMessageToProcess(CefRefPtr<CefBrowser> browser, const CefString& msgName)
    {
        auto msg = CefProcessMessage::Create(msgName);
        browser->SendProcessMessage(PID, msg);
    }

    // Sends the message to the process...
    template<CefProcessId PID, typename A0>
    void sendMessageToProcess(CefRefPtr<CefBrowser> browser, const CefString& msgName, const A0& arg0)
    {
        auto msg = CefProcessMessage::Create(msgName);
        auto args = msg->GetArgumentList();
        setArg(args, 0, arg0);
        browser->SendProcessMessage(PID, msg);
    }

    // Sends the message to the process...
    template<CefProcessId PID, typename A0, typename A1>
    void sendMessageToProcess(CefRefPtr<CefBrowser> browser, const CefString& msgName, const A0& arg0, const A1& arg1)
    {
        auto msg = CefProcessMessage::Create(msgName);
        auto args = msg->GetArgumentList();
        setArg(args, 0, arg0);
        setArg(args, 1, arg1);
        browser->SendProcessMessage(PID, msg);
    }

    // Sends the message to the process...
    template<CefProcessId PID, typename A0, typename A1, typename A2>
    void sendMessageToProcess(CefRefPtr<CefBrowser> browser, const CefString& msgName, const A0& arg0, const A1& arg1, const A2& arg2)
    {
        auto msg = CefProcessMessage::Create(msgName);
        auto args = msg->GetArgumentList();
        setArg(args, 0, arg0);
        setArg(args, 1, arg1);
        setArg(args, 2, arg2);
        browser->SendProcessMessage(PID, msg);
    }

    // Sends the message to the process...
    template<CefProcessId PID, typename A0, typename A1, typename A2, typename A3>
    void sendMessageToProcess(CefRefPtr<CefBrowser> browser, const CefString& msgName, const A0& arg0, const A1& arg1, const A2& arg2, const A3& arg3)
    {
        auto msg = CefProcessMessage::Create(msgName);
        auto args = msg->GetArgumentList();
        setArg(args, 0, arg0);
        setArg(args, 1, arg1);
        setArg(args, 2, arg2);
        setArg(args, 3, arg3);
        browser->SendProcessMessage(PID, msg);
    }

    // Executes js to update the GUI
    void execJs(CefRefPtr<CefBrowser> browser, const std::string& str);

    template<typename T>
    void execJsMethod(CefRefPtr<CefBrowser> browser, const std::string& method, T p1)
    {
        std::stringstream ss;
        ss << method << "(" << p1 << ");";
        auto str = ss.str();
        execJs(browser, str);
    }

    template<typename T, typename U>
    void execJsMethod(CefRefPtr<CefBrowser> browser, const std::string& method, T p1, U p2)
    {
        std::stringstream ss;
        ss << method << "(" << p1 << ", " << p2 << ");";
        auto str = ss.str();
        execJs(browser, str);
    }

    template<typename T, typename U, typename V>
    void execJsMethod(CefRefPtr<CefBrowser> browser, const std::string& method, T p1, U p2, V p3)
    {
        std::stringstream ss;
        ss << method << "(" << p1 << ", " << p2 << ", " << p3 << ");";
        auto str = ss.str();
        execJs(browser, str);
    }
}
