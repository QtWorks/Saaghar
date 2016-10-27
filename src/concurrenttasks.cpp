/***************************************************************************
 *  This file is part of Saaghar, a Persian poetry software                *
 *                                                                         *
 *  Copyright (C) 2010-2016 by S. Razi Alavizadeh                          *
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

#include "concurrenttasks.h"
#include "databasebrowser.h"
#include "tools.h"
#include "progressmanager.h"
#include "saagharapplication.h"
#include "futureprogress.h"

#include <QMetaType>
#include <QNetworkReply>
#include <QThread>
#include <QThreadPool>

#ifdef SAAGHAR_DEBUG
#include <QDateTime>
#endif

#define TASK_CANCELED if (isCanceled()) return QVariant();


#ifdef Q_OS_WIN
#include "qt_windows.h"

static void msleep(unsigned long msecs)
{
    ::Sleep(msecs);
}
#else
#include <sys/time.h>

static void thread_sleep(struct timespec* ti)
{
    pthread_mutex_t mtx;
    pthread_cond_t cnd;

    pthread_mutex_init(&mtx, 0);
    pthread_cond_init(&cnd, 0);

    pthread_mutex_lock(&mtx);
    (void) pthread_cond_timedwait(&cnd, &mtx, ti);
    pthread_mutex_unlock(&mtx);

    pthread_cond_destroy(&cnd);
    pthread_mutex_destroy(&mtx);
}

static void msleep(unsigned long msecs)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    struct timespec ti;

    ti.tv_nsec = (tv.tv_usec + (msecs % 1000) * 1000) * 1000;
    ti.tv_sec = tv.tv_sec + (msecs / 1000) + (ti.tv_nsec / 1000000000);
    ti.tv_nsec %= 1000000000;
    thread_sleep(&ti);
}
#endif

ConcurrentTask::ConcurrentTask(QObject* parent)
    : QObject(parent),
      QRunnable(),
      m_cancel(false),
      m_progressObject(0),
      m_displayFullNotification(sApp->displayFullNotification() && sApp->notificationPosition() != ProgressManager::Disabled),
      m_futureProgress(0),
      m_isQueued(false)
{
    // deleted by parent/child system of Qt
    setAutoDelete(false);
    qRegisterMetaType< SearchResults >("SearchResults");
}

ConcurrentTask::~ConcurrentTask()
{
}

void ConcurrentTask::start(const QString &type, const QVariantHash &argumants, bool queued)
{
    if (type != "SEARCH" && type != "UPDATE") {
        return;
    }

    if (ConcurrentTaskManager::instance()->isAllTaskCanceled()) {
        return;
    }

    m_type = type;
    m_options = argumants;
    m_isQueued = queued;

    m_shouldPrependTask = type == "UPDATE" && VAR_GET(m_options, checkByUser).toBool();

    if (sApp->notificationPosition() != ProgressManager::Disabled) {
        m_progressObject = new QFutureInterface<void>;

        ProgressManager::ProgressFlags progressFlags = m_shouldPrependTask
                ? (ProgressManager::ShowInApplicationIcon | ProgressManager::PrependInsteadAppend)
                : ProgressManager::ShowInApplicationIcon;

        m_futureProgress = sApp->progressManager()->addTimedTask(*m_progressObject,
                           VAR_GET(m_options, taskTitle).toString(),
                           m_type, 5,
                           progressFlags);

        connect(m_futureProgress, SIGNAL(canceled()), this, SLOT(setCanceled()));
    }

    ConcurrentTaskManager::instance()->addConcurrentTask(this);

    if (!m_isQueued) {
        sApp->tasksThreadPool()->start(this, (m_shouldPrependTask ? 1 : 0));
    }
}

#ifdef SAAGHAR_DEBUG
void ConcurrentTask::directStart(const QString &type, const QVariantHash &argumants)
{
    m_type = type;
    m_options = argumants;
    qint64 start = QDateTime::currentMSecsSinceEpoch();

    QVariant result = startSearch(m_options);

    qint64 end = QDateTime::currentMSecsSinceEpoch();
    qDebug() << __LINE__ << __FUNCTION__ << (end - start) << "NON THREADED" ;

    emit concurrentResultReady(m_type, result);
}
#endif

void ConcurrentTask::run()
{
    const qint64 start = QDateTime::currentMSecsSinceEpoch();

    QThread::Priority prio = QThread::currentThread()->priority();
    prio = prio == QThread::InheritPriority ? QThread::NormalPriority : prio;
    sApp->setPriority(QThread::currentThread());

    if (m_progressObject) {
        m_progressObject->reportStarted();
    }

    QVariant result;

    if (m_type == "SEARCH") {
        result = startSearch(m_options);
    }
    else if (m_type == "UPDATE") {
        result = checkForUpdates();
    }

    if (m_progressObject) {
        m_progressObject->reportFinished();
        delete m_progressObject;
        m_progressObject = 0;
    }

    QThread::currentThread()->setPriority(prio);

    const int duration = QDateTime::currentMSecsSinceEpoch() - start;
#ifdef SAAGHAR_DEBUG
    qDebug() << __LINE__ << __FUNCTION__ << duration;
#endif

    emit concurrentResultReady(m_type, result);


    if (duration < 20) {
        ::msleep(20 - duration);
    }
    else if (duration < 50) {
        ::msleep(20);
    }
    else if (duration < 100) {
        ::msleep(10);
    }
}

void ConcurrentTask::startQueued()
{
    if (m_isQueued) {
        m_isQueued = false;
        sApp->tasksThreadPool()->start(this, (m_shouldPrependTask ? 1 : 0));
        sApp->processEvents();
    }
}

QVariant ConcurrentTask::startSearch(const QVariantHash &options)
{
    TASK_CANCELED;

    const QString &theConnectionID = VAR_GET(options, connectionID).toString();
    const QString &connectionID = sApp->databaseBrowser()->getIdForDataBase(sApp->databaseBrowser()->databaseFileFromID(theConnectionID), QThread::currentThread());
    const QString &strQuery = VAR_GET(options, strQuery).toString();
    int PoetID = VAR_GET(options, PoetID).toInt();
    const QStringList &phraseList = VAR_GET(options, phraseList).toStringList();
    const QStringList &excludedList = VAR_GET(options, excludedList).toStringList();
    const QStringList &excludeWhenCleaning = VAR_GET(options, excludeWhenCleaning).toStringList();

    SearchResults searchResults;

    QSqlDatabase threadDatabase = sApp->databaseBrowser()->database(connectionID);
    if (!threadDatabase.isOpen()) {
        qDebug() << QString("ConcurrentTask::startSearch: A database for thread %1 could not be opened!").arg(QString::number((quintptr)QThread::currentThread()));
        return QVariant();
    }

    int andedPhraseCount = phraseList.size();
    int excludedCount = excludedList.size();
    int numOfFounded = 0;

    TASK_CANCELED;

    QSqlQuery q(threadDatabase);

#ifdef SAAGHAR_DEBUG
    int start = QDateTime::currentDateTime().toTime_t() * 1000 + QDateTime::currentDateTime().time().msec();
#endif

    q.exec(strQuery);

#ifdef SAAGHAR_DEBUG
    int end = QDateTime::currentDateTime().toTime_t() * 1000 + QDateTime::currentDateTime().time().msec();
    int miliSec = end - start;
    qDebug() << "duration=" << miliSec;
#endif

    int numOfNearResult = 0;
    int nextStep = 0;
    const int updateLenght = 217;

    int lastPoemID = -1;
    QList<GanjoorVerse*> verses;

    TASK_CANCELED

    while (q.next()) {
        TASK_CANCELED

        ++numOfNearResult;

        QSqlRecord qrec = q.record();
        int poemID = qrec.value(0).toInt();

//          if (idList.contains(poemID))
//              continue;//we need just first result

        QString verseText = qrec.value(1).toString();

        // assume title's order is zero!
        int verseOrder = (PoetID == -1000 ? 0 : qrec.value(2).toInt());

        QString foundVerse = Tools::cleanStringFast(verseText, excludeWhenCleaning);

        // for whole word option when word is in the start or end of verse
        foundVerse = " " + foundVerse + " ";

        //excluded list
        bool excludeCurrentVerse = false;
        for (int t = 0; t < excludedCount; ++t) {
            if (foundVerse.contains(excludedList.at(t))) {
                excludeCurrentVerse = true;
                break;
            }
        }

        if (!excludeCurrentVerse) {
            for (int t = 0; t < andedPhraseCount; ++t) {
                QString tphrase = phraseList.at(t);
                if (tphrase.contains("==")) {
                    tphrase.remove("==");
                    if (lastPoemID != poemID/* && findRhyme*/) {
                        lastPoemID = poemID;
                        int versesSize = verses.size();
                        for (int j = 0; j < versesSize; ++j) {
                            delete verses[j];
                            verses[j] = 0;
                        }

                        TASK_CANCELED;

                        verses = sApp->databaseBrowser()->getVerses(poemID, connectionID);
                    }

                    TASK_CANCELED;

                    excludeCurrentVerse = !sApp->databaseBrowser()->isRadif(verses, tphrase, verseOrder);
                    break;
                }
                if (tphrase.contains("=")) {
                    tphrase.remove("=");
                    if (lastPoemID != poemID/* && findRhyme*/) {
                        lastPoemID = poemID;
                        int versesSize = verses.size();
                        for (int j = 0; j < versesSize; ++j) {
                            delete verses[j];
                            verses[j] = 0;
                        }

                        TASK_CANCELED;

                        verses = sApp->databaseBrowser()->getVerses(poemID, connectionID);
                    }

                    TASK_CANCELED;

                    excludeCurrentVerse = !sApp->databaseBrowser()->isRhyme(verses, tphrase, verseOrder);
                    break;
                }
                if (!tphrase.contains("%")) {
                    //QChar(71,6): Simple He
                    //QChar(204,6): Persian Ye
                    QString YeAsKasre = QString(QChar(71, 6)) + " ";
                    if (tphrase.contains(YeAsKasre)) {
                        tphrase.replace(YeAsKasre, QString(QChar(71, 6)) + "\\s*" + QString(QChar(204, 6)) +
                                        "{0,2}\\s+");

                        QRegExp anySearch(".*" + tphrase + ".*", Qt::CaseInsensitive);

                        if (!anySearch.exactMatch(foundVerse)) {
                            excludeCurrentVerse = true;
                            break;
                        }
                    }
                    else {
                        if (!foundVerse.contains(tphrase))
                            //the verse doesn't contain an ANDed phrase
                            //maybe for ++ and +++ this should be removed
                        {
                            excludeCurrentVerse = true;
                            break;
                        }
                    }
                }
                else {
                    tphrase = tphrase.replace("%%", ".*");
                    tphrase = tphrase.replace("%", "\\S*");
                    QRegExp anySearch(".*" + tphrase + ".*", Qt::CaseInsensitive);
                    if (!anySearch.exactMatch(foundVerse)) {
                        excludeCurrentVerse = true;
                        break;
                    }
                }
            }
        }

        if (excludeCurrentVerse) {
            continue;
        }

        TASK_CANCELED;

        ++numOfFounded;

        if (m_displayFullNotification && numOfFounded > nextStep) {
            nextStep += updateLenght;
            emit searchStatusChanged(DatabaseBrowser::tr("Search Result(s): %1").arg(numOfFounded));
        }

        GanjoorPoem gPoem = sApp->databaseBrowser()->getPoem(poemID, connectionID);

        // TODO: Add connectionID to item
        searchResults.insertMulti(poemID, "verseText=" + verseText + "|poemTitle=" + gPoem._Title + "|poetName=" + sApp->databaseBrowser()->getPoetForCat(gPoem._CatID, connectionID)._Name);
    }

    //for the last result
    if (m_displayFullNotification) {
        emit searchStatusChanged(DatabaseBrowser::tr("Last-Search Result(s): %1").arg(numOfFounded));
    }

    qDeleteAll(verses);

    TASK_CANCELED;

    return QVariant::fromValue(searchResults);
}

