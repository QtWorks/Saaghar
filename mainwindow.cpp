﻿/***************************************************************************
 *  This file is part of Saaghar, a Persian poetry software                *
 *                                                                         *
 *  Copyright (C) 2010-2011 by S. Razi Alavizadeh                          *
 *  E-Mail: <s.r.alavizadeh@gmail.com>, WWW: <http://www.pojh.co.cc>       *
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 3 of the License,         *
 *  (at your option) any later version                                     *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details                            *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program; if not, see http://www.gnu.org/licenses/      *
 *                                                                         *
 ***************************************************************************/
 
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "SearchItemDelegate.h"
#include "version.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QLabel>
#include <QTableWidget>
#include <QFontDatabase>
#include <QClipboard>
#include <QProcess>
#include <QToolButton>
#include <QDir>
#include <QCloseEvent>
#include <QTextCodec>
#include <QPrinter>
#include <QPainter>
#include <QPrintDialog>
#include <QFileDialog>
#include <QPrintPreviewDialog>

const int ITEM_SEARCH_DATA = Qt::UserRole+10;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	QCoreApplication::setOrganizationName("Pojh");
	QCoreApplication::setApplicationName("Saaghar");
	QCoreApplication::setOrganizationDomain("Pojh.co.cc");
	
	QFontDatabase::addApplicationFont(":/resources/fonts/XB Sols.ttf");

	setWindowIcon(QIcon(":/resources/images/saaghar.png"));

#ifdef Q_WS_WIN
	QTextCodec::setCodecForLocale( QTextCodec::codecForName( "Windows-1256" ) );
#endif
	
	SaagharWidget::persianIranLocal = QLocale(QLocale::Persian, QLocale::Iran);
	SaagharWidget::persianIranLocal.setNumberOptions(QLocale::OmitGroupSeparator);

	defaultPrinter = 0;

#ifdef Q_WS_MAC
	QFileInfo portableSettings(QCoreApplication::applicationDirPath()+"/../Resources/settings.ini");
#else
	QFileInfo portableSettings(QCoreApplication::applicationDirPath()+"/settings.ini");
#endif

	if ( portableSettings.exists() && portableSettings.isWritable() ) 
		isPortable = true;
	else
		isPortable = false;

	saagharWidget = 0;
	pressedMouseButton = Qt::LeftButton;
    
	QString dataBaseCompleteName = "/ganjoor.s3db";

	if (isPortable)
	{
#ifdef Q_WS_MAC
		dataBaseCompleteName = QCoreApplication::applicationDirPath()+"/../Resources/"+dataBaseCompleteName;
		SaagharWidget::poetsImagesDir = QCoreApplication::applicationDirPath() + "/../Resources/poets_images/";
#else
		dataBaseCompleteName = QCoreApplication::applicationDirPath()+dataBaseCompleteName;
		SaagharWidget::poetsImagesDir = QCoreApplication::applicationDirPath()+"/poets_images/";
#endif
	}
	else
	{
#ifdef Q_WS_WIN
		dataBaseCompleteName = QDir::homePath()+"/Pojh/Saaghar/"+dataBaseCompleteName;
		SaagharWidget::poetsImagesDir = QCoreApplication::applicationDirPath()+"/poets_images/";
#endif
#ifdef Q_WS_X11
		dataBaseCompleteName = QDir::homePath()+"/usr/share/saaghar/"+dataBaseCompleteName;
		SaagharWidget::poetsImagesDir = "/usr/share/saaghar/poets_images/";
#endif
#ifdef Q_WS_MAC
		dataBaseCompleteName = QDir::homePath()+"/Library/Saaghar/"+dataBaseCompleteName;
		SaagharWidget::poetsImagesDir = QCoreApplication::applicationDirPath() + "/../Resources/poets_images/";
#endif
	}

	//create Parent Categories ToolBar
	parentCatsToolBar = new QToolBar(this);
    parentCatsToolBar->setObjectName(QString::fromUtf8("parentCatsToolBar"));
    parentCatsToolBar->setLayoutDirection(Qt::RightToLeft);
	parentCatsToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
	addToolBar(Qt::BottomToolBarArea, parentCatsToolBar);
	parentCatsToolBar->setWindowTitle(QApplication::translate("MainWindow", "Parent Categories", 0, QApplication::UnicodeUTF8));

    ui->setupUi(this);
	connect(ui->pushButtonSearchCancel, SIGNAL(clicked()), this, SLOT(searchWidgetClose()));
	connect(ui->pushButtonSearch, SIGNAL(clicked()), this, SLOT(searchStart()));
	
	loadGlobalSettings();
	setupUi();

	ui->searchWidget->hide();

	//create Tab Widget
	mainTabWidget = new QTabWidget(ui->centralWidget);
	mainTabWidget->setDocumentMode(true);
	mainTabWidget->setUsesScrollButtons(true);
	mainTabWidget->setMovable(true);

	//create ganjoor DataBase browser
	SaagharWidget::ganjoorDataBase = new QGanjoorDbBrowser(dataBaseCompleteName);
	SaagharWidget::ganjoorDataBase->setObjectName(QString::fromUtf8("ganjoorDataBaseBrowser"));

	loadTabWidgetSettings();

	ui->gridLayout->setMargin(0);
	
	ui->gridLayout->addWidget(mainTabWidget, 0, 0, 1, 1);

	if (!openedTabs.isEmpty())
	{
		QStringList openedTabsList = openedTabs.split("|", QString::SkipEmptyParts);
		if (!openedTabsList.isEmpty())
		{
			for (int i=0; i<openedTabsList.size(); ++i)
			{
				QStringList tabViewData = openedTabsList.at(i).split("=", QString::SkipEmptyParts);
				if (tabViewData.size() == 2 && (tabViewData.at(0) == "PoemID" || tabViewData.at(0) == "CatID") )
				{
					bool Ok = false;
					int id = tabViewData.at(1).toInt(&Ok);
					if (Ok)
						newTabForItem(tabViewData.at(0), id, true);
				}
			}
		}
	}

	if (mainTabWidget->count() < 1)
		insertNewTab();

	ui->centralWidget->setLayoutDirection(Qt::RightToLeft);

	connect(mainTabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
	connect(mainTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloser(int)));

    createConnections();
	currentTabChanged(mainTabWidget->currentIndex());
}

MainWindow::~MainWindow()
{
	delete SaagharWidget::ganjoorDataBase;
    delete ui;
}

void MainWindow::searchWidgetClose()
{
	ui->searchWidget->close();
	searchToolbarAction->setChecked(false);
}

void MainWindow::searchStart()
{
	QString phrase = ui->lineEditSearchText->text();
	phrase.remove(" ");
	phrase.remove("\t");
	if (phrase.isEmpty())
	{
		ui->lineEditSearchText->setText("");
		QMessageBox::information(this, tr("Error"), tr("The search phrase can not be empty."));
		return;
	}
	QString currentItemData = ui->comboBoxSearchRegion->itemData(ui->comboBoxSearchRegion->currentIndex(), Qt::UserRole).toString();
	
	if (currentItemData == "ALL_OPENED_TAB")
	{
		for (int i = 0; i < mainTabWidget->count(); ++i)
		{
			SaagharWidget *tmp = getSaagharWidget(i);
			QAbstractItemDelegate *tmpDelegate = tmp->tableViewWidget->itemDelegate();

			delete tmpDelegate;
			tmpDelegate = 0;

			tmp->scrollToFirstItemContains(ui->lineEditSearchText->text());
			tmp->tableViewWidget->setItemDelegate(new SaagharItemDelegate(tmp->tableViewWidget, saagharWidget->tableViewWidget->style(), ui->lineEditSearchText->text()));
		}
	}
	else if (currentItemData == "CURRENT_TAB")
	{
		if (saagharWidget)
		{
			QAbstractItemDelegate *currentDelegate = saagharWidget->tableViewWidget->itemDelegate();
			saagharWidget->scrollToFirstItemContains(ui->lineEditSearchText->text());
			saagharWidget->tableViewWidget->setItemDelegate(new SaagharItemDelegate(saagharWidget->tableViewWidget, saagharWidget->tableViewWidget->style(), ui->lineEditSearchText->text()));

			delete currentDelegate;
			currentDelegate = 0;
		}
	}
	else
	{
		showSearchResults(ui->lineEditSearchText->text(), 0, ui->spinBoxMaxSearchResult->value(), ui->comboBoxSearchRegion->itemData(ui->comboBoxSearchRegion->currentIndex(), Qt::UserRole).toInt());
	}

}

