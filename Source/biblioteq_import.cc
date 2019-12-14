#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>

#include "biblioteq.h"
#include "biblioteq_import.h"
#include "biblioteq_misc_functions.h"

biblioteq_import::biblioteq_import(biblioteq *parent):QMainWindow(parent)
{
  m_ui.setupUi(this);
  connect(m_ui.add_book_row,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAddBookRow(void)));
  connect(m_ui.books_templates,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slotBooksTemplates(int)));
  connect(m_ui.close,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotClose(void)));
  connect(m_ui.delete_book_row,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotDeleteBookRow(void)));
  connect(m_ui.reset,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(m_ui.save,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotImport(void)));
  connect(m_ui.select_csv_file,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotSelectCSVFile(void)));
}

void biblioteq_import::changeEvent(QEvent *event)
{
  if(event)
    switch(event->type())
      {
      case QEvent::LanguageChange:
	{
	  m_ui.retranslateUi(this);
	  break;
	}
      default:
	break;
      }

  QMainWindow::changeEvent(event);
}

void biblioteq_import::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void biblioteq_import::importBooks(QProgressDialog *progress)
{
  if(!progress)
    return;

  QFile file(m_ui.csv_file->text());

  if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  qint64 ct = 0;

  while(ct++, !file.atEnd())
    {
      QString data(file.readLine().trimmed());

      if(ct == 1 && m_ui.books_templates->currentIndex() == 1)
	// Ignore the column names.

	continue;

      if(!data.isEmpty())
	{
	  /*
	  ** Separate by the delimiter.
	  */

	  QStringList list
	    (data.split(QRegExp(QString("%1(?=([^\"]*\"[^\"]*\")*[^\"]*$)").
				arg(m_ui.delimiter->text()))));

	  for(int i = 0; i < list.size(); i++)
	    {
	    }
	}
    }

  file.close();
}

void biblioteq_import::show(QMainWindow *parent)
{
  static bool resized = false;

  if(parent && !resized)
    resize(qRound(0.50 * parent->size().width()),
	   qRound(0.80 * parent->size().height()));

  resized = true;
  biblioteq_misc_functions::center(this, parent);
  showNormal();
  activateWindow();
  raise();
}

void biblioteq_import::slotAddBookRow(void)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  m_ui.books->setRowCount(m_ui.books->rowCount() + 1);

  QTableWidgetItem *item = new QTableWidgetItem();

  item->setText(QString::number(m_ui.books->rowCount()));
  m_ui.books->setItem(m_ui.books->rowCount() - 1, 0, item);

  QWidget *widget = new QWidget();
  QComboBox *comboBox = new QComboBox();

  comboBox->addItems(QStringList()
		     << "<ignored>"
		     << "accession_number"
		     << "author"
		     << "binding_type"
		     << "callnumber"
		     << "category"
		     << "condition"
		     << "description"
		     << "deweynumber"
		     << "edition"
		     << "id"
		     << "isbn13"
		     << "keyword"
		     << "language"
		     << "lccontrolnumber"
		     << "location"
		     << "marc_tags"
		     << "monetary_units"
		     << "originality"
		     << "pdate"
		     << "place"
		     << "price"
		     << "publisher"
		     << "quantity"
		     << "title");
  comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  QHBoxLayout *layout = new QHBoxLayout(widget);
  QSpacerItem *spacer = new QSpacerItem
    (40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);

  layout->addWidget(comboBox);
  layout->addSpacerItem(spacer);
  layout->setContentsMargins(0, 0, 0, 0);
  m_ui.books->setCellWidget(m_ui.books->rowCount() - 1, 1, widget);
  QApplication::restoreOverrideCursor();
}

