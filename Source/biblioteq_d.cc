#include <QDir>
#include <QFileDialog>
#include <QSettings>

#include "biblioteq.h"
#include "biblioteq_documentationwindow.h"
#include "biblioteq_files.h"
#include "biblioteq_otheroptions.h"
#include "biblioteq_sqlite_merge_databases.h"

QString biblioteq::formattedISBN10(const QString &str) const
{
  return m_otheroptions->isbn10DisplayFormat(str);
}

QString biblioteq::formattedISBN13(const QString &str) const
{
  return m_otheroptions->isbn13DisplayFormat(str);
}

bool biblioteq::isCurrentItemAPhotograph(void) const
{
  return biblioteq_misc_functions::getColumnString
    (ui.table, ui.table->currentRow(), ui.table->columnNumber("Type")) ==
    "Photograph Collection";
}

bool biblioteq::showBookReadStatus(void) const
{
  return m_otheroptions->showBookReadStatus();
}

bool biblioteq::showMainTableImages(void) const
{
  return m_otheroptions->showMainTableImages();
}

void biblioteq::slotContributors(void)
{
  if(!m_contributors)
    {
      m_contributors = new biblioteq_documentationwindow(this);
      m_contributors->setHtml
	(tr("<html>"
	    "<b>The following people have made BiblioteQ beautiful. "
	    "Thank You!</b><br><br>"
	    "Ana Monteiro<br>"
	    "Arti<br>"
	    "Bill Burns<br>"
	    "Ceres<br>"
	    "Csanád Baksay<br>"
	    "Fares Othman<br>"
	    "Fátima Dias<br>"
	    "Frans<br>"
	    "J. Cornavin<br>"
	    "Jeepee<br>"
	    "Lazaros S.<br>"
	    "Leif-W<br>"
	    "Marty<br>"
	    "Nick<br>"
	    "Nicola<br>"
	    "ResetFlag<br>"
	    "Safranil<br>"
	    "SigmaX<br>"
	    "a12554<br>"
	    "caalma<br>"
	    "coldacid<br>"
	    "fmra357<br>"
	    "jerzyPL<br>"
	    "jferby<br>"
	    "luismontilla<br>"
	    "meteos77<br>"
	    "numibesi<br>"
	    "ozgurcan<br>"
	    "robindegen<br>"
	    "sidheban<br>"
	    "sit42<br>"
	    "sunyuyangg555<br>"
	    "tamascz<br>"
	    "wohali<br>"
	    "yasbean"
	    "</html>"));
      m_contributors->setWindowTitle(tr("BiblioteQ: Contributors"));
    }

  m_contributors->show();
}

void biblioteq::slotExportAsPNG(void)
{
  QFileDialog dialog(this);

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix("png");
  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter(tr("PNG (*.png)"));
  dialog.setOption(QFileDialog::DontUseNativeDialog);
  dialog.setWindowTitle(tr("BiblioteQ: Export Icons View As PNG"));
  dialog.exec();
  QApplication::processEvents();

  if(dialog.result() == QDialog::Accepted)
    {
      QApplication::setOverrideCursor(Qt::WaitCursor);

      QImage image
	(ui.graphicsView->scene()->itemsBoundingRect().size().toSize(),
	 QImage::Format_RGB32);
      QPainter painter;

      image.fill(Qt::white);
      painter.begin(&image);
      ui.graphicsView->scene()->render(&painter);
      painter.end();
      image.save(dialog.selectedFiles().value(0), "PNG", 100);
      QApplication::restoreOverrideCursor();
    }
}

void biblioteq::slotMembersPagesChanged(int value)
{
  QSettings settings;

  settings.setValue("membersPerPage", value);
}

void biblioteq::slotMergeSQLiteDatabases(void)
{
  if(!m_sqliteMergeDatabases)
    m_sqliteMergeDatabases = new biblioteq_sqlite_merge_databases(this);

  biblioteq_misc_functions::center(m_sqliteMergeDatabases, this);
  m_sqliteMergeDatabases->showNormal();
  m_sqliteMergeDatabases->activateWindow();
  m_sqliteMergeDatabases->raise();
}

void biblioteq::slotPrintIconsView(void)
{
  if(menuBar())
    menuBar()->repaint();

  QApplication::processEvents();

  QPrinter printer;

  printer.setColorMode(QPrinter::Color);
  printer.setOutputFormat(QPrinter::PdfFormat);

  QScopedPointer<QPrintDialog> dialog(new QPrintDialog(&printer, this));

  if(dialog->exec() == QDialog::Accepted)
    {
      QApplication::processEvents();
      QApplication::setOverrideCursor(Qt::WaitCursor);

      QPainter painter(&printer);

      painter.setRenderHint(QPainter::Antialiasing);
      ui.graphicsView->scene()->render(&painter);
      QApplication::restoreOverrideCursor();
    }

  QApplication::processEvents();
}

void biblioteq::slotSaveGeneralSearchCaseSensitivity(bool state)
{
  QSettings settings;

  settings.setValue("generalSearchCaseSensitivity", state);
}

void biblioteq::slotShowDocumentation(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  repaint();
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!m_documentation.value(action))
    {
      m_documentation[action] = new biblioteq_documentationwindow(this);

      QFile file;

      if(action == ui.action_English)
	file.setFileName(":/BiblioteQ.html");
      else
	{
#ifdef Q_OS_ANDROID
	  file.setFileName("assets:/BiblioteQ-Fr.html");
#elif defined(Q_OS_MACOS)
	  file.setFileName
	    (QCoreApplication::applicationDirPath() +
	     "/../../../Documentation/Contributed/BiblioteQ-Fr.html");
#else
	  file.setFileName
	    (QString("Documentation%1Contributed%1BiblioteQ-Fr.html").
	     arg(QDir::separator()));
#endif
	}

      if(file.open(QIODevice::ReadOnly))
	m_documentation.value(action)->setHtml(file.readAll());
    }

  m_documentation.value(action)->show();
  QApplication::restoreOverrideCursor();
}

void biblioteq::slotShowFiles(void)
{
  if(!m_files)
    m_files = new biblioteq_files(this);

#ifdef Q_OS_ANDROID
  m_files->showMaximized();
#else
  biblioteq_misc_functions::center(m_files, this);
  m_files->showNormal();
#endif
  m_files->activateWindow();
  m_files->raise();
}

void biblioteq::slotShowReleaseNotes(void)
{
  repaint();
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!m_releaseNotes)
    {
      m_releaseNotes = new biblioteq_documentationwindow
	(QUrl("qrc:/Release-Notes.html"), this);
      m_releaseNotes->setWindowTitle(tr("BiblioteQ: Release Notes"));
    }

  m_releaseNotes->show();
  QApplication::restoreOverrideCursor();
}
