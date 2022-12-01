// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_strip.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/adapters.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/cxx17_backports.h"
#include "base/feature_list.h"
#include "base/i18n/rtl.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/numerics/safe_conversions.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "base/stl_util.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_group_theme.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_types.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_container_impl.h"
#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"
#include "chrome/browser/ui/views/tabs/tab_group_header.h"
#include "chrome/browser/ui/views/tabs/tab_group_highlight.h"
#include "chrome/browser/ui/views/tabs/tab_group_underline.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_controller.h"
#include "chrome/browser/ui/views/tabs/tab_slot_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout_types.h"
#include "chrome/browser/ui/views/tabs/tab_strip_observer.h"
#include "chrome/browser/ui/views/tabs/tab_strip_types.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "chrome/browser/ui/views/tabs/z_orderable_tab_container_element.h"
#include "chrome/browser/ui/views/touch_uma/touch_uma.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/tab_groups/tab_group_color.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "compound_tab_container.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/pathops/SkPathOps.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_constants.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom.h"
#include "ui/base/interaction/element_identifier.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/list_selection_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/color/color_provider.h"
#include "ui/display/display.h"
#include "ui/gfx/animation/throb_animation.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/range/range.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/cascading_property.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/interaction/element_tracker_views.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/rect_based_targeting_utils.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_model_utils.h"
#include "ui/views/view_observer.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/windows_version.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/win/hwnd_util.h"
#include "ui/views/win/hwnd_util.h"
// begin Add by TangramTeam
#include "third_party/webruntime/UniverseForChromium.h"
extern CommonUniverse::CWebRTImpl* g_pWebRTImpl;  // 20200108
// end Add by TangramTeam
#endif

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

