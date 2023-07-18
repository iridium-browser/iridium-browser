---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: chromium-graphics
title: Chromium Graphics // Chrome GPU
---

**[<img alt="image"
src="/developers/design-documents/chromium-graphics/graphics.png">](/developers/design-documents/chromium-graphics/graphics.png)**

**Contacts**

[graphics-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/graphics-dev)
is the best way to get in touch with the folks knowledgeable about the Chromium
graphics stack.

**Starter Tasks**

[Small to medium sized clean up task meta
list](https://bugs.chromium.org/p/chromium/issues/detail?id=822915). Please ask
on specific bugs if you want to start working on it and needs more guidance.

**Technical Info**

Start with this overview of [GPU accelerated compositing in
Chrome](/developers/design-documents/gpu-accelerated-compositing-in-chrome),
which includes an overview of the original non-accelerated path.

For current status of feature development, see the [GPU architecture
roadmap](/developers/design-documents/gpu-accelerated-compositing-in-chrome/gpu-architecture-roadmap).

To suggest and vote on presentation topics, see [Compositor/GPU/Graphics talks
you want to
see](https://docs.google.com/spreadsheets/d/1IUl8IumXjBvxZVcd-K84uEvMcE71hJSWkHhMn0Lshr4/edit?usp=sharing).

Presentations:

*   List of [Googler-only presentation slides and
            videos](http://go/gpu-tech-talk-schedule)

*   [Life of a Pixel](http://bit.ly/chromium-loap)
*   [Surface
            Aggregation](https://docs.google.com/presentation/d/14FlKgkh0-4VvM5vLeCV8OTA7YoBasWlwKIJyNnUJltM)
            \[[googler-only
            video](https://drive.google.com/file/d/0BwPS_JpKyELWTURjMS13dUJxR1k/view)\]
*   [History of the World of Chrome Graphics part
            1](https://docs.google.com/presentation/d/1dCfAxJYIgYlnC49SH3hIeyQVIlkbPPb9QRsKfp-6P0g/edit)
            \[[googler-only
            video](https://drive.google.com/file/d/0BwPS_JpKyELWUUhvUHctT1QzNDA/view)\]
*   [Blink Property
            Trees](https://docs.google.com/presentation/d/1ak7YVrJITGXxqQ7tyRbwOuXB1dsLJlfpgC4wP7lykeo)
            \[[googler-only
            video](https://drive.google.com/file/d/0BwPS_JpKyELWUE1lRWxPXzQtdE0/view)\]
*   [Compositor Property
            Trees](https://docs.google.com/presentation/d/1V7gCqKR-edNdRDv0bDnJa_uEs6iARAU2h5WhgxHyejQ)
            \[[googler-only
            video](https://drive.google.com/file/d/0BwPS_JpKyELWTTJ5aWNfenhPQ0k/view)\]
*   [The compositing stack after Surfaces/Display
            compositor](https://docs.google.com/presentation/d/1ou3qdnFhKdjR6gKZgDwx3YHDwVUF7y8eoqM3rhErMMQ/edit#slide=id.p)
*   [Tile
            Management](https://docs.google.com/presentation/d/1gBYqSX92dMHa_UFek3F0D0g4-dt8xvRq0hIifC2IS7Y/edit#slide=id.p)
            \[[googler-only
            video](https://drive.google.com/a/google.com/file/d/0B5eS4VhPbSBzUmZ2UVNZTm1wZmM/view?usp=sharing)
            and
            [notes](https://docs.google.com/document/d/16vWNxkI54E3swcq1IQvDR-LsPLXfhtlNh6Rbkbro2fI/edit#heading=h.57tap1txoipr)\]
*   [Impl-side
            painting](https://docs.google.com/a/chromium.org/presentation/d/1nPEC4YRz-V1m_TsGB0pK3mZMRMVvHD1JXsHGr8I3Hvc)
            \[[googler-only video](http://go/implside-painting-talk-video)\]
*   [TaskGraphRunner and raster task
            scheduling](https://docs.google.com/presentation/d/1dsPwTzJKaLPfd1wMwRkXz--5PqCa43n5L2_EhJ-Qb-g/edit#slide=id.g4dfba32bf_097)
            \[[googler-only
            video](https://drive.google.com/a/google.com/file/d/0BwPS_JpKyELWYXBVUDNfa2VLa3c/view)
            and
            [notes](https://docs.google.com/document/d/16vWNxkI54E3swcq1IQvDR-LsPLXfhtlNh6Rbkbro2fI/edit#heading=h.e1xk0xwayrkn)\]
*   [Checkerboards: Scheduling Compositor Input and
            Output](https://docs.google.com/presentation/u/2/d/1IaMfmCDspmpQwA1IGF6MP6XjuXwb2daxopAUwvgDOxM/edit#slide=id.p)
            \[[googler-only
            video](https://drive.google.com/a/google.com/file/d/0BwPS_JpKyELWQzlIckRsTFRHRDg/view)
            and
            [notes](https://docs.google.com/document/d/16vWNxkI54E3swcq1IQvDR-LsPLXfhtlNh6Rbkbro2fI/edit#heading=h.wjy9kg2zq8rl)\]
*   [Compositor and Display
            Scheduling](https://docs.google.com/presentation/d/1FpTy5DpIGKt8r2t785y6yrHETkg8v7JfJ26zUxaNDUg/edit?usp=sharing)
            \[[googler-only
            video](https://drive.google.com/a/google.com/file/d/0B_got0batQ0TUDJsUFRPeWVOcEk/view?usp=sharing)
            and
            [notes](https://docs.google.com/document/d/16vWNxkI54E3swcq1IQvDR-LsPLXfhtlNh6Rbkbro2fI/edit#heading=h.klitp6r86anv)\]
*   [Gpu
            Scheduler](https://docs.google.com/a/chromium.org/presentation/d/1QPUu0Nb2_nANLE8VApdMzzrifA6iG_BDG9Cd2L4BFV8/edit?usp=sharing)
            \[[googler-only
            video](https://drive.google.com/a/google.com/file/d/0BwPS_JpKyELWb0k0NmNURU1uclk/view)
            and
            [notes](https://docs.google.com/document/d/16vWNxkI54E3swcq1IQvDR-LsPLXfhtlNh6Rbkbro2fI/edit#heading=h.lnpsv7tfpoew)\]
*   [Image
            Decoding](https://docs.google.com/document/d/13UsG1IVEIqRg5yaQ9ZmF7dXQprI6KCrSy82CK7Xwfkw/edit)
            \[[googler-only
            video](https://drive.google.com/a/google.com/file/d/0BwPS_JpKyELWMEdQdlE2M29JUm8/view)
            and
            [slides](https://docs.google.com/presentation/d/1qLgH323yzj5yb9S7mJVmTxXtzsLyYjwgyeuJrgfLDgw/edit#slide=id.p)\]
*   [Native one-copy texture uploads for
            ChromeOS](https://01.org/blogs/2016/native-one-copy-texture-uploads-for-chrome-OS)
*   [Tessellated GPU Path
            Rendering](https://docs.google.com/presentation/d/1tyroXtcGwOvU1LPFxVU-vtBiDkLTcxZ62v2S9wqZ77w/edit#slide=id.p)
*   [Tessellating Edge-AA GPU Path
            Rendering](https://docs.google.com/presentation/d/1DpM5QS6kCkIqQN034Zz6oFm201Gd2wvq6Z30QfWNhcA/edit?usp=sharing)
*   [WebGL 2.0
            Updates](https://www.khronos.org/webgl/wiki/Presentations#September_2016_WebGL_Meetups)
            \[[googler-only
            presentation](https://docs.google.com/a/google.com/presentation/d/1_V_vDLTTpx7XX7_P2J-Nehy-adhdRJItiN-4Pm9QGHQ/edit?usp=sharing)\]
*   [Background on color
            spaces](https://docs.google.com/presentation/d/1c4zjeWDEpHG36gCPZmXjCH7Rlp5_N9p1qyHRIe0AALY/edit?usp=sharing)
            \[[googler-only
            video](https://drive.google.com/file/d/0B6kh5pYRi1dKWGMtaFU2MkZIVjQ/view?usp=sharing)\]
*   [Global Memory
            Coordination](https://docs.google.com/presentation/d/1H2TN3DMRBlOWrpMqqkWlYeKuc7ecGH4-3tr4zqH5LdQ/edit?usp=sharing)
*   [The RenderSurfaceLayerList data
            structure](https://docs.google.com/a/chromium.org/presentation/d/11f3A8cdfSSKmYazetxy9ochHuHqsmSEk3RW3DTYBDIc)
*   [OOP-D: Out-of-Process Display Compositor
            Talk](https://docs.google.com/presentation/d/1PfaIDZ5oJTEuAEJR8aj-B9QC-r1Pht_jQXwjifM1jQI/edit?usp=sharing)
*   [Chromium OS - Compositor
            Pipeline](https://docs.google.com/presentation/d/1MZ0QtHPwxc5yBdfLmsPMgqIhNel5_Uyki-Mem2usW9s/edit#slide=id.g1e31a87b8b_2_285)
            \[[video](https://drive.google.com/file/d/1TVxTBRzXG58268shmCcoeU4gTaQPxFJK/view)\]
*   [Chromium Fast Rounded
            Corners](https://docs.google.com/presentation/d/1Lhrm-1Rg97hQK3dQh_GS6DdhSyC1bZRcLmsj61HW-cA/edit#slide=id.g1e31a87b8b_2_285)

**Design Docs**

*   [An organized list of design
            docs](https://github.com/ds-hwang/wiki/wiki/Chromium-docs)

Major design docs:

*   [Graphics and Skia](/developers/design-documents/graphics-and-skia)
*   [Aura](/developers/design-documents/aura-desktop-window-manager)

    [Threaded
    compositing](/developers/design-documents/compositor-thread-architecture)

    [Impl-side
    painting](http://www.chromium.org/developers/design-documents/impl-side-painting)

    [Zero-input latency
    scheduler](https://docs.google.com/a/chromium.org/document/d/1LUFA8MDpJcDHE0_L2EHvrcwqOMJhzl5dqb0AlBSqHOY/edit)

    [GPU Accelerated
    Rasterization](https://docs.google.com/a/chromium.org/document/d/1Vi1WNJmAneu1IrVygX7Zd1fV7S_2wzWuGTcgGmZVRyE/edit#heading=h.7g13ueq2lwwd)

    [Property
    trees](https://docs.google.com/document/d/1VWjdq8hCJlNbak5ZyAsnLh-0--Hl_wht0xyuagODl8A/edit#heading=h.tf9gh6ldf3qj)

    Motivation for property trees: [Compositing Corner
    Cases](https://docs.google.com/document/d/1hajeBrjGuVG8EtDwyiQnV36oP_1mC8DO8N_7e61MiiE/edit#)

    [Unified BeginFrame
    scheduling](https://docs.google.com/document/d/13xtO-_NSSnNZRRS1Xq3xGNKZawKc8HQxOid5boBUyX8/edit#)

More design docs:

*   [Video playback and the
            compositor](/developers/design-documents/video-playback-and-compositor)
*   [RenderText and Chrome UI text
            drawing](/developers/design-documents/rendertext)
*   [GPU Command
            Buffer](/developers/design-documents/gpu-command-buffer)
*   [GPU Program
            Caching](https://docs.google.com/a/chromium.org/document/d/1Vceem-nF4TCICoeGSh7OMXxfGuJEJYblGXRgN9V9hcE/edit)
*   [Surfaces](/developers/design-documents/chromium-graphics/surfaces)
            (New delegated rendering)

    [Ubercompositor](https://docs.google.com/a/chromium.org/document/d/1ziMZtS5Hf8azogi2VjSE6XPaMwivZSyXAIIp0GgInNA/edit)
    (Old delegated rendering)

*   [16 bpp texture
            support](https://docs.google.com/a/chromium.org/document/d/1TebAdNKbTUIe3-46RaEggT2dwGIdphOjyjm5AIGdhNw/edit)
*   [Image Filters](/developers/design-documents/image-filters)
*   [Synchronous compositing for Android
            WebView](https://docs.google.com/a/chromium.org/document/d/1jw9Xyuovw32NR73u6uQEVk7-fxNtpS7QWAoDMJhF5W8/edit)
*   [Partial Texture
            Updates](https://docs.google.com/a/chromium.org/document/d/1yvSVVgJ8bFyWjXGHpb8wDNtGdx8W5co7W0gbzjdFRj0)
*   [ANGLE WebGL 2
            Planning](https://docs.google.com/document/d/1MkJxb1bB9_WNeCViVZ4bf4opCH_NhqFn049xGq6lf4Q/edit?usp=sharing)
*   [Asynchronous GPU
            Rasterization](https://docs.google.com/a/chromium.org/document/d/1MAUJrOGMuD56hV4JhKp5bTgDv3d9rXRbAftviF8ZmWE/edit?usp=sharing)
            (Client side of GPU scheduling)
*   [Color correct rendering
            support](https://docs.google.com/document/d/1BMyXXTmiAragmt5ukVBIIOLDthd7JcJBgGMt-PwuTHY/edit#)
*   [PictureImageLayer and Directly Composited
            Images](https://docs.google.com/document/d/1sMGAkWhhZT8AfXCAfv4RjT1QxQnkpYKNFW6VXHB7kKk/edit#)
*   [Discardable GPU
            Memory](https://docs.google.com/document/d/1LoNv02sntMa7PPK-TZTuMgc3UuWFqKpOdEqtFvcm_QE/edit?usp=sharing)
*   [GL Command Buffer
            Extensions](https://chromium.googlesource.com/chromium/src/gpu/+/HEAD/GLES2/extensions/CHROMIUM)
    *   [Mailbox
                Extension](https://chromium.googlesource.com/chromium/src/gpu/+/HEAD/GLES2/extensions/CHROMIUM/CHROMIUM_texture_mailbox.txt)
*   [cc::Surfaces for
            Videos](https://docs.google.com/document/d/1tIWUfys0fH2L7h1uH8r53uIrjQg1Ee15ttTMlE0X2Ow/edit#)
*   [Command Buffer Multi
            Flush](https://docs.google.com/document/d/1mvX3VGIrlWtIP8ZBJdzPp9Nf-7TfnrN-cyPy6angVU4/edit)
*   [Lightweight GPU Sync Points
            (SyncTokens)](https://docs.google.com/document/d/1XwBYFuTcINI84ShNvqifkPREs3sw5NdaKzKqDDxyeHk/edit)
*   [Gpu Service
            Scheduler](https://docs.google.com/document/d/1AdgzXmJuTNM2g4dWfHwhlFhs5KVe733_6aRXqIhX43w/edit#heading=h.o5mpe5uzxfv0)
*   [Expected Power Savings from Partial Tree
            Updates](https://drive.google.com/file/d/1KyGuiUm5jm50zsAKmaxAHcFclfbOhHVd/view)
*   [OOP-D: Out-of-Process Display
            Compositor](https://docs.google.com/document/d/1tFdX9StXn9do31hddfLuZd0KJ_dBFgtYmxgvGKxd0rY/edit?usp=sharing)
*   [Viz
            Hit-testing](https://docs.google.com/document/d/1iSZSHdZvV6fx8ee-9SidgIlAUAkxFQBNfjRc8I3PJuQ/edit)
*   [Display Compositing, SkiaRenderer, Skia
            compositing](https://docs.google.com/document/d/1-9zS9c_mJYluu_aX1FvKZuz63FG6R6lWybqJL16ndCo/edit)
*   [Chrome Windows Direct Composition
            Explainer](https://docs.google.com/document/d/1WpdM9VWd-c13IbxJF6eoY7dGaaguTsMXawANWoc36WE/edit?usp=sharing)
*   [Windows Video
            Overlays](https://docs.google.com/document/d/1hmjMKb1DU9ZD2_NzPfFQYGFS1Urc4dqr_mdGdQxPT-M/edit)
*   [Surface Synchronization API
            Guide](https://docs.google.com/document/d/1-54c3c9nZYKxPgKI_ALrpZrwDsXV5VVC2FHlgMh_TOY/edit?usp=sharing)

**Infrastructure**

*   [GPU
            Testing](http://www.chromium.org/developers/testing/gpu-testing)
*   [Rendering
            Benchmarks](/developers/design-documents/rendering-benchmarks)

**Other interesting links**

*   [How to get Ganesh / GPU
            Rasterization](/developers/design-documents/chromium-graphics/how-to-get-gpu-rasterization)
*   [Rendering Architecture
            Diagrams](/developers/design-documents/rendering-architecture-diagrams)
*   Blink:
    *   [Presentation about Blink / Compositor
                interaction](https://docs.google.com/a/chromium.org/presentation/d/1dDE5u76ZBIKmsqkWi2apx3BqV8HOcNf4xxBdyNywZR8/edit#slide=id.p)
    *   [Blink phases of
                rendering](https://docs.google.com/a/chromium.org/document/d/1jxbw-g65ox8BVtPUZajcTvzqNcm5fFnxdi4wbKq-QlY/edit#heading=h.rxj0p5cgef9y)
    *   [How repaint
                works](https://docs.google.com/a/chromium.org/document/d/1jxbw-g65ox8BVtPUZajcTvzqNcm5fFnxdi4wbKq-QlY/edit#heading=h.rxj0p5cgef9y)
*   [Presentation on ANGLE architecture and
            plans](https://docs.google.com/presentation/d/1CucIsdGVDmdTWRUbg68IxLE5jXwCb2y1E9YVhQo0thg/pub?start=false&loop=false)
*   [Debugging Chromium with NVIDIA's
            Nsight](/developers/design-documents/chromium-graphics/debugging-with-nsight)
*   [Chromium WebView graphics
            slides](https://docs.google.com/a/chromium.org/presentation/d/1pYAGn2AYJ7neFDlDZ9DmLHpwMIskzMUXjFXYR7yfUko/edit)
*   [GPU Triage
            Guide](https://docs.google.com/document/d/1Sr1rUl2a5_RBCkLtxfx4qE-xUIJfYraISdSz_I6Ft38/edit#heading=h.vo10gbuchnj4)

**Graphics Meetup Waterloo April, 2019**

*   [Roadmap](https://chromium.github.io/mus-preso/gpumeetup19/slides.html)
            (rjkroege)
*   [OOP-R](https://docs.google.com/presentation/d/1eOjb7CIbXAj3AXGjgWC_RnEY-qUe5ZMc8jfFXE-WPoE/edit)
            (enne)
*   [OOP-D](https://docs.google.com/presentation/d/1y2KWsTyrVxQ-WcVAKHywxwJGcABxotFqRQ4eTYlXomM/edit?usp=sharing)
            (kylechar)
*   [SkiaRenderer](https://docs.google.com/presentation/d/1r-pzeK26Ib5TE7qQbXUjLLsJbitF2Co-j_8sG06Wlr0/edit?usp=sharing)
            (backer)
*   [Test
            Infra/Bots](https://docs.google.com/presentation/d/1_BEinU9Jp-L4YGS_FsCTwV7vLLHRO1gW4AmNAtdsN4Q/edit?usp=sharing)
            (jonross)
*   [Android
            WebView](https://docs.google.com/presentation/d/1KpNg6AB1Ll8_A8dJCcjxc9pa_vmBLKhKnazJrdO5P7s/edit?usp=sharing)
            (boliu)
*   Animations (flackr, no link as it's google internal)
*   [Angle Status
            Update](https://docs.google.com/presentation/d/1KabL_0D-pVE5Oe2iGzCxxQhNC7-CPdR57EZ72jIItwI/edit?usp=sharing)
            (geofflang)
*   [Project
            Pastel](https://docs.google.com/presentation/d/1b22BAbf59juu6RBlJ7XqWFGV3Tj1Q_DftrU6MECDU28/edit?usp=sharing)
            (nicolascapens)
*   [Smooth Interactive
            Animations](https://docs.google.com/presentation/d/14Rfeg7j14zkfWzXT3erLIIOgolx_M6VuqP5NXlLwBCA/edit?usp=sharing)
            (majidvp)
*   [VRR on
            ChromeOS](https://docs.google.com/presentation/d/1RSjR4iYZihfV26uxuprpoq84Dy7-PoJNjquDdUAvdhc/edit?usp=sharing)
            (dcastagna)
