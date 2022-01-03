// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/keyboard_shortcuts.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/tools/active_tool.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui_context.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "ui/accelerator.h"
#include "ui/message.h"

#include <algorithm>
#include <set>
#include <vector>

#define XML_KEYBOARD_FILE_VERSION "1"
#define KEY_SHORTCUT(a) app::Strings::keyboard_shortcuts_ ##a()

namespace {

  struct KeyShortcutAction {
    const char* name;
    std::string userfriendly;
    app::KeyAction action;
    app::KeyContext context;
  };

  struct KeyShortcutWheelAction {
    const char* name;
    const std::string userfriendly;
    app::WheelAction action;
  };

  static std::vector<KeyShortcutAction>& shortcut_actions()
  {
    static std::vector<KeyShortcutAction> actions;
    if (actions.empty()) {
      actions = std::vector<KeyShortcutAction> {
        { "CopySelection"       , KEY_SHORTCUT(copy_selection)        , app::KeyAction::CopySelection, app::KeyContext::TranslatingSelection },
        { "SnapToGrid"          , KEY_SHORTCUT(snap_to_grid)          , app::KeyAction::SnapToGrid, app::KeyContext::TranslatingSelection },
        { "LockAxis"            , KEY_SHORTCUT(lock_axis)             , app::KeyAction::LockAxis, app::KeyContext::TranslatingSelection },
        { "FineControl"         , KEY_SHORTCUT(fine_translating)      , app::KeyAction::FineControl , app::KeyContext::TranslatingSelection },
        { "MaintainAspectRatio" , KEY_SHORTCUT(maintain_aspect_ratio) , app::KeyAction::MaintainAspectRatio, app::KeyContext::ScalingSelection },
        { "ScaleFromCenter"     , KEY_SHORTCUT(scale_from_center)     , app::KeyAction::ScaleFromCenter, app::KeyContext::ScalingSelection },
        { "FineControl"         , KEY_SHORTCUT(fine_scaling)          , app::KeyAction::FineControl , app::KeyContext::ScalingSelection },
        { "AngleSnap"           , KEY_SHORTCUT(angle_snap)            , app::KeyAction::AngleSnap, app::KeyContext::RotatingSelection },
        { "AddSelection"        , KEY_SHORTCUT(add_selection)         , app::KeyAction::AddSelection, app::KeyContext::SelectionTool },
        { "SubtractSelection"   , KEY_SHORTCUT(subtract_selection)    , app::KeyAction::SubtractSelection, app::KeyContext::SelectionTool },
        { "IntersectSelection"  , KEY_SHORTCUT(intersect_selection)   , app::KeyAction::IntersectSelection, app::KeyContext::SelectionTool },
        { "AutoSelectLayer"     , KEY_SHORTCUT(auto_select_layer)     , app::KeyAction::AutoSelectLayer, app::KeyContext::MoveTool },
        { "StraightLineFromLastPoint", KEY_SHORTCUT(line_from_last_point), app::KeyAction::StraightLineFromLastPoint, app::KeyContext::FreehandTool },
        { "AngleSnapFromLastPoint", KEY_SHORTCUT(angle_from_last_point) , app::KeyAction::AngleSnapFromLastPoint, app::KeyContext::FreehandTool },
        { "MoveOrigin"          , KEY_SHORTCUT(move_origin)           , app::KeyAction::MoveOrigin, app::KeyContext::ShapeTool },
        { "SquareAspect"        , KEY_SHORTCUT(square_aspect)         , app::KeyAction::SquareAspect, app::KeyContext::ShapeTool },
        { "DrawFromCenter"      , KEY_SHORTCUT(draw_from_center)      , app::KeyAction::DrawFromCenter, app::KeyContext::ShapeTool },
        { "RotateShape"         , KEY_SHORTCUT(rotate_shape)          , app::KeyAction::RotateShape, app::KeyContext::ShapeTool },
        { "LeftMouseButton"     , KEY_SHORTCUT(trigger_left_mouse_button) , app::KeyAction::LeftMouseButton, app::KeyContext::Any },
        { "RightMouseButton"    , KEY_SHORTCUT(trigger_right_mouse_button) , app::KeyAction::RightMouseButton, app::KeyContext::Any }
      };
    }
    return actions;
  }

