#include "roo_dashboard/meters/radial_gauge.h"

#include <cmath>

#include "roo_display.h"
#include "roo_display/color/gradient.h"
#include "roo_display/core/offscreen.h"
#include "roo_display/filter/front_to_back_writer.h"
#include "roo_display/font/font.h"
#include "roo_display/shape/basic.h"
#include "roo_display/shape/smooth.h"
#include "roo_display/ui/string_printer.h"
#include "roo_display/ui/text_label.h"
#include "roo_smooth_fonts/NotoSans_Condensed/15.h"
#include "roo_windows/core/widget.h"

namespace roo_dashboard {

using namespace roo_display;
using namespace roo_windows;

static constexpr float kPi = 3.141592653589793238462643383279502884;

namespace {

struct Point {
  int16_t x;
  int16_t y;
};

const Point polarToCart(float deg, float radius, Point center) {
  return Point{
      .x = (int16_t)(center.x +
                     std::roundf(sin(deg * 2 * kPi / 360.0) * radius)),
      .y = (int16_t)(center.y +
                     std::roundf(-cos(deg * 2 * kPi / 360.0) * radius))};
}

const FpPoint polarToCartFp(float deg, float radius, FpPoint center) {
  return FpPoint{.x = center.x + sinf(deg * 2 * kPi / 360.0) * radius,
                 .y = center.y - cosf(deg * 2 * kPi / 360.0) * radius};
}

class GaugeBase : public Drawable {
 public:
  GaugeBase(const RadialGauge::Spec* spec) : spec_(spec) {}

  Box extents() const override { return spec_->extents; }

 private:
  void drawTo(const Surface& s) const override {
    float scale = spec_->deg_scale_end - spec_->deg_scale_start;
    float len = scale * (2 * kPi * spec_->radius + spec_->scale_width) / 360.0;
    int dividers = (int)(len / 3);
    float deg = spec_->deg_scale_start;
    Point center = {.x = spec_->x_center, .y = spec_->y_center};

    float first_divider =
        (int)(spec_->min_scale_value /
              (spec_->divider_spacing / spec_->ticks_per_divider)) *
        (spec_->divider_spacing / spec_->ticks_per_divider);
    int idx = (int)(spec_->min_scale_value /
                    (spec_->divider_spacing / spec_->ticks_per_divider)) %
              spec_->ticks_per_divider;
    float divider = first_divider;
    while (divider <= spec_->max_scale_value) {
      float deg = divider / (spec_->max_scale_value - spec_->min_scale_value) *
                      (spec_->deg_scale_end - spec_->deg_scale_start) +
                  spec_->deg_scale_start;
      int out_radius = spec_->radius + spec_->scale_width;
      if (idx == 0) {
        out_radius += 5;
        const Font& font = font_NotoSans_Condensed_15();
        TextLabel label(StringPrintf("%2.f", divider), font, color::Black);
        Point label_pos =
            polarToCart(deg, out_radius + font.metrics().ascent(), center);
        s.drawObject(label, label_pos.x - label.metrics().width() / 2,
                     label_pos.y + label.metrics().height() / 2);
      } else {
        out_radius -= 5;
      }
      Point p0 = polarToCart(deg, out_radius, center);
      Point p1 = polarToCart(deg, spec_->radius, center);
      s.drawObject(Line(p0.x, p0.y, p1.x, p1.y, color::Black));
      divider += (spec_->divider_spacing / spec_->ticks_per_divider);
      idx++;
      if (idx >= spec_->ticks_per_divider) idx = 0;
    }

    Point p0 = polarToCart(deg, spec_->radius + spec_->scale_width, center);
    Point p1 = polarToCart(deg, spec_->radius, center);
    float value = spec_->min_scale_value;
    for (int i = 1; i <= dividers; ++i) {
      float new_deg = spec_->deg_scale_start + (scale * i) / dividers;
      Point new_p0 =
          polarToCart(new_deg, spec_->radius + spec_->scale_width, center);
      Point new_p1 = polarToCart(new_deg, spec_->radius, center);
      float new_value =
          spec_->min_scale_value +
          (i * (spec_->max_scale_value - spec_->min_scale_value) / dividers);
      s.drawObject(Line(p1.x, p1.y, new_p1.x, new_p1.y, color::Black));
      s.drawObject(FilledTriangle(p0.x, p0.y, p1.x, p1.y, new_p0.x, new_p0.y,
                                  spec_->scale_color(value)));
      s.drawObject(FilledTriangle(new_p1.x, new_p1.y, p1.x, p1.y, new_p0.x,
                                  new_p0.y, spec_->scale_color(value)));
      p0 = new_p0;
      p1 = new_p1;
      deg = new_deg;
      value = new_value;
    }
  }

