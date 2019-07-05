/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "import.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

// FIXME: Only used for test code
#include "panel/panelfocusmanager.h"
#include "panel/project/project.h"
// End test code

#include "project/item/footage/footage.h"
#include "task/probe/probe.h"
#include "task/taskmanager.h"

ImportTask::ImportTask(Folder *parent, const QStringList &urls) :
  urls_(urls),
  parent_(parent)
{
  set_text(tr("Importing %1 files").arg(urls.size()));
}

bool ImportTask::Action()
{
  for (int i=0;i<urls_.size();i++) {

    const QString& url = urls_.at(i);

    QFileInfo file_info(url);

    // Check if this file is a diretory
    if (file_info.isDir()) {

      // Use QDir to get a list of all the files in the directory
      QDir dir(url);

      // QDir::entryList only returns filenames, we can use entryInfoList() to get full paths
      QFileInfoList entry_list = dir.entryInfoList();

      // Only proceed if the empty actually has files in it
      if (!entry_list.isEmpty()) {
        // Create a folder corresponding to the directory
        Folder* f = new Folder();

        f->set_name(file_info.fileName());

        parent_->add_child(f);

        // Convert QFileInfoList into QStringList
        QStringList full_urls;

        foreach (QFileInfo info, entry_list) {
          if (info.fileName() != ".." && info.fileName() != ".") {
            full_urls.append(info.absoluteFilePath());
          }
        }

        // Start a new import task for this folder too
        ImportTask* it = new ImportTask(f, full_urls);

        it->moveToThread(qApp->thread());

        olive::task_manager.AddTask(it);
      }

    } else {

      Footage* f = new Footage();

      // FIXME: Is it possible for a file to go missing between the Import dialog and here?
      //        And what is the behavior/result of that?

      f->set_filename(url);
      f->set_name(file_info.fileName());
      f->set_timestamp(file_info.lastModified());

      parent_->add_child(f);

      // Create ProbeTask to analyze this media
      ProbeTask* pt = new ProbeTask(f);

      // The task won't work unless it's in the main thread and we're definitely not
      // FIXME: Should Tasks check what thread they're in and move themselves to the main thread?
      pt->moveToThread(qApp->thread());

      // Queue task in task manager
      olive::task_manager.AddTask(pt);

    }

    emit ProgressChanged(i * 100 / urls_.size());

  }

  // FIXME: No good very bad test code to update the ProjectExplorer model/view
  //        A better solution would be for the Project itself to signal that a new row was being added
  ProjectPanel* p = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();
  p->set_project(p->project());
  // End test code

  return true;
}
