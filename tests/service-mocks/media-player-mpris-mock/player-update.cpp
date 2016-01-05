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
#include "MediaPlayerMprisMockInterface.h"

#include <memory>

QMap<QString, QString> getPropertiesMap()
{
    QMap<QString, QString> retMap;

    retMap["CANPLAY"] = "setCanPlay";
    retMap["CANPAUSE"] = "setCanPause";
    retMap["CANGONEXT"] = "setCanGoNext";
    retMap["CANGOPREVIOUS"] = "setCanGoPrevious";

    return retMap;
}

bool getBoolValue(QString const & str)
{
    if (str == "TRUE")
    {
        return true;
    }
    return false;
}

QTextStream& qStdErr()
{
    static QTextStream ts( stderr );
    return ts;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 4)
    {
        qStdErr() << "usage: " << argv[0] << "TEST_PLAYER_NAME  PropertyName true|false\n";
        return 1;
    }

    QString state = QString(argv[3]).toUpper();
    QString property = QString(argv[2]).toUpper();
    QString playerName = QString(argv[1]);

    auto propertiesMap = getPropertiesMap();

    std::shared_ptr<MediaPlayerMprisMockInterface> iface(
                       new MediaPlayerMprisMockInterface("org.mpris.MediaPlayer2." + playerName,
                                                         "/org/mpris/MediaPlayer2",
                                                         QDBusConnection::sessionBus()));
    if (!iface)
    {
        qWarning() << argv[0] << ": error creating interface";
        return 1;
    }

    QMap<QString, QString>::iterator iter = propertiesMap.find(property);
    if (iter == propertiesMap.end())
    {
        qWarning() << argv[0] << ": property " << property << " was not found.";
        return 1;
    }

    QDBusReply<void> set_prop = iface->call(QLatin1String((*iter).toStdString().c_str()),
                                QVariant::fromValue(getBoolValue(state)));

    if (!set_prop.isValid())
    {
        qWarning() << argv[0] << ": D-Bus error: " << set_prop.error().message();
        return 1;
    }

    return 0;
}

