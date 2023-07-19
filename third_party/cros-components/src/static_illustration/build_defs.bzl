"""Build defs for static_illustration template sources."""

load("//javascript/polymer/build_defs:html_template.bzl", "ts_html_template")

def gen_gm3_illustrations(name, srcs):
    """Processes a list of svg illustrations specified by srcs and outputs HTMLTemplateElements.

    Args:
        name: Name of this macro.
        srcs: A list of svg files to generate HTMLTemplateElements.
    """
    for file in srcs:
        ts_html_template(
            name = file + "_gm3",
            src = file,
            out = file + ".ts",
            no_immediate_template_creation = False,
        )
