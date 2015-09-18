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

#include "AccountsDefs.h"
#include "AccountsServiceSoundMock.h"
#include "AccountsServiceSoundMockAdaptor.h"
#include "AccountsMock.h"
#include "AccountsMockAdaptor.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QDBusConnection connection = QDBusConnection::systemBus();
    if (!connection.interface()->isServiceRegistered(ACCOUNTS_SERVICE))
    {
        auto service = new AccountsServiceSoundMock(&app);
        new SoundAdaptor(service);

        auto accounts_service = new AccountsMock(&app);
        new AccountsAdaptor(accounts_service);

        if (!connection.registerService(ACCOUNTS_SERVICE))
        {
            qFatal("Could not register AccountsService Volume service.");
        }

        if (!connection.registerObject(USER_PATH, service))
        {
            qFatal("Could not register AccountsService Volume object.");
        }

        if (!connection.registerObject(ACCOUNTS_PATH, accounts_service))
        {
            qFatal("Could not register Accounts object.");
        }
    }
    else
    {
        qDebug() << "Service is already registered!.";
    }
    return app.exec();
}
