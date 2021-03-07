// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browser_url_handler_impl.h"

#include <stddef.h>

#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "content/browser/renderer_host/debug_urls.h"
#include "content/browser/webui/web_ui_impl.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/url_utils.h"
#include "third_party/blink/public/common/chrome_debug_urls.h"
#include "url/gurl.h"
#include "net/url_request/url_request.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "base/strings/utf_string_conversions.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace content {

static bool handle_trace_scheme(GURL *url, BrowserContext *)
{
	if (!url->SchemeIs(url::kTraceScheme))
		return false;
	iridium::log_request("handle_trace_scheme", *url);
	*url = url->strip_trk();
	return false;
}

static bool trace_scheme_revlookup(GURL *url, BrowserContext *)
{
	if (url->SchemeIs(url::kTraceScheme))
		return false;
	*url = GURL(url::kTraceScheme + (":" + url->spec()));
	return true;
}

// Handles rewriting view-source URLs for what we'll actually load.
static bool HandleViewSource(GURL* url, BrowserContext* browser_context) {
  if (url->SchemeIs(kViewSourceScheme)) {
    // Load the inner URL instead.
    *url = GURL(url->GetContent());

    // Bug 26129: limit view-source to view the content and not any
    // other kind of 'active' url scheme like 'javascript' or 'data'.
    static const char* const default_allowed_sub_schemes[] = {
        url::kHttpScheme,
        url::kHttpsScheme,
        kChromeUIScheme,
        url::kFileScheme,
        url::kFileSystemScheme
    };

    // Merge all the schemes for which view-source is allowed by default, with
    // the view-source schemes defined by the ContentBrowserClient.
    std::vector<std::string> all_allowed_sub_schemes;
    for (size_t i = 0; i < std::size(default_allowed_sub_schemes); ++i)
      all_allowed_sub_schemes.push_back(default_allowed_sub_schemes[i]);
    GetContentClient()->browser()->GetAdditionalViewSourceSchemes(
        &all_allowed_sub_schemes);

    bool is_sub_scheme_allowed = false;
    for (size_t i = 0; i < all_allowed_sub_schemes.size(); ++i) {
      if (url->SchemeIs(all_allowed_sub_schemes[i].c_str())) {
        is_sub_scheme_allowed = true;
        break;
      }
    }

    if (!is_sub_scheme_allowed) {
      *url = GURL(url::kAboutBlankURL);
      return false;
    }

    return true;
  }
  return false;
}

// Turns a non view-source URL into the corresponding view-source URL.
static bool ReverseViewSource(GURL* url, BrowserContext* browser_context) {
  // No action necessary if the URL is already view-source:
  if (url->SchemeIs(kViewSourceScheme))
    return false;
  // Recreate the url with the view-source scheme.
  *url = GURL(kViewSourceScheme + std::string(":") + url->spec());
  return true;
}

static bool DebugURLHandler(GURL* url, BrowserContext* browser_context) {
  // Circumvent processing URLs that the renderer process will handle.
  return blink::IsRendererDebugURL(*url);
}

// static
BrowserURLHandler* BrowserURLHandler::GetInstance() {
  return BrowserURLHandlerImpl::GetInstance();
}

// static
BrowserURLHandler::URLHandler BrowserURLHandler::null_handler() {
  // Required for VS2010: http://connect.microsoft.com/VisualStudio/feedback/details/520043/error-converting-from-null-to-a-pointer-type-in-std-pair
  return nullptr;
}

// static
BrowserURLHandlerImpl* BrowserURLHandlerImpl::GetInstance() {
  return base::Singleton<BrowserURLHandlerImpl>::get();
}

BrowserURLHandlerImpl::BrowserURLHandlerImpl() {
  AddHandlerPair(&DebugURLHandler, BrowserURLHandlerImpl::null_handler());

  // view-source: should take precedence over other rewriters, so it's
  // important to add it before calling up to the content client.
  AddHandlerPair(&HandleViewSource, &ReverseViewSource);

  GetContentClient()->browser()->BrowserURLHandlerCreated(this);
  AddHandlerPair(&handle_trace_scheme, &trace_scheme_revlookup);
}

BrowserURLHandlerImpl::~BrowserURLHandlerImpl() {
}

void BrowserURLHandlerImpl::AddHandlerPair(URLHandler handler,
                                           URLHandler reverse_handler) {
  url_handlers_.push_back(HandlerPair(handler, reverse_handler));
}

void BrowserURLHandlerImpl::RewriteURLIfNecessary(
    GURL* url,
    BrowserContext* browser_context) {
  DCHECK(url);
  DCHECK(browser_context);
  bool ignored_reverse_on_redirect;
  RewriteURLIfNecessary(url, browser_context, &ignored_reverse_on_redirect);
}

