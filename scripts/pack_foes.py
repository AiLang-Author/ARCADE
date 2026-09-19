#!/usr/bin/env python3
"""Flatten enemy/queen sheets: expand <use>, bake transforms, 5-view or decade strip."""
import re, math, shutil
from pathlib import Path

def parse_nums(s):
    return [float(x) for x in re.findall(r"[-+]?(?:\d*\.\d+|\d+)(?:[eE][-+]?\d+)?", s)]

def xform_parse(s):
    a, b, c, d, e, f = 1, 0, 0, 1, 0, 0
    if not s:
        return (a, b, c, d, e, f)
    for m in re.finditer(r"(translate|scale|rotate|matrix)\s*\(([^)]*)\)", s):
        kind, args = m.group(1), parse_nums(m.group(2))
        if kind == "translate":
            tx = args[0] if args else 0
            ty = args[1] if len(args) > 1 else 0
            na, nb, nc, nd, ne, nf = 1, 0, 0, 1, tx, ty
        elif kind == "scale":
            sx = args[0] if args else 1
            sy = args[1] if len(args) > 1 else sx
            na, nb, nc, nd, ne, nf = sx, 0, 0, sy, 0, 0
        elif kind == "rotate":
            ang = math.radians(args[0] if args else 0)
            cos, sin = math.cos(ang), math.sin(ang)
            if len(args) >= 3:
                cx, cy = args[1], args[2]
                na, nb, nc, nd, ne, nf = (
                    cos, sin, -sin, cos,
                    cx - cos * cx + sin * cy,
                    cy - sin * cx - cos * cy,
                )
            else:
                na, nb, nc, nd, ne, nf = cos, sin, -sin, cos, 0, 0
        else:
            aa = (args + [0] * 6)[:6]
            na, nb, nc, nd, ne, nf = aa
        a, b, c, d, e, f = (
            a * na + c * nb, b * na + d * nb,
            a * nc + c * nd, b * nc + d * nd,
            a * ne + c * nf + e, b * ne + d * nf + f,
        )
    return (a, b, c, d, e, f)

def mul(M, N):
    a, b, c, d, e, f = M
    na, nb, nc, nd, ne, nf = N
    return (
        a * na + c * nb, b * na + d * nb,
        a * nc + c * nd, b * nc + d * nd,
        a * ne + c * nf + e, b * ne + d * nf + f,
    )

def apply(M, x, y):
    a, b, c, d, e, f = M
    return (a * x + c * y + e, b * x + d * y + f)

def attr(blob, name):
    m = re.search(r'\b%s="([^"]*)"' % re.escape(name), blob)
    return m.group(1) if m else None

def strip_xml(xml):
    xml = re.sub(r"<metadata[\s\S]*?</metadata>", "", xml, flags=re.I)
    xml = re.sub(r"<title[\s\S]*?</title>", "", xml, flags=re.I)
    xml = re.sub(r"<defs[\s\S]*?</defs>", "", xml, flags=re.I)
    xml = re.sub(r'\sclip-path="[^"]*"', "", xml)
    xml = re.sub(r'\sdata-frame="[^"]*"', "", xml)
    xml = re.sub(r'\sxmlns:c2pa="[^"]*"', "", xml)
    xml = re.sub(r'\sfill="none"', "", xml)
    xml = re.sub(r'\sshape-rendering="[^"]*"', "", xml)
    return xml

def expand_use(xml):
    ids = {}
    for m in re.finditer(r'<g([^>]*\sid="([^"]+)"[^>]*)>([\s\S]*?)</g>', xml):
        ids[m.group(2)] = (m.group(1), m.group(3))
    out = xml
    changed = True
    while changed:
        changed = False
        uses = list(re.finditer(r"<use\b[^>]*/?>", out))
        if not uses:
            break
        for u in reversed(uses):
            tag = u.group(0)
            href = (attr(tag, "href") or attr(tag, "xlink:href") or "").lstrip("#")
            if href not in ids:
                out = out[: u.start()] + out[u.end() :]
                changed = True
                continue
            gattrs, body = ids[href]
            gattrs = re.sub(r'\sid="[^"]*"', "", gattrs)
            inner = "<g%s>%s</g>" % (gattrs, body)
            tm = attr(tag, "transform") or ""
            repl = ('<g transform="%s">%s</g>' % (tm, inner)) if tm else inner
            out = out[: u.start()] + repl + out[u.end() :]
            changed = True
    return re.sub(r'\sid="[^"]*"', "", out)

def top_groups(xml):
    """Top-level <g>...</g> with nested groups kept intact."""
    groups = []
    i = 0
    n = len(xml)
    while True:
        m = re.search(r"<g\b", xml[i:], re.I)
        if not m:
            break
        start = i + m.start()
        depth = 0
        j = start
        while j < n:
            open_g = re.match(r"<g\b[^>]*>", xml[j:], re.I)
            close_g = re.match(r"</g>", xml[j:], re.I)
            self_g = re.match(r"<g\b[^>]*/>", xml[j:], re.I)
            if self_g:
                j += self_g.end()
                continue
            if open_g:
                depth += 1
                j += open_g.end()
                continue
            if close_g:
                depth -= 1
                j += close_g.end()
                if depth == 0:
                    groups.append(xml[start:j])
                    i = j
                    break
                continue
            j += 1
        else:
            break
    return groups