void MainWindow::searchWidgetSetVisible(bool visible)
{
	if (visible)
	{
		ui->comboBoxSearchRegion->addItem(tr("Current Tab"), "CURRENT_TAB");
		ui->comboBoxSearchRegion->addItem(tr("All Opened Tab"), "ALL_OPENED_TAB");
		ui->comboBoxSearchRegion->addItem(tr("All"), "0");

		QList<GanjoorPoet *> poets = SaagharWidget::ganjoorDataBase->getPoets();
		for(int i=0; i<poets.size(); ++i)
		{
			ui->comboBoxSearchRegion->addItem(poets.at(i)->_Name, QString::number(poets.at(i)->_ID));
		}
		ui->lineEditSearchText->setFocus();
	}
	ui->searchWidget->setVisible(visible);
}

void MainWindow::currentTabChanged(int tabIndex)
{
	if ( getSaagharWidget(tabIndex) )
	{
		saagharWidget = getSaagharWidget(tabIndex);
		saagharWidget->loadSettings();
		saagharWidget->showParentCategory(SaagharWidget::ganjoorDataBase->getCategory(saagharWidget->currentCat));//just update parentCatsToolbar
		saagharWidget->resizeTable(saagharWidget->tableViewWidget);

		if (saagharWidget->tableViewWidget->columnCount() == 1 && saagharWidget->tableViewWidget->rowCount() > 0 && saagharWidget->currentCat != 0)
		{
			QTableWidgetItem *item = saagharWidget->tableViewWidget->item(0,0);
			if (item)
			{
				QString text = item->text();
				int textWidth = saagharWidget->tableViewWidget->fontMetrics().boundingRect(text).width();
				int totalWidth = saagharWidget->tableViewWidget->columnWidth(0)-82;
				totalWidth = qMax(82, totalWidth);
				int numOfRow = textWidth/totalWidth ;
				saagharWidget->tableViewWidget->setRowHeight(0, 2*saagharWidget->tableViewWidget->rowHeight(0)+(saagharWidget->tableViewWidget->fontMetrics().height()*(numOfRow/*+1*/)));
			}
		}

		updateCaption();
		actionPreviousPoem->setEnabled( !SaagharWidget::ganjoorDataBase->getPreviousPoem(saagharWidget->currentPoem, saagharWidget->currentCat).isNull() );
		actionNextPoem->setEnabled( !SaagharWidget::ganjoorDataBase->getNextPoem(saagharWidget->currentPoem, saagharWidget->currentCat).isNull() );
	}
}

SaagharWidget *MainWindow::getSaagharWidget(int tabIndex)
{
	if (tabIndex == -1) return 0;
	QWidget *curTabContent = mainTabWidget->widget(tabIndex);
	QObjectList tabContentList = curTabContent->children();

	for (int i=0; i< tabContentList.size(); ++i)
	{
		if (qobject_cast<SaagharWidget *>(tabContentList.at(i)))
		{
			return qobject_cast<SaagharWidget *>(tabContentList.at(i));
		}
	}
	return 0;
}

void MainWindow::tabCloser(int tabIndex)
{
	//if EditMode app need to ask about save changes!
	disconnect(mainTabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));

	QWidget *closedTabContent = mainTabWidget->widget(tabIndex);
	mainTabWidget->setUpdatesEnabled(false);
	mainTabWidget->removeTab(tabIndex);
	mainTabWidget->setUpdatesEnabled(true);
	
	delete closedTabContent;

	if (mainTabWidget->count() == 1) //there is just one tab present.
		mainTabWidget->setTabsClosable(false);
	currentTabChanged(mainTabWidget->currentIndex());//maybe we don't need this line as well disconnect and connect.
	connect(mainTabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
}

void MainWindow::insertNewTab()
{
	QWidget *tabContent = new QWidget();
	QGridLayout *tabGridLayout = new QGridLayout(tabContent);
	tabGridLayout->setObjectName(QString::fromUtf8("tabGridLayout"));
	QTableWidget *tabTableWidget = new QTableWidget(tabContent);
	//table defaults
    tabTableWidget->setObjectName(QString::fromUtf8("mainTableWidget"));
    tabTableWidget->setLayoutDirection(Qt::RightToLeft);
    tabTableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tabTableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tabTableWidget->setShowGrid(false);
    tabTableWidget->setGridStyle(Qt::NoPen);
    tabTableWidget->horizontalHeader()->setVisible(false);
    tabTableWidget->verticalHeader()->setVisible(false);

	saagharWidget = new SaagharWidget( tabContent, parentCatsToolBar, tabTableWidget);
	saagharWidget->setObjectName(QString::fromUtf8("saagharWidget"));

	connect(saagharWidget, SIGNAL(captionChanged()), this, SLOT(updateCaption()));

	connect(saagharWidget->tableViewWidget, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(tableItemClick(QTableWidgetItem *)));
	connect(saagharWidget->tableViewWidget, SIGNAL(itemPressed(QTableWidgetItem *)), this, SLOT(tableItemPress(QTableWidgetItem *)));
	
	//Enable/Disable navigation actions
	connect(saagharWidget, SIGNAL(navPreviousActionState(bool)),	actionPreviousPoem, SLOT(setEnabled(bool)) );
	connect(saagharWidget, SIGNAL(navNextActionState(bool)),	actionNextPoem, SLOT(setEnabled(bool)) );
	actionPreviousPoem->setEnabled( !SaagharWidget::ganjoorDataBase->getPreviousPoem(saagharWidget->currentPoem, saagharWidget->currentCat).isNull() );
	actionNextPoem->setEnabled( !SaagharWidget::ganjoorDataBase->getNextPoem(saagharWidget->currentPoem, saagharWidget->currentCat).isNull() );
	
	//Updating table on changing of selection
	connect(saagharWidget->tableViewWidget, SIGNAL(itemSelectionChanged()), this, SLOT(tableSelectChanged()));

	tabGridLayout->setContentsMargins(3,4,3,3);
	tabGridLayout->addWidget(tabTableWidget, 0, 0, 1, 1);

	mainTabWidget->setUpdatesEnabled(false);

	mainTabWidget->setCurrentIndex(mainTabWidget->addTab(tabContent,""));//add and gets focus to it
	if (mainTabWidget->count() > 1)
		mainTabWidget->setTabsClosable(true);//there are two or more tabs opened

	mainTabWidget->setUpdatesEnabled(true);
}

void MainWindow::newTabForItem(QString type, int id, bool noError)
{
	insertNewTab();
	saagharWidget->processClickedItem(type, id, noError);
}

void MainWindow::updateCaption()
{
	mainTabWidget->setTabText(mainTabWidget->currentIndex(), saagharWidget->currentCaption );
	setWindowTitle(QString::fromLocal8Bit("ساغر")+":"+saagharWidget->currentCaption);
}

void MainWindow::tableSelectChanged()
{//Bug in Qt: old selection is not repainted properly
	if (saagharWidget)
		saagharWidget->tableViewWidget->viewport()->update();
}

void MainWindow::actionExportAsPDFClicked()
{
	QPrinter printer(QPrinter::HighResolution);
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setColorMode(QPrinter::Color);
	printer.setPageSize(QPrinter::A4);
	printer.setCreator("Saaghar");
	printer.setPrinterName("Saaghar");
	printer.setFontEmbeddingEnabled(true);
	printPreview(&printer);
}