namespace {

ui::mojom::DragEventSource EventSourceFromEvent(const ui::LocatedEvent& event) {
  return event.IsGestureEvent() ? ui::mojom::DragEventSource::kTouch
                                : ui::mojom::DragEventSource::kMouse;
}

std::unique_ptr<TabContainer> MakeTabContainer(
    TabStrip* tab_strip,
    TabHoverCardController* hover_card_controller,
    TabDragContext* drag_context) {
  if (base::FeatureList::IsEnabled(features::kSplitTabStrip)) {
    return std::make_unique<CompoundTabContainer>(
        raw_ref<TabContainerController>(*tab_strip), hover_card_controller,
        drag_context, *tab_strip, tab_strip);
  }
  return std::make_unique<TabContainerImpl>(
      *tab_strip, hover_card_controller, drag_context, *tab_strip, tab_strip);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// TabStrip::TabDragContextImpl
//
class TabStrip::TabDragContextImpl : public TabDragContext,
                                     public views::BoundsAnimatorObserver {
 public:
  METADATA_HEADER(TabDragContextImpl);
  explicit TabDragContextImpl(TabStrip* tab_strip)
      : tab_strip_(tab_strip), bounds_animator_(this) {
    SetCanProcessEventsWithinSubtree(false);

    bounds_animator_.AddObserver(this);
  }
  // If a window is closed during a drag session, all our tabs will be taken
  // from us before our destructor is even called.
  ~TabDragContextImpl() override = default;

  void Layout() override { SetBoundsRect(parent()->GetLocalBounds()); }

  bool OnMouseDragged(const ui::MouseEvent& event) override {
    ContinueDrag(this, event);
    return true;
  }

  void OnMouseReleased(const ui::MouseEvent& event) override {
    EndDrag(END_DRAG_COMPLETE);
  }

  void OnMouseCaptureLost() override { EndDrag(END_DRAG_CAPTURE_LOST); }

  void OnGestureEvent(ui::GestureEvent* event) override {
    switch (event->type()) {
      case ui::ET_GESTURE_SCROLL_END:
      case ui::ET_SCROLL_FLING_START:
      case ui::ET_GESTURE_END:
        EndDrag(END_DRAG_COMPLETE);
        break;

      case ui::ET_GESTURE_LONG_TAP: {
        EndDrag(END_DRAG_CANCEL);
        break;
      }

      case ui::ET_GESTURE_SCROLL_UPDATE:
        ContinueDrag(this, *event);
        break;

      case ui::ET_GESTURE_TAP_DOWN:
        EndDrag(END_DRAG_CANCEL);
        break;

      default:
        break;
    }
    event->SetHandled();

    // TabDragContext gets event capture as soon as a drag session begins, which
    // precludes TabStrip from ever getting events like tap or long tap. Forward
    // this on to TabStrip so it can respond to those events.
    tab_strip_->OnGestureEvent(event);
  }

  bool IsDragStarted() const {
    return drag_controller_ && drag_controller_->started_drag();
  }

  void TabWasAdded() {
    if (drag_controller_)
      drag_controller_->TabWasAdded();
  }

  void OnTabWillBeRemoved(content::WebContents* contents) {
    if (drag_controller_)
      drag_controller_->OnTabWillBeRemoved(contents);
  }

  bool CanRemoveTabIfDragging(content::WebContents* contents) const {
    return drag_controller_ ? drag_controller_->CanRemoveTabDuringDrag(contents)
                            : true;
  }

  void MaybeStartDrag(TabSlotView* source,
                      const ui::LocatedEvent& event,
                      const ui::ListSelectionModel& original_selection) {
    std::vector<TabSlotView*> dragging_views;
    int x = source->GetMirroredXInView(event.x());
    int y = event.y();

    // Build the set of selected tabs to drag and calculate the offset from the
    // source.
    ui::ListSelectionModel selection_model;
    if (source->GetTabSlotViewType() ==
        TabSlotView::ViewType::kTabGroupHeader) {
      dragging_views.push_back(source);

      const gfx::Range grouped_tabs =
          tab_strip_->controller_->ListTabsInGroup(source->group().value());
      for (auto index = grouped_tabs.start(); index < grouped_tabs.end();
           ++index) {
        dragging_views.push_back(GetTabAt(index));
        // Set |selection_model| if and only if the original selection does not
        // match the group exactly. See TabDragController::Init() for details
        // on how |selection_model| is used.
        if (!original_selection.IsSelected(index))
          selection_model = original_selection;
      }
      if (grouped_tabs.length() != original_selection.size())
        selection_model = original_selection;
    } else {
      for (int i = 0; i < GetTabCount(); ++i) {
        Tab* other_tab = GetTabAt(i);
        if (tab_strip_->IsTabSelected(other_tab)) {
          dragging_views.push_back(other_tab);
          if (other_tab == source)
            x += GetSizeNeededForViews(dragging_views) - other_tab->width();
        }
      }
      if (!original_selection.IsSelected(tab_strip_->GetModelIndexOf(source)))
        selection_model = original_selection;
    }

    DCHECK(!dragging_views.empty());
    DCHECK(base::Contains(dragging_views, source));

    // Delete the existing DragController before creating a new one. We do this
    // as creating the DragController remembers the WebContents delegates and we
    // need to make sure the existing DragController isn't still a delegate.
    drag_controller_.reset();

    DCHECK(event.type() == ui::ET_MOUSE_PRESSED ||
           event.type() == ui::ET_GESTURE_TAP_DOWN ||
           event.type() == ui::ET_GESTURE_SCROLL_BEGIN);

    drag_controller_ = std::make_unique<TabDragController>();
    drag_controller_->Init(this, source, dragging_views, gfx::Point(x, y),
                           event.x(), std::move(selection_model),
                           EventSourceFromEvent(event));
    if (drag_controller_set_callback_)
      std::move(drag_controller_set_callback_).Run(drag_controller_.get());
  }

  void ContinueDrag(views::View* view, const ui::LocatedEvent& event) {
    if (drag_controller_.get() &&
        drag_controller_->event_source() == EventSourceFromEvent(event)) {
      gfx::Point screen_location(event.location());
      views::View::ConvertPointToScreen(view, &screen_location);

      // Note: |tab_strip_| can be destroyed during drag, also destroying
      // |this|.
      base::WeakPtr<TabDragContext> weak_ptr(weak_factory_.GetWeakPtr());
      drag_controller_->Drag(screen_location);

      if (!weak_ptr)
        return;
    }
  }

  bool EndDrag(EndDragReason reason) {
    if (!drag_controller_.get())
      return false;
    bool started_drag = drag_controller_->started_drag();
    drag_controller_->EndDrag(reason);
    return started_drag;
  }

  bool IsTabStripCloseable() const {
    // Allow the close in two scenarios:
    // . The user is not actively dragging the tabstrip.
    // . In the process of reverting the drag, and the last tab is being
    //   removed (so that it can be inserted back into the source tabstrip).
    return !IsDragSessionActive() ||
           drag_controller_->IsRemovingLastTabForRevert();
  }

  // TabDragContext:
  Tab* GetTabAt(int i) const override { return tab_strip_->tab_at(i); }

  int GetIndexOf(const TabSlotView* view) const override {
    return tab_strip_->GetModelIndexOf(view);
  }

  int GetTabCount() const override { return tab_strip_->GetTabCount(); }

  bool IsTabPinned(const Tab* tab) const override {
    return tab_strip_->IsTabPinned(tab);
  }

  int GetPinnedTabCount() const override {
    return tab_strip_->GetModelPinnedTabCount();
  }

  TabGroupHeader* GetTabGroupHeader(
      const tab_groups::TabGroupId& group) const override {
    return tab_strip_->group_header(group);
  }

  TabStripModel* GetTabStripModel() override {
    return static_cast<BrowserTabStripController*>(
               tab_strip_->controller_.get())
        ->model();
  }

  TabDragController* GetDragController() override {
    return drag_controller_.get();
  }

  void OwnDragController(
      std::unique_ptr<TabDragController> controller) override {
    DCHECK(controller);
    DCHECK(!drag_controller_);
    drag_controller_ = std::move(controller);
    if (drag_controller_set_callback_)
      std::move(drag_controller_set_callback_).Run(drag_controller_.get());
  }

  void DestroyDragController() override {
    // begin Add by TangramTeam
    // HWND hwnd = NULL;
    // if (tab_strip_->GetWidget())
    //  hwnd = views::HWNDForWidget(tab_strip_->GetWidget());
    // if (::IsWindow(hwnd) && (::GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD))
    // {
    //  SetNewTabButtonVisible(false);
    //} else  // end Add by TangramTeam
    //  SetNewTabButtonVisible(true);
    drag_controller_.reset();
  }

  std::unique_ptr<TabDragController> ReleaseDragController() override {
    return std::move(drag_controller_);
  }

  void SetDragControllerCallbackForTesting(
      base::OnceCallback<void(TabDragController*)> callback) override {
    drag_controller_set_callback_ = std::move(callback);
  }

  void UpdateAnimationTarget(TabSlotView* tab_slot_view,
                             const gfx::Rect& target_bounds) override {
    if (bounds_animator_.IsAnimating(tab_slot_view))
      bounds_animator_.SetTargetBounds(tab_slot_view, target_bounds);
  }

  bool IsDragSessionActive() const override {
    return drag_controller_ != nullptr;
  }

  bool IsEndingDrag() const override {
    // The drag is ending if we're animating tabs back to the TabContainer, or
    // if the TabDragController is in the kStopped state.
    return (drag_controller_ == nullptr && bounds_animator_.IsAnimating()) ||
           (drag_controller_ && !drag_controller_->active());
  }

  void FinishEndingDrag() override {
    // Finishing animations will return tabs to the TabContainer via
    // ResetDraggingStateDelegate::AnimationEnded.
    bounds_animator_.Complete();
  }

  bool IsActiveDropTarget() const override {
    for (int i = 0; i < GetTabCount(); ++i) {
      const Tab* const tab = GetTabAt(i);
      if (tab->dragging())
        return true;
    }
    return false;
  }

  int GetActiveTabWidth() const override {
    return tab_strip_->GetActiveTabWidth();
  }

  int GetTabDragAreaWidth() const override {
    // There are two cases here (with tab scrolling enabled):
    // 1) If the tab strip is not wider than the tab strip region (and thus
    // not scrollable), returning the available width for tabs rather than the
    // actual width for tabs allows tabs to be dragged past the current bounds
    // of the tabstrip, anywhere along the tab strip region.
    // N.B. The available width for tabs in this case needs to ignore tab
    // closing mode.
    // 2) If the tabstrip is wider than the tab strip region (and thus is
    // scrollable), returning the tabstrip width allows tabs to be dragged
    // anywhere within the tabstrip, not just in the leftmost region of it.
    return std::max(
        tab_strip_->tab_container_->GetAvailableWidthForTabContainer(),
        tab_strip_->width());
  }

  int TabDragAreaBeginX() const override {
    return tab_strip_->GetMirroredXWithWidthInView(0, GetTabDragAreaWidth());
  }

  int TabDragAreaEndX() const override {
    return TabDragAreaBeginX() + GetTabDragAreaWidth();
  }

  int GetHorizontalDragThreshold() const override {
    constexpr int kHorizontalMoveThreshold = 16;  // DIPs.

    double ratio = static_cast<double>(tab_strip_->GetInactiveTabWidth()) /
                   TabStyle::GetStandardWidth();
    return base::ClampRound(ratio * kHorizontalMoveThreshold);
  }

  int GetInsertionIndexForDraggedBounds(
      const gfx::Rect& dragged_bounds,
      std::vector<TabSlotView*> dragged_views,
      int num_dragged_tabs,
      absl::optional<tab_groups::TabGroupId> group) const override {
    // If the strip has no tabs, the only position to insert at is 0.
    if (!GetTabCount())
      return 0;

    absl::optional<int> index;
    // If we're dragging a group by its header, the first element of
    // |dragged_views| is a group header, and the second one is the first tab
    // in that group.
    int first_dragged_tab_index = group.has_value() ? 1 : 0;
    if (static_cast<size_t>(first_dragged_tab_index) >= dragged_views.size()) {
      // TODO(tbergquist): This shouldn't happen, but we're getting crashes
      // that indicate that it might be anyways. This logging might help
      // narrow down exactly which cases it's happening in.
      NOTREACHED()
          << "Calculating a drag insertion index from invalid dependencies: "
          << "Dragging a group: " << group.has_value()
          << ", dragged_views.size(): " << dragged_views.size()
          << ", num_dragged_tabs: " << num_dragged_tabs;
    } else {
      int first_dragged_tab_model_index =
          tab_strip_->GetModelIndexOf(dragged_views[first_dragged_tab_index]);
      index =
          CalculateInsertionIndex(dragged_bounds, first_dragged_tab_model_index,
                                  num_dragged_tabs, std::move(group));
    }

    if (!index) {
      const int last_tab_right =
          tab_strip_->tab_container_->GetIdealBounds(GetTabCount() - 1).right();
      index = (dragged_bounds.right() > last_tab_right) ? GetTabCount() : 0;
    }

    const Tab* last_visible_tab = tab_strip_->GetLastVisibleTab();
    int last_insertion_point =
        last_visible_tab ? (GetIndexOf(last_visible_tab) + 1) : 0;

    // Clamp the insertion point to keep it within the visible region.
    last_insertion_point = std::max(0, last_insertion_point - num_dragged_tabs);

    // Ensure the first dragged tab always stays in the visible index range.
    return std::min(*index, last_insertion_point);
  }

  std::vector<gfx::Rect> CalculateBoundsForDraggedViews(
      const std::vector<TabSlotView*>& views) override {
    DCHECK(!views.empty());

    std::vector<gfx::Rect> bounds;
    const int overlap = TabStyle::GetTabOverlap();
    int x = 0;
    for (const TabSlotView* view : views) {
      const int width = view->width();
      bounds.emplace_back(x, 0, width, view->height());
      x += width - overlap;
    }

    return bounds;
  }

  void SetBoundsForDrag(const std::vector<TabSlotView*>& views,
                        const std::vector<gfx::Rect>& bounds) override {
    tab_strip_->tab_container_->CancelAnimation();
    DCHECK_EQ(views.size(), bounds.size());
    for (size_t i = 0; i < views.size(); ++i)
      views[i]->SetBoundsRect(bounds[i]);

    // Ensure that the tab strip and its parent views are correctly re-laid out
    // after repositioning dragged tabs. This avoids visual/layout issues such
    // as https://crbug.com/1151092.
    tab_strip_->PreferredSizeChanged();

    // Reset the layout size as we've effectively laid out a different size.
    // This ensures a layout happens after the drag is done.
    tab_strip_->tab_container_->InvalidateIdealBounds();
    if (views.at(0)->group().has_value())
      tab_strip_->tab_container_->UpdateTabGroupVisuals(
          views.at(0)->group().value());
  }

  void StartedDragging(const std::vector<TabSlotView*>& views) override {
    // Let the controller know that the user started dragging tabs.
    tab_strip_->controller_->OnStartedDragging(
        views.size() == static_cast<size_t>(tab_strip_->GetModelCount()));

    // No tabs should be dragging at this point.
    for (int i = 0; i < GetTabCount(); ++i)
      DCHECK(!GetTabAt(i)->dragging());

    tab_strip_->tab_container_->CompleteAnimationAndLayout();

    for (TabSlotView* dragged_view : views) {
      AddChildView(dragged_view);
      dragged_view->set_dragging(true);
    }

    // If this is a header drag, start painting the group highlight.
    TabGroupHeader* header = views::AsViewClass<TabGroupHeader>(views[0]);
    if (header) {
      tab_strip_->tab_container_->GetGroupViews()[header->group().value()]
          ->highlight()
          ->SetVisible(true);
      // Make sure the bounds of the group views are up to date right now
      // instead of waiting for subsequent drag events - if we are dragging a
      // window by a group header, we won't get any more events. See
      // https://crbug.com/1344774.
      tab_strip_->tab_container_->UpdateTabGroupVisuals(
          header->group().value());
    }

    tab_strip_->tab_container_->SetTabSlotVisibility();
    tab_strip_->SchedulePaint();
  }

  void DraggedTabsDetached() override {
    // Let the controller know that the user is not dragging this tabstrip's
    // tabs anymore.
    tab_strip_->controller_->OnStoppedDragging();
  }

  void StoppedDragging(const std::vector<TabSlotView*>& views) override {
    // Let the controller know that the user stopped dragging tabs.
    tab_strip_->controller_->OnStoppedDragging();

    // Animate the dragged views to their ideal positions. We'll hand them back
    // to TabContainer when the animation ends.
    for (TabSlotView* view : views) {
      gfx::Rect ideal_bounds;

      TabGroupHeader* header = views::AsViewClass<TabGroupHeader>(view);
      if (header) {
        // Disable the group highlight now that the drag is ended.
        tab_strip_->tab_container_->GetGroupViews()[header->group().value()]
            ->highlight()
            ->SetVisible(false);
        ideal_bounds =
            tab_strip_->tab_container_->GetIdealBounds(header->group().value());
      } else {
        ideal_bounds = tab_strip_->tab_container_->GetIdealBounds(
            tab_strip_->GetModelIndexOf(view));
      }

      bounds_animator_.AnimateViewTo(
          view, ideal_bounds,
          std::make_unique<ResetDraggingStateDelegate>(
              *tab_strip_->tab_container_, *view));
    }
  }

  void LayoutDraggedViewsAt(const std::vector<TabSlotView*>& views,
                            TabSlotView* source_view,
                            const gfx::Point& location,
                            bool initial_drag) override {
    std::vector<gfx::Rect> bounds = CalculateBoundsForDraggedViews(views);
    DCHECK_EQ(views.size(), bounds.size());

    int active_tab_model_index = GetIndexOf(source_view);
    int active_tab_index = static_cast<int>(
        std::find(views.begin(), views.end(), source_view) - views.begin());
    for (size_t i = 0; i < views.size(); ++i) {
      TabSlotView* view = views[i];
      gfx::Rect new_bounds = bounds[i];
      new_bounds.Offset(location.x(), location.y());
      int consecutive_index =
          active_tab_model_index - (active_tab_index - static_cast<int>(i));
      // If this is the initial layout during a drag and the tabs aren't
      // consecutive animate the view into position. Do the same if the tab is
      // already animating (which means we previously caused it to animate).
      if ((initial_drag && GetIndexOf(views[i]) != consecutive_index) ||
          bounds_animator_.IsAnimating(views[i])) {
        bounds_animator_.SetTargetBounds(views[i], new_bounds);
      } else {
        view->SetBoundsRect(new_bounds);
      }
    }
    tab_strip_->tab_container_->SetTabSlotVisibility();
    // The rightmost tab may have moved, which would change the tabstrip's
    // preferred width.
    tab_strip_->PreferredSizeChanged();

    // If the dragged tabs are in a group, we need to update the bounds of the
    // corresponding underline and header.
    if (views[0]->group()) {
      tab_strip_->tab_container_->UpdateTabGroupVisuals(
          views[0]->group().value());
    }
  }

  // Forces the entire tabstrip to lay out.
  void ForceLayout() override {
    tab_strip_->InvalidateLayout();
    tab_strip_->tab_container_->CompleteAnimationAndLayout();
  }

  void PaintChildren(const views::PaintInfo& paint_info) override {
    std::vector<ZOrderableTabContainerElement> orderable_children;
    for (views::View* child : children())
      orderable_children.emplace_back(child);

    // Sort in ascending order by z-value. Stable sort breaks ties by child
    // index.
    std::stable_sort(orderable_children.begin(), orderable_children.end());

    for (const ZOrderableTabContainerElement& child : orderable_children)
      child.view()->Paint(paint_info);
  }

  void OnBoundsAnimatorProgressed(views::BoundsAnimator* animator) override {}
  void OnBoundsAnimatorDone(views::BoundsAnimator* animator) override {
    // Send the Container a message to simulate a mouse moved event at the
    // current mouse position. This tickles the Tab the mouse is currently over
    // to show the "hot" state of the close button, or to show the hover card,
    // etc.  Note that this is not required (and indeed may crash!) during a
    // drag session.
    if (!IsDragSessionActive()) {
      // The widget can apparently be null during shutdown.
      views::Widget* widget = GetWidget();
      if (widget)
        widget->SynthesizeMouseMoveEvent();
    }
  }

 private:
  // Animates tabs after a drag has ended, then hands them back to
  // |tab_container_|.
  class ResetDraggingStateDelegate : public gfx::AnimationDelegate {
   public:
    ResetDraggingStateDelegate(TabContainer& tab_container,
                               TabSlotView& slot_view)
        : tab_container_(tab_container), slot_view_(slot_view) {
      slot_view_->set_animating(true);
    }
    ResetDraggingStateDelegate(const ResetDraggingStateDelegate&) = delete;
    ResetDraggingStateDelegate& operator=(const ResetDraggingStateDelegate&) =
        delete;
    ~ResetDraggingStateDelegate() override = default;

    void AnimationProgressed(const gfx::Animation* animation) override {
      tab_container_->OnTabSlotAnimationProgressed(&*slot_view_);
    }

    void AnimationEnded(const gfx::Animation* animation) override {
      AnimationProgressed(animation);
      slot_view_->set_animating(false);
      slot_view_->set_dragging(false);
      tab_container_->StoppedDraggingView(&*slot_view_);
    }

    void AnimationCanceled(const gfx::Animation* animation) override {
      AnimationEnded(animation);
    }

   private:
    const raw_ref<TabContainer> tab_container_;
    const raw_ref<TabSlotView> slot_view_;
  };

  // Determines the index to move the dragged tabs to. The dragged tabs must
  // already be in the tabstrip. |dragged_bounds| is the union of the bounds
  // of the dragged tabs and group header, if any. |first_dragged_tab_index| is
  // the current model index in this tabstrip of the first dragged tab. The
  // dragged tabs must be in the tabstrip already!
  int CalculateInsertionIndex(
      const gfx::Rect& dragged_bounds,
      int first_dragged_tab_index,
      int num_dragged_tabs,
      absl::optional<tab_groups::TabGroupId> dragged_group) const {
    // This method assumes that the dragged tabs and group are already in the
    // tabstrip (i.e. it doesn't support attaching a drag to a new tabstrip).
    // This assumption is critical because it means that tab width won't change
    // after this method's recommendation is implemented.

    // For each possible insertion index, determine what the ideal bounds of
    // the dragged tabs would be at that index. This corresponds to where they
    // would slide to if the drag session ended now. We want to insert at the
    // index that minimizes the distance between the corresponding ideal bounds
    // and the current bounds of the tabs. This is equivalent to minimizing:
    //   - the distance of the aforementioned slide,
    //   - the width of the gaps in the tabstrip, or
    //   - the amount of tab overlap.
    int min_distance_index = -1;
    int min_distance = std::numeric_limits<int>::max();
    for (int candidate_index = 0; candidate_index <= GetTabCount();
         ++candidate_index) {
      if (!IsValidInsertionIndex(candidate_index, first_dragged_tab_index,
                                 num_dragged_tabs, dragged_group)) {
        continue;
      }

      // If there's a group header here, and we're dragging a group, we might
      // end up on either side of that header. Check both cases to find the
      // best option.
      // TODO(tbergquist): Use this approach to determine if a tab should be
      // added to the group. This is calculated elsewhere and may require some
      // plumbing and/or duplicated code.
      const int left_ideal_x = CalculateIdealX(
          candidate_index, first_dragged_tab_index, dragged_bounds);
      const int left_distance = std::abs(dragged_bounds.x() - left_ideal_x);

      const int right_ideal_x =
          left_ideal_x + CalculateIdealXAdjustmentIfAddedToGroup(
                             candidate_index, dragged_group);
      const int right_distance = std::abs(dragged_bounds.x() - right_ideal_x);

      const int distance = std::min(left_distance, right_distance);
      if (distance < min_distance) {
        min_distance = distance;
        min_distance_index = candidate_index;
      }
    }

    if (min_distance_index == -1) {
      NOTREACHED();
      return 0;
    }

    // When moving a tab within a tabstrip, the target index is expressed as if
    // the tabs are not in the tabstrip, i.e. it acts like the tabs are first
    // removed and then re-inserted at the target index. We need to adjust the
    // target index to account for this.
    if (min_distance_index > first_dragged_tab_index)
      min_distance_index -= num_dragged_tabs;

    return min_distance_index;
  }

  // Dragging can't insert tabs into some indices.
  bool IsValidInsertionIndex(
      int candidate_index,
      int first_dragged_tab_index,
      int num_dragged_tabs,
      absl::optional<tab_groups::TabGroupId> dragged_group) const {
    if (candidate_index == 0)
      return true;

    // If |candidate_index| is right after one of the tabs we're dragging,
    // inserting here would be nonsensical - we can't insert the dragged tabs
    // into the middle of the dragged tabs. That's just silly.
    if (candidate_index > first_dragged_tab_index &&
        candidate_index <= first_dragged_tab_index + num_dragged_tabs) {
      return false;
    }

    // This might be in the middle of a group, which may or may not be fine.
    absl::optional<tab_groups::TabGroupId> left_group =
        GetTabAt(candidate_index - 1)->group();
    absl::optional<tab_groups::TabGroupId> right_group =
        tab_strip_->IsValidModelIndex(candidate_index)
            ? GetTabAt(candidate_index)->group()
            : absl::nullopt;
    if (left_group.has_value() && left_group == right_group) {
      // Can't drag a group into another group.
      if (dragged_group.has_value())
        return false;
      // Can't drag a tab into a collapsed group.
      if (tab_strip_->IsGroupCollapsed(left_group.value()))
        return false;
    }

    return true;
  }

  // Determines the x position that the dragged tabs would have if they were
  // inserted at |candidate_index|. If there's a group header at that index,
  // this assumes the dragged tabs *would not* be inserted into the group,
  // and would therefore end up to the left of that header.
  int CalculateIdealX(int candidate_index,
                      int first_dragged_tab_index,
                      gfx::Rect dragged_bounds) const {
    if (candidate_index == 0)
      return 0;

    const int tab_overlap = TabStyle::GetTabOverlap();

    // We'll insert just right of the tab at |candidate_index| - 1.
    int ideal_x =
        tab_strip_->tab_container_->GetIdealBounds(candidate_index - 1).right();

    // If the dragged tabs are currently left of |candidate_index|, moving
    // them to |candidate_index| would move the tab at |candidate_index| - 1
    // to the left by |num_dragged_tabs| slots. This would change the ideal x
    // for the dragged tabs, as well, by the width of the dragged tabs.
    if (candidate_index - 1 > first_dragged_tab_index)
      ideal_x -= dragged_bounds.width() - tab_overlap;

    return ideal_x - tab_overlap;
  }

  // There might be a group starting at |candidate_index|. If there is,
  // this determines how the ideal x would change if the dragged tabs were
  // added to that group, thereby moving them to that header's right.
  int CalculateIdealXAdjustmentIfAddedToGroup(
      int candidate_index,
      absl::optional<tab_groups::TabGroupId> dragged_group) const {
    // If the tab to the right of |candidate_index| is the first tab in a
    // (non-collapsed) group, we are sharing this model index with a group
    // header. We might end up on either side of it, so we need to check
    // both positions.
    if (!dragged_group.has_value() &&
        tab_strip_->IsValidModelIndex(candidate_index)) {
      absl::optional<tab_groups::TabGroupId> left_group =
          tab_strip_->IsValidModelIndex(candidate_index - 1)
              ? GetTabAt(candidate_index - 1)->group()
              : absl::nullopt;
      absl::optional<tab_groups::TabGroupId> right_group =
          GetTabAt(candidate_index)->group();
      if (right_group.has_value() && left_group != right_group) {
        if (tab_strip_->IsGroupCollapsed(right_group.value()))
          return 0;
        const int header_width =
            GetTabGroupHeader(*right_group)->bounds().width() -
            TabStyle::GetTabOverlap();
        return header_width;
      }
    }

    return 0;
  }

  const raw_ptr<TabStrip> tab_strip_;

  // Responsible for animating tabs during drag sessions.
  views::BoundsAnimator bounds_animator_;

  // The controller for a drag initiated from a Tab. Valid for the lifetime of
  // the drag session.
  std::unique_ptr<TabDragController> drag_controller_;

  // Only used in tests.
  base::OnceCallback<void(TabDragController*)> drag_controller_set_callback_;

  base::WeakPtrFactory<TabDragContext> weak_factory_{this};
};

BEGIN_METADATA(TabStrip, TabDragContextImpl, views::View);
END_METADATA

///////////////////////////////////////////////////////////////////////////////
// TabStrip, public:

TabStrip::TabStrip(std::unique_ptr<TabStripController> controller)
    : controller_(std::move(controller)),
      hover_card_controller_(std::make_unique<TabHoverCardController>(this)),
      drag_context_(*AddChildView(std::make_unique<TabDragContextImpl>(this))),
      tab_container_(
          *AddChildViewAt(MakeTabContainer(this,
                                           hover_card_controller_.get(),
                                           base::to_address(drag_context_)),
                          0)) {
  // TODO(pbos): This is probably incorrect, the background of individual tabs
  // depend on their selected state. This should probably be pushed down into
  // tabs.
  views::SetCascadingColorProviderColor(this, views::kCascadingBackgroundColor,
                                        kColorToolbar);
  Init();

  SetProperty(views::kElementIdentifierKey, kTabStripElementId);
}

TabStrip::~TabStrip() {
  // Eliminate the hover card first to avoid order-of-operation issues.
  hover_card_controller_.reset();

  // Disengage the drag controller before doing any additional cleanup. This
  // call can interact with child views so we can't reliably do it during member
  // destruction.
  drag_context_->DestroyDragController();

  // |tab_container_|'s tabs may call back to us or to |drag_context_| from
  // their destructors. Delete them first so that if they call back we aren't in
  // a weird state.
  RemoveChildViewT(&*tab_container_);
  RemoveChildViewT(&*drag_context_);

  CHECK(!IsInObserverList());
}

void TabStrip::SetAvailableWidthCallback(
    base::RepeatingCallback<int()> available_width_callback) {
  tab_container_->SetAvailableWidthCallback(available_width_callback);
}

// static
int TabStrip::GetSizeNeededForViews(const std::vector<TabSlotView*>& views) {
  int width = 0;
  for (const TabSlotView* view : views)
    width += view->width();
  if (!views.empty())
    width -= TabStyle::GetTabOverlap() * (views.size() - 1);
  return width;
}

void TabStrip::AddObserver(TabStripObserver* observer) {
  observers_.AddObserver(observer);
}

void TabStrip::RemoveObserver(TabStripObserver* observer) {
  observers_.RemoveObserver(observer);
}

void TabStrip::FrameColorsChanged() {
  for (int i = 0; i < GetTabCount(); ++i)
    tab_at(i)->FrameColorsChanged();
  UpdateContrastRatioValues();
  SchedulePaint();
}

void TabStrip::SetBackgroundOffset(int background_offset) {
  if (background_offset == background_offset_)
    return;
  background_offset_ = background_offset;
  OnPropertyChanged(&background_offset_, views::kPropertyEffectsPaint);
}

bool TabStrip::IsRectInWindowCaption(const gfx::Rect& rect) {
  return tab_container_->IsRectInWindowCaption(rect);
}

bool TabStrip::IsTabStripCloseable() const {
  return drag_context_->IsTabStripCloseable();
}

bool TabStrip::IsTabStripEditable() const {
  return !drag_context_->IsDragSessionActive() &&
         !drag_context_->IsActiveDropTarget();
}

bool TabStrip::IsTabCrashed(int tab_index) const {
  return tab_at(tab_index)->data().IsCrashed();
}

bool TabStrip::TabHasNetworkError(int tab_index) const {
  return tab_at(tab_index)->data().network_state == TabNetworkState::kError;
}

absl::optional<TabAlertState> TabStrip::GetTabAlertState(int tab_index) const {
  return Tab::GetAlertStateToShow(tab_at(tab_index)->data().alert_state);
}

void TabStrip::UpdateLoadingAnimations(const base::TimeDelta& elapsed_time) {
  for (int i = 0; i < GetTabCount(); i++)
    tab_at(i)->StepLoadingAnimation(elapsed_time);
}

void TabStrip::AddTabAt(int model_index, TabRendererData data) {
  const bool pinned = data.pinned;
  Tab* tab = tab_container_->AddTab(
      std::make_unique<Tab>(this), model_index,
      pinned ? TabPinned::kPinned : TabPinned::kUnpinned);

  tab->set_context_menu_controller(&context_menu_controller_);
  tab->AddObserver(this);
  selected_tabs_.IncrementFrom(model_index);

  // Setting data must come after all state from the model has been updated
  // above for the tab. Accessibility, in particular, reacts to data changed
  // callbacks.
  tab->SetData(std::move(data));

  for (TabStripObserver& observer : observers_)
    observer.OnTabAdded(model_index);

  // At the start of AddTabAt() the model and tabs are out sync. Any queries to
  // find a tab given a model index can go off the end of |tabs_|. As such, it
  // is important that we complete the drag *after* adding the tab so that the
  // model and tabstrip are in sync.
  drag_context_->TabWasAdded();

  // begin Add by TangramTeam
  HWND hwnd = views::HWNDForWidget(GetWidget());
  if (::IsWindow(hwnd) && (::GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD)) {
    // new_tab_button_->SetVisible(false);
  }
  // end Add by TangramTeam

  Profile* profile = controller_->GetProfile();
  if (profile) {
    if (profile->IsGuestSession())
      base::UmaHistogramCounts100("Tab.Count.Guest", GetTabCount());
    else if (profile->IsIncognitoProfile())
      base::UmaHistogramCounts100("Tab.Count.Incognito", GetTabCount());
  }

  if (new_tab_button_pressed_start_time_.has_value()) {
    base::UmaHistogramTimes(
        "TabStrip.TimeToCreateNewTabFromPress",
        base::TimeTicks::Now() - new_tab_button_pressed_start_time_.value());
    new_tab_button_pressed_start_time_.reset();
  }

  LogTabWidthsForTabScrolling();
}

void TabStrip::MoveTab(int from_model_index,
                       int to_model_index,
                       TabRendererData data) {
  DCHECK_GT(GetTabCount(), 0);

  Tab* moving_tab = tab_at(from_model_index);
  moving_tab->SetData(std::move(data));

  tab_container_->MoveTab(from_model_index, to_model_index);

  selected_tabs_.Move(from_model_index, to_model_index, /*length=*/1);

  for (TabStripObserver& observer : observers_)
    observer.OnTabMoved(from_model_index, to_model_index);
}

void TabStrip::RemoveTabAt(content::WebContents* contents,
                           int model_index,
                           bool was_active) {
  // OnTabWillBeRemoved should have ended any ongoing drags containing
  // `contents` already - unless the call is coming from inside the house! (i.e.
  // the TabDragController is doing the removing as part of reverting a drag)
  DCHECK(drag_context_->CanRemoveTabIfDragging(contents));
  tab_container_->RemoveTab(model_index, was_active);

  UpdateHoverCard(nullptr, HoverCardUpdateType::kTabRemoved);

  selected_tabs_.DecrementFrom(model_index);

  for (TabStripObserver& observer : observers_)
    observer.OnTabRemoved(model_index);
}

void TabStrip::OnTabWillBeRemoved(content::WebContents* contents,
                                  int model_index) {
  drag_context_->OnTabWillBeRemoved(contents);
}

void TabStrip::SetTabData(int model_index, TabRendererData data) {
  Tab* tab = tab_at(model_index);
  const bool pinned = data.pinned;
  const bool pinned_state_changed = tab->data().pinned != pinned;
  tab->SetData(std::move(data));

  if (HoverCardIsShowingForTab(tab))
    UpdateHoverCard(tab, HoverCardUpdateType::kTabDataChanged);

  if (pinned_state_changed)
    tab_container_->SetTabPinned(
        model_index, pinned ? TabPinned::kPinned : TabPinned::kUnpinned);
}

void TabStrip::AddTabToGroup(absl::optional<tab_groups::TabGroupId> group,
                             int model_index) {
  tab_at(model_index)->set_group(group);

  // Expand the group if the tab that is getting grouped is the active tab. This
  // can result in the group expanding in a series of actions where the final
  // active tab is not in the group.
  if (static_cast<size_t>(model_index) == selected_tabs_.active() &&
      group.has_value() && IsGroupCollapsed(group.value())) {
    ToggleTabGroupCollapsedState(
        group.value(), ToggleTabGroupCollapsedStateOrigin::kImplicitAction);
  }

  if (group.has_value())
    tab_container_->ExitTabClosingMode();
}

void TabStrip::OnGroupCreated(const tab_groups::TabGroupId& group) {
  tab_container_->OnGroupCreated(group);
}

void TabStrip::OnGroupEditorOpened(const tab_groups::TabGroupId& group) {
  tab_container_->OnGroupEditorOpened(group);
}

void TabStrip::OnGroupContentsChanged(const tab_groups::TabGroupId& group) {
  tab_container_->OnGroupContentsChanged(group);
}

void TabStrip::OnGroupVisualsChanged(
    const tab_groups::TabGroupId& group,
    const tab_groups::TabGroupVisualData* old_visuals,
    const tab_groups::TabGroupVisualData* new_visuals) {
  tab_container_->GetGroupViews()[group]->OnGroupVisualsChanged();
  // The group title may have changed size, so update bounds.
  // First exit tab closing mode, unless this change was a collapse, in which
  // case we want to stay in tab closing mode.
  bool is_collapsing = old_visuals && !old_visuals->is_collapsed() &&
                       new_visuals->is_collapsed();
  if (!is_collapsing)
    tab_container_->ExitTabClosingMode();
  tab_container_->StartBasicAnimation();

  // The active tab may need to repaint its group stroke if it's in |group|.
  int active_index = GetActiveIndex();
  if (IsValidModelIndex(active_index))
    tab_at(active_index)->SchedulePaint();
}

void TabStrip::ToggleTabGroup(const tab_groups::TabGroupId& group,
                              bool is_collapsing,
                              ToggleTabGroupCollapsedStateOrigin origin) {
  if (is_collapsing && GetWidget()) {
    if (origin != ToggleTabGroupCollapsedStateOrigin::kMouse &&
        origin != ToggleTabGroupCollapsedStateOrigin::kGesture) {
      return;
    }

    const int current_group_width =
        tab_container_->GetGroupViews()[group]->GetBounds().width();
    // A collapsed group only has the width of its header, which is slightly
    // smaller for collapsed groups compared to expanded groups.
    const int collapsed_group_width = tab_container_->GetGroupViews()[group]
                                          ->header()
                                          ->GetCollapsedHeaderWidth();
    const CloseTabSource source =
        origin == ToggleTabGroupCollapsedStateOrigin::kMouse
            ? CloseTabSource::CLOSE_TAB_FROM_MOUSE
            : CloseTabSource::CLOSE_TAB_FROM_TOUCH;

    tab_container_->EnterTabClosingMode(
        tab_container_->GetIdealBounds(GetModelCount() - 1).right() -
            current_group_width + collapsed_group_width,
        source);
  } else {
    tab_container_->ExitTabClosingMode();
  }
}

void TabStrip::OnGroupMoved(const tab_groups::TabGroupId& group) {
  tab_container_->OnGroupMoved(group);
}

void TabStrip::OnGroupClosed(const tab_groups::TabGroupId& group) {
  tab_container_->OnGroupClosed(group);
}

bool TabStrip::ShouldDrawStrokes() const {
  // If the controller says we can't draw strokes, don't.
  if (!controller_->CanDrawStrokes())
    return false;

  // The tabstrip normally avoids strokes and relies on the active tab
  // contrasting sufficiently with the frame background.  When there isn't
  // enough contrast, fall back to a stroke.  Always compute the contrast ratio
  // against the active frame color, to avoid toggling the stroke on and off as
  // the window activation state changes.
  constexpr float kMinimumContrastRatioForOutlines = 1.3f;
  const SkColor background_color = GetTabBackgroundColor(
      TabActive::kActive, BrowserFrameActiveState::kActive);
  const SkColor frame_color =
      controller_->GetFrameColor(BrowserFrameActiveState::kActive);
  const float contrast_ratio =
      color_utils::GetContrastRatio(background_color, frame_color);
  if (contrast_ratio < kMinimumContrastRatioForOutlines)
    return true;

  // Don't want to have to run a full feature query every time this function is
  // called.
  static const bool tab_outlines_in_low_contrast =
      base::FeatureList::IsEnabled(features::kTabOutlinesInLowContrastThemes);
  if (tab_outlines_in_low_contrast) {
    constexpr float kMinimumAbsoluteContrastForOutlines = 0.2f;
    const float background_luminance =
        color_utils::GetRelativeLuminance(background_color);
    const float frame_luminance =
        color_utils::GetRelativeLuminance(frame_color);
    const float contrast_difference =
        std::fabs(background_luminance - frame_luminance);
    if (contrast_difference < kMinimumAbsoluteContrastForOutlines)
      return true;
  }

  return false;
}

void TabStrip::SetSelection(const ui::ListSelectionModel& new_selection) {
  DCHECK(new_selection.active().has_value())
      << "We should never transition to a state where no tab is active.";
  Tab* const new_active_tab = tab_at(new_selection.active().value());
  Tab* const old_active_tab = selected_tabs_.active().has_value()
                                  ? tab_at(selected_tabs_.active().value())
                                  : nullptr;

  if (new_active_tab != old_active_tab) {
    if (old_active_tab) {
      old_active_tab->ActiveStateChanged();
      if (old_active_tab->group().has_value())
        tab_container_->UpdateTabGroupVisuals(old_active_tab->group().value());
    }
    if (new_active_tab->group().has_value()) {
      const tab_groups::TabGroupId new_group = new_active_tab->group().value();
      // If the tab that is about to be activated is in a collapsed group,
      // automatically expand the group.
      if (IsGroupCollapsed(new_group))
        ToggleTabGroupCollapsedState(
            new_group, ToggleTabGroupCollapsedStateOrigin::kImplicitAction);
      tab_container_->UpdateTabGroupVisuals(new_group);
    }

    new_active_tab->ActiveStateChanged();
    tab_container_->SetActiveTab(selected_tabs_.active(),
                                 new_selection.active());
    if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
      tab_container_->ScrollTabToVisible(new_selection.active().value());
    }
  }

  if (GetActiveTabWidth() == GetInactiveTabWidth()) {
    // When tabs are wide enough, selecting a new tab cannot change the
    // ideal bounds, so only a repaint is necessary.
    SchedulePaint();
  } else if (IsAnimating() || drag_context_->IsDragSessionActive()) {
    // The selection change will have modified the ideal bounds of the tabs
    // in |selected_tabs_| and |new_selection|.  We need to recompute and
    // retarget the animation to these new bounds. Note: This is safe even if
    // we're in the midst of mouse-based tab closure--we won't expand the
    // tabstrip back to the full window width--because PrepareForCloseAt() will
    // have set |override_available_width_for_tabs_| already.
    tab_container_->StartBasicAnimation();
  } else {
    // As in the animating case above, the selection change will have
    // affected the desired bounds of the tabs, but since we're in a steady
    // state we can just snap to the new bounds.
    tab_container_->CompleteAnimationAndLayout();
  }

  // Use STLSetDifference to get the indices of elements newly selected
  // and no longer selected, since selected_indices() is always sorted.
  ui::ListSelectionModel::SelectedIndices no_longer_selected =
      base::STLSetDifference<ui::ListSelectionModel::SelectedIndices>(
          selected_tabs_.selected_indices(), new_selection.selected_indices());
  ui::ListSelectionModel::SelectedIndices newly_selected =
      base::STLSetDifference<ui::ListSelectionModel::SelectedIndices>(
          new_selection.selected_indices(), selected_tabs_.selected_indices());

  new_active_tab->NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
  selected_tabs_ = new_selection;

  UpdateHoverCard(nullptr, HoverCardUpdateType::kSelectionChanged);

  // Notify all tabs whose selected state changed.
  for (auto tab_index :
       base::STLSetUnion<ui::ListSelectionModel::SelectedIndices>(
           no_longer_selected, newly_selected)) {
    tab_at(tab_index)->SelectedStateChanged();
  }
}

void TabStrip::ScrollTowardsTrailingTabs(int offset) {
  tab_container_->ScrollTabContainerByOffset(offset);
}

void TabStrip::ScrollTowardsLeadingTabs(int offset) {
  tab_container_->ScrollTabContainerByOffset(-offset);
}

void TabStrip::OnWidgetActivationChanged(views::Widget* widget, bool active) {
  if (active && selected_tabs_.active().has_value()) {
    // When the browser window is activated, fire a selection event on the
    // currently active tab, to help enable per-tab modes in assistive
    // technologies.
    tab_at(selected_tabs_.active().value())
        ->NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
  }
  UpdateHoverCard(nullptr, HoverCardUpdateType::kEvent);
}

void TabStrip::SetTabNeedsAttention(int model_index, bool attention) {
  tab_at(model_index)->SetTabNeedsAttention(attention);
}

int TabStrip::GetModelIndexOf(const TabSlotView* view) const {
  return tab_container_->GetModelIndexOf(view);
}

int TabStrip::GetTabCount() const {
  return tab_container_->GetTabCount();
}

int TabStrip::GetModelCount() const {
  return controller_->GetCount();
}

int TabStrip::GetModelPinnedTabCount() const {
  for (size_t i = 0; i < static_cast<size_t>(controller_->GetCount()); ++i) {
    if (!controller_->IsTabPinned(static_cast<int>(i)))
      return static_cast<int>(i);
  }

  // All tabs are pinned.
  return controller_->GetCount();
}

TabDragContext* TabStrip::GetDragContext() {
  return &*drag_context_;
}

bool TabStrip::IsAnimating() const {
  return tab_container_->IsAnimating();
}

void TabStrip::StopAnimating(bool layout) {
  if (layout) {
    tab_container_->CompleteAnimationAndLayout();
  } else {
    tab_container_->CancelAnimation();
  }
}

absl::optional<int> TabStrip::GetFocusedTabIndex() const {
  for (int i = 0; i < GetTabCount(); ++i) {
    if (tab_at(i)->HasFocus())
      return i;
  }
  return absl::nullopt;
}

views::View* TabStrip::GetTabViewForPromoAnchor(int index_hint) {
  return tab_at(base::clamp(index_hint, 0, GetTabCount() - 1));
}

views::View* TabStrip::GetDefaultFocusableChild() {
  int active = GetActiveIndex();
  return active != TabStripModel::kNoTab ? tab_at(active) : nullptr;
}

bool TabStrip::IsValidModelIndex(int index) const {
  return controller_->IsValidIndex(index);
}

int TabStrip::GetActiveIndex() const {
  return controller_->GetActiveIndex();
}

int TabStrip::NumPinnedTabsInModel() const {
  for (size_t i = 0; i < static_cast<size_t>(controller_->GetCount()); ++i) {
    if (!controller_->IsTabPinned(static_cast<int>(i)))
      return static_cast<int>(i);
  }

  // All tabs are pinned.
  return controller_->GetCount();
}

void TabStrip::OnDropIndexUpdate(int index, bool drop_before) {
  controller_->OnDropIndexUpdate(index, drop_before);
}

absl::optional<int> TabStrip::GetFirstTabInGroup(
    const tab_groups::TabGroupId& group) const {
  return controller_->GetFirstTabInGroup(group);
}

gfx::Range TabStrip::ListTabsInGroup(
    const tab_groups::TabGroupId& group) const {
  return controller_->ListTabsInGroup(group);
}

bool TabStrip::CanExtendDragHandle() const {
  return !controller_->IsFrameCondensed() &&
         !controller_->EverHasVisibleBackgroundTabShapes();
}

bool TabStrip::IsGroupCollapsed(const tab_groups::TabGroupId& group) const {
  return controller_->IsGroupCollapsed(group);
}

const ui::ListSelectionModel& TabStrip::GetSelectionModel() const {
  return controller_->GetSelectionModel();
}

Tab* TabStrip::tab_at(int index) const {
  return tab_container_->GetTabAtModelIndex(index);
}

void TabStrip::SelectTab(Tab* tab, const ui::Event& event) {
  int model_index = GetModelIndexOf(tab);

  if (IsValidModelIndex(model_index)) {
    if (!tab->IsActive()) {
      if (selected_tabs_.active().has_value()) {
        base::UmaHistogramSparse("Tabs.DesktopTabOffsetOfSwitch",
                                 selected_tabs_.active().value() - model_index);
      }
      base::UmaHistogramSparse("Tabs.DesktopTabOffsetFromLeftOfSwitch",
                               model_index);
      base::UmaHistogramSparse("Tabs.DesktopTabOffsetFromRightOfSwitch",
                               GetModelCount() - model_index - 1);
      base::UmaHistogramEnumeration("TabStrip.Tab.Views.ActivationAction",
                                    TabActivationTypes::kTab);

      if (tab->group().has_value()) {
        base::RecordAction(
            base::UserMetricsAction("TabGroups_SwitchGroupedTab"));
      }
    }

    // Selecting a tab via mouse affects what statistics we collect.
    if (event.type() == ui::ET_MOUSE_PRESSED && !tab->IsActive() &&
        hover_card_controller_) {
      hover_card_controller_->TabSelectedViaMouse(tab);
    }

    controller_->SelectTab(model_index, event);
    // begin Add by TangramTeam
    HWND hwnd = views::HWNDForWidget(GetWidget());
    if (::IsWindow(hwnd)) {
      ::SendMessage(hwnd, WM_TABCHANGE, model_index, 0);
    }
    // end Add by TangramTeam
  }
}

void TabStrip::ExtendSelectionTo(Tab* tab) {
  int model_index = GetModelIndexOf(tab);
  if (IsValidModelIndex(model_index))
    controller_->ExtendSelectionTo(model_index);
}

void TabStrip::ToggleSelected(Tab* tab) {
  int model_index = GetModelIndexOf(tab);
  if (IsValidModelIndex(model_index))
    controller_->ToggleSelected(model_index);
}

void TabStrip::AddSelectionFromAnchorTo(Tab* tab) {
  int model_index = GetModelIndexOf(tab);
  if (IsValidModelIndex(model_index))
    controller_->AddSelectionFromAnchorTo(model_index);
}

void TabStrip::CloseTab(Tab* tab, CloseTabSource source) {
  int index_to_close = tab_container_->GetModelIndexOfFirstNonClosingTab(tab);

  if (IsValidModelIndex(index_to_close))
    CloseTabInternal(index_to_close, source);
}

void TabStrip::ToggleTabAudioMute(Tab* tab) {
  int model_index = GetModelIndexOf(tab);
  if (IsValidModelIndex(model_index))
    controller_->ToggleTabAudioMute(model_index);
}

void TabStrip::ShiftTabNext(Tab* tab) {
  ShiftTabRelative(tab, 1);
}

void TabStrip::ShiftTabPrevious(Tab* tab) {
  ShiftTabRelative(tab, -1);
}

void TabStrip::MoveTabFirst(Tab* tab) {
  if (tab->closing())
    return;

  const int start_index = GetModelIndexOf(tab);
  if (!IsValidModelIndex(start_index))
    return;

  int target_index = 0;
  if (!controller_->IsTabPinned(start_index)) {
    while (target_index < start_index && controller_->IsTabPinned(target_index))
      ++target_index;
  }

  if (!IsValidModelIndex(target_index))
    return;

  if (target_index != start_index)
    controller_->MoveTab(start_index, target_index);

  // The tab may unintentionally land in the first group in the tab strip, so we
  // remove the group to ensure consistent behavior. Even if the tab is already
  // at the front, it should "move" out of its current group.
  if (tab->group().has_value())
    controller_->RemoveTabFromGroup(target_index);

  GetViewAccessibility().AnnounceText(
      l10n_util::GetStringUTF16(IDS_TAB_AX_ANNOUNCE_MOVED_FIRST));
}

void TabStrip::MoveTabLast(Tab* tab) {
  if (tab->closing())
    return;

  const int start_index = GetModelIndexOf(tab);
  if (!IsValidModelIndex(start_index))
    return;

  int target_index;
  if (controller_->IsTabPinned(start_index)) {
    int temp_index = start_index + 1;
    while (temp_index < GetTabCount() && controller_->IsTabPinned(temp_index))
      ++temp_index;
    target_index = temp_index - 1;
  } else {
    target_index = GetTabCount() - 1;
  }

  if (!IsValidModelIndex(target_index))
    return;

  if (target_index != start_index)
    controller_->MoveTab(start_index, target_index);

  // The tab may unintentionally land in the last group in the tab strip, so we
  // remove the group to ensure consistent behavior. Even if the tab is already
  // at the back, it should "move" out of its current group.
  if (tab->group().has_value())
    controller_->RemoveTabFromGroup(target_index);

  GetViewAccessibility().AnnounceText(
      l10n_util::GetStringUTF16(IDS_TAB_AX_ANNOUNCE_MOVED_LAST));
}

bool TabStrip::ToggleTabGroupCollapsedState(
    const tab_groups::TabGroupId group,
    ToggleTabGroupCollapsedStateOrigin origin) {
  return controller_->ToggleTabGroupCollapsedState(group, origin);
}

void TabStrip::NotifyTabGroupEditorBubbleOpened() {
  tab_container_->NotifyTabGroupEditorBubbleOpened();
}
void TabStrip::NotifyTabGroupEditorBubbleClosed() {
  tab_container_->NotifyTabGroupEditorBubbleClosed();
}

void TabStrip::ShowContextMenuForTab(Tab* tab,
                                     const gfx::Point& p,
                                     ui::MenuSourceType source_type) {
  controller_->ShowContextMenuForTab(tab, p, source_type);
}

bool TabStrip::IsActiveTab(const Tab* tab) const {
  int model_index = GetModelIndexOf(tab);
  return IsValidModelIndex(model_index) &&
         controller_->IsActiveTab(model_index);
}

bool TabStrip::IsTabSelected(const Tab* tab) const {
  int model_index = GetModelIndexOf(tab);
  return IsValidModelIndex(model_index) &&
         controller_->IsTabSelected(model_index);
}

bool TabStrip::IsTabPinned(const Tab* tab) const {
  if (tab->closing())
    return false;

  int model_index = GetModelIndexOf(tab);
  return IsValidModelIndex(model_index) &&
         controller_->IsTabPinned(model_index);
}

bool TabStrip::IsTabFirst(const Tab* tab) const {
  return GetModelIndexOf(tab) == 0;
}

bool TabStrip::IsFocusInTabs() const {
  return GetFocusManager() && Contains(GetFocusManager()->GetFocusedView());
}

void TabStrip::MaybeStartDrag(
    TabSlotView* source,
    const ui::LocatedEvent& event,
    const ui::ListSelectionModel& original_selection) {
  // Don't accidentally start any drag operations during animations if the
  // mouse is down... during an animation tabs are being resized automatically,
  // so the View system can misinterpret this easily if the mouse is down that
  // the user is dragging.
  if (IsAnimating() || controller_->HasAvailableDragActions() == 0)
    return;

  // Check that the source is either a valid tab or a tab group header, which
  // are the only valid drag targets.
  if (!IsValidModelIndex(GetModelIndexOf(source))) {
    DCHECK_EQ(source->GetTabSlotViewType(),
              TabSlotView::ViewType::kTabGroupHeader);
  }

  drag_context_->MaybeStartDrag(source, event, original_selection);
}

void TabStrip::ContinueDrag(views::View* view, const ui::LocatedEvent& event) {
  drag_context_->ContinueDrag(view, event);
}

bool TabStrip::EndDrag(EndDragReason reason) {
  return drag_context_->EndDrag(reason);
}

Tab* TabStrip::GetTabAt(const gfx::Point& point) {
  views::View* view = GetEventHandlerForPoint(point);
  if (!view)
    return nullptr;  // No tab contains the point.

  // Walk up the view hierarchy until we find a tab, or the TabStrip.
  while (view && view != this && view->GetID() != VIEW_ID_TAB)
    view = view->parent();

  return view && view->GetID() == VIEW_ID_TAB ? static_cast<Tab*>(view)
                                              : nullptr;
}

const Tab* TabStrip::GetAdjacentTab(const Tab* tab, int offset) {
  int index = GetModelIndexOf(tab);
  if (index < 0)
    return nullptr;
  index += offset;
  return IsValidModelIndex(index) ? tab_at(index) : nullptr;
}

void TabStrip::OnMouseEventInTab(views::View* source,
                                 const ui::MouseEvent& event) {
  // Record time from cursor entering the tabstrip to first tap on a tab to
  // switch.
  if (mouse_entered_tabstrip_time_.has_value() &&
      event.type() == ui::ET_MOUSE_PRESSED && views::IsViewClass<Tab>(source)) {
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "TabStrip.TimeToSwitch",
        base::TimeTicks::Now() - mouse_entered_tabstrip_time_.value());
    mouse_entered_tabstrip_time_.reset();
  }
}

