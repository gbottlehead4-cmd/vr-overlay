#!/usr/bin/env python3
"""Generate VisorVR's raster brand assets from the vector geometry below.

Replaces the old rasterize.ps1 + ImageMagick pipeline, which rendered artwork
that is not ours. Run from this directory:

    python make-assets.py

Outputs: icon.ico, WiXUIBanner.png, WiXUIDialog.png.
VisorVR_Icon.svg is the hand-maintained vector twin of the same geometry; keep
the two in sync if you change proportions.
"""

from PIL import Image, ImageDraw

# --- geometry, in a 1024x1024 master ---
MASTER = 1024
BG_RADIUS = 224
BG_TOP = (0x1B, 0x1F, 0x2E)
BG_BOTTOM = (0x0F, 0x10, 0x13)
BG_BORDER = (0x2E, 0x33, 0x46)

VISOR_BOX = (152, 362, 872, 662)  # left, top, right, bottom
VISOR_RADIUS = 150
VISOR_TOP = (0x8F, 0x95, 0xFF)
VISOR_BOTTOM = (0x5C, 0x64, 0xE6)

NOTCH_BOX = (362, 605, 662, 795)  # ellipse carving the nose bridge
GLINT_BOX = (210, 405, 814, 465)
GLINT_RADIUS = 30

ICON_SIZES = [16, 20, 24, 30, 32, 36, 40, 48, 60, 64, 72, 80, 96, 128, 256]


def vertical_gradient(size, top, bottom):
    img = Image.new("RGB", (1, size[1]))
    for y in range(size[1]):
        t = y / max(size[1] - 1, 1)
        img.putpixel(
            (0, y),
            tuple(round(a + (b - a) * t) for a, b in zip(top, bottom)),
        )
    return img.resize(size, Image.NEAREST)


def render_master():
    canvas = Image.new("RGBA", (MASTER, MASTER), (0, 0, 0, 0))

    # Rounded background, gradient-filled via a mask.
    bg_mask = Image.new("L", (MASTER, MASTER), 0)
    ImageDraw.Draw(bg_mask).rounded_rectangle(
        (0, 0, MASTER - 1, MASTER - 1), radius=BG_RADIUS, fill=255
    )
    bg = vertical_gradient((MASTER, MASTER), BG_TOP, BG_BOTTOM).convert("RGBA")
    canvas.paste(bg, (0, 0), bg_mask)

    border = Image.new("RGBA", (MASTER, MASTER), (0, 0, 0, 0))
    ImageDraw.Draw(border).rounded_rectangle(
        (4, 4, MASTER - 5, MASTER - 5),
        radius=BG_RADIUS - 4,
        outline=BG_BORDER + (255,),
        width=8,
    )
    canvas.alpha_composite(border)

    # Visor: a rounded bar with an ellipse punched out of its lower edge.
    visor_mask = Image.new("L", (MASTER, MASTER), 0)
    vd = ImageDraw.Draw(visor_mask)
    vd.rounded_rectangle(VISOR_BOX, radius=VISOR_RADIUS, fill=255)
    vd.ellipse(NOTCH_BOX, fill=0)
    visor = vertical_gradient((MASTER, MASTER), VISOR_TOP, VISOR_BOTTOM).convert("RGBA")
    canvas.paste(visor, (0, 0), visor_mask)

    # Glint across the top of the visor, clipped to the visor itself.
    glint = Image.new("L", (MASTER, MASTER), 0)
    ImageDraw.Draw(glint).rounded_rectangle(
        GLINT_BOX, radius=GLINT_RADIUS, fill=70
    )
    glint = Image.composite(glint, Image.new("L", (MASTER, MASTER), 0), visor_mask)
    canvas.paste(Image.new("RGBA", (MASTER, MASTER), (255, 255, 255, 255)), (0, 0), glint)

    return canvas


def main():
    master = render_master()

    master.resize((256, 256), Image.LANCZOS).save(
        "icon.ico",
        sizes=[(s, s) for s in ICON_SIZES],
    )
    print("wrote icon.ico")

    def mark(px):
        return master.resize((px, px), Image.LANCZOS)

    # MSI dialog: white sheet with a light left gutter holding the mark.
    dialog = Image.new("RGB", (493, 312), "white")
    ImageDraw.Draw(dialog).rectangle((0, 0, 164, 312), fill="#eeeeee")
    m = mark(132)
    dialog.paste(m, (16, (312 - 132) // 2), m)
    dialog.save("WiXUIDialog.png")
    print("wrote WiXUIDialog.png")

    # MSI banner: white strip, mark on the right.
    banner = Image.new("RGB", (493, 58), "white")
    m = mark(40)
    banner.paste(m, (493 - 40 - 9, 9), m)
    banner.save("WiXUIBanner.png")
    print("wrote WiXUIBanner.png")


if __name__ == "__main__":
    main()
