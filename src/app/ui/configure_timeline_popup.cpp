// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/configure_timeline_popup.h"

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/loop_tag.h"
#include "app/transaction.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/message.h"
#include "ui/slider.h"
#include "ui/theme.h"

#include "timeline_conf.xml.h"

namespace app {

using namespace ui;

ConfigureTimelinePopup::ConfigureTimelinePopup()
  : PopupWindow("Timeline Settings", ClickBehavior::CloseOnClickInOtherWindow)
  , m_lockUpdates(false)
{
  setHotRegion(gfx::Region(manager()->bounds())); // for the color selector

  setAutoRemap(false);
  setBorder(gfx::Border(4*guiscale()));

  m_box = new app::gen::TimelineConf();
  addChild(m_box);

  m_box->merge()->Click.connect(base::Bind<void>(&ConfigureTimelinePopup::onChangeType, this));
  m_box->tint()->Click.connect(base::Bind<void>(&ConfigureTimelinePopup::onChangeType, this));
  m_box->opacity()->Change.connect(base::Bind<void>(&ConfigureTimelinePopup::onOpacity, this));
  m_box->opacityStep()->Change.connect(base::Bind<void>(&ConfigureTimelinePopup::onOpacityStep, this));
  m_box->resetOnionskin()->Click.connect(base::Bind<void>(&ConfigureTimelinePopup::onResetOnionskin, this));
  m_box->loopTag()->Click.connect(base::Bind<void>(&ConfigureTimelinePopup::onLoopTagChange, this));
  m_box->currentLayer()->Click.connect(base::Bind<void>(&ConfigureTimelinePopup::onCurrentLayerChange, this));
  m_box->behind()->Click.connect(base::Bind<void>(&ConfigureTimelinePopup::onPositionChange, this));
  m_box->infront()->Click.connect(base::Bind<void>(&ConfigureTimelinePopup::onPositionChange, this));

  m_box->celThumbOpacity()->Change.connect(base::Bind<void>(&ConfigureTimelinePopup::onCelThumbOpacityChange, this));
  m_box->celBackground()->Change.connect(&ConfigureTimelinePopup::onCelBackgroundChange, this);
  m_box->celShowThumb()->Click.connect(base::Bind<void>(&ConfigureTimelinePopup::onCelShowThumbChange, this));
  m_box->celShowZoom()->Click.connect(base::Bind<void>(&ConfigureTimelinePopup::onCelShowZoomChange, this));
  m_box->celZoomSize()->Change.connect(base::Bind<void>(&ConfigureTimelinePopup::onCelZoomSizeChange, this));

  m_celPreviewPrefConn =
    docPref().celPreview.AfterChange.connect(
      base::Bind<void>(&ConfigureTimelinePopup::updateWidgetsFromCurrentSettings, this));

  static_assert(doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR == 0 &&
                doc::algorithm::RESIZE_METHOD_BILINEAR == 1 &&
                doc::algorithm::RESIZE_METHOD_ROTSPRITE == 2,
                "ResizeMethod enum has changed");
  m_box->celQuality()->addItem("Nearest-neighbor");
  m_box->celQuality()->addItem("Bilinear");
  m_box->celQuality()->addItem("RotSprite");
  m_box->celQuality()->Change.connect(&ConfigureTimelinePopup::onCelQualityChange, this);
}

app::Document* ConfigureTimelinePopup::doc()
{
  return UIContext::instance()->activeDocument();
}

DocumentPreferences& ConfigureTimelinePopup::docPref()
{
  return Preferences::instance().document(doc());
}

void ConfigureTimelinePopup::updateWidgetsFromCurrentSettings()
{
  DocumentPreferences& docPref = this->docPref();
  base::ScopedValue<bool> lockUpdates(m_lockUpdates, true, false);

  switch (docPref.onionskin.type()) {
    case app::gen::OnionskinType::MERGE:
      m_box->merge()->setSelected(true);
      break;
    case app::gen::OnionskinType::RED_BLUE_TINT:
      m_box->tint()->setSelected(true);
      break;
  }
  m_box->opacity()->setValue(docPref.onionskin.opacityBase());
  m_box->opacityStep()->setValue(docPref.onionskin.opacityStep());
  m_box->loopTag()->setSelected(docPref.onionskin.loopTag());
  m_box->currentLayer()->setSelected(docPref.onionskin.currentLayer());

  switch (docPref.onionskin.type()) {
    case app::gen::OnionskinType::MERGE:
      m_box->merge()->setSelected(true);
      break;
    case app::gen::OnionskinType::RED_BLUE_TINT:
      m_box->tint()->setSelected(true);
      break;
  }

  switch (docPref.onionskin.position()) {
    case render::OnionskinPosition::BEHIND:
      m_box->behind()->setSelected(true);
      break;
    case render::OnionskinPosition::INFRONT:
      m_box->infront()->setSelected(true);
      break;
  }

  m_box->celQuality()->setSelectedItemIndex((int)(docPref.celPreview.quality()));
  m_box->celThumbOpacity()->setValue(docPref.celPreview.thumbOpacity());
  m_box->celBackground()->setColor(docPref.celPreview.background());
  m_box->celShowThumb()->setSelected(docPref.celPreview.showThumb());
  m_box->celShowZoom()->setSelected(docPref.celPreview.showZoom());
  m_box->celZoomSize()->setValue(docPref.celPreview.zoomSize());

}

bool ConfigureTimelinePopup::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage: {
      updateWidgetsFromCurrentSettings();
      break;

    }
  }
  return PopupWindow::onProcessMessage(msg);
}

