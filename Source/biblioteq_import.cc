/*
** Copyright (c) 2006 - present, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from BiblioteQ without specific prior written permission.
**
** BIBLIOTEQ IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** BIBLIOTEQ, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>

#include "biblioteq.h"
#include "biblioteq_import.h"
#include "biblioteq_misc_functions.h"
#include "ui_biblioteq_generalmessagediag.h"

biblioteq_import::biblioteq_import(biblioteq *parent):QMainWindow(parent)
{
  m_qmain = parent;
  m_ui.setupUi(this);
  connect(m_qmain,
	  SIGNAL(fontChanged(const QFont &)),
	  this,
	  SLOT(slotSetGlobalFonts(const QFont &)));
  connect(m_ui.add_row,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAddRow(void)));
  connect(m_ui.close,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotClose(void)));
  connect(m_ui.delete_row,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotDeleteRow(void)));
  connect(m_ui.import_csv,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotImport(void)));
  connect(m_ui.refresh_preview,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotRefreshPreview(void)));
  connect(m_ui.reset,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(m_ui.select_csv_file,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotSelectCSVFile(void)));
  connect(m_ui.templates,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slotTemplates(int)));
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
	{
	  break;
	}
      }

  QMainWindow::changeEvent(event);
}

void biblioteq_import::importBooks(QProgressDialog *progress,
				   QStringList &errors,
				   const Templates importTemplate,
				   const int idIndex,
				   qint64 *imported,
				   qint64 *notImported)
{
  if(!progress || m_ui.csv_file->text().trimmed().isEmpty())
    return;

  QFile file(m_ui.csv_file->text());

  if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  QMapIterator<int, QPair<QString, QString> > it(m_mappings);
  QString f(""); // Field Names
  QString q(""); // Positional Placeholders
  QString queryString("");

  while(it.hasNext())
    {
      it.next();

      if(!it.value().first.contains("<ignored>"))
	{
	  f.append(it.value().first);
	  q.append("?");

	  if(it.hasNext())
	    {
	      f.append(",");
	      q.append(",");
	    }
	}
    }

  queryString.append("INSERT INTO book (");

  if(importTemplate == Templates::TEMPLATE_2)
    /*
    ** The description field is not exported because
    ** it is not a column in the Books category.
    */

    queryString.append("description,");

  if(m_qmain->getDB().driverName() != "QPSQL")
    queryString.append("myoid,");

  queryString.append(f);
  queryString.append(") VALUES (");

  if(importTemplate == Templates::TEMPLATE_2)
    queryString.append("?,"); // The description field!

  if(m_qmain->getDB().driverName() != "QPSQL")
    queryString.append("?,"); // The myoid field!

  queryString.append(q);
  queryString.append(")");

  if(m_qmain->getDB().driverName() == "QPSQL")
    queryString.append(" RETURNING myoid");

  if(imported)
    *imported = 0;

  if(notImported)
    *notImported = 0;

  QHash<QString, int> isbns;
  QTextStream in(&file);
  auto list(m_ui.ignored_rows->text().trimmed().split(' ',
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
						      Qt::SkipEmptyParts
#else
						      QString::SkipEmptyParts
#endif
						      ));
  qint64 ct = 0;

  while(ct++, !in.atEnd())
    {
      if(progress->wasCanceled())
	break;
      else
	{
	  progress->repaint();
	  QApplication::processEvents();
	}

      auto data(in.readLine().trimmed());

      if(list.contains(QString::number(ct)))
	continue;

      if(!data.isEmpty())
	{
	  /*
	  ** Separate by the delimiter.
	  */

	  auto list
	    (data.split
	     (QRegularExpression(QString("%1(?=([^\"]*\"[^\"]*\")*[^\"]*$)").
				 arg(m_ui.delimiter->text()))));

	  if(!list.isEmpty())
	    {
	      QMap<int, QString> values;
	      QSqlQuery query(m_qmain->getDB());
	      QString id("");
	      QString isbn13("");
	      auto duplicate = false;
	      int quantity = 1;
	      qint64 oid = 0;

	      query.prepare(queryString);

	      if(importTemplate == Templates::TEMPLATE_2)
		query.addBindValue("N/A"); // The description field.

	      if(m_qmain->getDB().driverName() != "QPSQL")
		{
		  QString errorstr("");

		  oid = biblioteq_misc_functions::getSqliteUniqueId
		    (m_qmain->getDB(), errorstr);

		  if(errorstr.isEmpty())
		    query.addBindValue(oid);
		}

	      /*
	      ** i = 1? Ignore the myoid field.
	      */

	      for(int i = 1; i <= list.size(); i++)
		{
		  if(!m_mappings.contains(i))
		    continue;
		  else if(m_mappings.value(i).first.contains("<ignored>"))
		    continue;

		  auto str(QString(list.at(i - 1)).remove('"').trimmed());

		  if(m_mappings.value(i).first == "binding_type")
		    {
		      if(!str.isEmpty())
			{
			  QSqlQuery query(m_qmain->getDB());

			  query.prepare
			    ("INSERT INTO book_binding_types "
			     "(binding_type) VALUES(?)");
			  query.addBindValue(str);
			  query.exec();
			}
		    }
		  else if(m_mappings.value(i).first == "condition")
		    {
		      if(!str.isEmpty())
			{
			  QSqlQuery query(m_qmain->getDB());

			  query.prepare
			    ("INSERT INTO book_conditions "
			     "(condition) VALUES(?)");
			  query.addBindValue(str);
			  query.exec();
			}
		    }
		  else if(m_mappings.value(i).first == "originality")
		    {
		      if(!str.isEmpty())
			{
			  QSqlQuery query(m_qmain->getDB());

			  query.prepare
			    ("INSERT INTO book_originality "
			     "(originality) VALUES(?)");
			  query.addBindValue(str);
			  query.exec();
			}
		    }
		  else if(m_mappings.value(i).first == "id")
		    {
		      str.remove('-');

		      if(str.length() == 10)
			id = str;

		      if(isbns.contains(id))
			duplicate = true;

		      if(!id.isEmpty())
			isbns[id] = isbns.value(id, 0) + 1;
		    }
		  else if(m_mappings.value(i).first == "isbn13")
		    {
		      str.remove('-');

		      if(id.length() == 10 && str.isEmpty())
			str = biblioteq_misc_functions::isbn10to13(id);

		      isbn13 = str;

		      if(isbns.contains(isbn13))
			duplicate = true;

		      if(!isbn13.isEmpty())
			isbns[isbn13] = isbns.value(str, 0) + 1;
		    }
		  else if(m_mappings.value(i).first == "language")
		    {
		      if(!str.isEmpty())
			{
			  QSqlQuery query(m_qmain->getDB());

			  query.prepare
			    ("INSERT INTO languages (language) VALUES(?)");
			  query.addBindValue(str);
			  query.exec();
			}
		    }
		  else if(m_mappings.value(i).first == "location")
		    {
		      if(!str.isEmpty())
			{
			  QSqlQuery query(m_qmain->getDB());

			  query.prepare
			    ("INSERT INTO locations "
			     "(location, type) VALUES(?, ?)");
			  query.addBindValue(str);
			  query.addBindValue("Book");
			  query.exec();
			}
		    }
		  else if(m_mappings.value(i).first == "monetary_units")
		    {
		      if(!str.isEmpty())
			{
			  QSqlQuery query(m_qmain->getDB());

			  query.prepare
			    ("INSERT INTO monetary_units "
			     "(monetary_unit) VALUES(?)");
			  query.addBindValue(str);
			  query.exec();
			}
		    }
		  else if(m_mappings.value(i).first == "pdate")
		    {
		      auto date
			(QDate::fromString(str,
					   biblioteq::s_databaseDateFormat));

		      if(!date.isValid())
			{
			  date = QDate::fromString(str, "yyyy");

			  if(date.isValid())
			    str = "01/01/" + date.toString("yyyy");
			}

		      if(!date.isValid())
			str = "01/01/2000";
		      else
			str = date.toString(biblioteq::s_databaseDateFormat);
		    }
		  else if(m_mappings.value(i).first == "price")
		    {
		      QLocale locale;
		      bool ok = true;
		      double price = locale.toDouble
			(str.remove(locale.currencySymbol()), &ok);

		      if(ok)
			str = QString::number(price);
		      else
			str = str.replace(',', '.');
		    }
		  else if(m_mappings.value(i).first == "quantity")
		    quantity = qBound
		      (1,
		       str.toInt(),
		       static_cast<int> (biblioteq::Limits::QUANTITY));
		  else if(m_mappings.value(i).first == "target_audience")
		    {
		      if(!str.isEmpty())
			{
			  QSqlQuery query(m_qmain->getDB());

			  query.prepare
			    ("INSERT INTO book_target_audiences "
			     "(target_audience) VALUES(?)");
			  query.addBindValue(str);
			  query.exec();
			}
		    }

		  if(str.isEmpty())
		    /*
		    ** If the value in the CSV file is empty,
		    ** refer to a substitution.
		    */

		    str = m_mappings.value(i).second;

		  values[i] = str;
		}

	      values[idIndex] = id; // ISBN-10?

	      QMapIterator<int, QString> it(values);

	      while(it.hasNext())
		{
		  it.next();

		  if(it.value().isEmpty())
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		    query.addBindValue(QVariant(QMetaType(QMetaType::QString)));
#else
		    query.addBindValue(QVariant(QVariant::String));
#endif
		  else
		    query.addBindValue(it.value());
		}

	      if(duplicate)
		{
		  QString errorstr("");

		  if(!id.isEmpty())
		    biblioteq_misc_functions::createBookCopy
		      (id, isbns.value(id), m_qmain->getDB(), errorstr);
		  else
		    biblioteq_misc_functions::createBookCopy
		      (isbn13, isbns.value(isbn13), m_qmain->getDB(), errorstr);

		  if(errorstr.isEmpty())
		    {
		      if(imported)
			*imported += 1;
		    }
		  else
		    {
		      errors << tr("Database error (%1) at row %2.").
			arg(errorstr).arg(ct);

		      if(notImported)
			*notImported += 1;
		    }
		}
	      else if(query.exec())
		{
		  if(m_qmain->getDB().driverName() == "QPSQL")
		    {
		      query.next();
		      oid = query.value(0).toLongLong();
		    }

		  QString errorstr("");

		  if(!id.isEmpty())
		    biblioteq_misc_functions::createInitialCopies
		      (id, quantity, m_qmain->getDB(), "Book", errorstr);
		  else if(!isbn13.isEmpty())
		    biblioteq_misc_functions::createInitialCopies
		      (isbn13, quantity, m_qmain->getDB(), "Book", errorstr);
		  else
		    biblioteq_misc_functions::createInitialCopies
		      (QString::number(oid),
		       quantity,
		       m_qmain->getDB(),
		       "Book",
		       errorstr);

		  if(!errorstr.isEmpty())
		    // Do not increase notImported.

		    errors << tr("biblioteq_misc_functions::"
				 "createInitialCopies() "
				 "error (%1) at row %2.").
		      arg(errorstr).arg(ct);

		  if(imported)
		    *imported += 1;
		}
	      else
		{
		  errors << tr("Database error (%1) at row %2.").
		    arg(query.lastError().text()).arg(ct);

		  if(notImported)
		    *notImported += 1;
		}
	    }
	  else if(notImported)
	    {
	      errors << tr("Empty row %1.").arg(ct);
	      *notImported += 1;
	    }
	}
    }

  file.close();
}