void MainWindow::actionExportClicked()
{
	QString exportedFileName = QFileDialog::getSaveFileName(this, tr("Export As"), QDir::homePath(), "HTML Document (*.html);;TeX - XePersian (*.tex);;Tab Separated (*.csv);;UTF-8 Text (*.txt)");
	if (exportedFileName.endsWith(".html", Qt::CaseInsensitive))
	{
		QString poemAsHTML = convertToHtml(saagharWidget);
		if (!poemAsHTML.isEmpty())
		{
			writeToFile(exportedFileName, poemAsHTML);
		}
		return;
	}

	if (exportedFileName.endsWith(".txt", Qt::CaseInsensitive))
	{
		QString tableAsText = tableToString(saagharWidget->tableViewWidget, "          "/*ten blank spaces*/, "\n", 0, 0, saagharWidget->tableViewWidget->rowCount(), saagharWidget->tableViewWidget->columnCount() );
		writeToFile(exportedFileName, tableAsText);
		return;
	}

	if (exportedFileName.endsWith(".csv", Qt::CaseInsensitive))
	{
		QString tableAsText = tableToString(saagharWidget->tableViewWidget, "\t", "\n", 0, 0, saagharWidget->tableViewWidget->rowCount(), saagharWidget->tableViewWidget->columnCount() );
		writeToFile(exportedFileName, tableAsText);
		return;
	}

	if (exportedFileName.endsWith(".tex", Qt::CaseInsensitive))
	{
		QString poemAsTeX = convertToTeX(saagharWidget);
		if (!poemAsTeX.isEmpty())
		{
			writeToFile(exportedFileName, poemAsTeX);
		}
		return;
	}
}

void MainWindow::writeToFile(QString fileName, QString textToWrite)
{
	QFile exportFile(fileName);
	if ( !exportFile.open( QIODevice::WriteOnly ) )
		{
		QMessageBox::warning( this,tr("Error"),tr("The file could not be saved. Please check if you have write permission."));
		return;
		}
	QTextStream out( &exportFile );
	out.setCodec("UTF-8");
	out << textToWrite;
	exportFile.close();
}

void MainWindow::print(QPrinter *printer)
{
	QPainter painter;
	painter.begin(printer);

	const int rows = saagharWidget->tableViewWidget->rowCount();
	const int columns = saagharWidget->tableViewWidget->columnCount();
	double totalWidth = 0.0;
	double currentHeight = 10.0;

	if (columns == 4)
	{
		totalWidth = qMax(saagharWidget->tableViewWidget->columnWidth(3), saagharWidget->minMesraWidth)+saagharWidget->tableViewWidget->columnWidth(2)+qMax(saagharWidget->tableViewWidget->columnWidth(1), saagharWidget->minMesraWidth);
	}
	else if (columns == 1)
	{
		totalWidth = saagharWidget->tableViewWidget->columnWidth(0);
	}
	int pageWidth = printer->pageRect().width();
	int pageHeight = printer->pageRect().height();

	const int extendedWidth = SaagharWidget::tableFont.pointSize()*4;
	const qreal scale = pageWidth/(totalWidth+extendedWidth );//printer->logicalDpiX() / 96;
	painter.scale(scale, scale);

	for (int row = 0; row < rows; ++row)
	{
		int thisRowHeight = saagharWidget->tableViewWidget->rowHeight(row);
		for (int col = 0; col < columns; ++col)
		{
			QTableWidgetItem *item = saagharWidget->tableViewWidget->item(row, col);
			int span = saagharWidget->tableViewWidget->columnSpan(row, col);
			QString text = "";

			if (item)
			{
				int rectX1=0;
				int rectX2=0;
				if (columns == 4)
				{
					if (span == 1)
					{
						if (col == 1)
						{
							rectX1 = saagharWidget->tableViewWidget->columnWidth(3)+saagharWidget->tableViewWidget->columnWidth(2);
							rectX2 = saagharWidget->tableViewWidget->columnWidth(3)+saagharWidget->tableViewWidget->columnWidth(2)+saagharWidget->tableViewWidget->columnWidth(1);
						}
						else if (col == 3)
						{
							rectX1 = 0;
							rectX2 = saagharWidget->tableViewWidget->columnWidth(3);
						}
					}
					else
					{
						rectX1 = 0;
						rectX2 = saagharWidget->tableViewWidget->columnWidth(3)+saagharWidget->tableViewWidget->columnWidth(2)+saagharWidget->tableViewWidget->columnWidth(1);
						int textWidth = saagharWidget->tableViewWidget->fontMetrics().boundingRect(item->text()).width();
						int numOfLine = textWidth/rectX2;
						thisRowHeight = thisRowHeight+(saagharWidget->tableViewWidget->fontMetrics().height()*numOfLine);
					}
				}
				else if (columns == 1)
				{
					rectX1 = 0;
					rectX2 = saagharWidget->tableViewWidget->columnWidth(0);
				}

				QRect mesraRect;
				mesraRect.setCoords(rectX1,currentHeight, rectX2, currentHeight+thisRowHeight);
				col = col+span-1;
				text = item->text();

				QFont fnt = SaagharWidget::tableFont;
				fnt.setPointSizeF(fnt.pointSizeF()/scale);
				painter.setFont(fnt);
				painter.setPen(QColor(SaagharWidget::textColor));
				painter.setLayoutDirection(Qt::RightToLeft);
				int flags = item->textAlignment();
				if (span > 1)
					flags |= Qt::TextWordWrap;
				painter.drawText(mesraRect, flags, text);
			}
		}

		currentHeight += thisRowHeight;
		if ((currentHeight+50)*scale >= pageHeight)
		{
			printer->newPage();
			currentHeight = 10.0;
		}
	}
	painter.end();
}

void MainWindow::actionPrintClicked()
{
	if (!defaultPrinter)
		defaultPrinter = new QPrinter(QPrinter::HighResolution);
	QPrintDialog printDialog(defaultPrinter, this);
	if (printDialog.exec() == QDialog::Accepted)
		print(defaultPrinter);
}

void MainWindow::actionPrintPreviewClicked()
{
	if (!defaultPrinter)
		defaultPrinter = new QPrinter(QPrinter::HighResolution);
	printPreview(defaultPrinter);
}

void MainWindow::printPreview(QPrinter *printer)
{
	QPrintPreviewDialog *previewDialog = new QPrintPreviewDialog(printer, this);
	connect( previewDialog, SIGNAL(paintRequested(QPrinter *) ), this, SLOT(print(QPrinter *)) );
	previewDialog->exec();
}

QString MainWindow::convertToHtml(SaagharWidget *saagharObject)
{
	GanjoorPoem curPoem = SaagharWidget::ganjoorDataBase->getPoem(saagharObject->currentPoem);
	if (curPoem.isNull()) return "";
	QString columnGroupFormat = QString("<COL WIDTH=%1>").arg(saagharObject->tableViewWidget->columnWidth(0));
	int numberOfCols = saagharObject->tableViewWidget->columnCount()-1;
	QString tableBody = "";
	if ( numberOfCols == 3)
	{
		columnGroupFormat = QString("<COL WIDTH=%1><COL WIDTH=%2><COL WIDTH=%3>").arg(saagharObject->tableViewWidget->columnWidth(1)).arg(saagharObject->tableViewWidget->columnWidth(2)).arg(saagharObject->tableViewWidget->columnWidth(3));
		for (int row = 0; row < saagharObject->tableViewWidget->rowCount(); ++row)
		{
			int span = saagharObject->tableViewWidget->columnSpan(row, 1);
			if (span == 1)
			{
				QTableWidgetItem *firstItem = saagharObject->tableViewWidget->item(row, 1);
				QTableWidgetItem *secondItem = saagharObject->tableViewWidget->item(row, 3);
				QString firstText = "";
				QString secondText = "";
				if (firstItem)
					firstText = firstItem->text();
				if (secondItem)
					secondText = secondItem->text();
				tableBody += QString("\n<TR>\n<TD HEIGHT=%1 ALIGN=LEFT>%2</TD>\n<TD></TD>\n<TD ALIGN=RIGHT>%3</TD>\n</TR>\n").arg(saagharObject->tableViewWidget->rowHeight(row)).arg(firstText).arg(secondText);
			}
			else
			{
				QTableWidgetItem *item = saagharObject->tableViewWidget->item(row, 1);
				QString mesraText = "";

				QString align = "RIGHT";
				if (SaagharWidget::centeredView)
					align = "CENTER";

				QStringList itemDataList;
				if (item)
				{
					mesraText = item->text();
					QVariant data = item->data(Qt::UserRole);
					if (data.isValid() && !data.isNull())
						itemDataList = data.toString().split("|", QString::SkipEmptyParts);
				}

				VersePosition itemPosition = CenteredVerse1;
				if (itemDataList.size() == 4)
				{
					itemPosition = (VersePosition)itemDataList.at(3).toInt();
				}

				switch (itemPosition)
				{
					case Paragraph :
					case Single :
							mesraText.replace(" ", "&nbsp;");
							align = "RIGHT";
						break;

					default:
						break;
				}
				tableBody += QString("\n<TR>\n<TD COLSPAN=3 HEIGHT=%1 WIDTH=%2 ALIGN=%3>%4</TD>\n</TR>\n").arg(saagharObject->tableViewWidget->rowHeight(row)).arg( saagharObject->tableViewWidget->width() ).arg(align).arg(mesraText);
			}
		}
	}
	else
	{
		return "";
	}

	QString tableAsHTML = QString("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n<HTML>\n<HEAD>\n<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=utf-8\">\n<TITLE>%1</TITLE>\n<META NAME=\"GENERATOR\" CONTENT=\"Saaghar, a Persian poetry software, http://www.pojh.co.cc/saaghar\">\n</HEAD>\n\n<BODY TEXT=%2>\n<FONT FACE=\"%3\">\n<TABLE ALIGN=%4 DIR=RTL FRAME=VOID CELLSPACING=0 COLS=%5 RULES=NONE BORDER=0>\n<COLGROUP>%6</COLGROUP>\n<TBODY>\n%7\n</TBODY>\n</TABLE>\n</BODY>\n</HTML>\n")
		.arg(curPoem._Title).arg(SaagharWidget::textColor.name()).arg(SaagharWidget::tableFont.family()).arg("CENTER").arg(numberOfCols).arg(columnGroupFormat).arg(tableBody);
	return tableAsHTML;
	/*******************************************************
	%1: title
	%2: Text Color, e.g: "#CC55AA"
	%3: Font Face, e.g: "XB Zar"
	%4: Table Align
	%5: number of columns
	%6: column group format, e.g: <COL WIDTH=259><COL WIDTH=10><COL WIDTH=293>
	%7: table body, e.g: <TR><TD HEIGHT=17 ALIGN=LEFT>Column1</TD><TD></TD><TD ALIGN=LEFT>Column3</TD></TR>
	*********************************************************/
}