  const RadialGauge::Spec* spec_;
};

class Needle : public Drawable {
 public:
  Needle(Point center, int16_t full_radius, int16_t thick_radius, float deg,
         Color color)
      : center_{(float)center.x, (float)center.y},
        tip_(polarToCartFp(deg, full_radius - 1, center_)),
        color_(color) {}

  Box extents() const {
    return SmoothWedgedLine(center_, 15, tip_, 2, color_).extents();
  }

 private:
  void drawTo(const Surface& s) const override {
    s.drawObject(SmoothWedgedLine(center_, 15, tip_, 2, color_));
  }

  Point p0_, p1_, p2_, p3_, p4_;
  FpPoint center_, tip_;
  Color color_;
};

}  // namespace

Dimensions RadialGauge::getSuggestedMinimumDimensions() const {
  return Dimensions((spec_.radius + spec_.scale_width) * 2,
                    (spec_.radius + spec_.scale_width) * 2);
}

void RadialGauge::paintWidgetContents(const Canvas& canvas, Clipper& clipper) {
  Widget::paintWidgetContents(canvas, clipper);
  previous_value_ = current_value_;
}

void RadialGauge::paint(const Canvas& canvas) const {
  GaugeBase base(&spec_);
  if (isInvalidated()) {
    canvas.drawObject(roo_display::Border(this->bounds().asBox(),
                                          base.extents(), canvas.bgcolor()));
  }
  Canvas my_canvas(canvas);
  my_canvas.clipToExtents(base.extents());
  if (my_canvas.clip_box().empty()) return;
  int16_t needle_radius = spec_.radius - 2;
  auto center = kCenter.toLeft().shiftBy(spec_.x_center + spec_.face_x_offset) |
                kMiddle.toTop().shiftBy(spec_.y_center + spec_.face_y_offset);
  if (isInvalidated()) {
    DrawingContext dc(my_canvas);
    dc.setFillMode(roo_display::FILL_MODE_VISIBLE);
    dc.setWriteOnce();
    dc.draw(
        FilledCircle::ByRadius(spec_.x_center, spec_.y_center, 7, color::Red));
    if (face_ != nullptr) {
      dc.draw(*face_, center);
    }
    dc.draw(base);
    Needle needle(Point{.x = spec_.x_center, .y = spec_.y_center},
                  needle_radius, needle_radius - 8, currentDeg(), color::Red);
    dc.draw(needle);
  } else {
    if (previous_value_ == current_value_) return;
    Needle old_needle(Point{.x = spec_.x_center, .y = spec_.y_center},
                      needle_radius, needle_radius - 8, previousDeg(),
                      color::Red);
    Needle needle(Point{.x = spec_.x_center, .y = spec_.y_center},
                  needle_radius, needle_radius - 8, currentDeg(), color::Red);
    {
      DrawingContext dc(my_canvas);
      dc.setFillMode(roo_display::FILL_MODE_VISIBLE);
      dc.draw(needle);
    }
    Box extents = old_needle.extents();
    roo_display::BitMaskOffscreen bitmask(extents, color::Black);

    DrawingContext mask_dc(bitmask);
    // Erase the old needle from the mask.
    mask_dc.erase(old_needle);
    // But mask back the new needle as we don't want it overwritten.
    mask_dc.draw(needle);
    // Also mask back the center circle.
    mask_dc.draw(
        FilledCircle::ByRadius(spec_.x_center, spec_.y_center, 7, color::Red));

    // Now, the clip mask passes the pixels of the old needle that are not
    // obstructed by the new needle.
    ClipMask mask(bitmask.buffer(),
                  bitmask.extents().translate(my_canvas.dx(), my_canvas.dy()));
    ClipMaskFilter filter(canvas.out(), &mask);
    my_canvas.set_out(&filter);
    if (face_ != nullptr) {
      auto offset =
          center.resolveOffset(Box(0, 0, 159, 127), face_->anchorExtents());
      DrawingContext dc(my_canvas);
      dc.draw(*face_, center);
      mask_dc.draw(*face_, offset.dx, offset.dy);
    }
    my_canvas.clearRect(extents);
  }
}

void RadialGauge::setValue(float value) {
  if (current_value_ == value) return;
  current_value_ = value;
  markDirty();
}

void RadialGauge::setFace(const roo_display::Drawable* face) {
  if (face_ == face) return;
  face_ = face;
  invalidateInterior();
}

void RadialGauge::setBounds(const Box& bounds) {
  if (spec_.extents == bounds) return;
  spec_.extents = bounds;
  invalidateInterior();
}

void RadialGauge::setCenter(int16_t x, int16_t y) {
  if (spec_.x_center == x && spec_.y_center == y) return;
  spec_.x_center = x;
  spec_.y_center = y;
  invalidateInterior();
}

void RadialGauge::setRangeAngles(float deg_scale_start, float deg_scale_end) {
  setRangeAngles(deg_scale_start, deg_scale_end, deg_scale_start - 5,
                 deg_scale_end + 5);
}

void RadialGauge::setRangeAngles(float deg_scale_start, float deg_scale_end,
                                 float deg_needle_start, float deg_needle_end) {
  if (spec_.deg_scale_start == deg_scale_start &&
      spec_.deg_scale_end == deg_scale_end &&
      spec_.deg_needle_start == deg_needle_start &&
      spec_.deg_needle_end == deg_needle_end) {
    return;
  }
  spec_.deg_scale_start = deg_scale_start;
  spec_.deg_scale_end = deg_scale_end;
  spec_.deg_needle_start = deg_needle_start;
  spec_.deg_needle_end = deg_needle_end;
  invalidateInterior();
}

void RadialGauge::setScaleRadius(float radius) {
  if (spec_.radius == radius) return;
  spec_.radius = radius;
  invalidateInterior();
}

void RadialGauge::setScaleWidth(int16_t width) {
  if (spec_.scale_width == width) return;
  spec_.scale_width = width;
  invalidateInterior();
}

void RadialGauge::setValueRange(float min_scale_value, float max_scale_value) {
  if (spec_.min_scale_value == min_scale_value &&
      spec_.max_scale_value == max_scale_value) {
    return;
  }
  spec_.min_scale_value = min_scale_value;
  spec_.max_scale_value = max_scale_value;
  invalidateInterior();
}

void RadialGauge::setDividers(float spacing, int16_t subdivision) {
  if (spec_.divider_spacing == spacing &&
      spec_.ticks_per_divider == subdivision) {
    return;
  }
  spec_.divider_spacing = spacing;
  spec_.ticks_per_divider = subdivision;
  invalidateInterior();
}

void RadialGauge::setScaleColoring(
    std::function<roo_display::Color(float)> coloring) {
  spec_.scale_color = coloring;
  invalidateInterior();
}

namespace {
float valToDeg(const RadialGauge::Spec& spec, float val) {
  float deg =
      spec.deg_scale_start + (val - spec.min_scale_value) /
                                 (spec.max_scale_value - spec.min_scale_value) *
                                 (spec.deg_scale_end - spec.deg_scale_start);
  if (deg < spec.deg_needle_start) deg = spec.deg_needle_start;
  if (deg > spec.deg_needle_end) deg = spec.deg_needle_end;
  return deg;
}
}  // namespace

float RadialGauge::currentDeg() const {
  return valToDeg(spec_, current_value_);
}

float RadialGauge::previousDeg() const {
  return valToDeg(spec_, previous_value_);
}

}  // namespace roo_dashboard