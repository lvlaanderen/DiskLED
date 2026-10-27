# DiskLED
DiskLED is a small tool that displays Windows performance counter data using an animated system tray icon.

DiskLED comes with a graphical configuration dialog that lets you choose from all performance counters available on the local system – you can monitor network throughput just as easily as hard disk activity or memory usage.

## Documentation and Release Builds

This is a fork of [DiskLED by Helge Klein](https://github.com/helgeklein/DiskLED) with more icon types, colours, a second counter, warnings and other improvements, see below. Thank you, Helge, for the original.

- **Download this version:** [releases](https://github.com/lvlaanderen/DiskLED/releases). Unzip it to a folder where you can write, DiskLED stores its configuration next to the executable. Use the `x64` zip on 64-bit Windows, otherwise the `x86` one. Earlier versions stay available on the same page.
- **The original:** documentation and release builds of the original DiskLED can be found [here](https://helgeklein.com/free-tools/diskled/).

Both versions are licensed under the Apache License 2.0, see `LICENSE`.

## Settings

Everything below can be set in the config dialog (double-click the tray icon). The settings are stored in `DiskLED.ini` next to the executable when **Update the INI file when applying settings** is ticked. The dialog has four tabs (data source, scale and values, appearance, alerts) and a live preview of the icon, animated with a test signal. It follows the Windows app theme (dark or light). **Quick pick** on the first tab sets up common data sources in one step: internet download and upload speed, disk activity, processor use and memory use. Choosing a counter fills in the value format, the scale and the 100% value for you. You can change all of them, and the 100% field accepts units such as `350 Mbit/s`.

### Icon types

`LEDType` in the INI:

- `Flicker`: the LED. A rectangle in the low colour whose brightness follows the load.
- `ColoredBar`: a Task Manager style bar with a dotted background.
- `CleanBar`: a plain bar rising from the bottom.
- `VUMeter`: stacked segments, lit up to the value and coloured by height. Unlit segments are dimmed.
- `Graph`: the last 16 values as bars, with a dim line at 50%.
- `Number`: the current value as text with a one-letter unit below it (B/K/M/G for bytes, b/k/m/g for bits, % for percent).

### Counter settings

These are stored per counter, in `[Performance Counter 1]` and `[Performance Counter 2]`:

- `Path`: the counter path. Written in English, so the INI file works on any Windows language. Paths in the system language from older versions still work.
- `100% Utilization`: the counter value that corresponds to 100%.
- `ValueFormat`: how values are shown in the tooltip and in the number icon: `Percent` (default), `Number`, `Bytes` (counter is bytes/s, shown as B/s, KB/s, MB/s), `Bits` (bytes/s shown as bit/s, kbit/s, Mbit/s) or `MBytes` (counter is in megabytes, shown as MB/GB/TB; with a fixed scale the tooltip also shows the total).
- `ScaleMode`: what corresponds to the full icon. `Fixed` (default): the 100% value. `Peak`: the highest of the last 16 values. `Decay`: a peak that falls by half every 30 seconds. `Auto`: the highest value seen so far, learned over time and kept in the INI as `LearnedMax`; it falls by half in a day, and for network counters it never exceeds the speed of the adapter. With `Peak`, `Decay` and `Auto` the `100% Utilization` value is the minimum scale. (Older versions used `AutoScale=1`, which is read as `Peak`.)
- `Invert`: `1` shows the 100% value minus the counter value, for example memory used from `\Memory\Available MBytes`.
- `AlertAbove` and `AlertSeconds`: show a notification when the value stays above this percentage of the scale (the value that fills the icon) for this many seconds. `0` turns the warning off. It is shown once per period above the threshold.

### Settings for all counters

In `[Performance Counter 1]`:

- `LEDType`: the icon type, see above.
- `UpdateInterval`: milliseconds between updates, 10 to 3600000. Use about 1000 for the graph.
- `SecondCounter`: `1` shows a second counter. The bar types show the two counters side by side in one icon and the graph one above the other. The number is too small to share, so it gets two tray icons, one per counter, each with its own unit. The tooltip lists both counters.
- `ColorLow`, `ColorMid`, `ColorHigh`: the colours as `RRGGBB` hex, for example `00DC00`.
- `ThresholdMid`, `ThresholdHigh`: the percentage of the scale where the mid and the high colour start (defaults 50 and 75). The LED only uses the low colour.

### Start with Windows

The **Start with Windows** checkbox adds or removes a per-user entry under `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` for this copy of the executable (no admin rights needed). Each copy, in its own folder with its own INI, gets its own entry. If you move the executable, untick and re-tick the option from the new location. An entry that was disabled in Task Manager's Startup tab shows as unticked, ticking it enables it again.

### Errors

If the counters cannot be read, for example because a network adapter disappeared or Windows has not finished starting, DiskLED shows an error icon and the problem in the tooltip, shows one notification, and tries to reconnect every 5 seconds. Applying settings in the dialog with a counter path that does not work shows a message, keeps the previous settings running and leaves the dialog open.

## Examples

Internet download and upload speed, one icon with two counters:

```ini
[Performance Counter 1]
LEDType=VUMeter
UpdateInterval=1000
SecondCounter=1
Path=\Network Adapter(<your adapter>)\Bytes Received/sec
100% Utilization=43750000
ValueFormat=Bits
ScaleMode=Decay

[Performance Counter 2]
Path=\Network Adapter(<your adapter>)\Bytes Sent/sec
100% Utilization=12500000
ValueFormat=Bits
ScaleMode=Decay
```

`100% Utilization` is in bytes per second, so for a 350 Mbit/s line it is 350000000 / 8 = 43750000.

Memory used, with a warning above 90% for one minute:

```ini
[Performance Counter 1]
LEDType=Number
UpdateInterval=1000
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
