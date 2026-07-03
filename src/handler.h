#pragma once

#include "include/cef_client.h"
#include <string>

class Handler : public CefClient,
                public CefLifeSpanHandler,
                public CefLoadHandler,
                public CefDisplayHandler {
public:
  Handler() = default;

  // CefClient
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }

  // CefLifeSpanHandler
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  bool DoClose(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

  // CefLoadHandler
  void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                 CefRefPtr<CefFrame> frame,
                 int httpStatusCode) override;

  // CefDisplayHandler
  bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                        cef_log_severity_t level,
                        const CefString& message,
                        const CefString& source,
                        int line) override;

  void CloseAllBrowsers(bool force_close);

private:
  int browser_count_ = 0;
  CefRefPtr<CefBrowser> main_browser_;

  IMPLEMENT_REFCOUNTING(Handler);
};
