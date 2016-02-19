/*
 * TrackYourTime - cross-platform time tracker
 * Copyright (C) 2015-2016  Alexander Basov <basovav@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui/settingswindow.h"
#include "ui/statisticwindow.h"
#include "ui/applicationswindow.h"
#include "ui/profileswindow.h"
#include "ui/aboutwindow.h"
#include "ui/app_settingswindow.h"
#include "ui/schedulewindow.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QThread>
#include <QTimer>
#include "tools/tools.h"
#include <QTranslator>
#include <QSystemTrayIcon>
#include "data/cdatamanager.h"
#include "data/cschedule.h"
#include "ui/ctrayicon.h"
#include "ui/notificationwindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("SRFGames");
    QCoreApplication::setOrganizationDomain("sol-online.org"),
    QCoreApplication::setApplicationName("TrackYourTime");

#ifdef Q_OS_MAC
    QDir dir(argv[0]);
    dir.cdUp();
    QString currentDir = dir.absolutePath();
    dir.cdUp();
    dir.cd("PlugIns");
    QCoreApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif

    QApplication a(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);

#ifdef Q_OS_MAC
    QDir::setCurrent(currentDir);
#endif



    cSettings settings;
    QString Language = QLocale::system().name();
    Language.truncate(Language.lastIndexOf('_'));
    Language = settings.db()->value(cDataManager::CONF_LANGUAGE_ID,Language).toString();
    if (settings.db()->value(cDataManager::CONF_FIRST_LAUNCH_ID,true).toBool()){
        settings.db()->setValue(cDataManager::CONF_FIRST_LAUNCH_ID,false);
        settings.db()->setValue(cDataManager::CONF_LANGUAGE_ID,Language);
        settings.db()->setValue(cDataManager::CONF_AUTORUN_ID,true);
        setAutorun();
        settings.db()->sync();
    }

    QTranslator translator;
    translator.load("lang_" + Language,QDir::currentPath()+"/data/languages");
    QApplication::installTranslator(&translator);

    cDataManager datamanager;
    cSchedule schedule(&datamanager);

    cTrayIcon trIcon(&datamanager);
    QObject::connect(&datamanager, SIGNAL(trayActive()), &trIcon, SLOT(setActive()));
    QObject::connect(&datamanager, SIGNAL(traySleep()), &trIcon, SLOT(setInactive()));
    QObject::connect(&datamanager, SIGNAL(trayShowHint(QString)), &trIcon, SLOT(showHint(QString)));
    QObject::connect(&datamanager, SIGNAL(profilesChanged()), &trIcon, SLOT(onProfilesChange()));

    ApplicationsWindow applicationsWindow(&datamanager);
    QObject::connect(&trIcon, SIGNAL(showApplications()), &applicationsWindow, SLOT(show()));
    QObject::connect(&datamanager, SIGNAL(profilesChanged()), &applicationsWindow, SLOT(onProfilesChange()));
    QObject::connect(&datamanager, SIGNAL(applicationsChanged()), &applicationsWindow, SLOT(onApplicationsChange()));

    ProfilesWindow profilesWindow(&datamanager);
    QObject::connect(&applicationsWindow, SIGNAL(showProfiles()), &profilesWindow, SLOT(show()));

    App_SettingsWindow app_settingsWindow(&datamanager);
    QObject::connect(&applicationsWindow, SIGNAL(showAppSettings(int)), &app_settingsWindow, SLOT(showApp(int)));
    QObject::connect(&datamanager, SIGNAL(debugScriptResult(QString,sSysInfo)), &app_settingsWindow, SLOT(onScriptResult(QString,sSysInfo)));

    SettingsWindow settingsWindow(&datamanager);
    QObject::connect(&trIcon, SIGNAL(showSettings()), &settingsWindow, SLOT(show()));
    QObject::connect(&settingsWindow, SIGNAL(preferencesChange()), &datamanager, SLOT(onPreferencesChanged()));

    ScheduleWindow scheduleWindow(&datamanager,&schedule);
    QObject::connect(&datamanager, SIGNAL(profilesChanged()), &scheduleWindow, SLOT(rebuild()));
    QObject::connect(&trIcon, SIGNAL(showSchedule()), &scheduleWindow, SLOT(show()));

    StatisticWindow statisticWindow(&datamanager);
    QObject::connect(&trIcon, SIGNAL(showStatistic()), &statisticWindow, SLOT(show()));

    AboutWindow aboutWindow;
    QObject::connect(&trIcon, SIGNAL(showAbout()), &aboutWindow, SLOT(show()));

    NotificationWindow notificationWindow(&datamanager);
    QObject::connect(&settingsWindow, SIGNAL(preferencesChange()), &notificationWindow, SLOT(onPreferencesChanged()));
    QObject::connect(&datamanager, SIGNAL(showNotification()), &notificationWindow, SLOT(onShow()));

    schedule.start();

    int result = a.exec();

    return result;
}
