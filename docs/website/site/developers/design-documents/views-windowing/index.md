---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: views-windowing
title: views Windowing
---

views provides support for creating dialog boxes and other kinds of windows
through its Widget object. The developer creates a WidgetDelegate (or
sub-interface) implementation that provides the Window with the necessary
information to display itself, and also provides callbacks for notifications
about windowing events.

#### A Simple Example

To create a simple window:

```c++
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

class WindowView : public views::WidgetDelegateView {
 public:
  WindowView() : label_(NULL) { Init(); }

  virtual ~WindowView() {}

 private:
  // Overridden from views::View:
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE {
    canvas->FillRect(GetLocalBounds(), SK_ColorWHITE);
  }

  virtual void Layout() OVERRIDE {
    gfx::Size ps = label_->GetPreferredSize();
    label_->SetBounds((width() - ps.width()) / 2, (height() - ps.height()) / 2,
                      ps.width(), ps.height());
  }

  virtual gfx::Size GetPreferredSize() OVERRIDE {
    gfx::Size ps = label_->GetPreferredSize();
    ps.set_width(ps.width() + 200);
    ps.set_height(ps.height() + 200);
    return ps;
  }

  // Overridden from views::WidgetDelegate:
  virtual string16 GetWindowTitle() const OVERRIDE {
    return ASCIIToUTF16("Hello World Window");
  }

  virtual bool CanResize() const OVERRIDE { return true; }

  virtual bool CanMaximize() const OVERRIDE { return true; }

  virtual views::View* GetContentsView() OVERRIDE { return this; }

  void Init() {
    label_ = new views::Label(ASCIIToUTF16("Hello, World!"));
    AddChildView(label_);
  }

  views::Label* label_;

  DISALLOW_COPY_AND_ASSIGN(WindowView);
};

...

views::Widget::CreateWindow(new WindowView)->Show();
```

The window will delete itself when the user closes it, which will cause the
RootView within it to be destroyed, including the WindowView.

#### WidgetDelegate

The WidgetDelegate is an interface that provides information to the Widget class
used when displaying the window, such as its title and icon, as well as
properties such as whether or not the window can be resized. It also provides
callbacks for events like closing. It has an accessor window() which provides
access to the associated Window object. A Widget has a ContentsView, provided by
the WidgetDelegate, which is a View that is inserted into the Window's client
area.

#### DialogDelegate

A DialogDelegate is a specialized kind of WidgetDelegate specific to dialog
boxes that have OK/Cancel buttons. The DialogDelegate and its associated
ClientView (see below) provide built in OK/Cancel buttons, keyboard handlers for
Esc/Enter and enabling for those features. As a user, you write a View that is
inserted into the DialogClientView that provides the contents of the dialog box,
and implement DialogDelegate instead of WidgetDelegate, which provides callbacks
for when the buttons are clicked as well as methods to provide the button text,
etc.

#### Client and Non Client Views

[<img alt="image"
src="/developers/design-documents/views-windowing/NonClientView.png">](/developers/design-documents/views-windowing/NonClientView.png)

Due to Chrome's non-standard window design, views supports custom rendered
non-client areas. This is supported via the Window class and NonClientFrameView
subclasses. To provide a custom window frame, a View subclasses
NonClientFrameView. This allows the overriding View to render and respond to
events from the non-client areas of a window. views contains two built in type
that do this - CustomFrameView and NativeFrameView. These are used for standard
dialog boxes and top level windows.

For the [Browser Window](/developers/design-documents/browser-window), different
subclasses of NonClientFrameView are used (GlassBrowserFrameView and
OpaqueBrowserFrameView). To allow these to be used the browser overrides
Window's CreateFrameViewForWindow method to construct the appropriate frame
view.

Aside from the RootView, the topmost View in a Window's View hierarchy is the
NonClientView. This view is a container of exactly two views - the
NonClientFrameView described above and a ClientView or subclass. The ClientView
subclass contains the contents of the client area of the window (the stuff
inside the titlebar/frame). A common ClientView subclass is DialogClientView,
which provides built in handling for OK/Cancel buttons and a dialog theme
background. The specific ClientView to be used is specified by a WidgetDelegate
in its CreateClientView implementation. The default implementation of
DialogDelegate automatically creates a DialogClientView. Custom WidgetDelegates
can implement this method to return their own ClientView, which is what
BrowserView does (the Browser window's WidgetDelegate implementor) to return
itself. The ClientView API is fairly minimal except it is given a chance to
perform non-client hit-testing, which is how the draggable title bar within the
TabStrip and the resize corner of windows is implemented.

The ClientView and NonClientFrameView are siblings because it is fairly common
for Views to do one-time initialization when they are inserted into a View
hierarchy and DWM toggling on Windows Vista and newer mean the
NonClientFrameView needs to be swapped out when the DWM is enabled or disabled.
As such if the ClientView were a child of the NonClientFrameView it would be
re-parented, meaning its children might re-initialize with negative
side-effects.

#### Creating a Window

Some simple code to create a window using the WindowView defined in the example
above:

```c++
  views::Widget* window = views::Widget::CreateWindow(new WindowView);
  window->Show();
```
