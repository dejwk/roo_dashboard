#pragma once

#include <functional>
#include <string>

#include "roo_windows/containers/vertical_layout.h"
#include "roo_windows/core/canvas.h"
#include "roo_windows/core/panel.h"
#include "roo_windows/core/preferred_size.h"
#include "roo_windows/core/widget.h"
#include "roo_windows/widgets/text_label.h"

namespace roo_dashboard {

class BaseProgressBar : public roo_windows::VerticalLayout {
 public:
  BaseProgressBar(const roo_windows::Environment& env);

  void setProgress(uint16_t progress) {
    if (progress < 0) progress = 0;
    if (progress > 1024) progress = 1024;
    if (progress == progress_) return;
    uint16_t pixel_threshold_old = (uint32_t)progress_ * width() / 1024;
    uint16_t pixel_threshold_new = (uint32_t)progress * width() / 1024;
    progress_ = progress;
    updateChildren();
    if (pixel_threshold_old != pixel_threshold_new) {
      invalidateInterior(roo_windows::Rect(
          std::min(pixel_threshold_old, pixel_threshold_new), 0,
          std::max(pixel_threshold_old, pixel_threshold_new) - 1,
          height() - 1));
    }
  }

  roo_windows::PreferredSize getPreferredSize() const override;

  // Set the 'complete' color to the specified color, and the 'incomplete' color
  // to the same color with translucency.
  void setColor(roo_display::Color color);

  void setColors(roo_display::Color complete, roo_display::Color incomplete);

  void paintWidgetContents(const roo_windows::Canvas& canvas,
                           roo_windows::Clipper& clipper) override;

 protected:
  virtual void updateChildren() {}
  uint16_t progress_;  // 0-1024 (corresponding to 0-100%).
  roo_display::Color complete_;
  roo_display::Color incomplete_;
};

// A progress bar that goes from 0% to 100%, showing percentage in the middle of
// the bar. You can adjust padding, font, and colors of the bar.
class PercentProgressBar : public BaseProgressBar {
 public:
  PercentProgressBar(const roo_windows::Environment& env);

  void setFont(const roo_display::Font& font);
  // void setGravity(roo_windows::HorizontalGravity gravity);

 protected:
  void updateChildren() override;

 private:
  roo_windows::TextLabel percent_;
};

}  // namespace roo_dashboard
