#!/usr/bin/env python3
"""Regenerates web_page.h from web_src/index.html (run after editing the web UI)."""
import pathlib, re
root = pathlib.Path(__file__).resolve().parent.parent
src = (root / "web_src" / "index.html").read_text(encoding="utf-8")
lines = [l.strip() for l in src.splitlines() if l.strip()]
body = "\n".join(lines)
assert ')SBHTML"' not in body
out = ("// AUTO-GENERATED from web_src/index.html by tools/make_web_page.py - do not edit by hand.\n"
       "#pragma once\n#include <Arduino.h>\n"
       'static const char SB_WEB_PAGE[] PROGMEM = R"SBHTML(' + body + ')SBHTML";\n')
(root / "web_page.h").write_text(out, encoding="utf-8")
print("web_page.h:", len(body), "bytes of HTML")
