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

#ifndef SAAGHARAPPLICATION_H
#define SAAGHARAPPLICATION_H

#define sApp SaagharApplication::instance()

#include "progressmanager.h"

#include <QApplication>
#include <QStringList>
#include <QPointer>

class DatabaseBrowser;
class OutlineModel;
class SaagharWindow;
class SettingsManager;
class QExtendedSplashScreen;

class QAction;
class QSettings;
class QThread;
class QThreadPool;

class SaagharApplication : public QApplication
{
    Q_OBJECT
public:
    SaagharApplication(int &argc, char** argv);
    ~SaagharApplication();

    static SaagharApplication* instance();

    enum PathType {
        DatabaseFile,
        DatabaseDirs,
        SettingsFile,
        BookmarksFile,
        AlbumFile,
        AlbumDir,
        ResourcesDir,
        UserDataDir
    };

    QString defaultPath(PathType type);
    void setDefaultPath(PathType type, const QString &path);

    ProgressManager* progressManager();
    QThreadPool* tasksThreadPool();
    DatabaseBrowser* databaseBrowser();
    SettingsManager* settingsManager();

    // empty means DatabaseBrowser::defaultConnectionId()
    OutlineModel* outlineModel(const QString &connectionID = QString());

    int tasksThreads() { return m_tasksThreads; }
    void setPriority(QThread* thread);
    bool displayFullNotification() { return m_displayFullNotification; }
    ProgressManager::Position notificationPosition() const { return m_notificationPosition; }

    bool isPortable() const;

    QStringList mainToolBarItems();
    void setMainToolBarItems(const QStringList &items);

    void applySettings();
    void saveSettings();

    void quitSaaghar();

    QStringList quickAccessBookmarks() const;
    QAction* quickAccessCustomizeAction();

private slots:
    void customizeQuickAccessBookmarks();
    void aboutToShowMainWindow();

private:
    void init();
    void setupPaths();
    void setupDatabasePaths();
    void setupTranslators();

    void setupInitialValues();
    void loadSettings();

    QPointer<QExtendedSplashScreen> m_splash;

    SaagharWindow* m_mainWindow;

    ProgressManagerPrivate* m_progressManager;
    QThreadPool* m_tasksThreadPool;
    DatabaseBrowser* m_databaseBrowser;
    SettingsManager* m_settingsManager;
    QHash<QString, OutlineModel*> m_outlineModels;

    int m_tasksThreads;
    bool m_displayFullNotification;
    ProgressManager::Position m_notificationPosition;

    mutable int m_isPortable;

    QHash<PathType, QString> m_paths;
};

#endif // SAAGHARAPPLICATION_H
