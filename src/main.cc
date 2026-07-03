#include "handler.h"
#include "include/cef_app.h"
#include "include/cef_command_line.h"

#if defined(OS_WIN)
#include "include/cef_sandbox_win.h"
#include <shellapi.h>
#endif

class KnuckleApp : public CefApp, public CefBrowserProcessHandler {
public:
  KnuckleApp() = default;

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  void OnContextInitialized() override {
    CefRefPtr<Handler> handler(new Handler(false));

    CefBrowserSettings settings;
    CefWindowInfo window_info;

    std::string url = "http://localhost:5173";

    CefRefPtr<CefCommandLine> cmd = CefCommandLine::GetGlobalCommandLine();
    if (cmd->HasSwitch("url")) {
      url = cmd->GetSwitchValue("url");
    }

#if defined(OS_WIN)
    window_info.SetAsPopup(nullptr, "Knuckle");
    window_info.style |= WS_VISIBLE;
#elif defined(OS_MAC)
    CefRect bounds = {0, 0, 0, 0};
    window_info.SetAsChild(nullptr, bounds);
#else
    window_info.SetAsChild(nullptr, 0, 0, 0, 0);
#endif

    CefBrowserHost::CreateBrowserSync(window_info, handler, url, settings,
                                      nullptr, nullptr);
  }

  IMPLEMENT_REFCOUNTING(KnuckleApp);
};

#if defined(OS_WIN)
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  CefMainArgs args(hInstance);
#else
int main(int argc, char* argv[]) {
  CefMainArgs args(argc, argv);
#endif

  void* sandbox_info = nullptr;
#if defined(CEF_USE_SANDBOX)
  CefScopedSandboxInfo scoped_sandbox;
  sandbox_info = scoped_sandbox.sandbox_info();
#endif

  CefRefPtr<KnuckleApp> app(new KnuckleApp);

  CefSettings settings;
  settings.no_sandbox = true;
  settings.multi_threaded_message_loop = false;

  int exit_code = CefExecuteProcess(args, app, sandbox_info);
  if (exit_code >= 0) {
    return exit_code;
  }

  if (!CefInitialize(args, settings, app, sandbox_info)) {
    return 1;
  }

  CefRunMessageLoop();
  CefShutdown();
  return 0;
}