void TabStrip::UpdateHoverCard(Tab* tab, HoverCardUpdateType update_type) {
  tab_container_->UpdateHoverCard(tab, update_type);
}

bool TabStrip::ShowDomainInHoverCards() const {
#if BUILDFLAG(IS_CHROMEOS_ASH)
  const auto* app_controller = GetBrowser()->app_controller();
  if (app_controller && app_controller->system_app())
    return false;
#endif
  return true;
}

bool TabStrip::HoverCardIsShowingForTab(Tab* tab) {
  return hover_card_controller_ &&
         hover_card_controller_->IsHoverCardShowingForTab(tab);
}

int TabStrip::GetBackgroundOffset() const {
  return background_offset_;
}

int TabStrip::GetStrokeThickness() const {
  return ShouldDrawStrokes() ? 1 : 0;
}

bool TabStrip::CanPaintThrobberToLayer() const {
  // Disable layer-painting of throbbers if dragging or if any tab animation is
  // in progress. Also disable in fullscreen: when "immersive" the tab strip
  // could be sliding in or out; for other modes, there's no tab strip.
  const bool dragging = drag_context_->IsDragStarted();
  const views::Widget* widget = GetWidget();
  return widget && !dragging && !IsAnimating() && !widget->IsFullscreen();
}

bool TabStrip::HasVisibleBackgroundTabShapes() const {
  return controller_->HasVisibleBackgroundTabShapes();
}

