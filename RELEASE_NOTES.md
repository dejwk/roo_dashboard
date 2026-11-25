# roo_dashboard 1.1.0

- Add `ModernRadialGauge` with rounded scale and value arcs, configurable sweep angles and stroke widths, Material 3 or custom colors, and a transparent center for composing labels. Value changes repaint only the affected area.
- Fix `RadialGauge` background painting and eliminate needle trails during updates. Add rendering regression tests for both gauges.
- Fix Arduino dependency discovery by adding top-level includes for libraries used through nested headers.
- Raise the minimum `roo_windows` version to 1.11.1 and migrate widgets to the updated theme and font APIs.
- Update `roo_testing` to 2.7.1 for local Bazel builds and 2.7.0 for CI. Add host profile configurations for Arduino, ESP-IDF, and Pico SDK, plus a runner for the nine-profile test matrix.

---

# roo_dashboard 1.0.2

- **Breaking API change:** Widget constructors now accept `roo_windows::ApplicationContext&`; pass `app.context()` when creating widgets.
- Require `roo_windows` 1.7.0 or newer and migrate widgets to its updated painting, theme, text-style, and layout APIs.
- Update Bazel to 9.2.0, `roo_testing` to 2.1.2, and GoogleTest to 1.18.0.bcr.1; add `rules_cc` 0.2.25.
- Adopt the Arduino ESP32 host build profile, centralize AddressSanitizer configuration, and modernize CI.
- Add usage and host-build documentation, umbrella-header guidance, and consolidated release notes.

---

# [roo_dashboard 1.0.1](https://github.com/dejwk/roo_dashboard/releases/tag/1.0.1)

Published 2025-11-01.

Updated CI, .gitignore.

---

# [roo_dashboard 1.0.0](https://github.com/dejwk/roo_dashboard/releases/tag/1.0.0)

Published 2025-10-19.



---