  static std::vector<KeyShortcutWheelAction>& shortcut_wheel_actions()
  {
    static std::vector<KeyShortcutWheelAction> wheel_actions;
    if (wheel_actions.empty()) {
      wheel_actions = std::vector<KeyShortcutWheelAction> {
        { "Zoom"          , KEY_SHORTCUT(zoom)                     , app::WheelAction::Zoom },
        { "VScroll"       , KEY_SHORTCUT(scroll_vertically)        , app::WheelAction::VScroll },
        { "HScroll"       , KEY_SHORTCUT(scroll_horizontally)      , app::WheelAction::HScroll },
        { "FgColor"       , KEY_SHORTCUT(color_fg_pal_entry)       , app::WheelAction::FgColor },
        { "BgColor"       , KEY_SHORTCUT(color_bg_pal_entry)       , app::WheelAction::BgColor },
        { "Frame"         , KEY_SHORTCUT(change_frame)             , app::WheelAction::Frame },
        { "BrushSize"     , KEY_SHORTCUT(change_brush_size)        , app::WheelAction::BrushSize },
        { "BrushAngle"    , KEY_SHORTCUT(change_brush_angle)       , app::WheelAction::BrushAngle },
        { "ToolSameGroup" , KEY_SHORTCUT(change_tool_same_group)   , app::WheelAction::ToolSameGroup },
        { "ToolOtherGroup", KEY_SHORTCUT(change_tool)              , app::WheelAction::ToolOtherGroup },
        { "Layer"         , KEY_SHORTCUT(change_layer)             , app::WheelAction::Layer },
        { "InkOpacity"    , KEY_SHORTCUT(change_ink_opacity)       , app::WheelAction::InkOpacity },
        { "LayerOpacity"  , KEY_SHORTCUT(change_layer_opacity)     , app::WheelAction::LayerOpacity },
        { "CelOpacity"    , KEY_SHORTCUT(change_cel_opacity)       , app::WheelAction::CelOpacity },
        { "Alpha"         , KEY_SHORTCUT(color_alpha)              , app::WheelAction::Alpha },
        { "HslHue"        , KEY_SHORTCUT(color_hsl_hue)            , app::WheelAction::HslHue },
        { "HslSaturation" , KEY_SHORTCUT(color_hsl_saturation)     , app::WheelAction::HslSaturation },
        { "HslLightness"  , KEY_SHORTCUT(color_hsl_lightness)      , app::WheelAction::HslLightness },
        { "HsvHue"        , KEY_SHORTCUT(color_hsv_hue)            , app::WheelAction::HsvHue },
        { "HsvSaturation" , KEY_SHORTCUT(color_hsv_saturation)     , app::WheelAction::HsvSaturation },
        { "HsvValue"      , KEY_SHORTCUT(color_hsv_value)          , app::WheelAction::HsvValue }
      };
    }
    return wheel_actions;
  }


  static struct {
    const char* name;
    app::KeyContext context;
  } contexts[] = {
    { ""                     , app::KeyContext::Any },
    { "Normal"               , app::KeyContext::Normal },
    { "Selection"            , app::KeyContext::SelectionTool },
    { "TranslatingSelection" , app::KeyContext::TranslatingSelection },
    { "ScalingSelection"     , app::KeyContext::ScalingSelection },
    { "RotatingSelection"    , app::KeyContext::RotatingSelection },
    { "MoveTool"             , app::KeyContext::MoveTool },
    { "FreehandTool"         , app::KeyContext::FreehandTool },
    { "ShapeTool"            , app::KeyContext::ShapeTool },
    { nullptr                , app::KeyContext::Any }
  };


  const char* get_shortcut(TiXmlElement* elem) {
    const char* shortcut = NULL;

#ifdef _WIN32
    if (!shortcut) shortcut = elem->Attribute("win");
#elif defined __APPLE__
    if (!shortcut) shortcut = elem->Attribute("mac");
#else
    if (!shortcut) shortcut = elem->Attribute("linux");
#endif

    if (!shortcut)
      shortcut = elem->Attribute("shortcut");

    return shortcut;
  }

  std::string get_user_friendly_string_for_keyaction(app::KeyAction action,
                                                     app::KeyContext context) {
    const auto& actions = shortcut_actions();
    for (const auto& a: actions) {
      if (action == a.action && context == a.context)
        return a.userfriendly;
    }
    return std::string();
  }