QString MainWindow::convertToTeX(SaagharWidget *saagharObject)
{
	GanjoorPoem curPoem = SaagharWidget::ganjoorDataBase->getPoem(saagharObject->currentPoem);
	if (curPoem.isNull()) return "";
	int numberOfCols = saagharObject->tableViewWidget->columnCount()-1;
	QString poemType = "traditionalpoem";
	QString tableBody = "";
	bool poemEnvironmentEnded = false;
	if ( numberOfCols == 3)
	{
		for (int row = 1; row < saagharObject->tableViewWidget->rowCount(); ++row)
		{
			int span = saagharObject->tableViewWidget->columnSpan(row, 1);
			if (span == 1)
			{
				QTableWidgetItem *firstItem = saagharObject->tableViewWidget->item(row, 1);
				QTableWidgetItem *secondItem = saagharObject->tableViewWidget->item(row, 3);
				QString firstText = "";
				QString secondText = "";
				if (firstItem)
					firstText = firstItem->text();
				if (secondItem)
					secondText = secondItem->text();
				
				tableBody += QString("%1 & %2 ").arg(firstText).arg(secondText);
				if (row+1 != saagharObject->tableViewWidget->rowCount())
					tableBody+="\\\\\n";
			}
			else
			{
				QString align = "r";
				if (row == 0 && SaagharWidget::centeredView)
					align = "c";
				QTableWidgetItem *item = saagharObject->tableViewWidget->item(row, 1);
				QString mesraText = "";
				QStringList itemDataList;
				if (item)
				{
					mesraText = item->text();
					QVariant data = item->data(Qt::UserRole);
					if (data.isValid() && !data.isNull())
						itemDataList = data.toString().split("|", QString::SkipEmptyParts);
				}

				VersePosition itemPosition = CenteredVerse1;
				if (itemDataList.size() == 4)
				{
					itemPosition = (VersePosition)itemDataList.at(3).toInt();
				}
				switch (itemPosition)
				{
					case Paragraph :
							tableBody += QString("%Empty Line: a tricky method, last beyt has TeX newline \"\\\\\"\n");//tricky, last beyt has TeX newline "\\"
							tableBody += QString("\n\\end{%1}\n").arg(poemType);
							tableBody += QString("%1").arg(mesraText);
							poemEnvironmentEnded = true;
							if (row+1 != saagharObject->tableViewWidget->rowCount())
							{
								tableBody+="\\\\\n";
								tableBody += QString("\n\\begin{%1}\n").arg(poemType);
								poemEnvironmentEnded = false;
							}
						break;
		
					case Single :
							mesraText.replace(" ","\\ ");
							poemType = "modernpoem";

					default:
							tableBody += QString("%1").arg(mesraText);
							if (row+1 != saagharObject->tableViewWidget->rowCount())
								tableBody+="\\\\\n";
						break;
				}
			}
		}
	}
	else
	{
		return "";
	}
	
	QString endOfEnvironment = "";
	if (!poemEnvironmentEnded)
		endOfEnvironment = QString("\\end{%1}\n").arg(poemType);

	QString	tableAsTeX = QString("%%%%%\n%This file is generated automatically by Saaghar %1, 2010 http://www.pojh.co.cc\n%%%%%\n%XePersian and bidipoem packages must have been installed on your TeX distribution for compiling this document\n%You can compile this document by running XeLaTeX on it, twice.\n%%%%%\n\\documentclass{article}\n\\usepackage{hyperref}%\n\\usepackage[Kashida]{xepersian}\n\\usepackage{bidipoem}\n\\settextfont{%2}\n\\hypersetup{\npdftitle={%3},%\npdfsubject={Poem},%\npdfkeywords={Poem, Persian},%\npdfcreator={Saaghar, a Persian poetry software, http://www.pojh.co.cc/saaghar},%\npdfview=FitV,\n}\n\\begin{document}\n\\begin{center}\n%3\\\\\n\\end{center}\n\\begin{%4}\n%5\n%6\\end{document}\n%End of document\n")
								 .arg(VER_FILEVERSION_STR).arg(SaagharWidget::tableFont.family()).arg(curPoem._Title).arg(poemType).arg(tableBody).arg(endOfEnvironment);
	return tableAsTeX;
	/*******************************************************
	%1: Saaghar Version: VER_FILEVERSION_STR
	%2: Font Family
	%3: Poem Title
	%4: Poem Type, traditionalpoem or modernpoem environment
	%5: Poem body, e.g:
	مصراع۱سطر۱‏ ‎&%
	مصراع۲سطر۱‏ \\ 
	بند ‎\\ 
	%6: End of environment if it's not ended
	*********************************************************/
}

void MainWindow::actionNewWindowClicked()
{
	QProcess::startDetached("\""+QCoreApplication::applicationFilePath()+"\"");
}

void MainWindow::actionNewTabClicked()
{
	insertNewTab();
}

void MainWindow::actionFaalClicked()
{
	int PoemID = SaagharWidget::ganjoorDataBase->getRandomPoemID(24);
    GanjoorPoem poem = SaagharWidget::ganjoorDataBase->getPoem(PoemID);
	if (!poem.isNull())
		saagharWidget->processClickedItem("PoemID", poem._ID, true);
    else
        actionFaalClicked();//not any random id exists, so repeat until finding a valid id
}

void MainWindow::aboutSaaghar()
{
	QMessageBox about(this);
	QPixmap pixmap(":/resources/images/saaghar.png");
	about.setIconPixmap(pixmap);
	about.setWindowTitle(tr("About Saaghar"));
	about.setTextFormat(Qt::RichText);
	about.setText(tr("<br />%1 is a persian poem viewer software, it uses \"ganjoor.net\" database, and some of its codes are ported to C++ and Qt from \"desktop ganjoor\" that is a C# .NET application written by %2.<br /><br />Logo Designer: %3<br /><br />Photos and Description Sources: ganjoor.net & WiKiPedia<br /><br />Author: <a href=\"http://www.pojh.co.cc/\">S. Razi Alavizadeh</a>,<br />Home Page: %4<br />Mailing List: %5<br /><br />Version: %6<br />Build Time: %7")
		.arg("<a href=\"http://www.pojh.co.cc/saaghar\">Saaghar</a>").arg("<a href=\"http://www.gozir.com/\">Hamid Reza Mohammadi</a>").arg("<a href=\"http://www.phototak.com/\">S. Nasser Alavizadeh</a>").arg("<a href=\"http://www.pojh.co.cc/saaghar\">http://www.pojh.co.cc/saaghar</a>").arg("<a href=\"http://groups.google.com/group/saaghar/\">http://groups.google.com/group/saaghar</a>").arg(VER_FILEVERSION_STR).arg(VER_FILEBUILDTIME_STR));
	about.setStandardButtons(QMessageBox::Ok);
	about.setEscapeButton(QMessageBox::Ok);

	about.exec();
}