void biblioteq_import::importPatrons(QProgressDialog *progress,
				     QStringList &errors,
				     qint64 *imported,
				     qint64 *notImported)
{
  if(!progress || m_ui.csv_file->text().trimmed().isEmpty())
    return;

  QFile file(m_ui.csv_file->text());

  if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  QMapIterator<int, QPair<QString, QString> > it(m_mappings);
  QString f("");
  QString q("");
  QString queryString("");

  while(it.hasNext())
    {
      it.next();

      if(!it.value().first.contains("<ignored>"))
	{
	  f.append(it.value().first);
	  q.append("?");

	  if(it.hasNext())
	    {
	      f.append(",");
	      q.append(",");
	    }
	}
    }

  queryString.append("INSERT INTO member (");
  queryString.append(f);
  queryString.append(") VALUES (");
  queryString.append(q);
  queryString.append(")");

  if(imported)
    *imported = 0;

  if(notImported)
    *notImported = 0;

  QTextStream in(&file);
  auto list(m_ui.ignored_rows->text().trimmed().split(' ',
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
						      Qt::SkipEmptyParts
#else
						      QString::SkipEmptyParts
#endif
						      ));
  qint64 ct = 0;

  while(ct++, !in.atEnd())
    {
      if(progress->wasCanceled())
	break;
      else
	{
	  progress->repaint();
	  QApplication::processEvents();
	}

      auto data(in.readLine().trimmed());

      if(list.contains(QString::number(ct)))
	continue;

      if(!data.isEmpty())
	{
	  /*
	  ** Separate by the delimiter.
	  */

	  auto list
	    (data.split
	     (QRegularExpression(QString("%1(?=([^\"]*\"[^\"]*\")*[^\"]*$)").
				 arg(m_ui.delimiter->text()))));

	  if(!list.isEmpty())
	    {
	      if(!m_qmain->getDB().transaction())
		{
		  errors << tr("Unable to create a database transaction at "
			       "row %1").arg(ct);
		  continue;
		}

	      QSqlQuery query(m_qmain->getDB());
	      QString memberid("");

	      query.prepare(queryString);

	      for(int i = 1; i <= list.size(); i++)
		{
		  if(!m_mappings.contains(i))
		    continue;
		  else if(m_mappings.value(i).first.contains("<ignored>"))
		    continue;

		  auto str(QString(list.at(i - 1)).remove('"').trimmed());

		  if(m_mappings.value(i).first == "memberid")
		    memberid = str;

		  if(str.isEmpty())
		    /*
		    ** If the value in the CSV file is empty,
		    ** refer to a substitution.
		    */

		    str = m_mappings.value(i).second;

		  if(str.isEmpty())
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		    query.addBindValue(QVariant(QMetaType(QMetaType::QString)));
#else
		    query.addBindValue(QVariant(QVariant::String));
#endif
		  else
		    query.addBindValue(str);
		}

	      auto ok = false;

	      if(query.exec())
		{
		  if(m_qmain->getDB().driverName() == "QPSQL")
		    {
		      QString error("");

		      biblioteq_misc_functions::DBAccount
			(memberid,
			 m_qmain->getDB(),
			 biblioteq_misc_functions::CREATE_USER,
			 error);

		      if(!error.isEmpty())
			errors << tr("Error (%1) in "
				     "biblioteq_misc_functions::DBAccount() "
				     "at row %2.").
			  arg(error).arg(ct);

		      if(error.isEmpty())
			{
			  if(imported)
			    *imported += 1;

			  ok = true;
			}
		      else if(notImported)
			*notImported += 1;
		    }
		  else
		    {
		      if(imported)
			*imported += 1;

		      ok = true;
		    }
		}
	      else
		{
		  errors << tr("Database error (%1) at row %2.").
		    arg(query.lastError().text()).arg(ct);

		  if(notImported)
		    *notImported += 1;
		}

	      if(ok)
		{
		  if(!m_qmain->getDB().commit())
		    errors << tr("Unable to commit the current database "
				 "transaction at row.").arg(ct);
		}
	      else
		m_qmain->getDB().rollback();
	    }
	  else if(notImported)
	    {
	      errors << tr("Empty row %1.").arg(ct);
	      *notImported += 1;
	    }
	}
    }

  file.close();
}

