---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: inspecting-ash
title: Inspecting Chrome Native UI with Chrome UI DevTools
---

Aura/Views UI can now be 'inspected' just like a webpage inspect-able using
Chrome DevTools. This is accomplished by re-using the existing frontend DevTools
inspector and creating a backend in Chrome and Components which interact with
DevTools inspector using the DevTools protocol language. We name this tool
Chrome UI Devtools. The design document for this tool can be found
[here](https://docs.google.com/document/d/1zpXnSLFrTbLRBJNnO2lWXV--nTOUWfBLPXwA9BDEsKA/edit?usp=sharing).

And,
[here](https://docs.google.com/presentation/d/1q3RBp-QEIx5snjbi3_FNl1pp8wf74DXFpt_NgT0KMB0/edit?usp=sharing)
is a slideshow containing numerous GIFs showing what this is like to use.

**Distances between a pinned element (in solid blue border) and a hovered
element (in solid green border). Guide lines are displayed along the pinned
element (could be either window,widget, or view):**

![](/developers/how-tos/inspecting-ash/distances.png)

**Picture 1: Distances between a pinned element and a hovered element.**

**UI Element hierarchy tree:**

![](/developers/how-tos/inspecting-ash/dom_tree.png)

**Picture 2:** **UI Element hierachy tree.**

**Current Features**

*   Display window/widget/view in UI Element hierarchy tree (picture 2).
*   Selecting nodes in the inspector displays their attributes (height,
            width, x, y, visibility, view text tool tip if exist) in the CSS
            side panel (picture 2).
*   Attributes (except tooltip) can be edited in the CSS side panel.
*   Any changes in the UI such as addition/removal/rearranging of
            elements in Aura/Views UI will be reflected in the Chrome UI
            Inspector.
*   Hovering over elements in the inspector highlights them in
            Aura/Views UI, displays guide lines around the elements and and
            expands them in the inspector.
*   Entering inspect mode by clicking on the arrow-in-box ![](/developers/how-tos/inspecting-ash/Screenshot%20from%202017-08-28%2014_28_36.png)
            and hovering on any window/widget/view will show the corresponding
            node in DOM tree and element guide lines.
*   Entering inspect mode by clicking on the arrow-in-box ![](/developers/how-tos/inspecting-ash/Screenshot%20from%202017-08-28%2014_28_36.png)
            and hovering on any DOM node will show the corresponding UI Element
            in Chrome UI and element guide lines.
*   Entering inspect mode and clicking on any element will pin the
            element. While the element is being pinned, hovering on another
            element displays the distances between the pinned element and the
            hovered element (as in picture 1). There are 7 position arrangements
            between 2 elements as described in [this design
            document.](https://docs.google.com/document/d/1ySba9uad3ClqlA9CExlII6r0kgyhRnE0QWo11x2Wmbg)
*   Pressing ESC or clicking on the arrow-in-box ![](/developers/how-tos/inspecting-ash/Screenshot%20from%202017-08-28%2014_28_36.png)
            to exit inspect mode.
*   Display hit test target window under mouse cursor in inspect mode.
            Steps are shown in this [presentation
            slide](https://docs.google.com/presentation/d/1ldW2rPAexu-nf-gIS1hUgcyPNpgF1fBaqppezyxBRLE)[.](https://docs.google.com/presentation/d/1ldW2rPAexu-nf-gIS1hUgcyPNpgF1fBaqppezyxBRLE)
*   Support platforms: ChromeOS, Linux and Windows.

**Planned Features**

*   Any animations initiated in in Aura/Views UI are displayed in the
            inspector under the "Animations" tab and can be replayed.

**Instructions**

1.  Run Chromium with default port 9223 using one of these 2 ways:
    *   Start with UI DevTools flag:
        *   On Windows : $ chrome.exe --enable-ui-devtools
        *   On ChromeOS, Linux: $ ./chrome --enable-ui-devtools
        *   If you want to use different port, add port in the flag
                    --enable-ui-devtools=&lt;port&gt;
    *   Start Chrome without flag (./chrome or chrome.exe):
        *   Go to Chrome Omnibox, type about:flags
        *   Go to enable native UI inspection, click Enable and restart
                    Chrome. This will start Chrome will listening port 9223.
2.  In your Chrome browser, go to Omnibox, either
    *   Type `**chrome://inspect#other** then click` `**inspect**` under
                UiDevToolsClient in the listing. This will open up the inspector
                in a new tab (Picture 3).
    *   Type direct link:
                **devtools://devtools/bundled/devtools_app.html?uiDevTools=true&ws=localhost:9223/0**

![](/developers/how-tos/inspecting-ash/chrome_inspect_other.png)

Picture 3: Open Chrome UI Inspection window

To remotely debug from different Chrome browser, navigate to
***devtools://devtools/bundled/devtools_app.html?uiDevTools=true&ws=&lt;ip
address&gt;:&lt;port&gt;/0*** (the 0 stands for the first inspect-able component
which is in Aura/Views UI*,* for now).
