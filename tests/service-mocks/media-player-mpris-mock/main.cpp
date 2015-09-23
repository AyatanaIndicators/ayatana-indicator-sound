/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <string>

#include "MediaPlayerMprisDefs.h"
#include "MediaPlayerMprisMock.h"
#include "MediaPlayerMprisMockAdaptor.h"
#include "MediaPlayer2MockAdaptor.h"

using namespace ubuntu::indicators::testing;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        qWarning() << "usage: " << argv[0] << "TEST_PLAYER_NAME";
        return 1;
    }
    QString playerName = QString(argv[1]);

    QCoreApplication app(argc, argv);
    QString playerService = QString("org.mpris.MediaPlayer2.") + playerName;
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.interface()->isServiceRegistered(playerService))
    {
        auto service = new MediaPlayerMprisMock(playerName, &app);
        new PlayerAdaptor(service);
        new MediaPlayer2Adaptor(service);

        if (!connection.registerService(playerService))
        {
            qDebug() << connection.lastError();
            qFatal("Could not register MediaPlayerMprisMock service.");
        }

        if (!connection.registerObject("/org/mpris/MediaPlayer2", service))
        {
            qFatal("Could not register MediaPlayerMprisMock object.");
        }
    }
    else
    {
        qDebug() << "Service is already registered!.";
    }
    return app.exec();
}