void biblioteq_import::loadPreview(void)
{
  if(m_ui.csv_file->text().trimmed().isEmpty())
    return;

  QScopedPointer<QProgressDialog> progress(new QProgressDialog(this));

  progress->setLabelText(tr("Reading the CSV file..."));
  progress->setMaximum(0);
  progress->setMinimum(0);
  progress->setModal(true);
  progress->setValue(0);
  progress->setWindowTitle(tr("BiblioteQ: Progress Dialog"));
  progress->show();
  progress->repaint();
  QApplication::processEvents();
  m_ui.preview->clear();
  m_ui.preview->setColumnCount(0);
  m_ui.preview->setRowCount(0);

  QFile file(m_ui.csv_file->text());

  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QTextStream in(&file);
      int row = 0;

      while(!in.atEnd() && !progress->wasCanceled())
	{
	  progress->repaint();
	  QApplication::processEvents();

	  auto data(in.readLine().trimmed());
	  auto headers
	    (data.split
	     (QRegularExpression
	      (QString("%1(?=([^\"]*\"[^\"]*\")*[^\"]*$)").
	       arg(m_ui.delimiter->text()))));
	  auto list(headers);

	  for(int i = 0; i < headers.size(); i++)
	    headers.replace
	      (i, QString("(%1) %2").arg(i + 1).arg(headers.at(i)));

	  if(row == 0)
	    {
	      for(int i = 0; i < m_ui.rows->rowCount(); i++)
		{
		  auto item = m_ui.rows->item(i, CSV_PREVIEW);

		  if(item)
		    item->setText(list.value(i));
		}

	      m_previewHeaders = list;
	      m_ui.preview->setColumnCount(headers.size());
	      m_ui.preview->setHorizontalHeaderLabels(headers);
	      m_ui.preview->resizeColumnsToContents();
	    }
	  else
	    {
	      m_ui.preview->setRowCount(row);

	      for(int i = 0; i < list.size(); i++)
		{
		  auto item = new QTableWidgetItem(list.at(i));

		  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		  m_ui.preview->setItem(row - 1, i, item);
		}
	    }

	  row += 1;
	}
    }

  file.close();
  progress->close();
  QApplication::processEvents();
}