bool TabStrip::ShouldPaintAsActiveFrame() const {
  return controller_->ShouldPaintAsActiveFrame();
}

SkColor TabStrip::GetTabSeparatorColor() const {
  return separator_color_;
}

SkColor TabStrip::GetTabBackgroundColor(
    TabActive active,
    BrowserFrameActiveState active_state) const {
  const auto* cp = GetColorProvider();
  if (!cp)
    return gfx::kPlaceholderColor;

  constexpr ChromeColorIds kColorIds[2][2] = {
      {kColorTabBackgroundInactiveFrameInactive,
       kColorTabBackgroundInactiveFrameActive},
      {kColorTabBackgroundActiveFrameInactive,
       kColorTabBackgroundActiveFrameActive}};

  using State = BrowserFrameActiveState;
  const bool tab_active = active == TabActive::kActive;
  const bool frame_active =
      (active_state == State::kActive) ||
      ((active_state == State::kUseCurrent) && ShouldPaintAsActiveFrame());
  return cp->GetColor(kColorIds[tab_active][frame_active]);
}

SkColor TabStrip::GetTabForegroundColor(TabActive active) const {
  const ui::ColorProvider* cp = GetColorProvider();
  if (!cp)
    return gfx::kPlaceholderColor;

  constexpr ChromeColorIds kColorIds[2][2] = {
      {kColorTabForegroundInactiveFrameInactive,
       kColorTabForegroundInactiveFrameActive},
      {kColorTabForegroundActiveFrameInactive,
       kColorTabForegroundActiveFrameActive}};

  const bool tab_active = active == TabActive::kActive;
  const bool frame_active = ShouldPaintAsActiveFrame();
  return cp->GetColor(kColorIds[tab_active][frame_active]);
}

