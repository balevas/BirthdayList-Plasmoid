#ifndef BIRTHDAYLIST_CONFIGHELPER_H
#define BIRTHDAYLIST_CONFIGHELPER_H

/**
 * @file    birthdaylist_confighelper.h
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


#include "ui_birthdaylist_config_contacts.h"
#include "ui_birthdaylist_config_events.h"
#include "ui_birthdaylist_config_table.h"
#include "ui_birthdaylist_config_colors.h"
#include "birthdaylist_aboutdata.h"
#include <QStringList>

namespace BirthdayList {
    class Model;
    class ModelConfiguration;
    class ViewConfiguration;
};
class KConfigDialog;
class KConfigGroup;


namespace BirthdayList
{
    class ConfigHelper : public QObject
    {
        Q_OBJECT
    public:
        ConfigHelper();
        ~ConfigHelper();
        
        void loadConfiguration(const KConfigGroup &configGroup, ModelConfiguration &modelConf, ViewConfiguration &viewConf);
        void storeConfiguration(KConfigGroup &configGroup, const ModelConfiguration &modelConf, const ViewConfiguration &viewConf);
        
        void createConfigurationUI(KConfigDialog *parent, Model *model, const ModelConfiguration &modelConf, const ViewConfiguration &viewConf);
        void updateConfigurationFromUI(ModelConfiguration &modelConf, ViewConfiguration &viewConf);
        
    private:
        void readAvailableNamedayLists();
        
        Ui::BirthdayListContactsConfig m_ui_contacts;
        Ui::BirthdayListEventsConfig m_ui_events;
        Ui::BirthdayListTableConfig m_ui_table;
        Ui::BirthdayListColorsConfig m_ui_colors;

        QStringList m_possibleDateFormats;
        int m_selectedDateFormat;
        
        QList<QString> m_namedayFiles;
        QList<QString> m_namedayLangStrings;
        
    private slots:
        /** Enables/disables some widgets in the configuration UI based on the current datasource selection */
        void dataSourceChanged(const QString &name);
        void namedayIdentificationChanged();
    };
};


#endif //BIRTHDAYLIST_CONFIGHELPER_H