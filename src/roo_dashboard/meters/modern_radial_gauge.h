#pragma once

#include <algorithm>

#include "roo_windows/core/number.h"
#include "roo_windows/core/widget.h"

namespace roo_dashboard {

/// A normalized value arc over a co-centered scale arc, both with round caps.
/// Angles are radians: zero points right, positive sweeps go counterclockwise.
/// The configured arc fits within the widget bounds, including stroke widths.
/// The center remains transparent; labels belong in the enclosing application.
class ModernRadialGauge : public roo_windows::Widget {
 public:
  /// Selects the Material 3 container/on-container pair or explicit colors.
  enum class Style : uint8_t {
    kPrimaryContainer,
    kSecondaryContainer,
    kTertiaryContainer,
    kErrorContainer,
    kCustom,
  };

  /// Creates a gauge with 8 dp scale and 4 dp value strokes. Finite values
  /// are clamped to [0, 1]; a nonfinite initial value becomes zero.
  explicit ModernRadialGauge(roo_windows::ApplicationContext& context,
                             float value = 0,
                             Style style = Style::kPrimaryContainer);

  /// Sets a normalized value, clamping finite inputs to [0, 1]. Returns false
  /// for nonfinite inputs without changing the current value.
  bool setValue(float value);

  /// Returns the normalized value; zero hides the value arc completely.
  float value() const { return value_; }

  /// Sets endpoint angles in radians, retaining their signed sweep. Rejects
  /// nonfinite angles and sweeps longer than one turn without making changes.
  /// The stored start is normalized to [-pi, pi]; an equal pair hides both
  /// arcs.
  bool setAngles(float start, float end);

  /// Returns the normalized start angle (default -pi/2).
  float startAngle() const { return start_angle_; }

  /// Returns the end angle (default pi/2), preserving the signed sweep.
  float endAngle() const { return end_angle_; }

  /// Sets the scale stroke width in logical dp, including fractional dp.
  void setScaleWidth(roo_windows::SmallNumber width);

  /// Returns the scale stroke width in dp.
  roo_windows::SmallNumber scaleWidth() const { return scale_width_; }

  /// Sets the value stroke width in logical dp, including fractional dp.
  void setValueWidth(roo_windows::SmallNumber width);

  /// Returns the value stroke width in dp.
  roo_windows::SmallNumber valueWidth() const { return value_width_; }

  /// Selects theme colors or the explicit custom pair.
  void setStyle(Style style);

  /// Returns the selected color style.
  Style style() const { return style_; }

  /// Selects kCustom and sets its scale/value colors. Before this is called,
  /// kCustom uses a light-gray scale and a black value arc.
  void setCustomColors(roo_display::Color scale, roo_display::Color value);

  /// Resolves the scale color on every paint. Overrides may depend on value()
  /// or application state; call invalidate() after changing external state.
  virtual roo_display::Color getScaleColor() const;

  /// Resolves the value color on every paint. Overrides may depend on value()
  /// or application state; call invalidate() after changing external state.
  virtual roo_display::Color getValueColor() const;

  /// Repaints the gauge and its ancestor backdrop after external color changes.
  void invalidate();

  /// Returns a nominal 96 dp square; the widget also fits rectangular layouts.
  roo_windows::Dimensions getSuggestedMinimumDimensions() const override;

  /// Requests repaint only where the last painted value or colors changed.
  /// Full invalidations are reconstructed by the ordinary ancestor pipeline.
  void paint(roo_windows::PaintContext& ctx) const override;

 protected:
  void paintWidgetContents(roo_windows::PaintContext& ctx) override;

  // Keep overlays available when a lower contributor repaints behind us.
  void emitPersistentDecoration(roo_windows::PaintContext& ctx) const override;

  roo_windows::Rect getDirectPaintExclusionBounds() const override {
    return roo_windows::Rect(0, 0, -1, -1);
  }

 private:
  float value_ = 0;
  float painted_value_ = 0;
  roo_display::Color painted_scale_color_ = roo_display::color::Transparent;
  roo_display::Color painted_value_color_ = roo_display::color::Transparent;
  bool painted_ = false;
  float start_angle_ = -1.5707963267948966f;
  float end_angle_ = 1.5707963267948966f;
  roo_windows::SmallNumber scale_width_ = 8;
  roo_windows::SmallNumber value_width_ = 4;
  roo_display::Color custom_scale_color_ = roo_display::color::LightGray;
  roo_display::Color custom_value_color_ = roo_display::color::Black;
  Style style_;
};

}  // namespace roo_dashboard
