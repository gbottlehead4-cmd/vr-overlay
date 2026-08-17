# VisorVR brand assets

`VisorVR_Icon.svg` is the mark. `make-assets.py` renders the raster assets the
build and installer consume:

| File              | Used by                                     |
| ----------------- | ------------------------------------------- |
| `icon.ico`        | app + utility executables (`Resource.rc`)   |
| `WiXUIBanner.png` | MSI installer banner                        |
| `WiXUIDialog.png` | MSI installer dialog                        |

Regenerate after changing the mark:

```
python make-assets.py
```

The script draws the geometry itself, so it needs only Pillow — no ImageMagick.
`VisorVR_Icon.svg` is a hand-maintained vector twin of the same geometry; keep
the proportions in the two in sync.

VisorVR is a fork of OpenKneeboard. None of the artwork here is inherited from
it: OpenKneeboard's logos are by Paul "Goldwolf" Whittingham and were removed
rather than modified.
