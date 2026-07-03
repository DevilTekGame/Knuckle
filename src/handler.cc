#include "handler.h"
#include "include/cef_browser.h"
#include "include/cef_app.h"
#include <iostream>

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