void biblioteq_import::reset(void)
{
  slotReset();
}

void biblioteq_import::show(QMainWindow *parent)
{
  static auto resized = false;

  if(parent && !resized)
    resize(qRound(0.50 * parent->size().width()),
	   qRound(0.80 * parent->size().height()));

  resized = true;
  biblioteq_misc_functions::center(this, parent);
  showNormal();
  activateWindow();
  raise();
}

void biblioteq_import::slotAddRow(void)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  m_ui.rows->setRowCount(m_ui.rows->rowCount() + 1);

  auto item = new QTableWidgetItem();

  item->setText(QString::number(m_ui.rows->rowCount()));
  m_ui.rows->setItem
    (m_ui.rows->rowCount() - 1, Columns::CSV_COLUMN_NUMBER, item);
  item = new QTableWidgetItem
    (m_previewHeaders.value(m_ui.rows->rowCount() - 1));
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_ui.rows->setItem(m_ui.rows->rowCount() - 1, Columns::CSV_PREVIEW, item);

  auto comboBox = new QComboBox();
  auto widget = new QWidget();

  switch(m_ui.templates->currentIndex())
    {
    case Templates::TEMPLATE_1:
    case Templates::TEMPLATE_2:
      {
	comboBox->addItems(QStringList()
			   << "<ignored>"
			   << "accession_number"
			   << "alternate_id_1"
			   << "author"
			   << "binding_type"
			   << "callnumber"
			   << "category"
			   << "condition"
			   << "date_of_reform"
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
			   << "multivolume_set_isbn"
			   << "origin"
			   << "originality"
			   << "pdate"
			   << "place"
			   << "price"
			   << "publisher"
			   << "purchase_date"
			   << "quantity"
			   << "target_audience"
			   << "title"
			   << "url"
			   << "volume_number");
	break;
      }
    case Templates::TEMPLATE_3:
      {
	comboBox->addItems(QStringList()
			   << "<ignored>"
			   << "city"
			   << "comments"
			   << "dob"
			   << "email"
			   << "expiration_date"
			   << "first_name"
			   << "general_registration_number"
			   << "last_name"
			   << "maximum_reserved_books"
			   << "memberclass"
			   << "memberid"
			   << "membersince"
			   << "membership_fees"
			   << "middle_init"
			   << "overdue_fees"
			   << "sex"
			   << "state_abbr"
			   << "street"
			   << "telephone_num"
			   << "zip");
	break;
      }
    default:
      {
	break;
      }
    }

  comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  comboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

  auto layout = new QHBoxLayout(widget);
  auto spacer = new QSpacerItem
    (40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);

  layout->addWidget(comboBox);
  layout->addSpacerItem(spacer);
  layout->setContentsMargins(0, 0, 0, 0);
  m_ui.rows->setCellWidget
    (m_ui.rows->rowCount() - 1, Columns::BIBLIOTEQ_TABLE_FIELD_NAME, widget);
  item = new QTableWidgetItem();
  item->setText("N/A");
  m_ui.rows->setItem
    (m_ui.rows->rowCount() - 1, Columns::SUBSTITUTE_VALUE, item);
  m_ui.rows->resizeRowsToContents();

  if(m_ui.bottom_scroll_on_add->isChecked())
    m_ui.rows->scrollToBottom();

  QApplication::restoreOverrideCursor();
}