def emit_shapes(xml):
    xml = expand_use(strip_xml(xml))
    out = []
    stack = [(1, 0, 0, 1, 0, 0)]
    token = re.compile(
        r"<(/?)(g|polygon|polyline|circle|ellipse|rect|svg|use)([^>]*?)(/?)>", re.I
    )
    for m in token.finditer(xml):
        close, tag, attrs = m.group(1), m.group(2).lower(), m.group(3)
        if tag == "g":
            if close:
                if len(stack) > 1:
                    stack.pop()
            else:
                stack.append(mul(stack[-1], xform_parse(attr(attrs, "transform") or "")))
            continue
        if close or tag in ("svg", "use"):
            continue
        M = stack[-1]
        fill = attr(attrs, "fill")
        if not fill or fill == "none":
            if not attr(attrs, "stroke"):
                continue
            fill = attr(attrs, "stroke")
        if tag in ("polygon", "polyline"):
            nums = parse_nums(attr(attrs, "points") or "")
            if len(nums) < 6:
                continue
            baked = []
            for k in range(0, len(nums) - 1, 2):
                x, y = apply(M, nums[k], nums[k + 1])
                baked.append("%.2f,%.2f" % (x, y))
            out.append((fill, " ".join(baked), "poly"))
        elif tag == "circle":
            cx = float(attr(attrs, "cx") or 0)
            cy = float(attr(attrs, "cy") or 0)
            r = float(attr(attrs, "r") or 0)
            px, py = apply(M, cx, cy)
            sx = math.hypot(M[0], M[1])
            sy = math.hypot(M[2], M[3])
            out.append((fill, (px, py, r * sx, r * sy), "ell"))
        elif tag == "ellipse":
            cx = float(attr(attrs, "cx") or 0)
            cy = float(attr(attrs, "cy") or 0)
            rx = float(attr(attrs, "rx") or 0)
            ry = float(attr(attrs, "ry") or 0)
            px, py = apply(M, cx, cy)
            sx = math.hypot(M[0], M[1])
            sy = math.hypot(M[2], M[3])
            out.append((fill, (px, py, rx * sx, ry * sy), "ell"))
        elif tag == "rect":
            x = float(attr(attrs, "x") or 0)
            y = float(attr(attrs, "y") or 0)
            w = float(attr(attrs, "width") or 0)
            h = float(attr(attrs, "height") or 0)
            pts = [apply(M, x, y), apply(M, x + w, y), apply(M, x + w, y + h), apply(M, x, y + h)]
            out.append((fill, " ".join("%.2f,%.2f" % p for p in pts), "poly"))
    return out

def fmt(shapes, dx=0, dy=0):
    lines = []
    for fill, data, kind in shapes:
        if kind == "poly":
            pts = []
            for pair in data.split():
                x, y = pair.split(",")
                pts.append("%.2f,%.2f" % (float(x) + dx, float(y) + dy))
            lines.append('    <polygon points="%s" fill="%s"/>' % (" ".join(pts), fill))
        else:
            px, py, rx, ry = data
            lines.append(
                '    <ellipse cx="%.2f" cy="%.2f" rx="%.2f" ry="%.2f" fill="%s"/>'
                % (px + dx, py + dy, rx, ry, fill)
            )
    return lines

def group_local(gxml):
    """Strip the outer translate(N,0) so shapes sit in 0..cell."""
    gxml = re.sub(
        r'<g([^>]*)transform="translate\(\s*[\d.]+\s*,\s*0\s*\)"([^>]*)>',
        r"<g\1\2>",
        gxml,
        count=1,
        flags=re.I,
    )
    return gxml

def pack_foe(path, out, cell=64, cols_map=None):
    raw = path.read_text(errors="replace")
    groups = top_groups(strip_xml(raw))
    frames = []
    for g in groups:
        tm = attr(g[: g.find(">") + 1] if ">" in g else "", "transform") or ""
        m = re.search(r"translate\(\s*([\d.]+)\s*,\s*0", tm)
        if m:
            frames.append((int(float(m.group(1)) // cell), g))
        else:
            frames.append((len(frames), g))
    by = {}
    for idx, g in frames:
        by[idx] = g
    idle_a = by.get(0, groups[0] if groups else "<g/>")
    idle_b = by.get(1, idle_a)
    dive = by.get(2, idle_a)
    if cols_map is None:
        cells = [dive, idle_a, idle_a, idle_b, dive]
        n = 5
        cw = cell
    else:
        cells = [by.get(i, idle_a) for i in cols_map]
        n = len(cells)
        cw = cell
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d">' % (n * cw, cell)]
    counts = []
    for i, g in enumerate(cells):
        shapes = emit_shapes(group_local(g))
        counts.append(len(shapes))
        parts.append("  <g>")
        parts.extend(fmt(shapes, i * cw))
        parts.append("  </g>")
    parts.append("</svg>\n")
    text = "\n".join(parts)
    assert "<use" not in text
    Path(out).write_text(text)
    print("%s cells=%d shapes=%s bytes=%d" % (Path(out).name, n, counts, len(text)))

def main():
    root = Path("/tmp/moreenemeies/views/sheets")
    if not root.exists():
        root = Path("/tmp/enemies_pack/views/sheets")
    dst = Path("/home/bob/Arcade/assets/enemies")
    classic = dst / "classic"
    classic.mkdir(exist_ok=True)
    for name in [
        "bee.svg", "butterfly.svg", "boss.svg", "scout.svg",
        "dart.svg", "heavy.svg", "flagship.svg", "captor.svg",
    ]:
        live = dst / name
        if live.exists() and not (classic / name).exists():
            shutil.copy2(live, classic / name)
        pack_foe(root / name, dst / name, cell=64)

    qdir = dst / "queen"
    qdir.mkdir(exist_ok=True)
    for name in ["queen-a.svg", "queen-c.svg", "queen-decades.svg"]:
        p = root / name
        if p.exists():
            shutil.copy2(p, qdir / name)

    # Decade strip: 10 × 128
    dec = root / "queen-decades.svg"
    if dec.exists():
        pack_foe(dec, dst / "queen.svg", cell=128, cols_map=list(range(10)))

if __name__ == "__main__":
    main()
