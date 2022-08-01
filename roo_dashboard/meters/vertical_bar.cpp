#include "vertical_bar.h"

#include <cmath>

#include "roo_display/core/color.h"
#include "roo_display/ui/string_printer.h"
#include "roo_smooth_fonts/NotoSans_Regular/12.h"
#include "roo_smooth_fonts/NotoSans_Regular/18.h"

using namespace roo_display;
using namespace roo_windows;

namespace roo_dashboard {

bool VerticalBar::Indicator::paint(const Surface& s) {
  // Figure out the region that needs to be redrawn.
  Box clip_box = bounds();
  if (!isInvalidated()) {
    if (value_ > previous_value_) {
      clip_box = Box(previous_value_ + 1, 0, value_, height() - 1);
    } else {
      clip_box = Box(value_ + 1, 0, previous_value_, height() - 1);
    }
    if (color_ != previous_color_) {
      // The box must include everything towards zero.
      if (clip_box.xMin() > 1) {
        clip_box = Box(1, 0, clip_box.xMax(), height() - 1);
      }
      if (clip_box.xMax() < -1) {
        clip_box = Box(clip_box.xMin(), 0, -1, height() - 1);
      }
    }
  }
  Surface news = s;
  if (news.clipToExtents(clip_box) != Box::CLIP_RESULT_EMPTY) {
    Color bar_color = alphaBlend(background(), color_);
    Color zero_color = alphaBlend(background(), theme().color.onSurface);
    news.drawObject(FilledRect(0, 0, std::min(value_, zero_offset_) - 1,
                               height() - 1, color::Transparent));
    if (value_ < zero_offset_) {
      news.drawObject(
          FilledRect(value_, 0, zero_offset_ - 1, height() - 1, bar_color));
    }
    news.drawObject(
        Line(zero_offset_, 0, zero_offset_, height() - 1, zero_color));
    if (value_ > zero_offset_) {
      news.drawObject(
          FilledRect(zero_offset_ + 1, 0, value_, height() - 1, bar_color));
    }
    news.drawObject(FilledRect(std::max(value_, zero_offset_) + 1, 0,
                               width() - 1, height() - 1, color::Transparent));
  }
  previous_color_ = color_;
  previous_value_ = value_;
  return true;
}

void VerticalBar::Indicator::setValue(float value) {
  if (std::isnan(value)) {
    value_ = 0;
    setEnabled(false);
    return;
  }
  // If we were nan (disabled) before, this will also make us dirty.
  setEnabled(true);
  int16_t new_value = (int16_t)(value * scale_) + zero_offset_;
  Color new_color = color_fn_(new_value);
  if (new_value != value_ || new_color != color_) {
    previous_value_ = value_;
    previous_color_ = color_;
    value_ = new_value;
    color_ = new_color;
    markDirty();
  }
}

VerticalBar::VerticalBar(const roo_windows::Environment& env, float scale,
                         int16_t zero_offset,
                         std::function<roo_display::Color(float val)> color_fn,
                         std::string title, std::string caption_template,
                         float initial_value)
    : Panel(env),
      title_(env, std::move(title), font_NotoSans_Regular_12(), kLeft | kBottom),
      indicator_(env, scale, zero_offset, color_fn, initial_value),
      caption_(env, "", font_NotoSans_Regular_18(), kLeft | kTop),
      caption_template_(std::move(caption_template)) {
  add(title_);
  add(indicator_);
  add(caption_);
}

void VerticalBar::setValue(float value) {
  if (value_ == value || (std::isnan(value_) && std::isnan(value))) return;
  value_ = value;
  indicator_.setValue(value);
  caption_.setContent(
      StringPrinter::sprintf(caption_template_.c_str(), value_));
  caption_.setVisibility(std::isnan(value_) ? GONE : VISIBLE);
}

Dimensions VerticalBar::onMeasure(MeasureSpec width, MeasureSpec height) {
  Dimensions title = title_.measure(width, MeasureSpec::Unspecified(18));
  indicator_.measure(width, MeasureSpec::Unspecified(25));
  caption_.measure(MeasureSpec::Unspecified(0), MeasureSpec::Unspecified(0));
  std::string test_str = StringPrinter::sprintf(caption_template_.c_str(),
                                                (500.0 / indicator_.scale()));
  int16_t preferred_width =
      caption_.font().getHorizontalStringMetrics(test_str).width();
  Dimensions preferred(
      std::max(preferred_width, title.width()) + indicator_.zero_offset(),
      title_.font().metrics().maxHeight() + 25 +
          caption_.font().metrics().maxHeight());
  return Dimensions(width.resolveSize(preferred.width()),
                    height.resolveSize(preferred.height()));
}

void VerticalBar::onLayout(bool changed, const roo_display::Box& box) {
  int16_t bar_height = box.height() - title_.font().metrics().maxHeight() -
                       caption_.font().metrics().maxHeight();
  int16_t y_min = 0;
  title_.layout(Box(indicator_.zero_offset(), y_min, box.width() - 1,
                    y_min + title_.font().metrics().maxHeight() - 1));
  y_min += title_.font().metrics().maxHeight();
  indicator_.layout(Box(0, y_min, box.width() - 1, y_min + bar_height - 1));
  y_min += bar_height;
  caption_.layout(
      Box(indicator_.zero_offset(), y_min, box.xMax() - 1, box.yMax() - 1));
}

}  // namespace roo_dashboard
