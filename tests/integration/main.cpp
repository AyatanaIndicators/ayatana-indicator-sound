/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Pete Woods <pete.woods@canonical.com>
 */

//#include <config.h>

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTimer>
#include <gtest/gtest.h>

#include <libqtdbusmock/DBusMock.h>

#include "dbus-types.h"

using namespace QtDBusMock;

class Runner: public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void run()
    {
        QCoreApplication::exit(RUN_ALL_TESTS());
    }
};

int main(int argc, char **argv)
{
    qputenv("LANG", "C.UTF-8");
    unsetenv("LC_ALL");

    /*
     * A couple of things rely on having a HOME directory:
     *   - The indicator itself relies on having a writable gsettings/dconf
     *     database, and the test relies on the functionality it provides.
     *   - The test starts Pulseaudio, which requires both a runtime and a
     *     state directory, both of which has a failback to the HOME.
     * Provide a temporary HOME for the test and its child, which both prevents
     * polluting the building user's HOME and allow the test to run where
     * HOME=/nonexistent (i.e. autobuilder).
     */
    QTemporaryDir tmpHome;
    if (!tmpHome.isValid())
        qFatal("Cannot create a temporary HOME for the test.");

    setenv("HOME", tmpHome.path().toLocal8Bit().constData(), true);

    QCoreApplication application(argc, argv);
    DBusMock::registerMetaTypes();
    DBusTypes::registerMetaTypes();
    ::testing::InitGoogleTest(&argc, argv);

    Runner runner;
    QTimer::singleShot(0, &runner, SLOT(run()));

    return application.exec();
}

#include "main.moc"
