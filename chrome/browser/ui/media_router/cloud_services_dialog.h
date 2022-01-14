// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_MEDIA_ROUTER_CLOUD_SERVICES_DIALOG_H_
#define CHROME_BROWSER_UI_MEDIA_ROUTER_CLOUD_SERVICES_DIALOG_H_

class Browser;

namespace media_router {

// Shows a dialog asking whether the user wants to enable casting to cloud
// services.
// The Views implementation of this is in cloud_services_dialog_view.cc.
// The non-Views implementation (no-op) is in cloud_services_dialog.cc.
void ShowCloudServicesDialog(Browser* browser);

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_MEDIA_ROUTER_CLOUD_SERVICES_DIALOG_H_
