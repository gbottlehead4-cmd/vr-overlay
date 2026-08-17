# VisorVR

Put web dashboards, reference pages and notes **inside your headset**, and place
them with your mouse while you're wearing it. Built for sim racing, and equally
at home in flight sims.

The thing that makes VisorVR different: **in-VR panel positioning**. Grab a panel
with the mouse, scroll to resize or push it away, hold Shift to tilt. Where you
put it is where it stays, across restarts.

VisorVR is a fork of [OpenKneeboard](https://github.com/OpenKneeboard/OpenKneeboard)
by Fred Emmott.

## Install

Download the portable zip, unpack it anywhere, and run **VisorVR.exe**.

There is no installer and you don't need administrator rights. VisorVR registers
its OpenXR layer for your user account on launch, so you can keep it on a USB
stick or move the folder later — it re-registers itself next time it starts.

Keep the folder together: `bin\VisorVRApp.exe` finds Chromium in `libexec` and
its data in `share` by looking beside its own folder, so `bin` on its own won't
work.

### "Windows protected your PC"

You will see this the first time you run it, and it does not mean anything is
wrong with the download:

> Windows protected your PC
> Microsoft Defender SmartScreen prevented an unrecognised app from starting.

Click **More info**, then **Run anyway**.

SmartScreen shows this for any application that isn't code-signed by a
certificate it already trusts. Certificates cost money every year, and VisorVR
is free and unfunded, so it is not signed. The trade-off is this warning. If you
would rather verify the download yourself, the full source is in this repository
and you can build it from scratch — see below.

Some antivirus products may also flag VisorVR. It draws overlays by loading a
layer into the running game, which is the same technique some malware uses, so
heuristics occasionally object. It is a false positive; report it to your AV
vendor if you hit it.

## Uninstall

Turn off **Settings → Virtual Reality → OpenXR support for 64-bit games**, then
delete the folder. The toggle removes the registry entry; without it you'd leave
one behind.

Your settings live in `%LOCALAPPDATA%\VisorVR` and are not removed with the
folder — delete that too if you want a clean sweep.

## Building

Windows, Visual Studio 2022 (Community — the Build Tools edition lacks the
packaging tasks WinUI needs), and CMake.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  "-DCMAKE_GENERATOR_INSTANCE=C:/Program Files/Microsoft Visual Studio/2022/Community" ^
  "-DCMAKE_TOOLCHAIN_FILE=%CD%/third-party/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
  "-DVCPKG_TARGET_TRIPLET=x64-windows-visorvr"
cmake --build build --config RelWithDebInfo --parallel 1
powershell -ExecutionPolicy Bypass -File scripts\make-portable.ps1
```

The first configure builds every dependency and takes a while. Use
`--parallel 1` if you hit `C1076` — the large XAML translation unit is memory
hungry.

## Support

VisorVR is free, and stays free. If it earns its place on your rig you can chip
in at [ko-fi.com/gidrux](https://ko-fi.com/gidrux) — entirely optional.

Bugs and questions: [Issues](https://github.com/gbottlehead4-cmd/vr-overlay/issues).

## License

See [LICENSE](LICENSE) — the OpenKneeboard Public License v1. It is derived from
the GPL v2 and is **not** the GPL: it additionally requires that anyone
distributing a modified build replaces the branding and unique identifiers,
which is why this fork is called VisorVR and not OpenKneeboard.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.

Third-party libraries are used and distributed under their own license terms;
their licence texts ship in `share\doc`.