void MainWindow::helpContents()
{
	QMessageBox help(this);
	QPixmap pixmap(":/resources/images/saaghar.png");
	help.setIconPixmap(pixmap);
	help.setWindowTitle(tr("Help Contents"));
	help.setTextFormat(Qt::RichText);
	help.setText(tr("<br />Saaghar Version: %1<br /><br />Mouse Left Click: Opens in current Tab<br />Mouse Right Click: Opens in new Tab<br /><br />Shortcuts:<br />Saaghar uses your Operating System standard shortcuts.").arg(VER_PRODUCTVERSION_STR));
	help.setStandardButtons(QMessageBox::Ok);
	help.setEscapeButton(QMessageBox::Ok);
	help.exec();
}

void MainWindow::setupUi()
{
	QString iconThemePath=":/resources/images/";
	if (!settingsIconThemePath.isEmpty() && settingsIconThemeState)
		iconThemePath = settingsIconThemePath;

	//Initialize main menu items
    menuFile = new QMenu(tr("&File"), ui->menuBar);
    menuFile->setObjectName(QString::fromUtf8("menuFile"));
    menuNavigation = new QMenu(tr("&Navigation"), ui->menuBar);
    menuNavigation->setObjectName(QString::fromUtf8("menuNavigation"));
    menuTools = new QMenu(tr("&Tools"), ui->menuBar);
    menuTools->setObjectName(QString::fromUtf8("menuTools"));
    menuHelp = new QMenu(tr("&Help"), ui->menuBar);
    menuHelp->setObjectName(QString::fromUtf8("menuHelp"));

	ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	actionHome = new QAction(QIcon(iconThemePath+"/home.png"),tr("&Home"), this);
	actionHome->setObjectName(QString::fromUtf8("actionHome"));

	actionPreviousPoem = new QAction(tr("&Previous"), this);
	actionPreviousPoem->setObjectName(QString::fromUtf8("actionPreviousPoem"));

	if (ui->mainToolBar->layoutDirection() == Qt::LeftToRight)
	{
		actionPreviousPoem->setShortcuts(QKeySequence::Back);
		actionPreviousPoem->setIcon(QIcon(iconThemePath+"/previous.png"));
	}
	else
	{
		actionPreviousPoem->setShortcuts(QKeySequence::Forward);
		actionPreviousPoem->setIcon(QIcon(iconThemePath+"/next.png"));
	}

	actionNextPoem = new QAction(tr("&Next"), this);
	actionNextPoem->setObjectName(QString::fromUtf8("actionNextPoem"));
	if (ui->mainToolBar->layoutDirection() == Qt::LeftToRight)
	{
		actionNextPoem->setShortcuts(QKeySequence::Forward);
		actionNextPoem->setIcon(QIcon(iconThemePath+"/next.png"));
	}
	else
	{
		actionNextPoem->setShortcuts(QKeySequence::Back);
		actionNextPoem->setIcon(QIcon(iconThemePath+"/previous.png"));
	}

	actionCopy = new QAction(QIcon(iconThemePath+"/copy.png"),tr("&Copy"), this);
	actionCopy->setObjectName(QString::fromUtf8("actionCopy"));
	actionCopy->setShortcuts(QKeySequence::Copy);

	searchToolbarAction = new QAction(QIcon(iconThemePath+"/search.png"),tr("&Search"), this);
	searchToolbarAction->setObjectName(QString::fromUtf8("searchToolbarAction"));
	searchToolbarAction->setShortcuts(QKeySequence::Find);
	searchToolbarAction->setCheckable(true);
	searchToolbarAction->setChecked(ui->searchWidget->isVisible());

	actionSettings = new QAction(QIcon(iconThemePath+"/settings.png"),tr("S&ettings") , this);
	actionSettings->setMenuRole(QAction::PreferencesRole);//needed for Mac OS X
	actionSettings->setObjectName(QString::fromUtf8("actionSettings"));

	actionViewInGanjoorSite = new QAction(QIcon(iconThemePath+"/browse_net.png"), tr("View in \"&ganjoor.net\""), this);
	actionViewInGanjoorSite->setObjectName(QString::fromUtf8("actionViewInGanjoorSite"));

	actionExit = new QAction(QIcon(iconThemePath+"/exit.png"), tr("E&xit"),this);
	actionExit->setMenuRole(QAction::QuitRole);//needed for Mac OS X
	actionExit->setObjectName(QString::fromUtf8("actionExit"));
	actionExit->setShortcut(Qt::CTRL | Qt::Key_Q);

	actionNewTab = new QAction(QIcon(iconThemePath+"/new_tab.png"), tr("New &Tab"),this);
	actionNewTab->setObjectName(QString::fromUtf8("actionNewTab"));
	actionNewTab->setShortcuts(QKeySequence::AddTab);

	actionNewWindow = new QAction(QIcon(iconThemePath+"/new_window.png"), tr("&New Window"),this);
	actionNewWindow->setObjectName(QString::fromUtf8("actionNewWindow"));
	actionNewWindow->setShortcuts(QKeySequence::New);

    actionAboutSaaghar = new QAction(QIcon(":/resources/images/saaghar.png"), tr("&About"),this);
	actionAboutSaaghar->setMenuRole(QAction::AboutRole);//needed for Mac OS X
    actionAboutSaaghar->setObjectName(QString::fromUtf8("actionAboutSaaghar"));

    actionAboutQt = new QAction(QIcon(":/resources/images/qt-logo.png"), tr("About &Qt"), this);
	actionAboutQt->setMenuRole(QAction::AboutQtRole);//needed for Mac OS X
    actionAboutQt->setObjectName(QString::fromUtf8("actionAboutQt"));

	actionFaal = new QAction(QIcon(iconThemePath+"/faal.png"), tr("&Faal"),this);
    actionFaal->setObjectName(QString::fromUtf8("actionFaal"));

	actionPrint = new QAction(QIcon(iconThemePath+"/print.png"), tr("&Print..."),this);
    actionPrint->setObjectName(QString::fromUtf8("actionPrint"));
	actionPrint->setShortcuts(QKeySequence::Print);

	actionPrintPreview = new QAction(QIcon(iconThemePath+"/print-preview.png"), tr("Print Pre&view..."),this);
    actionPrintPreview->setObjectName(QString::fromUtf8("actionPrintPreview"));

	actionExport = new QAction(QIcon(iconThemePath+"/export.png"), tr("&Export As..."),this);
    actionExport->setObjectName(QString::fromUtf8("actionExport"));
	actionExport->setShortcuts(QKeySequence::SaveAs);

	actionExportAsPDF = new QAction(QIcon(iconThemePath+"/export-pdf.png"), tr("Exp&ort As PDF..."),this);
    actionExportAsPDF->setObjectName(QString::fromUtf8("actionExportAsPDF"));

	actionHelpContents = new QAction(QIcon(":/resources/images/saaghar.png"), tr("&Help Contents..."),this);
    actionHelpContents->setObjectName(QString::fromUtf8("actionHelpContents"));
	actionHelpContents->setShortcuts(QKeySequence::HelpContents);

	//Inserting main menu items
	ui->menuBar->addMenu(menuFile);
	ui->menuBar->addMenu(menuNavigation);
	ui->menuBar->addMenu(menuTools);
	ui->menuBar->addMenu(menuHelp);

	//Inserting items of menus
    menuFile->addAction(actionNewTab);
    menuFile->addAction(actionNewWindow);
    menuFile->addSeparator();
	menuFile->addAction(actionExportAsPDF);
    menuFile->addAction(actionExport);
	menuFile->addSeparator();
	menuFile->addAction(actionPrintPreview);
	menuFile->addAction(actionPrint);
    menuFile->addSeparator();
    menuFile->addAction(actionExit);

	menuNavigation->addAction(actionHome);
    menuNavigation->addSeparator();
    menuNavigation->addAction(actionPreviousPoem);
    menuNavigation->addAction(actionNextPoem);
    menuNavigation->addSeparator();
	menuNavigation->addAction(actionFaal);

	menuTools->addAction(searchToolbarAction);
    menuTools->addSeparator();
    menuTools->addAction(actionViewInGanjoorSite);
    menuTools->addSeparator();
    menuTools->addAction(actionSettings);

	menuHelp->addAction(actionHelpContents);
    menuHelp->addSeparator();
	menuHelp->addAction(actionAboutSaaghar);
	menuHelp->addAction(actionAboutQt);

	//Inserting main toolbar Items
	ui->mainToolBar->addAction(actionHome);
	ui->mainToolBar->addSeparator();
	ui->mainToolBar->addAction(actionPreviousPoem);
	ui->mainToolBar->addAction(actionNextPoem);
	ui->mainToolBar->addSeparator();
	ui->mainToolBar->addAction(actionFaal);
	ui->mainToolBar->addSeparator();
	ui->mainToolBar->addAction(actionExportAsPDF);
	ui->mainToolBar->addAction(actionCopy);
	ui->mainToolBar->addAction(searchToolbarAction);
	ui->mainToolBar->addAction(actionNewTab);
	ui->mainToolBar->addSeparator();
	ui->mainToolBar->addAction(actionSettings);
}