QVariant ConcurrentTask::checkForUpdates()
{
    const QString checkByUser = VAR_GET(m_options, checkByUser).toBool()
                                ? QLatin1String("CHECK_BY_USER=TRUE") : QLatin1String("CHECK_BY_USER=FALSE");

    QEventLoop loop;

    QStringList updateInfoServers;
    updateInfoServers << "http://srazi.github.io/Saaghar/saaghar.version"
                      << "http://saaghar.sourceforge.net/saaghar.version"
                      << "http://en.saaghar.pozh.org/saaghar.version";

    QNetworkReply* reply;
    bool error = true;

    for (int i = 0; i < updateInfoServers.size(); ++i) {
        QNetworkRequest requestVersionInfo(QUrl(updateInfoServers.at(i)));
        QNetworkAccessManager* netManager = new QNetworkAccessManager();
        reply = netManager->get(requestVersionInfo);

        connect(this, SIGNAL(canceled()), &loop, SLOT(quit()));
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        if (isCanceled()) {
            loop.quit();
            break;
        }

        if (!reply->error()) {
            error = false;
            break;
        }
    }

    if (error || isCanceled()) {
        if (m_futureProgress) {
            m_futureProgress->setTitle(tr("Update: %1").arg(isCanceled() ? tr("Canceled by user") : tr("Error ocurred")));
            m_futureProgress->future().cancel();
        }

        return QStringList() << QLatin1String("ERROR=TRUE") << checkByUser << QLatin1String("DATA=");
    }

    return QStringList() << QLatin1String("ERROR=FALSE")
           << checkByUser
           << QString("DATA=%1").arg(QString::fromUtf8(reply->readAll()));
}

