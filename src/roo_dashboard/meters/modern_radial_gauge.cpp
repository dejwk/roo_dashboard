#include "roo_dashboard/meters/modern_radial_gauge.h"

#include <algorithm>
#include <cmath>

#include "roo_display/shape/smooth.h"
#include "roo_windows/core/theme.h"

namespace roo_dashboard {
namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTurn = 2 * kPi;

struct ArcGeometry {
  roo_display::FpPoint center;
  float radius;
};

// Finds the arc centerline bounds from its endpoints and included cardinal
// directions. Stroke/cap padding is independent of the current value so that
// both arcs retain exactly the same center and radius during updates.
ArcGeometry FitArc(int width, int height, float start, float sweep,
                   float stroke) {
  float min_x = std::min(std::cos(start), std::cos(start + sweep));
  float max_x = std::max(std::cos(start), std::cos(start + sweep));
  float min_y = std::min(-std::sin(start), -std::sin(start + sweep));
  float max_y = std::max(-std::sin(start), -std::sin(start + sweep));
  for (int i = 0; i < 4; ++i) {
    float angle = i * (kPi * 0.5f);
    float distance = std::fmod((angle - start) * (sweep < 0 ? -1 : 1), kTurn);
    if (distance < 0) distance += kTurn;
    if (distance > std::fabs(sweep) + 0.000001f) continue;
    min_x = std::min(min_x, std::cos(angle));
    max_x = std::max(max_x, std::cos(angle));
    min_y = std::min(min_y, -std::sin(angle));
    max_y = std::max(max_y, -std::sin(angle));
  }
  float available_x = width - stroke - 2;
  float available_y = height - stroke - 2;
  if (available_x <= 0 || available_y <= 0) return {};
  // Bound nearly zero-length arcs as well: a huge fitted radius would lose
  // float precision and exceed the rasterizer's integer coordinate range.
  float radius = std::min<float>(8192, std::max(width, height));
  if (max_x > min_x) radius = std::min(radius, available_x / (max_x - min_x));
  if (max_y > min_y) radius = std::min(radius, available_y / (max_y - min_y));
  if (radius < stroke * 0.5f) return {};
  return {{(width - 1 - radius * (min_x + max_x)) * 0.5f,
           (height - 1 - radius * (min_y + max_y)) * 0.5f},
          radius};
}

// Translates mathematical angles to the rasterizer's clockwise-from-up angles.
roo_display::SmoothShape ArcShape(const ArcGeometry& geometry, float start,
                                  float sweep, float width,
                                  roo_display::Color color) {
  if (width <= 0 || sweep == 0) return {};
  float native_start = kPi * 0.5f - (sweep > 0 ? start + sweep : start);
  native_start = std::remainder(native_start, kTurn);
  return roo_display::SmoothThickArc(
      geometry.center, geometry.radius, width, native_start,
      native_start + std::fabs(sweep), color, roo_display::ENDING_ROUNDED);
}

void AddArc(roo_windows::PaintContext& ctx, const ArcGeometry& geometry,
            float start, float sweep, float width, roo_display::Color color) {
  if (width <= 0 || sweep == 0 || color.a() == 0) return;
  ctx.addOverlayShape(ArcShape(geometry, start, sweep, width, color));
}

// Unions nonempty ink bounds without letting an empty origin enlarge damage.
roo_windows::Rect UnionInk(const roo_windows::Rect& a,
                           const roo_display::Box& b) {
  if (b.empty()) return a;
  return a.empty() ? roo_windows::Rect(b)
                   : roo_windows::Rect::Extent(a, roo_windows::Rect(b));
}

}  // namespace

ModernRadialGauge::ModernRadialGauge(roo_windows::ApplicationContext& context,
                                     float value, Style style)
    : Widget(context), style_(style) {
  setValue(value);
}

bool ModernRadialGauge::setValue(float value) {
  if (!std::isfinite(value)) return false;
  value = std::max(0.0f, std::min(1.0f, value));
  if (value_ == value) return true;

  const float previous_value = value_;
  const roo_display::Color previous_scale_color = getScaleColor();
  const roo_display::Color previous_value_color = getValueColor();
  value_ = value;

  const float sweep = end_angle_ - start_angle_;
  if (bounds().empty() || sweep == 0) return true;
  const float scale_width =
      roo_windows::Scaled(static_cast<float>(scale_width_));
  const float value_width =
      roo_windows::Scaled(static_cast<float>(value_width_));
  const ArcGeometry geometry = FitArc(width(), height(), start_angle_, sweep,
                                      std::max(scale_width, value_width));
  if (geometry.radius <= 0) return true;

  const roo_display::Color scale_color = getScaleColor();
  const roo_display::Color value_color = getValueColor();
  roo_windows::Rect damage(0, 0, -1, -1);
  if (previous_scale_color != scale_color &&
      (previous_scale_color.a() != 0 || scale_color.a() != 0)) {
    damage = UnionInk(damage, ArcShape(geometry, start_angle_, sweep,
                                       scale_width, roo_display::color::White)
                                  .extents());
  }
  if (previous_value_color.a() != 0 || value_color.a() != 0) {
    // A color change affects the visible prefix; otherwise only the interval
    // between endpoints can change. Arc extents include round caps and AA.
    const float first = previous_value_color == value_color
                            ? std::min(previous_value, value_)
                            : 0;
    const float last =
        std::max(previous_value_color.a() != 0 ? previous_value : 0,
                 value_color.a() != 0 ? value_ : 0);
    damage = UnionInk(damage, ArcShape(geometry, start_angle_ + sweep * first,
                                       sweep * (last - first), value_width,
                                       roo_display::color::White)
                                  .extents());
  }
  damage = roo_windows::Rect::Intersect(damage, bounds());
  if (!damage.empty()) invalidateInterior(damage);
  return true;
}

