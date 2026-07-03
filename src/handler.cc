#include "handler.h"
#include "include/cef_browser.h"
#include "include/cef_app.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#include <fstream>
#include <sstream>
#include <ctime>

Handler::Handler(const std::vector<std::string>& scripts,
                 const std::string& log_path)
    : scripts_(scripts), log_path_(log_path) {}

void Handler::LogError(const std::string& msg) {
  std::ofstream log(log_path_ + "/knuckle.log", std::ios::app);
  if (!log) return;
  std::time_t t = std::time(nullptr);
  char buf[32];
  buf[std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S",
                    std::localtime(&t))] = '\0';
  log << "[" << buf << "] ERROR: " << msg << std::endl;
}

void Handler::InjectScripts(CefRefPtr<CefFrame> frame) {
  for (const auto& script : scripts_) {
    std::string path = script;

#if defined(OS_WIN)
    bool is_absolute =
        (path.size() >= 3 && path[1] == ':') ||
        path[0] == '/' || path[0] == '\\';
#else
    bool is_absolute = !path.empty() && path[0] == '/';
#endif

    if (!is_absolute) {
      path = log_path_ + "/" + path;
    }

    std::ifstream file(path);
    if (!file) {
      LogError("Failed to open script: " + path);
      continue;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    if (content.empty()) {
      LogError("Empty script: " + path);
      continue;
    }

    frame->ExecuteJavaScript(content, path, 0);
  }
}

void Handler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  if (!main_browser_) {
    main_browser_ = browser;
  }
  browser_count_++;

#if defined(OS_WIN)
  HWND hwnd = browser->GetHost()->GetWindowHandle();
  HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(1));
  if (hIcon) {
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
    SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
  }
#endif
}

bool Handler::DoClose(CefRefPtr<CefBrowser> browser) {
  if (browser_count_ == 1) {
    return false;
  }
  return false;
}

void Handler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  if (browser->IsSame(main_browser_)) {
    main_browser_ = nullptr;
  }
  browser_count_--;
  if (browser_count_ == 0) {
    CefQuitMessageLoop();
  }
}

bool Handler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             int popup_id,
                             const CefString& target_url,
                             const CefString& target_frame_name,
                             cef_window_open_disposition_t target_disposition,
                             bool user_gesture,
                             const CefPopupFeatures& popupFeatures,
                             CefWindowInfo& windowInfo,
                             CefRefPtr<CefClient>& client,
                             CefBrowserSettings& settings,
                             CefRefPtr<CefDictionaryValue>& extra_info,
                             bool* no_javascript_access) {
  return true;
}

void Handler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) {
  if (frame->IsMain()) {
    InjectScripts(frame);
  }
}

bool Handler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                cef_log_severity_t level,
                                const CefString& message,
                                const CefString& source,
                                int line) {
  return false;
}

bool Handler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefRequest> request,
                              bool user_gesture,
                              bool is_redirect) {
  if (!frame->IsMain()) {
    return false;
  }
  if (initial_navigation_allowed_) {
    initial_navigation_allowed_ = false;
    return false;
  }
  if (is_redirect) {
    return false;
  }
  return true;
}

void Handler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefContextMenuParams> params,
                                   CefRefPtr<CefMenuModel> model) {
  model->Clear();
}

bool Handler::OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefContextMenuParams> params,
                                    int command_id,
                                    EventFlags event_flags) {
  return false;
}

bool Handler::OnDragEnter(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefDragData> dragData,
                           DragOperationsMask mask) {
  return true;
}

void Handler::CloseAllBrowsers(bool force_close) {
  if (main_browser_) {
    main_browser_->GetHost()->CloseBrowser(force_close);
  }
}