// Returns the accessible tab name for the tab.
std::u16string TabStrip::GetAccessibleTabName(const Tab* tab) const {
  const int model_index = GetModelIndexOf(tab);
  return IsValidModelIndex(model_index) ? controller_->GetAccessibleTabName(tab)
                                        : std::u16string();
}

absl::optional<int> TabStrip::GetCustomBackgroundId(
    BrowserFrameActiveState active_state) const {
  if (!TitlebarBackgroundIsTransparent())
    return controller_->GetCustomBackgroundId(active_state);

  constexpr int kBackgroundIdGlass = IDR_THEME_TAB_BACKGROUND_V;
  return GetThemeProvider()->HasCustomImage(kBackgroundIdGlass)
             ? absl::make_optional(kBackgroundIdGlass)
             : absl::nullopt;
}

float TabStrip::GetHoverOpacityForTab(float range_parameter) const {
  return gfx::Tween::FloatValueBetween(range_parameter, hover_opacity_min_,
                                       hover_opacity_max_);
}

float TabStrip::GetHoverOpacityForRadialHighlight() const {
  return radial_highlight_opacity_;
}

std::u16string TabStrip::GetGroupTitle(
    const tab_groups::TabGroupId& group) const {
  return controller_->GetGroupTitle(group);
}

std::u16string TabStrip::GetGroupContentString(
    const tab_groups::TabGroupId& group) const {
  return controller_->GetGroupContentString(group);
}
tab_groups::TabGroupColorId TabStrip::GetGroupColorId(
    const tab_groups::TabGroupId& group) const {
  return controller_->GetGroupColorId(group);
}