void biblioteq_import::slotClose(void)
{
#ifdef Q_OS_ANDROID
  hide();
#else
  close();
#endif
}

void biblioteq_import::slotDeleteRow(void)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  auto rows(biblioteq_misc_functions::selectedRows(m_ui.rows));

  for(int i = rows.size() - 1; i >= 0; i--)
    m_ui.rows->removeRow(rows.at(i));

  QApplication::restoreOverrideCursor();
}

void biblioteq_import::slotImport(void)
{
  if(m_ui.csv_file->text().trimmed().isEmpty() ||
     m_ui.templates->currentIndex() == 0)
    /*
    ** A template has not been selected!
    */

    return;

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

  QMap<int, QPair<QString, QString> > map;

  for(int i = 0; i < m_ui.rows->rowCount(); i++)
    {
      auto item1 = m_ui.rows->item(i, Columns::CSV_COLUMN_NUMBER);
      auto item2 = m_ui.rows->item(i, Columns::SUBSTITUTE_VALUE);
      auto widget = m_ui.rows->cellWidget
	(i, Columns::BIBLIOTEQ_TABLE_FIELD_NAME);

      if(!item1 || !item2 || !widget)
	continue;

      auto comboBox = widget->findChild<QComboBox *> ();

      if(!comboBox)
	continue;

      if(map.contains(item1->text().toInt()))
	{
	  m_ui.rows->selectRow(i);
	  QMessageBox::critical
	    (this,
	     tr("BiblioteQ: Error"),
	     tr("Duplicate mapping discovered in the table. Please "
		"review row %1.").arg(item1->row()));
	  return;
	}

      map[item1->text().toInt()] =
	QPair<QString, QString> (comboBox->currentText(), item2->text());
    }

  if(map.isEmpty())
    {
      QMessageBox::critical
	(this,
	 tr("BiblioteQ: Error"),
	 tr("Please define column mappings."));
      return;
    }

  QScopedPointer<QProgressDialog> progress(new QProgressDialog(this));

  progress->setLabelText(tr("Importing the CSV file..."));
  progress->setMaximum(0);
  progress->setMinimum(0);
  progress->setModal(true);
  progress->setValue(0);
  progress->setWindowTitle(tr("BiblioteQ: Progress Dialog"));
  progress->show();
  progress->repaint();
  QApplication::processEvents();
  m_mappings = map;

  QStringList errors;
  auto index = m_ui.templates->currentIndex();
  qint64 imported = 0;
  qint64 notImported = 0;

  if(index == Templates::TEMPLATE_1)
    /*
    ** ID's index is 1-based.
    */

    importBooks(progress.data(),
		errors,
		Templates::TEMPLATE_1,
		12, // ISBN-10
		&imported,
		&notImported);
  else if(index == Templates::TEMPLATE_2)
    /*
    ** ID's index is 1-based.
    */

    importBooks(progress.data(),
		errors,
		Templates::TEMPLATE_2,
		9, // ISBN-10
		&imported,
		&notImported);
  else if(index == Templates::TEMPLATE_3)
    importPatrons(progress.data(), errors, &imported, &notImported);

  progress->close();
  QApplication::processEvents();

  if(!errors.isEmpty())
    {
      QApplication::setOverrideCursor(Qt::WaitCursor);

      QDialog dialog(this);
      QString errorstr("");
      Ui_generalmessagediag ui;

      errors.prepend(tr("Imported: %1. Not imported: %2.\n").
		     arg(imported).
		     arg(notImported));

      for(int i = 0; i < errors.size(); i++)
	{
	  errorstr.append(errors.at(i));
	  errorstr.append('\n');
	}

      ui.setupUi(&dialog);
      ui.text->setPlainText(errorstr.trimmed());
      connect(ui.cancelButton,
	      SIGNAL(clicked(void)),
	      &dialog,
	      SLOT(close(void)));
      dialog.setWindowTitle(tr("BiblioteQ: Import Results"));
      QApplication::restoreOverrideCursor();
      dialog.exec();
    }
  else
    QMessageBox::information
      (this,
       tr("BiblioteQ: Information"),
       tr("Imported: %1. Not imported: %2.").arg(imported).arg(notImported));

  QApplication::processEvents();
}

