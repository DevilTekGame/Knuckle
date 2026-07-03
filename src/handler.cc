#include "handler.h"
#include "include/cef_browser.h"
#include "include/cef_app.h"
#include <iostream>

Handler::Handler(bool use_views) : use_views_(use_views) {}

void Handler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  if (!main_browser_) {
    main_browser_ = browser;
  }
  browser_count_++;
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

void Handler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefFrame> frame,
                        int httpStatusCode) {
  if (frame->IsMain()) {
    std::cout << "[Knuckle] Loaded: " << frame->GetURL().ToString()
              << " (status: " << httpStatusCode << ")" << std::endl;
  }
}

bool Handler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                               cef_log_severity_t level,
                               const CefString& message,
                               const CefString& source,
                               int line) {
  std::cout << "[console] " << message.ToString() << std::endl;
  return false;
}

void Handler::CloseAllBrowsers(bool force_close) {
  if (main_browser_) {
    main_browser_->GetHost()->CloseBrowser(force_close);
  }
}