bool ConcurrentTask::isCanceled()
{
    return m_cancel;
}

void ConcurrentTask::setCanceled()
{
    m_mutex.lock();
    m_cancel = true;
    m_mutex.unlock();

    emit canceled();
}


ConcurrentTaskManager* ConcurrentTaskManager::s_instance = 0;

ConcurrentTaskManager* ConcurrentTaskManager::instance()
{
    if (!s_instance) {
        s_instance = new ConcurrentTaskManager(sApp);
    }

    return s_instance;
}

ConcurrentTaskManager::~ConcurrentTaskManager()
{
}

void ConcurrentTaskManager::startQueuedTasks()
{
    foreach (const TaskPointer &wp, m_tasks) {
        if (wp && wp.data()) {
            wp.data()->startQueued();
        }
    }
}

void ConcurrentTaskManager::addConcurrentTask(ConcurrentTask* task)
{
    if (m_cancel) {
        return;
    }

    m_tasks.append(TaskPointer(task));
}

void ConcurrentTaskManager::finish()
{
    m_cancel = true;

    foreach (const TaskPointer &wp, m_tasks) {
        if (wp && wp.data()) {
            wp.data()->setCanceled();
        }
    }

    sApp->tasksThreadPool()->waitForDone();
}

bool ConcurrentTaskManager::isAllTaskCanceled()
{
    return m_cancel;
}

void ConcurrentTaskManager::switchToStartState()
{
    m_cancel = false;
}

ConcurrentTaskManager::ConcurrentTaskManager(QObject* parent)
    : QObject(parent),
      m_tasks(),
      m_cancel(false)
{
    connect(sApp->progressManager(), SIGNAL(allTasksCanceled()), this, SLOT(finish()));
}