void MainWindow::createConnections()
{
		//File
	connect(actionNewTab			,	SIGNAL(triggered())		,	this, SLOT(actionNewTabClicked())		);
	connect(actionNewWindow			,	SIGNAL(triggered())		,	this, SLOT(actionNewWindowClicked())	);
	connect(actionExportAsPDF		,	SIGNAL(triggered())		,	this, SLOT(actionExportAsPDFClicked())	);
	connect(actionExport			,	SIGNAL(triggered())		,	this, SLOT(actionExportClicked())		);
	connect(actionPrintPreview		,	SIGNAL(triggered())		,	this, SLOT(actionPrintPreviewClicked())	);
	connect(actionPrint				,	SIGNAL(triggered())		,	this, SLOT(actionPrintClicked())		);
	connect(actionExit				,	SIGNAL(triggered())		,	this, SLOT(close())						);

		//Navigation
	connect(actionHome				,	SIGNAL(triggered())		,	this, SLOT(actionHomeClicked())			);
	connect(actionPreviousPoem		,	SIGNAL(triggered())		,	this, SLOT(actionPreviousPoemClicked())	);
	connect(actionNextPoem			,	SIGNAL(triggered())		,	this, SLOT(actionNextPoemClicked())		);
	connect(actionFaal				,	SIGNAL(triggered())		,	this, SLOT(actionFaalClicked())			);

		//Tools
	connect(actionViewInGanjoorSite	,	SIGNAL(triggered())		,	this, SLOT(actionGanjoorSiteClicked())	);
	connect(actionCopy				,	SIGNAL(triggered())		,	this, SLOT(copySelectedItems()));
	connect(actionSettings			,	SIGNAL(triggered())		,	this, SLOT(globalSettings()));
	connect(searchToolbarAction		,	SIGNAL(toggled(bool))	,	this, SLOT(searchWidgetSetVisible(bool)));

		//Help
	connect(actionHelpContents		,	SIGNAL(triggered())		,	this, SLOT(helpContents())				);
	connect(actionAboutSaaghar		,	SIGNAL(triggered())		,	this, SLOT(aboutSaaghar())				);
	connect(actionAboutQt			,	SIGNAL(triggered())		,	qApp, SLOT(aboutQt())					);
}

//Settings Dialog
void MainWindow::globalSettings()
{

	Settings *settingsDlg = new Settings(this, settingsIconThemeState, settingsIconThemePath);
	if (settingsDlg->exec())
	{
		settingsDlg->acceptSettings( &settingsIconThemeState, &settingsIconThemePath);
		if (saagharWidget)
		{
			saagharWidget->loadSettings();

			if (saagharWidget->tableViewWidget->columnCount() == 1 && saagharWidget->tableViewWidget->rowCount() > 0 && saagharWidget->currentCat != 0)
			{
				QTableWidgetItem *item = saagharWidget->tableViewWidget->item(0,0);
				if (item)
				{
					QString text = item->text();
					int textWidth = saagharWidget->tableViewWidget->fontMetrics().boundingRect(text).width();
					int totalWidth = saagharWidget->tableViewWidget->columnWidth(0);
					int numOfRow = textWidth/totalWidth ;
					saagharWidget->tableViewWidget->setRowHeight(0, 2*saagharWidget->tableViewWidget->rowHeight(0)+(saagharWidget->tableViewWidget->fontMetrics().height()*(numOfRow/*+1*/)));
				}
			}
			else if ( saagharWidget->currentCat == 0 && saagharWidget->currentPoem == 0 ) //it's Home.
				saagharWidget->tableViewWidget->resizeRowsToContents();
		}
#ifndef Q_WS_MAC //This doesn't work on MACX and moved to "SaagharWidget::loadSettings()"
		//apply new text and background color, and also background picture
		loadTabWidgetSettings();
#endif
		saveGlobalSettings();//save new settings to disk
	}
}

QSettings *MainWindow::getSettingsObject()
{
	QString organization = "Pojh";
	QString settingsName = "Saaghar";
	QSettings::Format settingsFormat = QSettings::NativeFormat;

	if (isPortable)
	{
		settingsFormat = QSettings::IniFormat;
		QSettings::setPath(settingsFormat, QSettings::UserScope, QCoreApplication::applicationDirPath());
		organization = ".";
		settingsName = "settings";
	}
	//QSettings *config = 
	return	new QSettings(settingsFormat, QSettings::UserScope, organization, settingsName);
}

void MainWindow::loadGlobalSettings()
{
	QSettings *config = getSettingsObject();
	//state
	QByteArray mainWindowState = config->value("MainWindowState").toByteArray();

	restoreState( config->value("MainWindowState").toByteArray(), 1);
	
	openedTabs = config->value("openedTabs", "").toString();

	SaagharWidget::backgroundImageState = config->value("Background State",false).toBool();
	SaagharWidget::backgroundImagePath = config->value("Background Path", "").toString();
	settingsIconThemeState = config->value("Icon Theme State",false).toBool();
	settingsIconThemePath = config->value("Icon Theme Path", "").toString();

	QFontDatabase fontDb;
	QStringList allFonts = fontDb.families();
	QString defaultFont;

	//The "XB Sols" is embeded in executable.
	QString fontFamily=config->value("Font Family", "XB Sols").toString();
	int fontSize=config->value( "Font Size", 18).toInt();
	QFont fnt(fontFamily,fontSize);
	SaagharWidget::tableFont = fnt;
	SaagharWidget::showBeytNumbers = config->value("Show Beyt Numbers",true).toBool();
	SaagharWidget::textColor=config->value("Text Color",QColor(0x23,0x65, 0xFF)).value<QColor>();
	SaagharWidget::matchedTextColor=config->value("Matched Text Color",QColor(0x5E, 0xFF, 0x13)).value<QColor>();
	SaagharWidget::backgroundColor=config->value("Background Color",QColor(0xFE, 0xFD, 0xF2)).value<QColor>();
}

void MainWindow::loadTabWidgetSettings()
{
	QPalette p(mainTabWidget->palette());
	if ( SaagharWidget::backgroundImageState && !SaagharWidget::backgroundImagePath.isEmpty() )
	{
		p.setBrush(QPalette::Base, QBrush(QPixmap(SaagharWidget::backgroundImagePath)) );
	}
	else
	{
		p.setColor(QPalette::Base, SaagharWidget::backgroundColor );
	}
	p.setColor(QPalette::Text, SaagharWidget::textColor );
	mainTabWidget->setPalette(p);
}