SkColor TabStrip::GetPaintedGroupColor(
    const tab_groups::TabGroupColorId& color_id) const {
  return GetColorProvider()->GetColor(
      GetTabGroupTabStripColorId(color_id, ShouldPaintAsActiveFrame()));
}

void TabStrip::ShiftGroupLeft(const tab_groups::TabGroupId& group) {
  ShiftGroupRelative(group, -1);
}

void TabStrip::ShiftGroupRight(const tab_groups::TabGroupId& group) {
  ShiftGroupRelative(group, 1);
}

const Browser* TabStrip::GetBrowser() const {
  return controller_->GetBrowser();
}

///////////////////////////////////////////////////////////////////////////////
// TabStrip, views::View overrides:

views::SizeBounds TabStrip::GetAvailableSize(const views::View* child) const {
  // We can only reach here if SetAvailableWidthCallback() was never called,
  // e.g. if tab scrolling is disabled. Defer to our parent.
  DCHECK(child == &*tab_container_);
  return parent()->GetAvailableSize(this);
}

void TabStrip::Layout() {
  if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
    // With tab scrolling, the TabStrip is solely responsible for its own width
    // (It's the contents view of a ScrollView, and with sizing freedom comes
    // sizing responsibility).

    // We can figure out our width based on the preferences of our TabContainer
    // and the available width in the tab strip region:
    // We should never be larger than our container's preferred width.
    const int max_width = tab_container_->CalculatePreferredSize().width();
    // We should never be smaller than our container's minimum width.
    const int min_width = tab_container_->GetMinimumSize().width();
    // If we can, we should fit within the tab strip region to avoid scrolling.
    const int available_width =
        tab_container_->GetAvailableWidthForTabContainer();
    // Be as wide as possible subject to the above constraints.
    const int width = std::min(max_width, std::max(min_width, available_width));
    SetBounds(0, 0, width, GetLayoutConstant(TAB_HEIGHT));
  }
  views::View::Layout();

  // drag_context_ isn't part of normal layout since it overlays the tabstrip.
  drag_context_->Layout();
}