  std::string get_user_friendly_string_for_wheelaction(app::WheelAction wheelAction) {
    const auto& wheel_actions = shortcut_wheel_actions();
    for (const auto& action: wheel_actions) {
      if (wheelAction == action.action)
        return action.userfriendly;
    }
    return std::string();
  }

} // anonymous namespace


namespace base {

  template<> app::KeyAction convert_to(const std::string& from) {
    const auto& actions = shortcut_actions();
    for (const auto& action: actions) {
      if (from == action.name)
        return action.action;
    }
    return app::KeyAction::None;
  }

  template<> std::string convert_to(const app::KeyAction& from) {
    const auto& actions = shortcut_actions();
    for (const auto& a: actions) {
      if (from == a.action)
        return a.name;
    }
    return std::string();
  }

  template<> app::WheelAction convert_to(const std::string& from) {
    const auto& wheel_actions = shortcut_wheel_actions();
    for (const auto& wheel_action: wheel_actions) {
      if (from == wheel_action.name)
        return wheel_action.action;
    }
    return app::WheelAction::None;
  }

  template<> std::string convert_to(const app::WheelAction& from) {
    const auto& wheel_actions = shortcut_wheel_actions();
    for (const auto& wheel_action: wheel_actions) {
      if (from == wheel_action.action)
        return wheel_action.name;
    }
    return std::string();
  }

  template<> app::KeyContext convert_to(const std::string& from) {
    for (int c=0; contexts[c].name; ++c) {
      if (from == contexts[c].name)
        return contexts[c].context;
    }
    return app::KeyContext::Any;
  }

  template<> std::string convert_to(const app::KeyContext& from) {
    for (int c=0; contexts[c].name; ++c) {
      if (from == contexts[c].context)
        return contexts[c].name;
    }
    return std::string();
  }

} // namespace base