void MainWindow::saveGlobalSettings()
{
	QSettings *config = getSettingsObject();

	config->setValue("Background State", SaagharWidget::backgroundImageState);
	config->setValue("Background Path", SaagharWidget::backgroundImagePath);
	config->setValue("Icon Theme State", settingsIconThemeState);
	config->setValue("Icon Theme Path", settingsIconThemePath);

	//font
	config->setValue("Font Family", SaagharWidget::tableFont.family());
	config->setValue( "Font Size", SaagharWidget::tableFont.pointSize());

	config->setValue("Show Beyt Numbers", SaagharWidget::showBeytNumbers);
	config->setValue("Text Color", SaagharWidget::textColor);
	config->setValue("Matched Text Color", SaagharWidget::matchedTextColor);
	config->setValue("Background Color", SaagharWidget::backgroundColor);
}

QString MainWindow::tableToString(QTableWidget *table, QString mesraSeparator, QString beytSeparator, int startRow, int startColumn, int endRow, int endColumn)
{
	QString tableAsString = "";
	for(int row = startRow; row < endRow ; ++row )
	{
		bool secondColumn = true;
		for(int col = startColumn; col < endColumn ; ++col )
		{
			if (col == 0) continue;
			QTableWidgetItem *currentItem = table->item(row , col);
			QString text = "";
			if (currentItem)
			{
				QVariant data = currentItem->data(Qt::DisplayRole);
				if ( data.isValid() )
					text = data.toString();
			}
			tableAsString.append(text);
			if (secondColumn)
			{
				secondColumn = false;
				tableAsString.append(mesraSeparator);
			}
		}
		tableAsString.append(beytSeparator);
	}
	return tableAsString;
}

//copy to clipboard
void MainWindow::copySelectedItems()
{//TODO: we need improve this function for "export" functionality
	if (!saagharWidget || !saagharWidget->tableViewWidget) return;
	QString selectedText="";
	QList<QTableWidgetSelectionRange> selectedRanges = saagharWidget->tableViewWidget->selectedRanges();
	if (selectedRanges.isEmpty())
		return;
	foreach(QTableWidgetSelectionRange selectedRange, selectedRanges)
	{
		if ( selectedRange.rowCount() == 0 || selectedRange.columnCount() == 0 )
			return;
		selectedText += tableToString(saagharWidget->tableViewWidget, "          "/*ten blank spaces-Mesra separator*/, "\n"/*Beyt separator*/, selectedRange.topRow(), selectedRange.leftColumn(), selectedRange.bottomRow()+1, selectedRange.rightColumn()+1);
	}
	QApplication::clipboard()->setText(selectedText);
}

//Navigation
void MainWindow::actionHomeClicked()
{
    saagharWidget->showHome();
}

void MainWindow::actionPreviousPoemClicked()
{
	saagharWidget->previousPoem();
}

void MainWindow::actionNextPoemClicked()
{
	saagharWidget->nextPoem();
}

//Tools
void MainWindow::actionGanjoorSiteClicked()
{
	if ( saagharWidget->currentPageGanjoorUrl().isEmpty() )
    {
        QMessageBox::critical(this, tr("Error!"), tr("There is no equivalent page there at ganjoor website."), QMessageBox::Ok, QMessageBox::Ok );
    }
    else
    {
       QDesktopServices::openUrl(saagharWidget->currentPageGanjoorUrl());
    }
}

void MainWindow::resizeEvent( QResizeEvent * event )
{
	saagharWidget->resizeTable(saagharWidget->tableViewWidget);
	QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent( QCloseEvent * event )
{
	QFontDatabase::removeAllApplicationFonts();
	saveSaagharState();
	event->accept();
}

void MainWindow::saveSaagharState()
{
	QSettings *config = getSettingsObject();

	config->setValue("MainWindowState", saveState(1));
	QString openedTabs = "";
	for (int i = 0; i < mainTabWidget->count(); ++i)
	{
		SaagharWidget *tmp = getSaagharWidget(i);
		QString tabViewType;
		if (tmp->currentPoem > 0)
			tabViewType = "PoemID="+QString::number(tmp->currentPoem);
		else
			tabViewType = "CatID="+QString::number(tmp->currentCat);
		
		openedTabs += tabViewType+"|";
	}
	config->setValue("openedTabs", openedTabs);
}

//Search
void MainWindow::showSearchResults(QString phrase, int PageStart, int count, int PoetID, QWidget *dockWidget)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	QApplication::processEvents();
    
	QList<int> poemIDsList = SaagharWidget::ganjoorDataBase->getPoemIDsContainingPhrase(phrase, PageStart, count+1, PoetID);
    int tmpCount = count;
	bool HasMore = count>0 && poemIDsList.size() == count+1;
	bool navButtonsNeeded = false;
    if (poemIDsList.size() <= count)
	{
        count = poemIDsList.size();
		if (PageStart>0)
			navButtonsNeeded = true;//almost one of navigation button needs to be enabled
	}
    else
	{
        count = HasMore ? count : poemIDsList.size() - 1;
		navButtonsNeeded = true;//almost one of navigation button needs to be enabled
	}

    if (count < 1)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::information(this, tr("Search"), tr("No match found."));
		return;
	}
	QTableWidget *searchTable;
	QToolButton *searchNextPage, *searchPreviousPage;
	QAction *actSearchNextPage, *actSearchPreviousPage;
	QDockWidget  *searchResultWidget = 0;
	QWidget *searchResultContents;
	bool fromOtherPage = false;

	if (dockWidget)
	{
		fromOtherPage = true;
		qDeleteAll(dockWidget->children());
	}

	if (!fromOtherPage)
	{
		searchResultWidget = new QDockWidget(phrase, this);
		searchResultWidget->setObjectName(QString::fromUtf8("searchResultWidget"));
		searchResultWidget->setLayoutDirection(Qt::RightToLeft);
		searchResultWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
		searchResultWidget->setAttribute(Qt::WA_DeleteOnClose, true);
		searchResultContents = new QWidget();
		searchResultContents->setObjectName(QString::fromUtf8("searchResultContents"));
		searchResultContents->setAttribute(Qt::WA_DeleteOnClose, true);
	}
	else
	{
		searchResultContents = dockWidget;
	}

	QGridLayout *searchGridLayout = new QGridLayout(searchResultContents);
	searchGridLayout->setSpacing(6);
	searchGridLayout->setContentsMargins(11, 11, 11, 11);
	searchGridLayout->setObjectName(QString::fromUtf8("searchGridLayout"));
	QHBoxLayout *horizontalLayout = new QHBoxLayout();
	horizontalLayout->setSpacing(6);
	horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
	QGridLayout *searchTableGridLayout = new QGridLayout();
	searchTableGridLayout->setSpacing(6);
	searchTableGridLayout->setObjectName(QString::fromUtf8("gridLayout_3"));
	//create QTableWidget
	searchTable = new QTableWidget(searchResultContents);
	searchTable->setObjectName(QString::fromUtf8("searchTable"));
	searchTable->setAlternatingRowColors(true);
	searchTable->setSelectionMode(QAbstractItemView::NoSelection);
	searchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	searchTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	searchTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	searchTable->setRowCount(count);
	searchTable->setColumnCount(3);
	searchTable->verticalHeader()->setVisible(false);
	searchTable->horizontalHeader()->setVisible(false);
	searchTable->horizontalHeader()->setHighlightSections(false);
	searchTable->horizontalHeader()->setStretchLastSection(true);

	//create connections for mouse signals
	connect(searchTable, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(tableItemClick(QTableWidgetItem *)));
	connect(searchTable, SIGNAL(itemPressed(QTableWidgetItem *)), this, SLOT(tableItemPress(QTableWidgetItem *)));

	//install delagate on third column
	searchTable->setItemDelegateForColumn(2, new SaagharItemDelegate(searchTable, searchTable->style(), phrase));

	searchTableGridLayout->addWidget(searchTable, 0, 0, 1, 1);

	QVBoxLayout *searchNavVerticalLayout = new QVBoxLayout();
	searchNavVerticalLayout->setSpacing(6);
	searchNavVerticalLayout->setObjectName(QString::fromUtf8("searchNavVerticalLayout"));
	searchNextPage = new QToolButton(searchResultContents);
	searchNextPage->setObjectName(QString::fromUtf8("searchNextPage"));

	actSearchNextPage = new QAction(searchResultContents);
	actSearchNextPage->setIcon(actionNextPoem->icon());
	searchNextPage->setDefaultAction(actSearchNextPage);

	connect(searchNextPage, SIGNAL(triggered(QAction *)), this, SLOT(searchPageNavigationClicked(QAction *)));

	searchNextPage->setEnabled(false);
	searchNextPage->hide();
    
	searchNavVerticalLayout->addWidget(searchNextPage);

	searchPreviousPage = new QToolButton(searchResultContents);
	searchPreviousPage->setObjectName(QString::fromUtf8("searchPreviousPage"));
	
	actSearchPreviousPage = new QAction(searchResultContents);
	actSearchPreviousPage->setIcon(actionPreviousPoem->icon());
	searchPreviousPage->setDefaultAction(actSearchPreviousPage);

	connect(searchPreviousPage, SIGNAL(triggered(QAction *)), this, SLOT(searchPageNavigationClicked(QAction *)));

	searchPreviousPage->setEnabled(false);
	searchPreviousPage->hide();

	searchNavVerticalLayout->addWidget(searchPreviousPage);
	
	if (navButtonsNeeded)
	{//almost one of navigation button needs to be enabled
		searchNextPage->show();
		searchPreviousPage->show();
	}

	QSpacerItem *searchNavVerticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

	searchNavVerticalLayout->addItem(searchNavVerticalSpacer);

	horizontalLayout->addLayout(searchNavVerticalLayout);
	horizontalLayout->addLayout(searchTableGridLayout);

	searchGridLayout->addLayout(horizontalLayout, 0, 0, 1, 1);

	if (!fromOtherPage)
	{
		searchResultWidget->setWidget(searchResultContents);
		this->addDockWidget(static_cast<Qt::DockWidgetArea>(8), searchResultWidget);
		QList<QDockWidget *> tabifiedSearchResults = tabifiedDockWidgets( ui->searchWidget );
		
		tabifyDockWidget(ui->searchWidget, searchResultWidget);

		searchResultWidget->show();
		searchResultWidget->raise();
	}

	searchTable->setColumnWidth(0, searchTable->fontMetrics().boundingRect(QString::number((PageStart+count)*100)).width());
	int maxPoemWidth = 0, maxVerseWidth = 0;

    for (int i = 0; i < count; ++i)
    {
		GanjoorPoem poem = SaagharWidget::ganjoorDataBase->getPoem(poemIDsList.at(i));

        poem._HighlightText = phrase;
		
		//we need a modified verion of showParentCategory
		//showParentCategory(SaagharWidget::ganjoorDataBase->getCategory(poem._CatID));
		//adding Numbers
		QString localizedNumber = SaagharWidget::persianIranLocal.toString(PageStart+i+1);
		QTableWidgetItem *numItem = new QTableWidgetItem(localizedNumber);
		numItem->setFlags(Qt::NoItemFlags /*Qt::ItemIsEnabled*/);

		searchTable->setItem(i, 0, numItem);

		//add Items
		QTableWidgetItem *poemItem = new QTableWidgetItem(poem._Title);
		poemItem->setFlags(Qt::ItemIsEnabled);
		poemItem->setData(Qt::UserRole, "PoemID="+QString::number(poem._ID));

		int tmpWidth = searchTable->fontMetrics().boundingRect(poem._Title).width();
		if ( tmpWidth > maxPoemWidth )
			maxPoemWidth = tmpWidth;

        QString firstVerse = SaagharWidget::ganjoorDataBase->getFirstVerseContainingPhrase(poem._ID, phrase);

		QTableWidgetItem *verseItem = new QTableWidgetItem(firstVerse);
		verseItem->setFlags(Qt::ItemIsEnabled/*Qt::NoItemFlags*/);
		verseItem->setData(Qt::UserRole, "PoemID="+QString::number(poem._ID));
		//set search data
		verseItem->setData(ITEM_SEARCH_DATA, phrase);
		poemItem->setData(ITEM_SEARCH_DATA, phrase);

		//insert items to table
		searchTable->setItem(i, 1, poemItem);
		searchTable->setItem(i, 2, verseItem);

		tmpWidth = searchTable->fontMetrics().boundingRect(firstVerse).width();
		if ( tmpWidth > maxVerseWidth )
			maxVerseWidth = tmpWidth;
    }

	searchTable->setColumnWidth(1, maxPoemWidth+searchTable->fontMetrics().boundingRect("00").width()  );
	searchTable->setColumnWidth(2, maxVerseWidth+searchTable->fontMetrics().boundingRect("00").width() );

	if (PageStart > 0)
	{
		searchPreviousPage->setEnabled(true);
		actSearchPreviousPage->setData("actSearchPreviousPage|"+phrase+"|"+QString::number(PageStart - tmpCount)+"|"+QString::number(tmpCount)+"|"+QString::number(PoetID));
	}

	if (HasMore)
	{
		searchNextPage->setEnabled(true);
		actSearchNextPage->setData("actSearchNextPage|"+phrase+"|"+QString::number(PageStart + count)+"|"+QString::number(count)+"|"+QString::number(PoetID));
	}

    QApplication::restoreOverrideCursor();
}

