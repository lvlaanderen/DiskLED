# DiskLED
DiskLED is a small tool that displays Windows performance counter data using animated system tray icons.

DiskLED comes with a graphical settings window that lets you choose from all performance counters available on the local system – you can monitor network throughput just as easily as hard disk activity or memory usage. One DiskLED shows as many **meters** as you like, each as its own icon in the tray.

## Documentation and Release Builds

This is a fork of [DiskLED by Helge Klein](https://github.com/helgeklein/DiskLED) with several meters in one program, more icon types, colours, warnings and other improvements, see below. Thank you, Helge, for the original.

- **Download this version:** [releases](https://github.com/lvlaanderen/DiskLED/releases). Earlier versions stay available on the same page. Use the `x64` files on 64-bit Windows, otherwise the `x86` ones.
  - **Setup:** `DiskLED-<version>-<x64 or x86>-Setup.exe` installs DiskLED for all users (Program Files) or for you only, in a folder of your choice, and can start it with Windows.
  - **Zip:** `DiskLED-<version>-<x64 or x86>.zip` is for portable use or if you do not want an installer. Unzip it to a folder where you can write. It contains a file called `portable`, so the settings are stored next to the program, see below.
- **The original:** documentation and release builds of the original DiskLED can be found [here](https://helgeklein.com/free-tools/diskled/).

Both versions are licensed under the Apache License 2.0, see `LICENSE`.

## Settings

Everything below can be set in the settings window (double-click a tray icon, or start DiskLED again). **Add meter** starts a wizard in three steps: what to monitor (internet download or upload speed, disk activity, processor use, memory, or any other counter), which network adapter, disk or processor, and how it should look, with a preview. A meter can have a name, which is shown in its tooltip and in its warnings.

The meters are listed on the left. The settings of the selected meter are on the right, on four tabs (data source, scale and values, appearance, alerts), with a live preview of the icon, animated with a test signal. **Quick pick** on the first tab sets up common data sources in one step. Choosing a counter fills in the value format, the scale and the 100% value for you. You can change all of them, and the 100% field accepts units such as `350 Mbit/s`. The window follows the Windows app theme (dark or light).

Changes are shown on the tray icons right away, but they are only kept when you press **Save**. **Revert** goes back to what was saved, and closing the window with unsaved changes asks what to do. The title shows a `*` while there are unsaved changes.

### Where the settings are

The settings are stored in `%AppData%\DiskLED\DiskLED.ini`, so DiskLED works from a folder that you cannot write to, such as Program Files. The first time version 3 starts, a `DiskLED.ini` next to the program (from an older version) is copied there. The old file is renamed to `DiskLED.ini.migrated` if that is possible.

**Portable mode:** if a file called `portable` (or `portable.txt`) is next to `DiskLED.exe`, the settings are stored in `DiskLED.ini` next to the program instead, and nothing is copied. The zip download contains this file. Delete it to use `%AppData%`.

### The INI file

The file can be edited by hand while DiskLED is not running. Settings that do not belong to a meter are in `[General]`, every meter has a section `[Meter 1]`, `[Meter 2]` and so on. The number of a meter stays the same when other meters are removed. There can be up to 16 meters. Files of version 2.1 and older (`[Performance Counter 1]` and `[Performance Counter 2]`) are still read: a file with a second counter becomes two meters, and the old sections are replaced when you save.

`[General]`:

- `UpdateInterval`: milliseconds between updates for all meters, 10 to 3600000. Use about 1000 for the graph.

`[Meter n]`:

- `Name`: an optional name, up to 32 characters.
- `LEDType`: the icon type, see below.
- `ColorLow`, `ColorMid`, `ColorHigh`: the colours as `RRGGBB` hex, for example `00DC00`.
- `ThresholdMid`, `ThresholdHigh`: the percentage of the scale where the mid and the high colour start (defaults 50 and 75). The LED only uses the low colour.
- `Path`: the counter path. Written in English, so the INI file works on any Windows language. Paths in the system language from older versions still work.
- `100% Utilization`: the counter value that corresponds to 100%.
- `ValueFormat`: how values are shown in the tooltip and in the number icon: `Percent` (default), `Number`, `Bytes` (counter is bytes/s, shown as B/s, KB/s, MB/s), `Bits` (bytes/s shown as bit/s, kbit/s, Mbit/s) or `MBytes` (counter is in megabytes, shown as MB/GB/TB; with a fixed scale the tooltip also shows the total).
- `ScaleMode`: what corresponds to the full icon. `Fixed` (default): the 100% value. `Peak`: the highest of the last 16 values. `Decay`: a peak that falls by half every 30 seconds. `Auto`: the highest value seen so far, learned over time and kept in the INI as `LearnedMax`; it falls by half in a day, and for network counters it never exceeds the speed of the adapter. With `Peak`, `Decay` and `Auto` the `100% Utilization` value is the minimum scale. (Older versions used `AutoScale=1`, which is read as `Peak`.)
- `Invert`: `1` shows the 100% value minus the counter value, for example memory used from `\Memory\Available MBytes`.
- `AlertAbove` and `AlertSeconds`: show a notification when the value stays above this percentage of the scale (the value that fills the icon) for this many seconds. `0` turns the warning off. It is shown once per period above the threshold.

### Icon types

`LEDType` in the INI:

- `Flicker`: the LED. A rectangle in the low colour whose brightness follows the load.
- `ColoredBar`: a Task Manager style bar with a dotted background.
- `CleanBar`: a plain bar rising from the bottom.
- `VUMeter`: stacked segments, lit up to the value and coloured by height. Unlit segments are dimmed.
- `Graph`: the last 16 values as bars, with a dim line at 50%.
- `Number`: the current value as text with a one-letter unit below it (B/K/M/G for bytes, b/k/m/g for bits, % for percent).

### Tray icons

Every meter is its own icon. Windows may put new icons in the overflow area first: drag them to the visible part of the tray once, Windows remembers where you put them. Windows remembers an icon by the program and the number of the meter, so after moving DiskLED to another folder the icons are new again.

### Start with Windows

The **Start with Windows** checkbox adds or removes a per-user entry under `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` for the program (no admin rights needed). There is one entry, because only one DiskLED runs per user: starting it again opens the settings of the running one. If you move the program, untick and re-tick the option from the new location. An entry that was disabled in Task Manager's Startup tab shows as unticked, ticking it enables it again. The entries that version 2 made for each copy of the program are replaced.

### Errors

If the counters cannot be read, for example because a network adapter disappeared or Windows has not finished starting, the meter shows an error icon and the problem in the tooltip, shows one notification, and tries again every 5 seconds. A meter with a problem does not affect the other meters.

## Upgrading from version 2

Version 2 needed one folder, one program and one INI file for every counter. In version 3:

- If you used a single DiskLED, install version 3 or unzip it over the old folder. Your `DiskLED.ini` is read, with a second counter becoming a second meter.
- If you used several copies, install or unzip version 3 once and add a meter for each counter with the wizard. The INI files of the other copies are not imported. Remove their entries from Start with Windows (they are replaced by a single entry), and delete the old folders when you are done.

## Examples

Internet download and upload speed, as two meters with a graph:

```ini
[General]
UpdateInterval=1000

[Meter 1]
Name=Download
LEDType=Graph
Path=\Network Interface(<your adapter>)\Bytes Received/sec
100% Utilization=43750000
ValueFormat=Bits
ScaleMode=Decay

[Meter 2]
Name=Upload
LEDType=Graph
Path=\Network Interface(<your adapter>)\Bytes Sent/sec
100% Utilization=12500000
ValueFormat=Bits
ScaleMode=Decay
```

`100% Utilization` is in bytes per second, so for a 350 Mbit/s line it is 350000000 / 8 = 43750000.

Memory used, with a warning above 90% for one minute:

```ini
[Meter 1]
LEDType=Number
Path=\Memory\Available MBytes
100% Utilization=65536
ValueFormat=MBytes
Invert=1
AlertAbove=90
AlertSeconds=60
```

## Building

Open `DiskLED.sln` in Visual Studio 2022 with the **Desktop development with C++** workload and the **C++ MFC** component, or build from a developer command prompt:

```
msbuild DiskLED.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0
```

### Release packages

`installer\Build-Release.ps1` builds the x64 and x86 programs and makes the zip files in `dist\`. If [Inno Setup 6](https://jrsoftware.org/isinfo.php) is installed it also makes the setup programs (`installer\DiskLED.iss`):

```
powershell -ExecutionPolicy Bypass -File installer\Build-Release.ps1 -Version 3.0
```
