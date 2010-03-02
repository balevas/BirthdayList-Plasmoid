#ifndef BIRTHDAYLIST_DATAENGINE_H
#define BIRTHDAYLIST_DATAENGINE_H

/* ************************************************
 *  @name: birthdaylist_dataengine.cpp
 *  @author: Karol Slanina
 * ********************************************** */

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 or (at your option)    *
 *   any later version.                                                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   General Public Licencse for more details.                             *
 *                                                                         *
 *   A copy of the license should be part of the package. If not you can   *
 *   find it here: http://www.gnu.org/licenses/gpl.html                    *
 *                                                                         *
 ***************************************************************************/


#include <Plasma/DataEngine>
namespace KABC {
    class Addressee;
};

class BirthdayListDataEngine : public Plasma::DataEngine
{
    Q_OBJECT

    public:
        BirthdayListDataEngine(QObject* parent, const QVariantList& args);
        ~BirthdayListDataEngine();

        /**
        * @brief overloaded function List of names of the resources provided
        */
        QStringList sources() const;

    protected:
        /**
        * @brief overloaded function. Initially preparing the resources
        * @param name the name of the requested resource
        */
        bool sourceRequestEvent(const QString& name);

        /**
        * @brief overloaded function. Update the data of a resource
        * @param source the name of the resource to update
        */
        bool updateSourceEvent(const QString& source);

    private slots:
        void slotAddressbookChanged();

    private:
        bool updateNamedayLists();
        bool updateNamedayList(QString langCode);

        void setKabcAnniversaryField(QString kabcAnniversaryField);
        void setKabcNamedayField(QString kabcNamedayField);
        bool updateKabcContactInfo();
        QString getAddresseeName(KABC::Addressee &kabcAddressee);

        QString kabcAnniversaryField;
        QString kabcNamedayField;

        static const QString source_kabcContactInfo;
        static const QString source_kabcNamedayStringPrefix;
        static const QString source_kabcAnniversaryStringPrefix;
        static const QString source_namedayLists;
        static const QString source_namedayListPrefix;
};


#endif //BIRTHDAYLIST_DATAENGINE_H
