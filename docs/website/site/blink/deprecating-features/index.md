---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: deprecating-features
title: Deprecating Features
---

[TOC]

## How To Measure Usage and Notify Developers

1.  Add your feature to [web_feature.mojom's
            WebFeature](https://cs.chromium.org/chromium/src/third_party/WebKit/public/platform/web_feature.mojom?q=third_party/WebKit/public/platform/web_feature.mojom&sq=package:chromium&dr&l=16).
2.  Add a clever deprecation message to the big switch in
            [UseCounter::deprecationMessage](https://cs.chromium.org/chromium/src/third_party/WebKit/Source/core/frame/Deprecation.cpp?type=cs&q=Deprecation::DeprecationMessage%5C(&l=295).
3.  Instrument your code by:
    *   Adding
                `[DeprecateAs](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/WebKit/Source/bindings/IDLExtendedAttributes.md#DeprecateAs_m_a_c)=[your
                enum value here]` to the feature's IDL definition (see [these
                examples](https://cs.chromium.org/search/?q=DeprecateAs+file:%5Esrc/third_party/WebKit/Source/modules/+package:%5Echromium$&type=cs)).
    *   Adding a call to
                `[Deprecation::CountDeprecation](https://cs.chromium.org/chromium/src/third_party/WebKit/Source/core/frame/Deprecation.h?type=cs&l=43)`
                somewhere relevant (as we're dong for the
                [UserMediaRequest](https://cs.chromium.org/chromium/src/third_party/WebKit/Source/modules/mediastream/UserMediaRequest.cpp?type=cs&q=Deprecation::CountDeprecation%5C(document-%3EGetFrame%5C(%5C),&l=422)).

Note that `DeprecateAs` is intended to replace `MeasureAs` in the IDL file.
Specifying both is redundant.