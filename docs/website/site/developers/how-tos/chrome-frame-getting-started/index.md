---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: chrome-frame-getting-started
title: Chrome Frame
---

## Google Chrome Frame was an open source plug-in that seamlessly brought Google Chrome's open web technologies and speedy JavaScript engine to Internet Explorer.

*   ## If you have Chrome Frame installed, please uninstall it. It is no
            longer supported or updated.
*   ## You should continue to encourage your users to install and run
            [evergreen
            browsers](http://tomdale.net/2013/05/evergreen-browsers/), that is,
            browsers that auto-update their users to the latest and greatest.

## Google Chrome Frame is no longer supported and retired as of February 25, 2014.

## Please read our June 2013 [Chromium blog post](http://blog.chromium.org/2013/06/retiring-chrome-frame.html) for details.

### Chrome for Business

## Deploying [Chrome for Business](http://www.google.com/chrome/work) lets you configure over 100 policies to fit your organization's needs. As an admin, you have solid control over the browser deployment, including managing updates, supporting compatibility of older apps, and installing extensions globally.

### Chrome Legacy Browser Support

## If your organization wants to take advantage of the Chrome browser, but your users still need to access older websites and apps that require Internet Explorer, you can use [Legacy Browser Support](https://www.google.com/intl/en/chrome/business/browser/lbs.html) to easily and automatically switch between browsers. When your user clicks a link that requires a legacy browser to open (such as a site that requires ActiveX), the URL will automatically open in the legacy browser from Chrome. You can specify which URLs to launch into a second browser and deploy this Chrome policy for the organization. More info on [Legacy Browser Support in our help center.](http://support.google.com/chrome/a/bin/answer.py?hl=en&answer=3019558)

### How should I deliver feature-forward experiences across browsers now?

## Continue to use feature detection to identify capabilities of a browser. Scaling experiences across desktop and mobile in a performant way may require reducing the amount of effects for older browsers. You can define a browser support policy that outlines the various levels of support and/or consider a low-res edition where the content is very accessible, though it's lacking significant visual and interaction design.