void TabStrip::ChildPreferredSizeChanged(views::View* child) {
  PreferredSizeChanged();
}

BrowserRootView::DropIndex TabStrip::GetDropIndex(
    const ui::DropTargetEvent& event) {
  // BrowserView should talk directly to |tab_container_| instead of asking us.
  NOTREACHED();
  return tab_container_->GetDropIndex(event);
}

BrowserRootView::DropTarget* TabStrip::GetDropTarget(
    gfx::Point loc_in_local_coords) {
  return tab_container_->GetDropTarget(loc_in_local_coords);
}

views::View* TabStrip::GetViewForDrop() {
  // BrowserView should talk directly to |tab_container_| instead of asking us.
  NOTREACHED();
  return tab_container_->GetViewForDrop();
}

///////////////////////////////////////////////////////////////////////////////
// TabStrip, private:

void TabStrip::Init() {
  SetID(VIEW_ID_TAB_STRIP);
  // So we only get enter/exit messages when the mouse enters/exits the whole
  // tabstrip, even if it is entering/exiting a specific Tab, too.
  SetNotifyEnterExitOnChild(true);

  UpdateContrastRatioValues();

  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetChildViewIgnoredByLayout(&*drag_context_, true);
  tab_container_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kPreferred));
}

std::map<tab_groups::TabGroupId, TabGroupHeader*> TabStrip::GetGroupHeaders() {
  std::map<tab_groups::TabGroupId, TabGroupHeader*> group_headers;
  for (const auto& group_view_pair : tab_container_->GetGroupViews()) {
    group_headers.insert(std::make_pair(group_view_pair.first,
                                        group_view_pair.second->header()));
  }
  return group_headers;
}

void TabStrip::NewTabButtonPressed(const ui::Event& event) {
  new_tab_button_pressed_start_time_ = base::TimeTicks::Now();

  base::RecordAction(base::UserMetricsAction("NewTab_Button"));
  UMA_HISTOGRAM_ENUMERATION("Tab.NewTab", NewTabTypes::NEW_TAB_BUTTON,
                            NewTabTypes::NEW_TAB_ENUM_COUNT);
  if (event.IsMouseEvent()) {
    // Prevent the hover card from popping back in immediately. This forces a
    // normal fade-in.
    if (hover_card_controller_)
      hover_card_controller_->PreventImmediateReshow();

    const ui::MouseEvent& mouse = static_cast<const ui::MouseEvent&>(event);
    if (mouse.IsOnlyMiddleMouseButton()) {
      if (ui::Clipboard::IsSupportedClipboardBuffer(
              ui::ClipboardBuffer::kSelection)) {
        ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
        CHECK(clipboard);
        std::u16string clipboard_text;
        clipboard->ReadText(ui::ClipboardBuffer::kSelection,
                            /* data_dst = */ nullptr, &clipboard_text);
        if (!clipboard_text.empty())
          controller_->CreateNewTabWithLocation(clipboard_text);
      }
      return;
    }
  }

  controller_->CreateNewTab();
  if (event.type() == ui::ET_GESTURE_TAP)
    TouchUMA::RecordGestureAction(TouchUMA::kGestureNewTabTap);
}

bool TabStrip::ShouldHighlightCloseButtonAfterRemove() {
  return tab_container_->InTabClose();
}

bool TabStrip::TitlebarBackgroundIsTransparent() const {
#if BUILDFLAG(IS_WIN)
  // Windows 8+ uses transparent window contents (because the titlebar area is
  // drawn by the system and not Chrome), but the actual titlebar is opaque.
  if (base::win::GetVersion() >= base::win::Version::WIN8)
    return false;
#endif
  return GetWidget()->ShouldWindowContentsBeTransparent();
}

int TabStrip::GetActiveTabWidth() const {
  return tab_container_->GetActiveTabWidth();
}

int TabStrip::GetInactiveTabWidth() const {
  return tab_container_->GetInactiveTabWidth();
}

const Tab* TabStrip::GetLastVisibleTab() const {
  for (int i = GetTabCount() - 1; i >= 0; --i) {
    const Tab* tab = tab_at(i);

    // The tab is marked not visible in a collapsed group, but is "visible" in
    // the tabstrip if the header is visible.
    if (tab->GetVisible() ||
        (tab->group().has_value() &&
         group_header(tab->group().value())->GetVisible())) {
      return tab;
    }
  }
  // While in normal use the tabstrip should always be wide enough to have at
  // least one visible tab, it can be zero-width in tests, meaning we get here.
  return nullptr;
}

void TabStrip::CloseTabInternal(int model_index, CloseTabSource source) {
  if (!IsValidModelIndex(model_index))
    return;

  // If we're not allowed to close this tab for whatever reason, we should not
  // proceed.
  if (!controller_->BeforeCloseTab(model_index, source))
    return;

  if (!tab_container_->InTabClose() && IsAnimating()) {
    // Cancel any current animations. We do this as remove uses the current
    // ideal bounds and we need to know ideal bounds is in a good state.
    tab_container_->CompleteAnimationAndLayout();
  }

  if (GetWidget()) {
    // Enter tab closing mode now, but wait to calculate the width constraint
    // until RemoveTabAt() is called, since there are code paths that go through
    // RemoveTabAt() but not this method that must also set that constraint.
    tab_container_->EnterTabClosingMode(absl::nullopt, source);
  }

  UpdateHoverCard(nullptr, HoverCardUpdateType::kTabRemoved);
  if (tab_at(model_index)->group().has_value())
    base::RecordAction(base::UserMetricsAction("CloseGroupedTab"));
  controller_->CloseTab(model_index);
}