void biblioteq_import::slotRefreshPreview(void)
{
  loadPreview();
}

void biblioteq_import::slotReset(void)
{
  if(m_ui.rows->rowCount() > 0 ||
     !m_ui.csv_file->text().isEmpty() ||
     m_ui.delimiter->text() != ",")
    if(sender())
      if(QMessageBox::question(this,
			       tr("BiblioteQ: Question"),
			       tr("Are you sure that you wish to reset?"),
			       QMessageBox::No | QMessageBox::Yes,
			       QMessageBox::No) == QMessageBox::No)
	{
	  QApplication::processEvents();
	  return;
	}

  m_mappings.clear();
  m_previewHeaders.clear();
  m_ui.csv_file->clear();
  m_ui.delimiter->setText(",");
  m_ui.ignored_rows->clear();
  m_ui.preview->clear();
  m_ui.preview->setColumnCount(0);
  m_ui.preview->setRowCount(0);
  m_ui.rows->setRowCount(0);
  m_ui.templates->setCurrentIndex(0);
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
    {
      m_ui.csv_file->setText(dialog.selectedFiles().value(0));
      loadPreview();
    }
}

void biblioteq_import::slotSetGlobalFonts(const QFont &font)
{
  setFont(font);

  foreach(auto widget, findChildren<QWidget *> ())
    {
      widget->setFont(font);
      widget->update();
    }

  m_ui.rows->resizeRowsToContents();
  update();
}

