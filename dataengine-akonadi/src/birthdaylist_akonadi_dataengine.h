#ifndef BIRTHDAYLIST_AKONADI_DATAENGINE_H
#define BIRTHDAYLIST_AKONADI_DATAENGINE_H

/**
 * @file    birthdaylist_akonadi_dataengine.h
 * @author  Karol Slanina
 * @version 0.6.0
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

namespace Akonadi {
    class Monitor;
    class Collection;
};

/**
 * Data engine that provides the contact details (names, birthdays, namedays, anniversaries)
 * from the selected Akonadi collection.
 */
class BirthdayListAkonadiDataEngine : public Plasma::DataEngine {
    Q_OBJECT

public:
    BirthdayListAkonadiDataEngine(QObject* parent, const QVariantList& args);
    ~BirthdayListAkonadiDataEngine();

    /**
     * Returns the default list of sources provided by this dataengine:
     * - Collections: Provides the list of all relevant Akonadi collections
     * - ContactInfo: Provides contact details from the KDE Address Book
     *
     * Not provided:
     * - SetNamedayField_<string>: Informs the dataengine where to search for namedays in the current Akonadi collection.
     * - SetAnniversaryField_<string>: Informs the dataengine where to search for anniversaries in the current Akonadi collection.
     */
    QStringList sources() const;

protected:
    /** Initially prepares the given data source. */
    bool sourceRequestEvent(const QString& name);

    /** Updates the given data source. */
    bool updateSourceEvent(const QString& source);

private slots:
    /** Starts Akonadi server after the initialization of the data engine. */
    void initAkonadi();
    /** Receives the Akonadi collection update signals. */
    void slotCollectionChanged();
    /** Receives notification when Akonadi is started. */
    void slotAkonadiStarted();
    /** Receives notification when Akonadi is stopped. */
    void slotAkonadiStopped();

private:
    /** Updates the datasource for the list of all relevant Akonadi collections. */
    bool updateCollectionList();

    /** Sets the current Akonadi collection. */
    void setCurrentCollection(QString resource);

    /** Sets the date field where anniversaries can be found in the current Akonadi collection. */
    void setAnniversaryField(QString anniversaryField);

    /** Sets the date field where namedays can be found in the current Akonadi collection. */
    void setNamedayField(QString namedayField);

    /** Updates the datasource for the contact information from the current Akonadi collection. */
    bool updateContactInfo();


    Akonadi::Monitor *m_akonadiMonitor;
    QHash<QString, const Akonadi::Collection *> m_collectionMap;

    QString m_currentCollectionResource;
    QString m_anniversaryField;
    QString m_namedayField;

    static const QString source_akonadiCollections;
    static const QString source_contactInfo;
    static const QString source_setCurrentCollectionPrefix;
    static const QString source_setNamedayFieldPrefix;
    static const QString source_setAnniversaryFieldPrefix;
};


#endif //BIRTHDAYLIST_AKONADI_DATAENGINE_H
