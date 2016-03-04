// GTK Component of SHE library
// Copyright (C) 2016  Gabriel Rauter
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

//disable EMPTY_STRING macro already set in allegro, enabling it at the end of file
#pragma push_macro("EMPTY_STRING")
#undef EMPTY_STRING
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/gtk/native_dialogs.h"

#include "base/string.h"
#include "she/display.h"
#include "she/error.h"

#include <gtkmm/application.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/button.h>
#include <gtkmm/filefilter.h>
#include <glibmm/refptr.h>
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>

#include <string>
#include <map>

namespace she {

class FileDialogGTK3 : public FileDialog, public Gtk::FileChooserDialog {
public:
  FileDialogGTK3(Glib::RefPtr<Gtk::Application> app) :
  Gtk::FileChooserDialog(""), m_app(app), m_cancel(true) {
    this->add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    m_ok_button = this->add_button("_Open", Gtk::RESPONSE_OK);
    this->set_default_response(Gtk::RESPONSE_OK);
    m_filter_all = Gtk::FileFilter::create();
    m_filter_all->set_name("All formats");
    this->set_do_overwrite_confirmation();
    if (FileDialog::g_lastUsedDir().empty()) {
    #ifdef ASEPRITE_DEPRECATED_GLIB_SUPPORT
      FileDialog::g_lastUsedDir() = Glib::get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
    #else
       FileDialog::g_lastUsedDir() = Glib::get_user_special_dir(Glib::USER_DIRECTORY_DOCUMENTS);
    #endif
    }
  }

  void on_show() override {
    //setting the filename only works properly when the dialog is shown
    if (!m_file_name.empty()) {
      if (this->get_action() == Gtk::FILE_CHOOSER_ACTION_OPEN) {
        this->set_current_folder(m_file_name);
      } else {
        if (Glib::file_test(m_file_name, Glib::FILE_TEST_EXISTS)) {
          this->set_filename(m_file_name);
        } else {
          this->set_current_folder(FileDialog::g_lastUsedDir());
          this->set_current_name(m_file_name);
        }
      }
    } else {
      this->set_current_folder(FileDialog::g_lastUsedDir());
    }
    //TODO set position centered to parent window, need she::screen to provide info
    this->set_position(Gtk::WIN_POS_CENTER);
    Gtk::FileChooserDialog::on_show();
    this->raise();
  }

  void on_response(int response_id) override {
    switch(response_id) {
      case(Gtk::RESPONSE_OK): {
        m_cancel = false;
        m_file_name = this->get_filename();
        FileDialog::g_lastUsedDir() = this->get_current_folder();
        break;
      }
    }
    this->hide();
  }

  void dispose() override {
    for (auto& window  : m_app->get_windows()) {
      window->close();
    }
    m_app->quit();
    delete this;
  }

  void toOpenFile() override {
    this->set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
    m_ok_button->set_label("_Open");
  }

  void toSaveFile() override {
    this->set_action(Gtk::FILE_CHOOSER_ACTION_SAVE);
    m_ok_button->set_label("_Save");
  }

  void setTitle(const std::string& title) override {
    this->set_title(title);
  }

  void setDefaultExtension(const std::string& extension) override {
    m_default_extension = extension;
  }

  void addFilter(const std::string& extension, const std::string& description) override {
    auto filter = Gtk::FileFilter::create();
    filter->set_name(description);
    filter->add_pattern("*." + extension);
    m_filter_all->add_pattern("*." + extension);
    m_filters[extension] = filter;
  }

  std::string fileName() override {
    return m_file_name;
  }

  void setFileName(const std::string& filename) override {
    m_file_name = filename;
  }

  bool show(Display* parent) override {
    //keep pointer on parent display to get information later
    m_display = parent;

    //add filters in order they will appear
    this->add_filter(m_filter_all);

    for (const auto& filter : m_filters) {
      this->add_filter(filter.second)	;
      if (filter.first.compare(m_default_extension) == 0) {
        this->set_filter(filter.second);
      }
    }

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    this->add_filter(filter_any);

    //Run dialog in context of a gtk application so it can be destroys properly
    m_app->run(*this);
    return !m_cancel;
  }

private:
  std::string m_file_name;
  std::string m_default_extension;
  Glib::RefPtr<Gtk::FileFilter> m_filter_all;
  std::map<std::string, Glib::RefPtr<Gtk::FileFilter>> m_filters;
  Gtk::Button* m_ok_button;
  Glib::RefPtr<Gtk::Application> m_app;
  Display* m_display;
  bool m_cancel;
};

NativeDialogsGTK3::NativeDialogsGTK3()
{
}

FileDialog* NativeDialogsGTK3::createFileDialog()
{
  m_app = Gtk::Application::create();
  FileDialog* dialog = new FileDialogGTK3(m_app);
  return dialog;
}

} // namespace she
#pragma pop_macro("EMPTY_STRING")