void biblioteq_import::slotTemplates(int index)
{
  switch(index)
    {
    case 0:
      {
	break;
      }
    default:
      {
	if(m_ui.rows->rowCount() > 0)
	  {
	    if(QMessageBox::question(this,
				     tr("BiblioteQ: Question"),
				     tr("Populate the table with "
					"Template %1 values?").arg(index),
				     QMessageBox::No | QMessageBox::Yes,
				     QMessageBox::No) == QMessageBox::No)
	      {
		QApplication::processEvents();
		break;
	      }
	  }

	m_ui.ignored_rows->setText("1");
	m_ui.rows->setRowCount(0);

	QStringList list;

	if(index == Templates::TEMPLATE_1)
	  list << "accession_number"
	       << "alternate_id_1"
	       << "author"
	       << "binding_type"
	       << "callnumber"
	       << "category"
	       << "condition"
	       << "date_of_reform"
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
	       << "multivolume_set_isbn"
	       << "origin"
	       << "originality"
	       << "pdate"
	       << "place"
	       << "price"
	       << "publisher"
	       << "purchase_date"
	       << "quantity"
	       << "target_audience"
	       << "title"
	       << "url"
	       << "volume_number";
	else if(index == Templates::TEMPLATE_2)
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
	       << "<ignored>" // Availability
	       << "<ignored>" // Total Reserved
	       << "originality"
	       << "condition"
	       << "accession_number"
	       << "alternate_id_1"
	       << "target_audience"
	       << "volume_number"
	       << "date_of_reform"
	       << "origin"
	       << "purchase_date";
	else if(index == Templates::TEMPLATE_3)
	  list << "city"
	       << "comments"
	       << "dob"
	       << "email"
	       << "expiration_date"
	       << "first_name"
	       << "general_registration_number"
	       << "last_name"
	       << "maximum_reserved_books"
	       << "memberclass"
	       << "memberid"
	       << "membersince"
	       << "membership_fees"
	       << "middle_init"
	       << "overdue_fees"
	       << "sex"
	       << "state_abbr"
	       << "street"
	       << "telephone_num"
	       << "zip";

	for(int i = 0; i < list.size(); i++)
	  {
	    slotAddRow();

	    if(list.at(i) == "id" || list.at(i) == "isbn13")
	      {
		auto item = m_ui.rows->item(i, Columns::SUBSTITUTE_VALUE);

		if(item)
		  item->setText("");
	      }

	    auto widget = m_ui.rows->cellWidget
	      (i, Columns::BIBLIOTEQ_TABLE_FIELD_NAME);

	    if(widget)
	      {
		auto comboBox = widget->findChild<QComboBox *> ();

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
    }
}