void TabStrip::UpdateContrastRatioValues() {
  // There may be no controller in unit tests, and the call to
  // GetTabBackgroundColor() below requires one, so bail early if it is absent.
  if (!controller_)
    return;

  const SkColor inactive_bg = GetTabBackgroundColor(
      TabActive::kInactive, BrowserFrameActiveState::kUseCurrent);
  const auto get_blend = [inactive_bg](SkColor target, float contrast) {
    return color_utils::BlendForMinContrast(inactive_bg, inactive_bg, target,
                                            contrast);
  };

  const SkColor active_bg = GetTabBackgroundColor(
      TabActive::kActive, BrowserFrameActiveState::kUseCurrent);
  const auto get_hover_opacity = [active_bg, &get_blend](float contrast) {
    return get_blend(active_bg, contrast).alpha / 255.0f;
  };

  // The contrast ratio for the hover effect on standard-width tabs.
  // In the default color scheme, this corresponds to a hover opacity of 0.4.
  constexpr float kStandardWidthContrast = 1.11f;
  hover_opacity_min_ = get_hover_opacity(kStandardWidthContrast);

  // The contrast ratio for the hover effect on min-width tabs.
  // In the default color scheme, this corresponds to a hover opacity of 0.65.
  constexpr float kMinWidthContrast = 1.19f;
  hover_opacity_max_ = get_hover_opacity(kMinWidthContrast);

  // The contrast ratio for the radial gradient effect on hovered tabs.
  // In the default color scheme, this corresponds to a hover opacity of 0.45.
  constexpr float kRadialGradientContrast = 1.13728f;
  radial_highlight_opacity_ = get_hover_opacity(kRadialGradientContrast);

  const SkColor inactive_fg = GetTabForegroundColor(TabActive::kInactive);
  // The contrast ratio for the separator between inactive tabs.
  constexpr float kTabSeparatorContrast = 2.5f;
  separator_color_ = get_blend(inactive_fg, kTabSeparatorContrast).color;
}

void TabStrip::ShiftTabRelative(Tab* tab, int offset) {
  DCHECK_EQ(1, std::abs(offset));
  const int start_index = GetModelIndexOf(tab);
  int target_index = start_index + offset;

  if (!IsValidModelIndex(start_index))
    return;

  if (tab->closing())
    return;

  const auto old_group = tab->group();
  if (!IsValidModelIndex(target_index) ||
      controller_->IsTabPinned(start_index) !=
          controller_->IsTabPinned(target_index)) {
    // Even if we've reached the boundary of where the tab could go, it may
    // still be able to "move" out of its current group.
    if (old_group.has_value()) {
      AnnounceTabRemovedFromGroup(old_group.value());
      controller_->RemoveTabFromGroup(start_index);
    }
    return;
  }

  // If the tab is at a group boundary and the group is expanded, instead of
  // actually moving the tab just change its group membership.
  absl::optional<tab_groups::TabGroupId> target_group =
      tab_at(target_index)->group();
  if (old_group != target_group) {
    if (old_group.has_value()) {
      AnnounceTabRemovedFromGroup(old_group.value());
      controller_->RemoveTabFromGroup(start_index);
      return;
    } else if (target_group.has_value()) {
      // If the tab is at a group boundary and the group is collapsed, treat the
      // collapsed group as a tab and find the next available slot for the tab
      // to move to.
      if (IsGroupCollapsed(target_group.value())) {
        int candidate_index = target_index + offset;
        while (IsValidModelIndex(candidate_index) &&
               tab_at(candidate_index)->group() == target_group) {
          candidate_index += offset;
        }
        if (IsValidModelIndex(candidate_index)) {
          target_index = candidate_index - offset;
        } else {
          target_index = offset < 0 ? 0 : GetModelCount() - 1;
        }
      } else {
        // Read before adding the tab to the group so that the group description
        // isn't the tab we just added.
        AnnounceTabAddedToGroup(target_group.value());
        controller_->AddTabToGroup(start_index, target_group.value());
        views::ElementTrackerViews::GetInstance()->NotifyCustomEvent(
            kTabGroupedCustomEventId, tab);
        return;
      }
    }
  }

  controller_->MoveTab(start_index, target_index);
  GetViewAccessibility().AnnounceText(l10n_util::GetStringUTF16(
      ((offset > 0) ^ base::i18n::IsRTL()) ? IDS_TAB_AX_ANNOUNCE_MOVED_RIGHT
                                           : IDS_TAB_AX_ANNOUNCE_MOVED_LEFT));
}

void TabStrip::ShiftGroupRelative(const tab_groups::TabGroupId& group,
                                  int offset) {
  DCHECK_EQ(1, std::abs(offset));
  gfx::Range tabs_in_group = controller_->ListTabsInGroup(group);

  const int start_index = tabs_in_group.start();
  int target_index = start_index + offset;

  if (offset > 0)
    target_index += tabs_in_group.length() - 1;

  if (!IsValidModelIndex(start_index) || !IsValidModelIndex(target_index))
    return;

  // Avoid moving into the middle of another group by accounting for its size.
  absl::optional<tab_groups::TabGroupId> target_group =
      tab_at(target_index)->group();
  if (target_group.has_value()) {
    target_index +=
        offset *
        (controller_->ListTabsInGroup(target_group.value()).length() - 1);
  }

  if (!IsValidModelIndex(target_index))
    return;

  if (controller_->IsTabPinned(start_index) !=
      controller_->IsTabPinned(target_index))
    return;

  controller_->MoveGroup(group, target_index);
}

void TabStrip::LogTabWidthsForTabScrolling() {
  int active_tab_width = GetActiveTabWidth();
  int inactive_tab_width = GetInactiveTabWidth();

  if (active_tab_width > 1) {
    UMA_HISTOGRAM_EXACT_LINEAR("Tabs.ActiveTabWidth", active_tab_width, 257);
  }
  if (inactive_tab_width > 1) {
    UMA_HISTOGRAM_EXACT_LINEAR("Tabs.InactiveTabWidth", inactive_tab_width,
                               257);
  }
}

// TabStrip:TabContextMenuController:
// ----------------------------------------------------------

TabStrip::TabContextMenuController::TabContextMenuController(TabStrip* parent)
    : parent_(parent) {}

void TabStrip::TabContextMenuController::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& point,
    ui::MenuSourceType source_type) {
  // We are only intended to be installed as a context-menu handler for tabs, so
  // this cast should be safe.
  DCHECK(views::IsViewClass<Tab>(source));
  Tab* const tab = static_cast<Tab*>(source);
  // begin Add by TangramTeam
  if (!tab->closing()) {
    HWND hwnd = views::HWNDForWidget(source->GetWidget());
    if (::IsWindow(hwnd) && (::GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD)) {
      return;
    }
  }
  // end Add by TangramTeam
  if (tab->closing())
    return;
  parent_->ShowContextMenuForTab(tab, point, source_type);
}

void TabStrip::OnMouseEntered(const ui::MouseEvent& event) {
  mouse_entered_tabstrip_time_ = base::TimeTicks::Now();
}

void TabStrip::OnMouseExited(const ui::MouseEvent& event) {
  UpdateHoverCard(nullptr, HoverCardUpdateType::kHover);
}

void TabStrip::AddedToWidget() {
  GetWidget()->AddObserver(this);
}

void TabStrip::RemovedFromWidget() {
  GetWidget()->RemoveObserver(this);
}

void TabStrip::OnGestureEvent(ui::GestureEvent* event) {
  switch (event->type()) {
    case ui::ET_GESTURE_LONG_TAP: {
      tab_container_->HandleLongTap(event);
      break;
    }

    case ui::ET_GESTURE_TAP: {
      const int active_index = GetActiveIndex();
      DCHECK_NE(-1, active_index);
      Tab* active_tab = tab_at(active_index);
      TouchUMA::GestureActionType action = TouchUMA::kGestureTabNoSwitchTap;
      if (active_tab->tab_activated_with_last_tap_down())
        action = TouchUMA::kGestureTabSwitchTap;
      TouchUMA::RecordGestureAction(action);
      break;
    }

    default:
      break;
  }
  event->SetHandled();
}

void TabStrip::OnViewFocused(views::View* observed_view) {
  TabSlotView* slot_view = views::AsViewClass<TabSlotView>(observed_view);
  if (!slot_view)
    return;

  absl::optional<int> index = GetModelIndexOf(slot_view);
  if (index.has_value())
    controller_->OnKeyboardFocusedTabChanged(index);
}

void TabStrip::OnViewBlurred(views::View* observed_view) {
  controller_->OnKeyboardFocusedTabChanged(absl::nullopt);
}

void TabStrip::OnTouchUiChanged() {
  tab_container_->CompleteAnimationAndLayout();
  PreferredSizeChanged();
}

void TabStrip::AnnounceTabAddedToGroup(tab_groups::TabGroupId group_id) {
  const std::u16string group_title = GetGroupTitle(group_id);
  const std::u16string contents_string = GetGroupContentString(group_id);
  GetViewAccessibility().AnnounceText(
      group_title.empty()
          ? l10n_util::GetStringFUTF16(
                IDS_TAB_AX_ANNOUNCE_TAB_ADDED_TO_UNNAMED_GROUP, contents_string)
          : l10n_util::GetStringFUTF16(
                IDS_TAB_AX_ANNOUNCE_TAB_ADDED_TO_NAMED_GROUP, group_title,
                contents_string));
}

void TabStrip::AnnounceTabRemovedFromGroup(tab_groups::TabGroupId group_id) {
  const std::u16string group_title = GetGroupTitle(group_id);
  const std::u16string contents_string = GetGroupContentString(group_id);
  GetViewAccessibility().AnnounceText(
      group_title.empty()
          ? l10n_util::GetStringFUTF16(
                IDS_TAB_AX_ANNOUNCE_TAB_REMOVED_FROM_UNNAMED_GROUP,
                contents_string)
          : l10n_util::GetStringFUTF16(
                IDS_TAB_AX_ANNOUNCE_TAB_REMOVED_FROM_NAMED_GROUP, group_title,
                contents_string));
}

BEGIN_METADATA(TabStrip, views::View)
ADD_PROPERTY_METADATA(int, BackgroundOffset)
ADD_READONLY_PROPERTY_METADATA(int, TabCount)
ADD_READONLY_PROPERTY_METADATA(int, ModelCount)
ADD_READONLY_PROPERTY_METADATA(int, ModelPinnedTabCount)
ADD_READONLY_PROPERTY_METADATA(absl::optional<int>, FocusedTabIndex)
ADD_READONLY_PROPERTY_METADATA(int, StrokeThickness)
ADD_READONLY_PROPERTY_METADATA(SkColor,
                               TabSeparatorColor,
                               ui::metadata::SkColorConverter)
ADD_READONLY_PROPERTY_METADATA(float, HoverOpacityForRadialHighlight)
ADD_READONLY_PROPERTY_METADATA(int, ActiveTabWidth)
ADD_READONLY_PROPERTY_METADATA(int, InactiveTabWidth)
END_METADATA
