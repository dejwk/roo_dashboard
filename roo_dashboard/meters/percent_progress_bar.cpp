#include "percent_progress_bar.h"

#include <cmath>

#include "roo_display/core/color.h"
#include "roo_display/core/rasterizable.h"
#include "roo_display/filter/background.h"
#include "roo_windows/core/theme.h"
#include "roo_smooth_fonts/NotoSans_Regular/12.h"
#include "roo_smooth_fonts/NotoSans_Regular/18.h"

using namespace roo_display;
using namespace roo_windows;

namespace roo_dashboard {

namespace {

class BarRaster : public roo_display::Rasterizable {
 public:
  BarRaster(roo_display::Box extents, Color complete, Color incomplete,
            int16_t threshold)
      : extents_(std::move(extents)),
        complete_(complete),
        incomplete_(incomplete),
        threshold_(threshold) {}

  void ReadColors(const int16_t* x, const int16_t* y, uint32_t count,
                  Color* result) const override {
    while (count-- > 0) {
      *result++ = *x++ < threshold_ ? complete_ : incomplete_;
    }
  }

  bool ReadColorRect(int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax,
                     Color* result) const override {
    if (xMin >= extents_.xMin() && xMax <= extents_.xMax() &&
        yMin >= extents_.yMin() && yMax <= extents_.yMax()) {
      if (xMax < threshold_) {
        *result = complete_;
        return true;
      }
      if (xMin >= threshold_) {
        *result = incomplete_;
        return true;
      }
    }
    // Fall back to the slow path.
    return Rasterizable::ReadColorRect(xMin, yMin, xMax, yMax, result);
  }

  roo_display::Box extents() const override { return extents_; }

 private:
  roo_display::Box extents_;
  Color complete_;
  Color incomplete_;
  int16_t threshold_;
};

Color defaultIncompleteColor(const Theme& theme, Color complete) {
  complete.set_a(theme.state.disabled);
  return complete;
}

}  // namespace

BaseProgressBar::BaseProgressBar(const roo_windows::Environment& env)
    : roo_windows::VerticalLayout(env),
      progress_(0),
      complete_(env.theme().color.secondary),
      incomplete_(defaultIncompleteColor(env.theme(), complete_)) {}

void BaseProgressBar::setColor(roo_display::Color color) {
  complete_ = color;
  incomplete_ = defaultIncompleteColor(theme(), color);
}

PreferredSize BaseProgressBar::getPreferredSize() const {
return PreferredSize(PreferredSize::MatchParentWidth(),
                        PreferredSize::WrapContentHeight());
}

void BaseProgressBar::setColors(roo_display::Color complete,
                                roo_display::Color incomplete) {
  complete_ = complete;
  incomplete_ = incomplete;
}

void BaseProgressBar::paintWidgetContents(const Canvas& canvas,
                                          Clipper& clipper) {
  if (!isDirty()) {
    Panel::paintWidgetContents(canvas, clipper);
    return;
  }
  Canvas my_canvas(canvas);
  if (progress_ == 0) {
    my_canvas.set_bgcolor(alphaBlend(canvas.bgcolor(), incomplete_));
    Panel::paintWidgetContents(my_canvas, clipper);
    return;
  }
  if (progress_ == 1024) {
    my_canvas.set_bgcolor(alphaBlend(canvas.bgcolor(), complete_));
    Panel::paintWidgetContents(my_canvas, clipper);
    return;
  }

  BarRaster bar(Box(canvas.dx(), canvas.dy(), width() + canvas.dx() - 1,
                    height() + canvas.dy() - 1),
                alphaBlend(canvas.bgcolor(), complete_),
                alphaBlend(canvas.bgcolor(), incomplete_),
                (uint32_t)progress_ * width() / 1024 + canvas.dx());
  BackgroundFilter filter(my_canvas.out(), &bar);
  my_canvas.set_out(&filter);
  my_canvas.set_bgcolor(color::Background);
  Panel::paintWidgetContents(my_canvas, clipper);
}

PercentProgressBar::PercentProgressBar(const roo_windows::Environment& env)
    : BaseProgressBar(env),
      percent_(env, "0%%", *env.theme().font.button, kMiddle | kCenter) {
  setGravity(Gravity(kHorizontalGravityCenter, kVerticalGravityMiddle));
  percent_.setPadding(PADDING_NONE);
  percent_.setMargins(MARGIN_NONE);
  setPadding(PADDING_TINY);
  add(percent_);
}

void PercentProgressBar::updateChildren() {
  percent_.setTextf("%d%%", progress_ * 100 / 1024);
}

}  // namespace roo_dashboard
