#include <windows.h>
#include "include/cef_sandbox_win.h"
#include "render_process.h"

// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically if using the required compiler version. Pass -DUSE_SANDBOX=OFF
// to the CMake command-line to disable use of the sandbox.
// Uncomment this line to manually enable sandbox support.
// #define CEF_USE_SANDBOX 1

#if defined(CEF_USE_SANDBOX)
    // The cef_sandbox.lib static library is currently built with VS2013. It may not
    // link successfully with other VS versions.
    #pragma comment(lib, "cef_sandbox.lib")
#endif

// Entry point function for all processes.
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Enable High-DPI support on Windows 7 or newer.
    CefEnableHighDPISupport();

    void* sandbox_info = nullptr;

#if defined(CEF_USE_SANDBOX)
    // Manage the life span of the sandbox information object. This is necessary
    // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
    CefScopedSandboxInfo scoped_sandbox;
    sandbox_info = scoped_sandbox.sandbox_info();
#endif

    // Provide CEF with command-line arguments.
    CefMainArgs main_args(hInstance);

    // SimpleApp implements application-level callbacks for the browser process.
    // It will create the first browser instance in OnContextInitialized() after CEF has initialized.
    CefRefPtr<expupd::RenderProcess> app(new expupd::RenderProcess);

    // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
    // that share the same executable. This function checks the command-line and,
    // if this is a sub-process, executes the appropriate logic.
    int exit_code = CefExecuteProcess(main_args, app.get(), sandbox_info);

    if (exit_code >= 0)
    {
        // The sub-process has completed so return here.
        return exit_code;
    }

    // Specify CEF global settings here.
    CefSettings settings;

#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif

    settings.single_process = true;
    settings.background_color = CefColorSetARGB(255, 20, 20, 20);
    settings.remote_debugging_port = 1389;

    // Initialize CEF.
    CefInitialize(main_args, settings, app.get(), sandbox_info);

    // Run the CEF message loop. This will block until CefQuitMessageLoop() is called.
    CefRunMessageLoop();

    // Shut down CEF.
    CefShutdown();

    return 0;
}
