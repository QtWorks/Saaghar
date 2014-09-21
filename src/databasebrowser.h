/***************************************************************************
 *  This file is part of Saaghar, a Persian poetry software                *
 *                                                                         *
 *  Copyright (C) 2010-2014 by S. Razi Alavizadeh                          *
 *  E-Mail: <s.r.alavizadeh@gmail.com>, WWW: <http://pozh.org>             *
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

#ifndef DATABASEBROWSER_H
#define DATABASEBROWSER_H

#define dbBrowser DatabaseBrowser::instance()

#include <QMap>
#include <QObject>
#include <QWidget>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QProgressDialog>
#include <QVariant>

#include "databaseupdater.h"
#include "databaseelements.h"
#include "settings.h"
#include "qtwin.h"

#include <QDebug>

#ifdef EMBEDDED_SQLITE
#include "sqlite-driver/qsql_sqlite.h"
#include "sqlite3.h"
#endif

class QTreeWidgetItem;

typedef QMap<int,QString> SearchResults;


class DatabaseBrowser : public QObject
{
    Q_OBJECT
public:
    DatabaseBrowser(QString sqliteDbCompletePath = "ganjoor.s3db");
    ~DatabaseBrowser();

    static DatabaseBrowser* instance();

    static QSqlDatabase database(const QString &connectionID = s_defaultDatabaseName, bool open = true);

    static QString defaultDatabasename();
    static void setDefaultDatabasename(const QString &databaseaName);

    bool isConnected(const QString &connectionID = s_defaultDatabaseName);
    bool isValid(QString connectionID = s_defaultDatabaseName);

    bool isRhyme(const QList<GanjoorVerse*> &verses, const QString &phrase, int verseOrder = -1);
    bool isRadif(const QList<GanjoorVerse*> &verses, const QString &phrase, int verseOrder = -1);

    QVariantList importGanjoorBookmarks();
    QString getBeyt(int poemID, int firstMesraID,  const QString &separator = "       "/*7 spaces*/);

    QList<GanjoorPoet*> getDataBasePoets(const QString fileName);
    QList<GanjoorPoet*> getConflictingPoets(const QString fileName);
    QList<GanjoorPoet*> getPoets(const QString &connectionID = s_defaultDatabaseName, bool sort = true);
    QList<GanjoorCat*> getSubCategories(int CatID);
    QList<GanjoorCat> getParentCategories(GanjoorCat Cat);
    QList<GanjoorPoem*> getPoems(int CatID);
    QList<GanjoorVerse*> getVerses(int PoemID);
    QList<GanjoorVerse*> getVerses(int PoemID, int Count);

    QString getFirstMesra(int PoemID);//just first Mesra
    GanjoorCat getCategory(int CatID);
    GanjoorPoem getPoem(int PoemID);
    GanjoorPoem getNextPoem(int PoemID, int CatID);
    GanjoorPoem getNextPoem(GanjoorPoem poem);
    GanjoorPoem getPreviousPoem(int PoemID, int CatID);
    GanjoorPoem getPreviousPoem(GanjoorPoem poem);
    GanjoorPoet getPoetForCat(int CatID);
    GanjoorPoet getPoet(int PoetID);
    GanjoorPoet getPoet(QString PoetName);
    QString getPoetDescription(int PoetID);
    QString getPoemMediaSource(int PoemID);
    void setPoemMediaSource(int PoemID, const QString &fileName);
    //Search
    //QList<int> getPoemIDsContainingPhrase(QString phrase, int PageStart, int Count, int PoetID);
    //QString getFirstVerseContainingPhrase(int PoemID, QString phrase);
    //new Search Method
    //QList<int> getPoemIDsContainingPhrase_NewMethod(const QString &phrase, int PoetID, bool skipNonAlphabet);
    //QStringList getVerseListContainingPhrase(int PoemID, const QString &phrase);
    //another new approch
    bool getPoemIDsByPhrase(int PoetID, const QStringList &phraseList, const QStringList &excludedList = QStringList(), bool* canceled = 0, bool slowSearch = false);

    //Faal
    int getRandomPoemID(int* CatID);
    void removePoetFromDataBase(int PoetID);
    bool importDataBase(const QString filename);

    QList<QTreeWidgetItem*> loadOutlineFromDataBase(int parentID = 0);

    //STATIC Variables
    static DataBaseUpdater* dbUpdater;
    static QStringList dataBasePath;

public slots:
    void addDataSets();

private:
    bool createEmptyDataBase(const QString &connectionID = s_defaultDatabaseName);
    bool poetHasSubCats(int poetID, const QString &connectionID = s_defaultDatabaseName);

    SearchResults startSearch(const QString &strQuery, const QSqlDatabase &db, int PoetID, const QStringList &phraseList,
                              const QStringList &excludedList = QStringList(), const QStringList &excludeWhenCleaning = QStringList(),
                              bool* Canceled = 0, bool slowSearch = false);

    int cachedMaxCatID, cachedMaxPoemID;
    int getNewPoemID();
    int getNewPoetID();
    int getNewCatID();
    void removeCatFromDataBase(const GanjoorCat &gCat);
    static QString getIdForDataBase(const QString &sqliteDataBaseName);

    static bool comparePoetsByName(GanjoorPoet* poet1, GanjoorPoet* poet2);
    static bool compareCategoriesByName(GanjoorCat* cat1, GanjoorCat* cat2);
    bool m_addRemoteDataSet;


    static QString s_defaultDatabaseName;

    static DatabaseBrowser* s_instance;

signals:
    void searchStatusChanged(const QString &);
    void concurrentResultReady(const QString &type, const QVariant &results);

#ifdef EMBEDDED_SQLITE
private:
    static QSQLiteDriver* sqlDriver;
#endif
};

Q_DECLARE_METATYPE(SearchResults)

#endif // DATABASEBROWSER_H