namespace app {

using namespace ui;

//////////////////////////////////////////////////////////////////////
// Key

Key::Key(Command* command, const Params& params, const KeyContext keyContext)
  : m_type(KeyType::Command)
  , m_useUsers(false)
  , m_keycontext(keyContext)
  , m_command(command)
  , m_params(params)
{
}

Key::Key(const KeyType type, tools::Tool* tool)
  : m_type(type)
  , m_useUsers(false)
  , m_keycontext(KeyContext::Any)
  , m_tool(tool)
{
}

Key::Key(const KeyAction action,
         const KeyContext keyContext)
  : m_type(KeyType::Action)
  , m_useUsers(false)
  , m_keycontext(keyContext)
  , m_action(action)
{
  if (m_keycontext != KeyContext::Any)
    return;

  // Automatic key context
  switch (action) {
    case KeyAction::None:
      m_keycontext = KeyContext::Any;
      break;
    case KeyAction::CopySelection:
    case KeyAction::SnapToGrid:
    case KeyAction::LockAxis:
    case KeyAction::FineControl:
      m_keycontext = KeyContext::TranslatingSelection;
      break;
    case KeyAction::AngleSnap:
      m_keycontext = KeyContext::RotatingSelection;
      break;
    case KeyAction::MaintainAspectRatio:
    case KeyAction::ScaleFromCenter:
      m_keycontext = KeyContext::ScalingSelection;
      break;
    case KeyAction::AddSelection:
    case KeyAction::SubtractSelection:
    case KeyAction::IntersectSelection:
      m_keycontext = KeyContext::SelectionTool;
      break;
    case KeyAction::AutoSelectLayer:
      m_keycontext = KeyContext::MoveTool;
      break;
    case KeyAction::StraightLineFromLastPoint:
    case KeyAction::AngleSnapFromLastPoint:
      m_keycontext = KeyContext::FreehandTool;
      break;
    case KeyAction::MoveOrigin:
    case KeyAction::SquareAspect:
    case KeyAction::DrawFromCenter:
    case KeyAction::RotateShape:
      m_keycontext = KeyContext::ShapeTool;
      break;
    case KeyAction::LeftMouseButton:
      m_keycontext = KeyContext::Any;
      break;
    case KeyAction::RightMouseButton:
      m_keycontext = KeyContext::Any;
      break;
  }
}

Key::Key(const WheelAction wheelAction)
  : m_type(KeyType::WheelAction)
  , m_useUsers(false)
  , m_keycontext(KeyContext::MouseWheel)
  , m_action(KeyAction::None)
  , m_wheelAction(wheelAction)
{
}

void Key::add(const ui::Accelerator& accel,
              const KeySource source,
              KeyboardShortcuts& globalKeys)
{
  Accelerators* accels = &m_accels;

  if (source == KeySource::UserDefined) {
    if (!m_useUsers) {
      m_useUsers = true;
      m_users = m_accels;
    }
    accels = &m_users;
  }

  // Remove the accelerator from other commands
  if (source == KeySource::UserDefined) {
    globalKeys.disableAccel(accel, m_keycontext, this);
    m_userRemoved.remove(accel);
  }

  // Add the accelerator
  accels->add(accel);
}

const ui::Accelerator* Key::isPressed(const Message* msg,
                                      KeyboardShortcuts& globalKeys) const
{
  if (auto keyMsg = dynamic_cast<const KeyMessage*>(msg)) {
    for (const Accelerator& accel : accels()) {
      if (accel.isPressed(keyMsg->modifiers(),
                          keyMsg->scancode(),
                          keyMsg->unicodeChar()) &&
          (m_keycontext == KeyContext::Any ||
           m_keycontext == globalKeys.getCurrentKeyContext())) {
        return &accel;
      }
    }
  }
  else if (auto mouseMsg = dynamic_cast<const MouseMessage*>(msg)) {
    for (const Accelerator& accel : accels()) {
      if ((accel.modifiers() == mouseMsg->modifiers()) &&
          (m_keycontext == KeyContext::Any ||
           // TODO we could have multiple mouse wheel key-context,
           // like "sprite editor" context, or "timeline" context,
           // etc.
           m_keycontext == KeyContext::MouseWheel)) {
        return &accel;
      }
    }
  }
  return nullptr;
}

bool Key::isPressed() const
{
  for (const Accelerator& accel : this->accels()) {
    if (accel.isPressed())
      return true;
  }
  return false;
}

bool Key::isLooselyPressed() const
{
  for (const Accelerator& accel : this->accels()) {
    if (accel.isLooselyPressed())
      return true;
  }
  return false;
}

bool Key::hasAccel(const ui::Accelerator& accel) const
{
  return accels().has(accel);
}

void Key::disableAccel(const ui::Accelerator& accel)
{
  if (!m_useUsers) {
    m_useUsers = true;
    m_users = m_accels;
  }

  m_users.remove(accel);

  if (m_accels.has(accel))
    m_userRemoved.add(accel);
}

void Key::reset()
{
  m_users.clear();
  m_userRemoved.clear();
  m_useUsers = false;
}

void Key::copyOriginalToUser()
{
  m_users = m_accels;
  m_userRemoved.clear();
  m_useUsers = true;
}

std::string Key::triggerString() const
{
  switch (m_type) {
    case KeyType::Command:
      m_command->loadParams(m_params);
      return m_command->friendlyName();
    case KeyType::Tool:
    case KeyType::Quicktool: {
      std::string text = m_tool->getText();
      if (m_type == KeyType::Quicktool)
        text += " (quick)";
      return text;
    }
    case KeyType::Action:
      return get_user_friendly_string_for_keyaction(m_action,
                                                    m_keycontext);
    case KeyType::WheelAction:
      return get_user_friendly_string_for_wheelaction(m_wheelAction);
  }
  return "Unknown";
}

//////////////////////////////////////////////////////////////////////
// KeyboardShortcuts

KeyboardShortcuts* KeyboardShortcuts::instance()
{
  static KeyboardShortcuts* singleton = NULL;
  if (!singleton)
    singleton = new KeyboardShortcuts();
  return singleton;
}

KeyboardShortcuts::KeyboardShortcuts()
{
}

KeyboardShortcuts::~KeyboardShortcuts()
{
  clear();
}

void KeyboardShortcuts::setKeys(const KeyboardShortcuts& keys,
                                const bool cloneKeys)
{
  if (cloneKeys) {
    for (const KeyPtr& key : keys)
      m_keys.push_back(std::make_shared<Key>(*key));
  }
  else {
    m_keys = keys.m_keys;
  }
  UserChange();
}

void KeyboardShortcuts::clear()
{
  m_keys.clear();
}

void KeyboardShortcuts::importFile(TiXmlElement* rootElement, KeySource source)
{
  // <keyboard><commands><key>
  TiXmlHandle handle(rootElement);
  TiXmlElement* xmlKey = handle
    .FirstChild("commands")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* command_name = xmlKey->Attribute("command");
    const char* command_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (command_name) {
      Command* command = Commands::instance()->byId(command_name);
      if (command) {
        // Read context
        KeyContext keycontext = KeyContext::Any;
        const char* keycontextstr = xmlKey->Attribute("context");
        if (keycontextstr)
          keycontext = base::convert_to<KeyContext>(std::string(keycontextstr));

        // Read params
        Params params;

        TiXmlElement* xmlParam = xmlKey->FirstChildElement("param");
        while (xmlParam) {
          const char* param_name = xmlParam->Attribute("name");
          const char* param_value = xmlParam->Attribute("value");

          if (param_name && param_value)
            params.set(param_name, param_value);

          xmlParam = xmlParam->NextSiblingElement();
        }

        // add the keyboard shortcut to the command
        KeyPtr key = this->command(command_name, params, keycontext);
        if (key && command_key) {
          Accelerator accel(command_key);

          if (!removed) {
            key->add(accel, source, *this);

            // Add the shortcut to the menuitems with this command
            // (this is only visual, the
            // "CustomizedGuiManager::onProcessMessage" is the only
            // one that process keyboard shortcuts)
            if (key->accels().size() == 1) {
              AppMenus::instance()->applyShortcutToMenuitemsWithCommand(
                command, params, key);
            }
          }
          else
            key->disableAccel(accel);
        }
      }
    }

    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for tools
  // <keyboard><tools><key>
  xmlKey = handle
    .FirstChild("tools")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (tool_id) {
      tools::Tool* tool = App::instance()->toolBox()->getToolById(tool_id);
      if (tool) {
        KeyPtr key = this->tool(tool);
        if (key && tool_key) {
          LOG(VERBOSE, "KEYS: Shortcut for tool %s: %s\n", tool_id, tool_key);
          Accelerator accel(tool_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for quicktools
  // <keyboard><quicktools><key>
  xmlKey = handle
    .FirstChild("quicktools")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (tool_id) {
      tools::Tool* tool = App::instance()->toolBox()->getToolById(tool_id);
      if (tool) {
        KeyPtr key = this->quicktool(tool);
        if (key && tool_key) {
          LOG(VERBOSE, "KEYS: Shortcut for quicktool %s: %s\n", tool_id, tool_key);
          Accelerator accel(tool_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts for sprite editor customization
  // <keyboard><actions><key>
  xmlKey = handle
    .FirstChild("actions")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* action_id = xmlKey->Attribute("action");
    const char* action_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (action_id) {
      KeyAction action = base::convert_to<KeyAction, std::string>(action_id);
      if (action != KeyAction::None) {
        // Read context
        KeyContext keycontext = KeyContext::Any;
        const char* keycontextstr = xmlKey->Attribute("context");
        if (keycontextstr)
          keycontext = base::convert_to<KeyContext>(std::string(keycontextstr));

        KeyPtr key = this->action(action, keycontext);
        if (key && action_key) {
          LOG(VERBOSE, "KEYS: Shortcut for action %s/%s: %s\n", action_id,
              (keycontextstr ? keycontextstr: "Any"), action_key);
          Accelerator accel(action_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts for mouse wheel customization
  // <keyboard><wheel><key>
  xmlKey = handle
    .FirstChild("wheel")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* action_id = xmlKey->Attribute("action");
    const char* action_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (action_id) {
      WheelAction action = base::convert_to<WheelAction, std::string>(action_id);
      if (action != WheelAction::None) {
        KeyPtr key = this->wheelAction(action);
        if (key && action_key) {
          LOG(VERBOSE, "KEYS: Shortcut for wheel action %s: %s\n", action_id, action_key);
          Accelerator accel(action_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }
}

void KeyboardShortcuts::importFile(const std::string& filename, KeySource source)
{
  XmlDocumentRef doc = app::open_xml(filename);
  TiXmlHandle handle(doc.get());
  TiXmlElement* xmlKey = handle.FirstChild("keyboard").ToElement();

  importFile(xmlKey, source);
}

void KeyboardShortcuts::exportFile(const std::string& filename)
{
  XmlDocumentRef doc(new TiXmlDocument());

  TiXmlElement keyboard("keyboard");
  TiXmlElement commands("commands");
  TiXmlElement tools("tools");
  TiXmlElement quicktools("quicktools");
  TiXmlElement actions("actions");
  TiXmlElement wheel("wheel");

  keyboard.SetAttribute("version", XML_KEYBOARD_FILE_VERSION);

  exportKeys(commands, KeyType::Command);
  exportKeys(tools, KeyType::Tool);
  exportKeys(quicktools, KeyType::Quicktool);
  exportKeys(actions, KeyType::Action);
  exportKeys(wheel, KeyType::WheelAction);

  keyboard.InsertEndChild(commands);
  keyboard.InsertEndChild(tools);
  keyboard.InsertEndChild(quicktools);
  keyboard.InsertEndChild(actions);
  keyboard.InsertEndChild(wheel);

  TiXmlDeclaration declaration("1.0", "utf-8", "");
  doc->InsertEndChild(declaration);
  doc->InsertEndChild(keyboard);
  save_xml(doc, filename);
}

void KeyboardShortcuts::exportKeys(TiXmlElement& parent, KeyType type)
{
  for (KeyPtr& key : m_keys) {
    // Save only user defined accelerators.
    if (key->type() != type)
      continue;

    for (const ui::Accelerator& accel : key->userRemovedAccels())
      exportAccel(parent, key.get(), accel, true);

    for (const ui::Accelerator& accel : key->userAccels())
      exportAccel(parent, key.get(), accel, false);
  }
}

void KeyboardShortcuts::exportAccel(TiXmlElement& parent, const Key* key, const ui::Accelerator& accel, bool removed)
{
  TiXmlElement elem("key");

  switch (key->type()) {

    case KeyType::Command: {
      elem.SetAttribute("command", key->command()->id().c_str());

      if (key->keycontext() != KeyContext::Any) {
        elem.SetAttribute("context",
                          base::convert_to<std::string>(key->keycontext()).c_str());
      }

      for (const auto& param : key->params()) {
        if (param.second.empty())
          continue;

        TiXmlElement paramElem("param");
        paramElem.SetAttribute("name", param.first.c_str());
        paramElem.SetAttribute("value", param.second.c_str());
        elem.InsertEndChild(paramElem);
      }
      break;
    }

    case KeyType::Tool:
    case KeyType::Quicktool:
      elem.SetAttribute("tool", key->tool()->getId().c_str());
      break;

    case KeyType::Action:
      elem.SetAttribute("action",
                        base::convert_to<std::string>(key->action()).c_str());
      if (key->keycontext() != KeyContext::Any)
        elem.SetAttribute("context",
                          base::convert_to<std::string>(key->keycontext()).c_str());
      break;

    case KeyType::WheelAction:
      elem.SetAttribute("action",
                        base::convert_to<std::string>(key->wheelAction()).c_str());
      break;
  }

  elem.SetAttribute("shortcut", accel.toString().c_str());

  if (removed)
    elem.SetAttribute("removed", "true");

  parent.InsertEndChild(elem);
}

void KeyboardShortcuts::reset()
{
  for (KeyPtr& key : m_keys)
    key->reset();
}

KeyPtr KeyboardShortcuts::command(const char* commandName, const Params& params, KeyContext keyContext)
{
  Command* command = Commands::instance()->byId(commandName);
  if (!command)
    return nullptr;

  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Command &&
        key->keycontext() == keyContext &&
        key->command() == command &&
        key->params() == params) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(command, params, keyContext);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::tool(tools::Tool* tool)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Tool &&
        key->tool() == tool) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(KeyType::Tool, tool);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::quicktool(tools::Tool* tool)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Quicktool &&
        key->tool() == tool) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(KeyType::Quicktool, tool);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::action(KeyAction action,
                                 KeyContext keyContext)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Action &&
        key->action() == action &&
        key->keycontext() == keyContext) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(action, keyContext);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::wheelAction(WheelAction wheelAction)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::WheelAction &&
        key->wheelAction() == wheelAction) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(wheelAction);
  m_keys.push_back(key);
  return key;
}

void KeyboardShortcuts::disableAccel(const ui::Accelerator& accel,
                                     const KeyContext keyContext,
                                     const Key* newKey)
{
  for (KeyPtr& key : m_keys) {
    if (key->keycontext() == keyContext &&
        key->hasAccel(accel) &&
        // Tools can contain the same keyboard shortcut
        (key->type() != KeyType::Tool ||
         newKey == nullptr ||
         newKey->type() != KeyType::Tool)) {
      key->disableAccel(accel);
    }
  }
}

KeyContext KeyboardShortcuts::getCurrentKeyContext()
{
  Doc* doc = UIContext::instance()->activeDocument();
  if (doc &&
      doc->isMaskVisible() &&
      // The active key context will be the selectedTool() (in the
      // toolbox) instead of the activeTool() (which depends on the
      // quick tool shortcuts).
      //
      // E.g. If we have the rectangular marquee tool selected
      // (selectedTool()) are going to press keys like alt+left or
      // alt+right to move the selection edge in the selection
      // context, the alt key switches the activeTool() to the
      // eyedropper, but we want to use alt+left and alt+right in the
      // original context (the selection tool).
      App::instance()->activeToolManager()
        ->selectedTool()->getInk(0)->isSelection())
    return KeyContext::SelectionTool;
  else
    return KeyContext::Normal;
}

bool KeyboardShortcuts::getCommandFromKeyMessage(const Message* msg, Command** command, Params* params)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Command &&
        key->isPressed(msg, *this)) {
      if (command) *command = key->command();
      if (params) *params = key->params();
      return true;
    }
  }
  return false;
}

tools::Tool* KeyboardShortcuts::getCurrentQuicktool(tools::Tool* currentTool)
{
  if (currentTool && currentTool->getInk(0)->isSelection()) {
    KeyPtr key = action(KeyAction::CopySelection, KeyContext::TranslatingSelection);
    if (key && key->isPressed())
      return NULL;
  }

  tools::ToolBox* toolbox = App::instance()->toolBox();

  // Iterate over all tools
  for (tools::Tool* tool : *toolbox) {
    KeyPtr key = quicktool(tool);

    // Collect all tools with the pressed keyboard-shortcut
    if (key && key->isPressed()) {
      return tool;
    }
  }

  return NULL;
}

KeyAction KeyboardShortcuts::getCurrentActionModifiers(KeyContext context)
{
  KeyAction flags = KeyAction::None;

  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Action &&
        key->keycontext() == context &&
        key->isLooselyPressed()) {
      flags = static_cast<KeyAction>(int(flags) | int(key->action()));
    }
  }

  return flags;
}

WheelAction KeyboardShortcuts::getWheelActionFromMouseMessage(const KeyContext context,
                                                              const ui::Message* msg)
{
  WheelAction wheelAction = WheelAction::None;
  const ui::Accelerator* bestAccel = nullptr;
  KeyPtr bestKey;
  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::WheelAction &&
        key->keycontext() == context) {
      const ui::Accelerator* accel = key->isPressed(msg, *this);
      if ((accel) &&
          (!bestAccel || bestAccel->modifiers() < accel->modifiers())) {
        bestAccel = accel;
        wheelAction = key->wheelAction();
      }
    }
  }
  return wheelAction;
}

bool KeyboardShortcuts::hasMouseWheelCustomization() const
{
  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::WheelAction &&
        !key->userAccels().empty())
      return true;
  }
  return false;
}

void KeyboardShortcuts::clearMouseWheelKeys()
{
  for (auto it=m_keys.begin(); it!=m_keys.end(); ) {
    if ((*it)->type() == KeyType::WheelAction)
      it = m_keys.erase(it);
    else
      ++it;
  }
}

void KeyboardShortcuts::addMissingMouseWheelKeys()
{
  for (int wheelAction=int(WheelAction::First);
       wheelAction<=int(WheelAction::Last); ++wheelAction) {
    auto it = std::find_if(
      m_keys.begin(), m_keys.end(),
      [wheelAction](const KeyPtr& key) -> bool {
        return key->wheelAction() == (WheelAction)wheelAction;
      });
    if (it == m_keys.end()) {
      KeyPtr key = std::make_shared<Key>((WheelAction)wheelAction);
      m_keys.push_back(key);
    }
  }
}

void KeyboardShortcuts::setDefaultMouseWheelKeys(const bool zoomWithWheel)
{
  clearMouseWheelKeys();

  KeyPtr key;
  key = std::make_shared<Key>(WheelAction::Zoom);
  key->add(Accelerator(zoomWithWheel ? kKeyNoneModifier:
                                       kKeyCtrlModifier, kKeyNil, 0),
           KeySource::Original, *this);
  m_keys.push_back(key);

  if (!zoomWithWheel) {
    key = std::make_shared<Key>(WheelAction::VScroll);
    key->add(Accelerator(kKeyNoneModifier, kKeyNil, 0),
             KeySource::Original, *this);
    m_keys.push_back(key);
  }

  key = std::make_shared<Key>(WheelAction::HScroll);
  key->add(Accelerator(kKeyShiftModifier, kKeyNil, 0),
           KeySource::Original, *this);
  m_keys.push_back(key);

  key = std::make_shared<Key>(WheelAction::FgColor);
  key->add(Accelerator(kKeyAltModifier, kKeyNil, 0),
           KeySource::Original, *this);
  m_keys.push_back(key);

  key = std::make_shared<Key>(WheelAction::BgColor);
  key->add(Accelerator((KeyModifiers)(kKeyAltModifier | kKeyShiftModifier), kKeyNil, 0),
           KeySource::Original, *this);
  m_keys.push_back(key);

  if (zoomWithWheel) {
    key = std::make_shared<Key>(WheelAction::BrushSize);
    key->add(Accelerator(kKeyCtrlModifier, kKeyNil, 0),
             KeySource::Original, *this);
    m_keys.push_back(key);

    key = std::make_shared<Key>(WheelAction::Frame);
    key->add(Accelerator((KeyModifiers)(kKeyCtrlModifier | kKeyShiftModifier), kKeyNil, 0),
             KeySource::Original, *this);
    m_keys.push_back(key);
  }
}

void KeyboardShortcuts::addMissingKeysForCommands()
{
  std::set<std::string> commandsAlreadyAdded;
  for (const KeyPtr& key : m_keys) {
    if (key->type() != KeyType::Command)
      continue;

    if (key->params().empty())
      commandsAlreadyAdded.insert(key->command()->id());
  }

  std::vector<std::string> ids;
  Commands* commands = Commands::instance();
  commands->getAllIds(ids);

  for (const std::string& id : ids) {
    Command* command = commands->byId(id.c_str());

    // Don't add commands that need params (they will be added to
    // the list using the list of keyboard shortcuts from gui.xml).
    if (command->needsParams())
      continue;

    auto it = commandsAlreadyAdded.find(command->id());
    if (it != commandsAlreadyAdded.end())
      continue;

    // Create the new Key element in KeyboardShortcuts for this
    // command without params.
    this->command(command->id().c_str());
  }
}

std::string key_tooltip(const char* str, const app::Key* key)
{
  std::string res;
  if (str)
    res += str;
  if (key && !key->accels().empty()) {
    res += " (";
    res += key->accels().front().toString();
    res += ")";
  }
  return res;
}

void clear_tool_actions()
{
  shortcut_actions().clear();
}

void clear_wheel_actions()
{
  shortcut_wheel_actions().clear();
}

std::string convertKeyContextToUserFriendlyString(KeyContext keyContext)
{
  switch (keyContext) {
    case KeyContext::Any:
      return std::string();
    case KeyContext::Normal:
      return KEY_SHORTCUT(key_context_normal);
    case KeyContext::SelectionTool:
      return KEY_SHORTCUT(key_context_selection);
    case KeyContext::TranslatingSelection:
      return KEY_SHORTCUT(key_context_translating_selection);
    case KeyContext::ScalingSelection:
      return KEY_SHORTCUT(key_context_scaling_selection);
    case KeyContext::RotatingSelection:
      return KEY_SHORTCUT(key_context_rotating_selection);
    case KeyContext::MoveTool:
      return KEY_SHORTCUT(key_context_move_tool);
    case KeyContext::FreehandTool:
      return KEY_SHORTCUT(key_context_freehand_tool);
    case KeyContext::ShapeTool:
      return KEY_SHORTCUT(key_context_shape_tool);
  }
  return std::string();
}

} // namespace app

#undef KEY_SHORTCUT
