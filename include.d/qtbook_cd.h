#ifndef _QTBOOK_CD_H_
#define _QTBOOK_CD_H_

/*
** -- Qt Includes --
*/

#include <QMenu>
#include <QDialog>
#include <QMainWindow>
#include <QStringList>
#include <QGraphicsScene>

/*
** -- Local Includes --
*/

#include "qtbook.h"
#include "ui_cdinfo.h"
#include "ui_tracks.h"
#include "copy_editor.h"
#include "qtbook_item.h"
#include "misc_functions.h"
#include "borrowers_editor.h"

class copy_editor;
class borrowers_editor;

class qtbook_cd: public QMainWindow, public qtbook_item
{
  Q_OBJECT

 public:
  qtbook_cd(QMainWindow *, const QStringList &, const QStringList &,
	    const QStringList &, const QStringList &, const QStringList &,
	    const QString &, const int);
  ~qtbook_cd();
  void insert(void);
  void modify(const int);
  void search(const QString & = "", const QString & = "");
  void updateWindow(const int);
  virtual void closeEvent(QCloseEvent *);

 private:
  QDialog *tracks_diag;
  Ui_cdDialog cd;
  Ui_tracksDialog trd;

 private slots:
  void slotGo(void);
  void slotPrint(void);
  void slotQuery(void);
  void slotReset(void);
  void slotCancel(void);
  void slotShowUsers(void);
  void slotSaveTracks(void);
  void slotDeleteTrack(void);
  void slotInsertTrack(void);
  void slotSelectImage(void);
  void slotComputeRuntime(void);
  void slotCloseTracksBrowser(void);
  void slotPopulateCopiesEditor(void);
  void slotPopulateTracksBrowser(void);
};

#endif
