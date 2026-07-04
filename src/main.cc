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
#include "include/wrapper/cef_library_loader.h"
#endif

#include <string>
#include <sstream>
#include <vector>

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

  void OnBeforeCommandLineProcessing(
      const CefString& process_type,
      CefRefPtr<CefCommandLine> command_line) override {
    if (process_type.empty() && command_line->HasSwitch("proxy")) {
      command_line->AppendSwitchWithValue(
          "proxy-server", command_line->GetSwitchValue("proxy"));
    }
  }

  void OnContextInitialized() override {
    CefRefPtr<CefCommandLine> cmd = CefCommandLine::GetGlobalCommandLine();

    std::vector<std::string> scripts;
    if (cmd->HasSwitch("script")) {
      std::string val = cmd->GetSwitchValue("script");
      std::stringstream ss(val);
      std::string item;
      while (std::getline(ss, item, ',')) {
        size_t s = item.find_first_not_of(" \t\r\n");
        size_t e = item.find_last_not_of(" \t\r\n");
        if (s != std::string::npos) {
          scripts.push_back(item.substr(s, e - s + 1));
        }
      }
    }

    CefRefPtr<Handler> handler(new Handler(scripts, base_path_));

    CefBrowserSettings settings;
    CefWindowInfo window_info;

    std::string url = "file:///" + base_path_ + "/app/index.html";

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

  std::string profile;
  int wargc;
  LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  if (wargv) {
    for (int i = 1; i < wargc; i++) {
      std::wstring arg(wargv[i]);
      if (arg.find(L"--profile=") == 0) {
        profile.assign(arg.begin() + 10, arg.end());
        break;
      }
    }
    LocalFree(wargv);
  }
#else
int main(int argc, char* argv[]) {
  CefMainArgs args(argc, argv);

#if defined(OS_MAC)
  CefScopedLibraryLoader library_loader;
  if (!library_loader.LoadInMain())
    return 1;
#endif

  std::string profile;
  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg.find("--profile=") == 0) {
      profile = arg.substr(10);
      break;
    }
  }
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
  if (!profile.empty()) {
    settings.cache_path = GetExeDirectory() + "/cache-" + profile;
  }

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