void MainWindow::tableItemPress(QTableWidgetItem *)
{
	pressedMouseButton = QApplication::mouseButtons();
}

void MainWindow::searchPageNavigationClicked(QAction *action)
{
	QWidget *actParent = action->parentWidget();

	QVariant actionData = action->data();
	if ( !actionData.isValid() || actionData.isNull() ) return;
	QStringList dataList = actionData.toString().split("|", QString::SkipEmptyParts);
	if (actParent)
		showSearchResults(dataList.at(1), dataList.at(2).toInt(), dataList.at(3).toInt(), dataList.at(4).toInt(), actParent);
	else	
		showSearchResults(dataList.at(1), dataList.at(2).toInt(), dataList.at(3).toInt(), dataList.at(4).toInt(), 0);
}

void MainWindow::tableItemClick(QTableWidgetItem *item)
{
	QTableWidget *senderTable = item->tableWidget();
	if (!saagharWidget || !senderTable)	return;

	disconnect(senderTable, SIGNAL(itemClicked(QTableWidgetItem *)), 0, 0);
	QStringList itemData = item->data(Qt::UserRole).toString().split("=", QString::SkipEmptyParts);
	
	//qDebug() << "itemData=" << item->data(Qt::UserRole).toString() << "-itemData-Size=" << itemData.size();

	if (itemData.size()!=2 || itemData.at(0) == "VerseData" )
	{
		connect(senderTable, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(tableItemClick(QTableWidgetItem *)));
		return;
	}

	//search data
	QVariant searchData = item->data(ITEM_SEARCH_DATA);
	QStringList searchDataList = QStringList();
	bool setupSaagharItemDelegate = false;
	if (searchData.isValid() && !searchData.isNull())
	{
		searchDataList = searchData.toString().split("|", QString::SkipEmptyParts);
		if (!searchDataList.at(0).isEmpty())
			setupSaagharItemDelegate = true;
	}

	bool OK = false;
	int idData = itemData.at(1).toInt(&OK);
	bool noError = false;

	if (OK && SaagharWidget::ganjoorDataBase)
		noError = true;
	//right click event
	if (pressedMouseButton == Qt::RightButton)
	{
		//qDebug() << "tableItemClick-Qt::RightButton";

		newTabForItem(itemData.at(0), idData, noError);
		if (setupSaagharItemDelegate)
		{
			saagharWidget->scrollToFirstItemContains(searchDataList.at(0));
			saagharWidget->tableViewWidget->setItemDelegate(new SaagharItemDelegate(saagharWidget->tableViewWidget, saagharWidget->tableViewWidget->style(), searchDataList.at(0)));
		}
		connect(senderTable, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(tableItemClick(QTableWidgetItem *)));
		return;
	}

	saagharWidget->processClickedItem(itemData.at(0), idData, noError);
	if (setupSaagharItemDelegate)
	{
		saagharWidget->scrollToFirstItemContains(searchDataList.at(0));
		saagharWidget->tableViewWidget->setItemDelegate(new SaagharItemDelegate(saagharWidget->tableViewWidget, saagharWidget->tableViewWidget->style(), searchDataList.at(0)));
	}

	connect(senderTable, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(tableItemClick(QTableWidgetItem *)));
}
