# roo_dashboard

Dashboard UI components built on the Roo display and windowing stack.

Requires roo_windows 1.10.0 or newer. Construct widgets with the application
context, for example `roo_dashboard::Thermometer thermometer(app.context());`.

## Modern radial gauge

`ModernRadialGauge` draws a rounded scale arc with a co-centered value arc on
its centerline. The value is normalized to 0–1; finite values are clamped and
nonfinite values are rejected. Zero hides the value arc. There are no ticks,
markers or numeric labels. The center and surrounding area preserve the
ancestor's background, so applications can compose a separate label with it.

```cpp
#include "roo_dashboard/meters/modern_radial_gauge.h"

roo_dashboard::ModernRadialGauge gauge(app.context());
gauge.setValue(0.65f);
gauge.setScaleWidth(12);     // dp, stored as roo_windows::SmallNumber
gauge.setValueWidth(7.5f);   // fractional dp, scaled by ROO_WINDOWS_ZOOM
gauge.setAngles(3.14159265f, 0);  // upper semicircle, left to right
app.add(gauge, roo_display::Box(0, 0, 159, 79));
```

Angles use radians with zero pointing right and positive angles counterclockwise.
Defaults are −π/2 to π/2 (a right semicircle), with 8 dp and 4 dp strokes. The
configured arc and its round caps fit the widget bounds; both arcs retain the
same center and centerline radius regardless of their widths or the current
value. Signed sweeps of up to one full turn are supported. An equal start/end
pair draws nothing; layouts too small for the configured strokes also draw
nothing.

`ModernRadialGauge::Style` offers `kPrimaryContainer` (default),
`kSecondaryContainer`, `kTertiaryContainer`, `kErrorContainer`, and `kCustom`.
Theme styles use the corresponding container color for the scale and
on-container color for the value. `setCustomColors(scale, value)` selects
`kCustom`. Both `getScaleColor()` and `getValueColor()` are virtual and may use
`value()` to choose dynamic colors; call `invalidate()` when external inputs
used by an override change. Colors are resolved from the active theme on each
paint. The widget keeps no bitmap cache and composes its arcs through the
windowing framework.

## Host emulation

Host builds use the roo_testing 2.0 Arduino ESP32 profile. With Bazelisk 1.21
or newer, a plain command defaults to that profile and prints a notice:

    bazel test ...
    bazel test ... --config=asan
    bazel test ... --config=roo_testing_arduino_esp32

The files under .roo_testing/bazelrc/esp32 are vendored from roo_testing;
follow their canonical-source headers when refreshing them.
