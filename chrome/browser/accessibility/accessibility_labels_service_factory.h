// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ACCESSIBILITY_ACCESSIBILITY_LABELS_SERVICE_FACTORY_H_
#define CHROME_BROWSER_ACCESSIBILITY_ACCESSIBILITY_LABELS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;
class AccessibilityLabelsService;

// Factory to get or create an instance of AccessibilityLabelsService from
// a Profile.
class AccessibilityLabelsServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static AccessibilityLabelsService* GetForProfile(Profile* profile);

  static AccessibilityLabelsService* GetForProfileIfExists(Profile* profile);

  static AccessibilityLabelsServiceFactory* GetInstance();

  // Used to create instances for testing.
  static KeyedService* BuildInstanceFor(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<AccessibilityLabelsServiceFactory>;

  AccessibilityLabelsServiceFactory();
  ~AccessibilityLabelsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
};

#endif  // CHROME_BROWSER_ACCESSIBILITY_ACCESSIBILITY_LABELS_SERVICE_FACTORY_H_
