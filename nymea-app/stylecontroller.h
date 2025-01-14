/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, GNU version 3. This project is distributed in the hope that it
* will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef STYLECONTROLLER_H
#define STYLECONTROLLER_H

#include <QObject>

class StyleController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentStyle READ currentStyle WRITE setCurrentStyle NOTIFY currentStyleChanged)
    Q_PROPERTY(QStringList allStyles READ allStyles CONSTANT)
    Q_PROPERTY(bool locked READ locked CONSTANT)

public:
    explicit StyleController(const QString &defaultStyle, QObject *parent = nullptr);

    QString currentStyle() const;
    void setCurrentStyle(const QString &currentStyle);
    void lockToStyle(const QString &style);

    QStringList allStyles() const;
    bool locked() const;

    Q_INVOKABLE void setSystemFont(const QFont &font);

signals:
    void currentStyleChanged();

private:
    QString m_defaultStyle;
    bool m_locked = false;
};

#endif // STYLECONTROLLER_H
