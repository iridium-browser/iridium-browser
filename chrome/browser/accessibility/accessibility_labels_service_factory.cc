// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/accessibility/accessibility_labels_service_factory.h"

#include "chrome/browser/accessibility/accessibility_labels_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
AccessibilityLabelsService* AccessibilityLabelsServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AccessibilityLabelsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AccessibilityLabelsService*
AccessibilityLabelsServiceFactory::GetForProfileIfExists(Profile* profile) {
  return static_cast<AccessibilityLabelsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/false));
}

// static
AccessibilityLabelsServiceFactory*
AccessibilityLabelsServiceFactory::GetInstance() {
  return base::Singleton<AccessibilityLabelsServiceFactory>::get();
}

// static
KeyedService* AccessibilityLabelsServiceFactory::BuildInstanceFor(
    Profile* profile) {
  return new AccessibilityLabelsService(profile);
}

AccessibilityLabelsServiceFactory::AccessibilityLabelsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AccessibilityLabelsService",
          BrowserContextDependencyManager::GetInstance()) {}

AccessibilityLabelsServiceFactory::~AccessibilityLabelsServiceFactory() {}

content::BrowserContext*
AccessibilityLabelsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

KeyedService* AccessibilityLabelsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return BuildInstanceFor(static_cast<Profile*>(profile));
}
