#include "handler.h"
#include "include/cef_app.h"
#include "include/cef_command_line.h"

#if defined(OS_WIN)
#include "include/cef_sandbox_win.h"
#include <shellapi.h>
#elif defined(OS_LINUX)
#include <unistd.h>
#include <climits>
#elif defined(OS_MAC)
#include <mach-o/dyld.h>
#include <climits>
#endif

#include <string>

namespace {

std::string GetExeDirectory() {
#if defined(OS_WIN)
  char path[MAX_PATH];
  GetModuleFileNameA(nullptr, path, MAX_PATH);
  std::string s(path);
  auto pos = s.find_last_of("\\/");
  s = s.substr(0, pos);
  for (auto& c : s)
    if (c == '\\') c = '/';
  return s;
#elif defined(OS_LINUX)
  char path[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (len != -1) {
    path[len] = '\0';
    std::string s(path);
    auto pos = s.find_last_of("/\\");
    return s.substr(0, pos);
  }
  return ".";
#elif defined(OS_MAC)
  char path[PATH_MAX];
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) == 0) {
    std::string s(path);
    auto pos = s.find_last_of("/\\");
    return s.substr(0, pos);
  }
  return ".";
#endif
}

}  // namespace

class KnuckleApp : public CefApp, public CefBrowserProcessHandler {
public:
  explicit KnuckleApp(const std::string& base_path)
      : base_path_(base_path) {}

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  void OnContextInitialized() override {
    CefRefPtr<Handler> handler(new Handler());

    CefBrowserSettings settings;
    CefWindowInfo window_info;

    std::string url = "file:///" + base_path_ + "/app/index.html";

    CefRefPtr<CefCommandLine> cmd = CefCommandLine::GetGlobalCommandLine();
    if (cmd->HasSwitch("app")) {
      url = cmd->GetSwitchValue("app");
    } else if (cmd->HasSwitch("url")) {
      url = cmd->GetSwitchValue("url");
    }

#if defined(OS_WIN)
    window_info.SetAsPopup(nullptr, "Knuckle");
    window_info.style |= WS_VISIBLE;
#else
    CefRect bounds = {0, 0, 0, 0};
    window_info.SetAsChild(0, bounds);
#endif
    window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;

    CefBrowserHost::CreateBrowserSync(window_info, handler, url, settings,
                                      nullptr, nullptr);
  }

  IMPLEMENT_REFCOUNTING(KnuckleApp);

private:
  std::string base_path_;
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

  CefRefPtr<KnuckleApp> app(new KnuckleApp(GetExeDirectory()));

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