void ConfigureTimelinePopup::onChangeType()
{
  if (m_lockUpdates)
    return;

  docPref().onionskin.type(m_box->merge()->isSelected() ?
    app::gen::OnionskinType::MERGE:
    app::gen::OnionskinType::RED_BLUE_TINT);
}

void ConfigureTimelinePopup::onOpacity()
{
  if (m_lockUpdates)
    return;

  docPref().onionskin.opacityBase(m_box->opacity()->getValue());
}

void ConfigureTimelinePopup::onOpacityStep()
{
  if (m_lockUpdates)
    return;

  docPref().onionskin.opacityStep(m_box->opacityStep()->getValue());
}

void ConfigureTimelinePopup::onResetOnionskin()
{
  DocumentPreferences& docPref = this->docPref();

  docPref.onionskin.type(docPref.onionskin.type.defaultValue());
  docPref.onionskin.opacityBase(docPref.onionskin.opacityBase.defaultValue());
  docPref.onionskin.opacityStep(docPref.onionskin.opacityStep.defaultValue());
  docPref.onionskin.loopTag(docPref.onionskin.loopTag.defaultValue());
  docPref.onionskin.currentLayer(docPref.onionskin.currentLayer.defaultValue());
  docPref.onionskin.position(docPref.onionskin.position.defaultValue());

  updateWidgetsFromCurrentSettings();
}

void ConfigureTimelinePopup::onLoopTagChange()
{
  docPref().onionskin.loopTag(m_box->loopTag()->isSelected());
}

void ConfigureTimelinePopup::onCurrentLayerChange()
{
  docPref().onionskin.currentLayer(m_box->currentLayer()->isSelected());
}

void ConfigureTimelinePopup::onPositionChange()
{
  docPref().onionskin.position(m_box->behind()->isSelected() ?
                               render::OnionskinPosition::BEHIND:
                               render::OnionskinPosition::INFRONT);
}

void ConfigureTimelinePopup::onCelThumbOpacityChange()
{
  docPref().celPreview.thumbOpacity(m_box->celThumbOpacity()->getValue());
}

void ConfigureTimelinePopup::onCelQualityChange()
{
  docPref().celPreview.quality(
    (doc::algorithm::ResizeMethod)(m_box->celQuality()->getSelectedItemIndex())
  );
}

void ConfigureTimelinePopup::onCelBackgroundChange(const app::Color& color)
{
  docPref().celPreview.background(color);
}

void ConfigureTimelinePopup::onCelShowThumbChange()
{
  docPref().celPreview.showThumb(m_box->celShowThumb()->isSelected());
}

void ConfigureTimelinePopup::onCelShowZoomChange()
{
  docPref().celPreview.showZoom(m_box->celShowZoom()->isSelected());
}

void ConfigureTimelinePopup::onCelZoomSizeChange()
{
  docPref().celPreview.zoomSize(m_box->celZoomSize()->getValue());
}


} // namespace app