std::vector<GURL> BrowserURLHandlerImpl::GetPossibleRewrites(
    const GURL& url,
    BrowserContext* browser_context) {
  std::vector<GURL> rewrites;
  for (const auto& it : url_handlers_) {
    const URLHandler& handler = it.first;
    if (!handler)
      continue;

    GURL mutable_url(url);
    if (handler(&mutable_url, browser_context))
      rewrites.push_back(std::move(mutable_url));
  }

  return rewrites;
}

void BrowserURLHandlerImpl::RewriteURLIfNecessary(
    GURL* url,
    BrowserContext* browser_context,
    bool* reverse_on_redirect) {
  DCHECK(url);
  DCHECK(browser_context);
  DCHECK(reverse_on_redirect);

  if (!url->is_valid()) {
    *reverse_on_redirect = false;
    return;
  }

  for (const auto& it : url_handlers_) {
    const URLHandler& handler = it.first;
    bool has_reverse_rewriter = it.second;
    if (handler && handler(url, browser_context)) {
      *reverse_on_redirect = has_reverse_rewriter;
      return;
    }
  }
}

bool BrowserURLHandlerImpl::ReverseURLRewrite(
    GURL* url, const GURL& original, BrowserContext* browser_context) {
  for (const auto& it : url_handlers_) {
    const URLHandler& handler = it.first;
    const URLHandler& reverse_rewriter = it.second;
    if (reverse_rewriter) {
      GURL test_url(original);
      if (!handler) {
        if (reverse_rewriter(url, browser_context))
          return true;
      } else if (handler(&test_url, browser_context)) {
        return reverse_rewriter(url, browser_context);
      }
    }
  }
  return false;
}

void BrowserURLHandlerImpl::RemoveHandlerForTesting(URLHandler handler) {
  const auto it =
      base::ranges::find(url_handlers_, handler, &HandlerPair::first);
  DCHECK(url_handlers_.end() != it);
  url_handlers_.erase(it);
}

class TrkBar : public ConfirmInfoBarDelegate {
	public:
	static void Create(infobars::InfoBarManager *, const GURL &);
	bool ShouldExpire(const NavigationDetails &) const override;
	bool EqualsDelegate(infobars::InfoBarDelegate *) const override;

	private:
	TrkBar(const GURL &);
	std::u16string GetMessageText() const override;
	int GetButtons() const override;
	infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier(void) const override;

	GURL m_url;
	TrkBar(TrkBar &&) noexcept = delete;
	void operator=(TrkBar &&) noexcept = delete;
};

void TrkBar::Create(infobars::InfoBarManager *mgr, const GURL &url)
{
	std::unique_ptr<ConfirmInfoBarDelegate> tb(new TrkBar(url));
	mgr->AddInfoBar(CreateConfirmInfoBar(std::move(tb)), true);
}

bool TrkBar::ShouldExpire(const NavigationDetails &) const
{
	return false;
}

bool TrkBar::EqualsDelegate(infobars::InfoBarDelegate *ov) const
{
	/*
	 * Make each TrkBar considered equal to any other TrkBar for
	 * the purposes of AddInfoBar(,replace:true).
	 */
	return ov->GetIdentifier() == TRACKING_ALERT_INFOBAR_DELEGATE;
}

TrkBar::TrkBar(const GURL &url) :
	ConfirmInfoBarDelegate(), m_url(url)
{}

std::u16string TrkBar::GetMessageText(void) const
{
	return base::ASCIIToUTF16("A graylisted transmission was made (please report me): " + m_url.spec());
}

int TrkBar::GetButtons() const
{
	return BUTTON_NONE;
}

infobars::InfoBarDelegate::InfoBarIdentifier TrkBar::GetIdentifier() const
{
	return TRACKING_ALERT_INFOBAR_DELEGATE;
}

}  // namespace content

namespace iridium {

static void gfxlog_request2(const std::string &caller, const GURL &url)
{
	auto browser = chrome::FindLastActive();
	if (browser == nullptr)
		return;
	auto svc = infobars::ContentInfoBarManager::FromWebContents(browser->tab_strip_model()->GetActiveWebContents());
	content::TrkBar::Create(svc, url);
}

void gfxlog_request(const char *caller, const GURL &url)
{
	textlog_request(caller, url);
	if (url.scheme() != url::kTraceScheme && !url.is_trq())
		return;
	auto e = content::GetUIThreadTaskRunner({});
	if (e != nullptr)
		e->PostTask(FROM_HERE, base::BindOnce(&gfxlog_request2,
			std::string(caller), GURL(url)));
}

static struct log_install {
	log_install() { iridium::log_request = gfxlog_request; }
} log_installer;

}
