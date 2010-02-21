#ifndef KABCENGINE_H
#define KABCENGINE_H

/* ************************************************
 *  @name: kabcengine.h
 *  @author: Meinhard Ritscher
 *  @date: 2008-10-21
 *
 *  $Id:  $
 *
 *  Description
 *  ============
 *  A data engine providing a list of birthdays
 *  and a list of anninversaries from the standard
 *  KDE address book.
 *  Code from Jan Hambrecht from the kicker applet
 *  KBirthday has been used.
 *
 *  Histrory
 *  ============
 *
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
#include <kabc/stdaddressbook.h>

// This enables us to use a QPair as an QMetatype
typedef QPair<QString, QDate> KabEntry;
typedef QList<QVariant> KabEventList;

class AddressBook;

class KabcEngine : public Plasma::DataEngine
{
    Q_OBJECT

    public:
        KabcEngine(QObject* parent, const QVariantList& args);
        ~KabcEngine();

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
        void slotAddressbookChanged(AddressBook*);

    private:
        void updateEventList(const QString &name);
        /** 
         * @brief helper function for getting an anniversary date. If none exists 
         */
        QDate getAnniversary( const KABC::Addressee &addressee );

private: // Private attributes

    /** @brief List of all found birthday events. */
        KabEventList* m_pBirthdayList;

    /** @brief List of all found anniversary events. */
        KabEventList* m_pAnniversaryList;

    /** @brief A pointer to kde addressbook. */
        KABC::AddressBook *m_pAddressbook;
};

K_EXPORT_PLASMA_DATAENGINE(kabc, KabcEngine);

#endif
