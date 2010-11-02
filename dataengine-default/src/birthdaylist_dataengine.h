#ifndef BIRTHDAYLIST_DATAENGINE_H
#define BIRTHDAYLIST_DATAENGINE_H

/**
 * @file    birthdaylist_dataengine.h
 * @author  Karol Slanina
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 */


#include <Plasma/DataEngine>

namespace KABC {
    class Addressee;
};


/**
 * Data engine that provides the contact details (names, birthdays, namedays, anniversaries)
 * from the KDE Address Book.
 * It also provides the names from the nameday calendar for the given language.
 */
class BirthdayListDataEngine : public Plasma::DataEngine {
    Q_OBJECT

public:
    BirthdayListDataEngine(QObject* parent, const QVariantList& args);
    ~BirthdayListDataEngine();

    /**
     * Returns the default list of sources provided by this dataengine:
     * - NamedayLists: Provides information about the languages, for which the nameday calendars are available.
     * - ContactInfo: Provides contact details from the KDE Address Book
     *
     * Not provided:
     * - NamedayList_<langcode>: Provides names from the nameday calendar for the given language
     */
    QStringList sources() const;

protected:
    /** Initially prepares the given data source. */
    bool sourceRequestEvent(const QString& name);

    /** Updates the given data source. */
    bool updateSourceEvent(const QString& source);

private slots:
    /** Receives the KDE Address Book update signals. */
    void slotAddressbookChanged();

private:
    /** Updates the datasource for the list of all nameday calendars. */
    bool updateNamedayLists();

    /** Updates the datasource for the nameday calendar of the given language. */
    bool updateNamedayList(QString langCode);

    /** Updates the datasource for the KDE Address Book contact information. */
    bool updateContactInfo();


    static const QString source_namedayLists;
    static const QString source_namedayListPrefix;
    static const QString source_contactInfo;
};


#endif //BIRTHDAYLIST_DATAENGINE_H
