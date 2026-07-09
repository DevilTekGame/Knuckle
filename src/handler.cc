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

void Handler::InjectScripts(CefRefPtr<CefFrame> frame) {
  for (size_t i = 0; i < scripts_.size(); i++) {
    if (disabled_scripts_.count(i)) continue;
    InjectScript(frame, i);
  }
}

void Handler::InjectScript(CefRefPtr<CefFrame> frame, size_t index) {
  if (index >= scripts_.size()) return;
  std::string path = scripts_[index];

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
    return;
  }
  std::stringstream ss;
  ss << file.rdbuf();
  std::string content = ss.str();
  if (content.empty()) {
    LogError("Empty script: " + path);
    return;
  }

  frame->ExecuteJavaScript(content, path, 0);
}

std::string Handler::EscapeJS(const std::string& s) {
  std::string r;
  for (char c : s) {
    if (c == '\\') r += "\\\\";
    else if (c == '\'') r += "\\'";
    else if (c == '\n') r += "\\n";
    else if (c == '\r') r += "\\r";
    else r += c;
  }
  return r;
}

std::string Handler::PanelCreationJS() {
  std::string html;
  html += "<style>"
  ".kn-switch{position:relative;display:inline-block;width:36px;height:20px;margin-right:10px;flex-shrink:0}"
  ".kn-switch input{opacity:0;width:0;height:0}"
  ".kn-switch .kn-slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#555;border-radius:20px;transition:.3s}"
  ".kn-switch .kn-slider::before{position:absolute;content:'';height:14px;width:14px;left:3px;bottom:3px;background:#ccc;border-radius:50%;transition:.3s}"
  ".kn-switch input:checked+.kn-slider{background:#4ea8de}"
  ".kn-switch input:checked+.kn-slider::before{transform:translateX(16px);background:#fff}"
  "</style>";
  html += "<div style=\"padding:12px 16px;font-size:15px;font-weight:600;border-bottom:1px solid #333\">Scripts</div>";
  html += "<div style=\"padding:8px 0\">";
  if (scripts_.empty()) {
    html += "<div style=\"padding:8px 16px;color:#888;font-size:13px\">No scripts loaded.</div>";
  } else {
    for (size_t i = 0; i < scripts_.size(); i++) {
      std::string fname = scripts_[i];
      size_t sep = fname.find_last_of("/\\");
      if (sep != std::string::npos) fname = fname.substr(sep + 1);
      bool disabled = disabled_scripts_.count(i) > 0;
      html += "<label style=\"display:flex;align-items:center;padding:8px 16px;cursor:pointer\">";
      html += "<label class=\"kn-switch\">";
      html += "<input type=\"checkbox\"";
      if (!disabled) html += " checked";
      html += " onchange=\"console.log('KSCRIPT_TOGGLE:" + std::to_string(i) + ":'+this.checked)\">";
      html += "<span class=\"kn-slider\"></span>";
      html += "</label>";
      html += "<span style=\"font-size:14px\">" + fname + "</span>";
      html += "</label>";
    }
  }
  html += "</div>";

  std::string js;
  js += "var d=document.createElement('div');d.id='kn-panel';";
  js += "d.innerHTML='" + EscapeJS(html) + "';";
  js += "d.style.cssText='position:fixed;top:0;right:-340px;width:340px;height:100%;background:#1e1e1e;color:#ccc;font-family:-apple-system,BlinkMacSystemFont,\"Segoe UI\",Roboto,sans-serif;font-size:14px;overflow-y:auto;z-index:2147483647;transition:right 0.2s ease;box-shadow:-4px 0 12px rgba(0,0,0,0.3)';";
  js += "document.body.appendChild(d);";
  js += "window.__knPanel=d;window.__knVisible=false;";
  return js;
}

void Handler::InjectPanel(CefRefPtr<CefFrame> frame) {
  std::string js = "(function(){";
  js += "if(document.getElementById('kn-panel'))return;";
  js += PanelCreationJS();
  js += "})();";
  frame->ExecuteJavaScript(js, "", 0);
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
    InjectPanel(frame);
  }
}

bool Handler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                cef_log_severity_t level,
                                const CefString& message,
                                const CefString& source,
                                int line) {
  if (!browser->IsSame(main_browser_)) return false;
  std::string msg = message.ToString();
  const std::string prefix = "KSCRIPT_TOGGLE:";
  if (msg.compare(0, prefix.size(), prefix) == 0) {
    size_t colon = msg.find(':', prefix.size());
    if (colon != std::string::npos) {
      size_t idx = std::stoul(msg.substr(prefix.size(), colon - prefix.size()));
      bool enabled = msg.substr(colon + 1) == "true";
      if (enabled) {
        disabled_scripts_.erase(idx);
        CefRefPtr<CefFrame> frame = browser->GetMainFrame();
        if (frame) InjectScript(frame, idx);
      } else {
        disabled_scripts_.insert(idx);
      }
    }
    return true;
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
      if (browser->IsSame(main_browser_)) {
        CefRefPtr<CefFrame> frame = browser->GetMainFrame();
        if (frame) {
          frame->ExecuteJavaScript(
            "(function(){"
            "var p=document.getElementById('kn-panel');"
            "if(!p){"
            + PanelCreationJS() +
            "p=window.__knPanel;"
            "}"
            "window.__knVisible=!window.__knVisible;"
            "p.style.right=window.__knVisible?'0px':'-340px';"
            "})()",
            "", 0);
        }
        return true;
      }
    }
  }
  return false;
}

void Handler::CloseAllBrowsers(bool force_close) {
  if (main_browser_) {
    main_browser_->GetHost()->CloseBrowser(force_close);
  }
}
