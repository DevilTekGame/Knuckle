#include "handler.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_app.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <algorithm>
#include <cctype>

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

std::string Handler::UrlEncode(const std::string& val) {
  std::string out;
  out.reserve(val.size() * 3);
  for (unsigned char c : val) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", c);
      out += buf;
    }
  }
  return out;
}

std::string Handler::BuildPanelHtml() {
  std::string html = R"(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#1e1e1e;color:#ccc;padding:16px}
h2{font-size:16px;margin-bottom:12px;color:#fff}
.script{display:flex;align-items:center;padding:8px 10px;border-radius:6px;cursor:pointer}
.script:hover{background:#2a2a2a}
.script+.script{margin-top:4px}
.script input[type=checkbox]{margin-right:10px;accent-color:#4ea8de;width:16px;height:16px;cursor:pointer}
.script label{flex:1;cursor:pointer;font-size:14px;word-break:break-all}
.empty{color:#666;font-size:13px;padding:8px 0}
</style>
</head>
<body>
<h2>Scripts</h2>
)";

  if (scripts_.empty()) {
    html += "<div class=\"empty\">No scripts loaded.</div>";
  } else {
    for (size_t i = 0; i < scripts_.size(); i++) {
      std::string fname = scripts_[i];
      size_t sep = fname.find_last_of("/\\");
      if (sep != std::string::npos) fname = fname.substr(sep + 1);
      bool disabled = disabled_scripts_.count(i) > 0;
      html += "<div class=\"script\"><input type=\"checkbox\" id=\"s" +
              std::to_string(i) + "\"" +
              (disabled ? "" : " checked") +
              " onchange=\"console.log('KSCRIPT_TOGGLE:" +
              std::to_string(i) + ":'+this.checked)\"><label for=\"s" +
              std::to_string(i) + "\">" + fname + "</label></div>";
    }
  }

  html += R"(</body></html>)";

  return "data:text/html," + UrlEncode(html);
}

void Handler::ToggleScriptPanel() {
  if (panel_browser_) {
    CloseScriptPanel();
  } else {
    OpenScriptPanel();
  }
}

void Handler::OpenScriptPanel() {
  std::string url = BuildPanelHtml();
  CefWindowInfo windowInfo;
  CefBrowserSettings settings;
#if defined(OS_WIN)
  windowInfo.SetAsPopup(nullptr, "Knuckle Scripts");
#else
  CefRect bounds = {20, 60, 380, 700};
  windowInfo.SetAsChild(0, bounds);
#endif
  windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
  CefBrowserHost::CreateBrowser(windowInfo, this, url, settings,
                                nullptr, nullptr);
}

void Handler::CloseScriptPanel() {
  if (panel_browser_) {
    panel_browser_->GetHost()->CloseBrowser(false);
    panel_browser_ = nullptr;
  }
}

void Handler::InjectScripts(CefRefPtr<CefFrame> frame) {
  for (size_t i = 0; i < scripts_.size(); i++) {
    if (disabled_scripts_.count(i)) continue;
    std::string path = scripts_[i];

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
  } else if (browser != main_browser_ && !panel_browser_) {
    panel_browser_ = browser;
#if defined(OS_WIN)
    HWND mainHwnd = main_browser_->GetHost()->GetWindowHandle();
    HWND panelHwnd = browser->GetHost()->GetWindowHandle();
    RECT rect = {};
    if (GetWindowRect(mainHwnd, &rect)) {
      int width = 380;
      int height = std::min(40 + (int)scripts_.size() * 36, 700);
      SetWindowPos(panelHwnd, nullptr,
                   rect.left + (rect.right - rect.left) - width - 20,
                   rect.top + 60, width, height, SWP_NOZORDER);
    }
#endif
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
  return false;
}

void Handler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  if (browser->IsSame(main_browser_)) {
    main_browser_ = nullptr;
  }
  if (browser->IsSame(panel_browser_)) {
    panel_browser_ = nullptr;
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
  if (panel_browser_ && browser->IsSame(panel_browser_)) {
    std::string msg = message.ToString();
    const std::string prefix = "KSCRIPT_TOGGLE:";
    if (msg.compare(0, prefix.size(), prefix) == 0) {
      size_t colon = msg.find(':', prefix.size());
      if (colon != std::string::npos) {
        size_t idx = std::stoul(msg.substr(prefix.size(), colon - prefix.size()));
        bool enabled = msg.substr(colon + 1) == "true";
        if (enabled)
          disabled_scripts_.erase(idx);
        else
          disabled_scripts_.insert(idx);
      }
      return true;
    }
  }
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
  if (!browser->IsSame(main_browser_)) {
    return false;
  }
  if (initial_navigation_allowed_) {
    initial_navigation_allowed_ = false;
    return false;
  }
  if (is_redirect) {
    return false;
  }
  if (request->GetURL() == browser->GetMainFrame()->GetURL()) {
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

bool Handler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                          const CefKeyEvent& event,
                          CefEventHandle os_event) {
  (void)os_event;
  if (event.type == KEYEVENT_RAWKEYDOWN) {
    if (event.windows_key_code == 116) {
      if (browser->IsSame(main_browser_)) {
        CefRefPtr<CefFrame> frame = browser->GetMainFrame();
        if (frame) {
          frame->LoadURL(frame->GetURL());
        }
        return true;
      }
    } else if (event.windows_key_code == 120) {
      ToggleScriptPanel();
      return true;
    }
  }
  return false;
}

void Handler::CloseAllBrowsers(bool force_close) {
  if (panel_browser_) {
    panel_browser_->GetHost()->CloseBrowser(force_close);
    panel_browser_ = nullptr;
  }
  if (main_browser_) {
    main_browser_->GetHost()->CloseBrowser(force_close);
  }
}