bool ModernRadialGauge::setAngles(float start, float end) {
  float sweep = end - start;
  if (!std::isfinite(start) || !std::isfinite(end) || !std::isfinite(sweep) ||
      std::fabs(sweep) > kTurn) {
    return false;
  }
  start = std::remainder(start, kTurn);
  end = start + sweep;
  if (start_angle_ == start && end_angle_ == end) return true;
  start_angle_ = start;
  end_angle_ = end;
  invalidate();
  return true;
}

void ModernRadialGauge::setScaleWidth(roo_windows::SmallNumber width) {
  if (scale_width_ == width) return;
  scale_width_ = width;
  invalidate();
}

void ModernRadialGauge::setValueWidth(roo_windows::SmallNumber width) {
  if (value_width_ == width) return;
  value_width_ = width;
  invalidate();
}

void ModernRadialGauge::setStyle(Style style) {
  if (style_ == style) return;
  style_ = style;
  invalidate();
}

void ModernRadialGauge::setCustomColors(roo_display::Color scale,
                                        roo_display::Color value) {
  if (style_ == Style::kCustom && custom_scale_color_ == scale &&
      custom_value_color_ == value) {
    return;
  }
  custom_scale_color_ = scale;
  custom_value_color_ = value;
  style_ = Style::kCustom;
  invalidate();
}

roo_display::Color ModernRadialGauge::getScaleColor() const {
  const auto& colors = theme().material3Theme().color;
  switch (style_) {
    case Style::kPrimaryContainer:
      return colors.primaryContainer;
    case Style::kSecondaryContainer:
      return colors.secondaryContainer;
    case Style::kTertiaryContainer:
      return colors.tertiaryContainer;
    case Style::kErrorContainer:
      return colors.errorContainer;
    case Style::kCustom:
      return custom_scale_color_;
  }
  return custom_scale_color_;
}

roo_display::Color ModernRadialGauge::getValueColor() const {
  const auto& colors = theme().material3Theme().color;
  switch (style_) {
    case Style::kPrimaryContainer:
      return colors.onPrimaryContainer;
    case Style::kSecondaryContainer:
      return colors.onSecondaryContainer;
    case Style::kTertiaryContainer:
      return colors.onTertiaryContainer;
    case Style::kErrorContainer:
      return colors.onErrorContainer;
    case Style::kCustom:
      return custom_value_color_;
  }
  return custom_value_color_;
}

void ModernRadialGauge::invalidate() { invalidateInterior(); }

void ModernRadialGauge::invalidateInterior() {
  invalidateInterior(getInteriorBounds());
}

void ModernRadialGauge::invalidateInterior(const roo_windows::Rect& rect) {
  Widget::invalidateInterior(rect);
  notifyParentInvalidatedRegion(rect.translate(offsetLeft(), offsetTop()));
}

roo_windows::Dimensions ModernRadialGauge::getSuggestedMinimumDimensions()
    const {
  return {roo_windows::Scaled(96), roo_windows::Scaled(96)};
}

void ModernRadialGauge::emitPersistentDecoration(
    roo_windows::PaintContext& ctx) const {
  roo_windows::PaintContext clipped = ctx.clipped(bounds());
  const float sweep = end_angle_ - start_angle_;
  const float scale_width =
      roo_windows::Scaled(static_cast<float>(scale_width_));
  const float value_width =
      roo_windows::Scaled(static_cast<float>(value_width_));
  if (clipped.empty() || sweep == 0) return;
  ArcGeometry geometry = FitArc(width(), height(), start_angle_, sweep,
                                std::max(scale_width, value_width));
  if (geometry.radius <= 0) return;
  // Always register current foreground before the scale, even on clean frames.
  // The ancestor composites these over the actual background in one write.
  AddArc(clipped, geometry, start_angle_, sweep * value_, value_width,
         getValueColor());
  AddArc(clipped, geometry, start_angle_, sweep, scale_width, getScaleColor());
}

}  // namespace roo_dashboard
