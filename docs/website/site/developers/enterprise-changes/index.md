---
breadcrumbs:
- - /developers
  - For Developers
page_name: enterprise-changes
title: Shipping changes that are enterprise-friendly
---

Shipping changes to enterprise customers requires some extra due-diligence.

Millions of users rely on Chromium browsers to do their job. For home use, we
strive to make the browser as simple and safe as we can, by taking complex
configurations off people's minds. However, in small and large enterprises, this
is a task left to specialists who need their browser to fit in the complex
puzzle of hardware and software that drives their organization. Enterprise
customers:

- Have complex and unique environments, supporting a wide range of apps and use
cases for their users
- Take time to adapt to changes, which may include testing and training
- Incur large costs because of disruptive changes

Even changes that are not targeted for enterprises may still have an effect on
them. (Non-exhaustive) examples:

- Major UI changes
- Changes that affect how the browser interacts with proxies, firewalls,
certificates, network protocols, and common enterprise configurations
- Changes that interact with policies, like changing default values
- Changes to web technologies and implementation of specs, especially
interventions ([example](https://www.chromestatus.com/feature/6172836527865856))
and deprecations

Any change that's likely to affect enterprises should follow these guidelines,
easing the burden for IT admins managing their fleets, and reducing the number
of changes that need to be reverted from the stable channel.

## How to ship enterprise-friendly changes

### 1. Give enterprises visibility

If your change has a Chrome launch bug, it includes an enterprise review.
The enterprise team will use this as an input to the enterprise release notes,
so no action is required from you yet.

If your change does not have a Chrome launch bug with an enterprise review,
describe your planned changes by joining and emailing the
[chromium-enterprise](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-enterprise)
technical discussion group, at least 3 milestones (~3 months) prior to launch
to stable, and sooner than that for highly disruptive changes.

Include:

- What is changing
- Why it's changing
- When it's expected to ship to stable
- What enterprises will have to do in response if applicable (update
server-side implementations to conform to a new standard, stop using an
API...)

We will announce the changes in the "Coming Soon" section of the [enterprise
release notes](https://support.google.com/chrome/a/answer/7679408?hl=en)
before your change ships to stable and in the "This release" section when
the change goes to stable. If you've announced the changes in the
[chromium-enterprise](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-enterprise)
technical discussion group, or if it has a Chrome launch bug, the enterprise
team will reach out to you to confirm the plan and the wording each time the
release notes are written (don't worry if schedules have changed since last
time; this is normal).

### 2. Give enterprises control

[Include a policy](/developers/how-tos/enterprise/adding-new-policies) to
control the new change, where possible. The specifics depend on the exact
change, but here's a best practice that works for most disruptive changes.

Introduce a policy that can force the new behavior on or force the behavior
off. If the policy is not set (i.e. for consumers), the behavior is defined
by a Finch config.

- The policy can be temporary. This is appropriate if eventually all users
should have the new behavior (e.g. it's a security-positive change). Having
a temporary policy mitigates unexpected incompatibilities in the enterprise,
and gives IT admins extra time to adjust their environment if this was a
surprise to them. If the policy is temporary:
  - Keep it at least 3 months after the change for small changes, or up to a
  year for major changes.
  - Specify in the policy description which milestone it will be removed when
  you introduce it.
  - Do not set a temporary policy to "deprecated" when you introduce it, even
  though it has an end milestone (this hides it from the documentation by
  default).
- Consider introducing the policy a release before making any changes on
default, so enterprises can opt-in to test it before the default behavior
changes.
- The policy must be added before the branch point of the milestone where it
applies; it can't be merged in after branch point. This is because policies
also must be added to management software that controls the browser, and the
time between branch point and stable release makes this possible. As a
Chromium developer, you don't need to take any action to add the policies to
management software, you only need to create the Chromium policy in time.
- Prefer simple policies like bools and enums, which can be ported to
enterprise management software more easily.

- Roll out the change using Finch, following the standard launch process (e.g.
[process for blink](/blink/launching-features)). Any enterprise that's set the
policy will not see any change, since the policy overrides the Finch config.

- If the policy was only intended as a temporary escape hatch, remove it in the
milestone communicated.

#### Can enterprise users set features directly?

We do not encourage enterprise users to use features or flags. Here are some
reasons why:

1) Features are mostly designed for experimental purposes. They can be
added, modified, or removed easily. Policies, on the other hand, are more stable
 configurations. They require documentation for administrators and backward
 compatibility support.
2) Features always apply to the whole browser instance and need to be
relaunched for any update. However, we encourage all policies to support
`dynamic_refresh: true` and `per_profile: true` whenever possible. This allows
policies to be updated without restarting the browser, and it also allows
policies to be applied to individual profiles instead of the entire browser
instance.
3) Features may be controlled by chrome://flags or a command-line switch.
However, these methods are difficult for administrators who control thousands of
devices. Most administrators prefer to use a dedicated management tool, such as
Group Policy Editor or admin.google.com, to manage Chrome. These tools can set
enterprise policies easily, but they cannot set feature flags.

A typical way to check a policy value together with a feature flag is to create
a utility function that checks both. If the policy value conflicts with the
feature flag setting, the policy value should always take precedence.

```
bool IsMyFeatureEnabled(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(prefs::kMyFeature)
    && base::FeatureList::IsEnabled(features::kMyFeature);
}
```

### Need more help?

Feel free to email
[chromium-enterprise@chromium.org](mailto:chromium-enterprise@chromium.org)
with any questions.