void biblioteq_import::slotBooksTemplates(int index)
{
  switch(index)
    {
    case 0:
      {
	break;
      }
    case 1:
      {
	if(m_ui.books->rowCount() > 0)
	  {
	    if(QMessageBox::question(this,
				     tr("BiblioteQ: Question"),
				     tr("Populate the Books table with "
					"Template 1 values?"),
				     QMessageBox::Yes | QMessageBox::No,
				     QMessageBox::No) == QMessageBox::No)
	      {
		QApplication::processEvents();
		break;
	      }
	  }

	m_ui.books->setRowCount(0);

	QStringList list;

	list << "title"
	     << "author"
	     << "publisher"
	     << "pdate"
	     << "place"
	     << "edition"
	     << "category"
	     << "language"
	     << "id"
	     << "price"
	     << "monetary_units"
	     << "quantity"
	     << "binding_type"
	     << "location"
	     << "isbn13"
	     << "lccontrolnumber"
	     << "callnumber"
	     << "deweynumber"
	     << "<ignored>"
	     << "<ignored>"
	     << "originality"
	     << "condition"
	     << "accession_number";

	for(int i = 0; i < list.size(); i++)
	  {
	    slotAddBookRow();

	    QWidget *widget = m_ui.books->cellWidget(i, 1);

	    if(widget)
	      {
		QComboBox *comboBox = widget->findChild<QComboBox *> ();

		if(comboBox)
		  {
		    comboBox->setCurrentIndex(comboBox->findText(list.at(i)));

		    if(comboBox->currentIndex() < 0)
		      comboBox->setCurrentIndex(0);
		  }
	      }
	  }

	break;
      }
    default:
      {
	break;
      }
    }

  m_ui.books_templates->setCurrentIndex(0);
}

void biblioteq_import::slotClose(void)
{
  close();
}

void biblioteq_import::slotDeleteBookRow(void)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  QModelIndexList list(m_ui.books->selectionModel()->selectedRows(0));

  for(int i = list.size() - 1; i >= 0; i--)
    m_ui.books->removeRow(list.at(i).row());

  QApplication::restoreOverrideCursor();
}

void biblioteq_import::slotImport(void)
{
  /*
  ** Test if the specified file is readable.
  */

  QFileInfo fileInfo(m_ui.csv_file->text());

  if(!fileInfo.isReadable())
    {
      if(fileInfo.absolutePath().isEmpty())
	QMessageBox::critical
	  (this,
	   tr("BiblioteQ: Error"),
	   tr("The specified file is not readable."));
      else
	QMessageBox::critical
	  (this,
	   tr("BiblioteQ: Error"),
	   tr("The file %1 is not readable.").arg(fileInfo.absolutePath()));

      return;
    }

  /*
  ** Test the various mappings.
  */

  QMap<int, QString> map;

  for(int i = 0; i < m_ui.books->rowCount(); i++)
    {
      QTableWidgetItem *item = m_ui.books->item(i, 0);
      QWidget *widget = m_ui.books->cellWidget(i, 1);

      if(!item || !widget)
	continue;

      QComboBox *comboBox = widget->findChild<QComboBox *> ();

      if(!comboBox)
	continue;

      if(map.contains(item->text().toInt()))
	{
	  m_ui.books->selectRow(i);
	  QMessageBox::critical
	    (this,
	     tr("BiblioteQ: Error"),
	     tr("Duplicate mapping discovered in the Books table. Please "
		"review row %1.").arg(item->row()));
	  return;
	}

      map[item->text().toInt()] = comboBox->currentText();
    }

  if(map.isEmpty())
    {
      QMessageBox::critical
	(this,
	 tr("BiblioteQ: Error"),
	 tr("Please define column mappings."));
      return;
    }

  QScopedPointer<QProgressDialog> progress;

  progress.reset(new(std::nothrow) QProgressDialog(this));

  if(!progress)
    return;

  progress->setLabelText(tr("Importing the CSV file..."));
  progress->setMaximum(0);
  progress->setMinimum(0);
  progress->setModal(true);
  progress->setWindowTitle(tr("BiblioteQ: Progress Dialog"));
  progress->show();
  progress->repaint();
  QApplication::processEvents();
  importBooks(progress.data());
}

void biblioteq_import::slotReset(void)
{
  if(m_ui.books->rowCount() > 0 ||
     !m_ui.csv_file->text().isEmpty() ||
     m_ui.delimiter->text() != ",")
    if(QMessageBox::question(this,
			     tr("BiblioteQ: Question"),
			     tr("Are you sure that you wish to reset?"),
			     QMessageBox::Yes | QMessageBox::No,
			     QMessageBox::No) == QMessageBox::No)
      {
	QApplication::processEvents();
	return;
      }

  m_ui.books->setRowCount(0);
  m_ui.csv_file->clear();
  m_ui.delimiter->setText(",");
}

void biblioteq_import::slotSelectCSVFile(void)
{
  QFileDialog dialog(this);

  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setNameFilter("CSV (*.csv)");
  dialog.setOption(QFileDialog::DontUseNativeDialog);
  dialog.setWindowTitle(tr("BiblioteQ: Select CSV Import File"));
  dialog.exec();
  QApplication::processEvents();

  if(dialog.result() == QDialog::Accepted)
    m_ui.csv_file->setText(dialog.selectedFiles().value(0));
}
