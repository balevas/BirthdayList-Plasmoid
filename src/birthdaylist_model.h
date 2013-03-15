#ifndef BIRTHDAYLIST_MODEL_H
#define BIRTHDAYLIST_MODEL_H

/**
 * @file    birthdaylist_model.h
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


#include <QStandardItemModel>
#include <QTimer>
#include "birthdaylist_aboutdata.h"

namespace BirthdayList {
    class AbstractAnnualEventEntry;
    class Source_Collections;
    class Source_Contacts;
    class AddresseeInfo;
};


namespace BirthdayList 
{
    struct ModelConfiguration
    {
        ModelConfiguration();
        
        // TODO make KABC source deprecated
        enum EventDataSource { EDS_Akonadi /*,EDS_KABC*/ };
        EventDataSource eventDataSource;
        /** Id of the Akonadi collection to be used */
        int akonadiCollectionId;

        int eventThreshold;
        int highlightThreshold;
        int pastThreshold;

        bool showNicknames;
        bool showNamedays;
        enum NamedayDisplayMode { NDM_AggregateEvents, NDM_IndividualEvents, NDM_AllCalendarNames };
        NamedayDisplayMode namedayDisplayMode;
        bool showAnniversaries;

        bool namedayByAnniversaryDateField;
        bool namedayByCustomDateField;
        /** Name of the Akonadi collection custom field where namedays can be found (used if the anniversary field is not used)*/
        QString namedayCustomDateFieldName;
        bool namedayByGivenName;
        /** Language code of the currently used nameday calendar */
        QString curNamedayFile;

        enum FilterType { FT_Off, FT_Category, FT_CustomField, FT_CustomFieldPrefix };
        FilterType filterType;
        QString customFieldName;
        QString customFieldPrefix;
        QString filterValue;

        QString dateFormat;
        
        bool textAlignmentLeft;
        
        struct ItemColorSettings {
            ItemColorSettings(bool isForeground, QBrush brushForeground, bool isBackground, QBrush brushBackground, bool highlightNoEvents) :
                isForeground(isForeground),
                brushForeground(brushForeground),
                isBackground(isBackground),
                brushBackground(brushBackground),
                highlightNoEvents(highlightNoEvents) {}
            
            bool isForeground;
            QBrush brushForeground;
            bool isBackground;
            QBrush brushBackground;
            bool highlightNoEvents;
        };
        
        ItemColorSettings todayColorSettings;
        ItemColorSettings highlightColorSettings;
        ItemColorSettings pastColorSettings;
    };

    
    class Model : public QStandardItemModel
    {
        Q_OBJECT
    public:
        explicit Model();
        ~Model();
        
        void setConfiguration(ModelConfiguration newConf);
        ModelConfiguration getConfiguration() const;
        
        QHash<QString, int> getAkonadiCollections();
        
    private:
        QDate getContactDateField(const AddresseeInfo &contactInfo, QString fieldName);
        /** Returns the nameday date by comparing the contact's given name with the calendar entries */
        QDate getNamedayByGivenName(QString givenName);
        /** Returns the name from the current nameday calendar belonging to the given date. */
        QString getNamedayString(QDate date);
        /** Sets the colors for the model items according to the applet configuration */
        void setModelItemStyle(const AbstractAnnualEventEntry *entry, QStandardItem *item, int colNum);

        ModelConfiguration m_conf;
        
        Source_Collections *m_source_collections;
        Source_Contacts *m_source_contacts;
        
        QTimer m_midnightTimer;
        
        /** Complete event list */
        QList<AbstractAnnualEventEntry*> m_listEntries;

        /** Local copy of the currently used nameday calendar */
        QHash<QString, QString> m_curLangNamedayList;
        
        void refreshContactEvents();
        void updateModel();

    private slots:
        void contactCollectionUpdated();
        void midnightUpdate();
    };
};


#endif //BIRTHDAYLIST_MODEL_H