/**
 * @file    birthdaylist_confighelper.cpp
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


#include "birthdaylist_confighelper.h"
#include "birthdaylist_model.h"
#include "birthdaylist_modelentry.h"
#include "birthdaylist_view.h"
#include <KConfigDialog>
#include <KConfigGroup>
#include <KStandardDirs>
#include <QFile> // needed for backward compatibility


BirthdayList::ConfigHelper::ConfigHelper() :
m_selectedDateFormat(0)
{
    m_possibleDateFormats << "d. M." << "dd. MM."
        << "d-M" << "dd-MM" << "M-d" << "MM-dd"
        << "d/M" << "dd/MM" << "M/d" << "MM/dd";
        
    readAvailableNamedayLists();
}

BirthdayList::ConfigHelper::~ConfigHelper()
{
}

void BirthdayList::ConfigHelper::loadConfiguration(const KConfigGroup &configGroup, ModelConfiguration &modelConf, ViewConfiguration &viewConf)
{
    QString eventDataSource = configGroup.readEntry("Event Data Source", "");
    /*if (eventDataSource == "Akonadi") modelConf.eventDataSource = ModelConfiguration::EDS_Akonadi;
    else modelConf.eventDataSource = ModelConfiguration::EDS_KABC;*/
    modelConf.eventDataSource = ModelConfiguration::EDS_Akonadi;

    modelConf.akonadiCollectionId = configGroup.readEntry("Akonadi Collection", -1);
    modelConf.namedayByAnniversaryDateField = configGroup.readEntry("Nameday By Anniversary Field", false);
    modelConf.namedayByCustomDateField = configGroup.readEntry("Nameday By Custom Field", false);
    modelConf.namedayCustomDateFieldName = configGroup.readEntry("Nameday Custom Field", "");
    modelConf.namedayByGivenName = configGroup.readEntry("Nameday By Given Name", false);

    modelConf.curNamedayFile = configGroup.readEntry("Nameday Calendar File", "");
    
    modelConf.showNicknames = configGroup.readEntry("Show Nicknames", true);
    
    QString filterType = configGroup.readEntry("Filter Type", "");
    if (filterType == "Category") modelConf.filterType = ModelConfiguration::FT_Category;
    else if (filterType == "Custom Field") modelConf.filterType = ModelConfiguration::FT_CustomField;
    else if (filterType == "Custom Field Prefix") modelConf.filterType = ModelConfiguration::FT_CustomFieldPrefix;
    else modelConf.filterType = ModelConfiguration::FT_Off;
    
    modelConf.customFieldName = configGroup.readEntry("Custom Field", "");
    modelConf.customFieldPrefix = configGroup.readEntry("Custom Field Prefix", "");
    modelConf.filterValue = configGroup.readEntry("Filter Value", "");
    
    viewConf.showColumnHeaders = configGroup.readEntry("Show Column Headers", true);
    //viewConf.showColName = configGroup.readEntry("Show Name Column", true);
    viewConf.showColName = true;
    viewConf.showColAge = configGroup.readEntry("Show Age Column", true);
    viewConf.showColDate = configGroup.readEntry("Show Date Column", true);
    viewConf.showColWhen = configGroup.readEntry("Show When Column", true);

    modelConf.showNamedays = configGroup.readEntry("Show Namedays", true);
    QString namedayDisplayMode = configGroup.readEntry("Nameday Display Mode", "");
    if (namedayDisplayMode == "IndividualEvents") modelConf.namedayDisplayMode = ModelConfiguration::NDM_IndividualEvents;
    else if (namedayDisplayMode == "AllCalendarNames") modelConf.namedayDisplayMode = ModelConfiguration::NDM_AllCalendarNames;
    else modelConf.namedayDisplayMode = ModelConfiguration::NDM_AggregateEvents;

    modelConf.showAnniversaries = configGroup.readEntry("Show Anniversaries", true);
    
    modelConf.todayColorSettings.isForeground = configGroup.readEntry("Todays Foreground Enabled", false);
    QColor todaysForeground = configGroup.readEntry("Todays Foreground Color", QColor(255, 255, 255));
    modelConf.todayColorSettings.brushForeground = QBrush(todaysForeground);
    modelConf.todayColorSettings.isBackground = configGroup.readEntry("Todays Background Enabled", true);
    QColor todaysBackground = configGroup.readEntry("Todays Background Color", QColor(128, 0, 0));
    modelConf.todayColorSettings.brushBackground = QBrush(todaysBackground);
    modelConf.todayColorSettings.highlightNoEvents = configGroup.readEntry("Todays Highlight No Events", true);

    modelConf.eventThreshold = configGroup.readEntry("Event Threshold", 30);
    modelConf.highlightThreshold = configGroup.readEntry("Highlight Threshold", 2);
    modelConf.highlightColorSettings.isForeground = configGroup.readEntry("Highlight Foreground Enabled", false);
    QColor highlightForeground = configGroup.readEntry("Highlight Foreground Color", QColor(255, 255, 255));
    modelConf.highlightColorSettings.brushForeground = QBrush(highlightForeground);
    modelConf.highlightColorSettings.isBackground = configGroup.readEntry("Highlight Background Enabled", true);
    QColor highlightBackground = configGroup.readEntry("Highlight Background Color", QColor(128, 0, 0));
    modelConf.highlightColorSettings.brushBackground = QBrush(highlightBackground);
    modelConf.highlightColorSettings.highlightNoEvents = configGroup.readEntry("Coming Highlight No Events", false);

    modelConf.pastThreshold = configGroup.readEntry("Past Threshold", 2);
    AbstractAnnualEventEntry::setPastThreshold(modelConf.pastThreshold);
    modelConf.pastColorSettings.isForeground = configGroup.readEntry("Past Foreground Enabled", false);
    QColor pastForeground = configGroup.readEntry("Past Foreground Color", QColor(0, 0, 0));
    modelConf.pastColorSettings.brushForeground = QBrush(pastForeground);
    modelConf.pastColorSettings.isBackground = configGroup.readEntry("Past Background Enabled", false);
    QColor pastBackground = configGroup.readEntry("Past Background Color", QColor(160, 160, 160));
    modelConf.pastColorSettings.brushBackground = QBrush(pastBackground);
    modelConf.pastColorSettings.highlightNoEvents = configGroup.readEntry("Past Highlight No Events", true);

    m_selectedDateFormat = configGroup.readEntry("Date Format", 0);
    modelConf.dateFormat = m_possibleDateFormats[m_selectedDateFormat];

    viewConf.columnWidthName = configGroup.readEntry("Name Column Width", 0);
    viewConf.columnWidthAge = configGroup.readEntry("Age Column Width", 0);
    viewConf.columnWidthDate = configGroup.readEntry("Date Column Width", 0);
    viewConf.columnWidthWhen = configGroup.readEntry("When Column Width", 0);
}

void BirthdayList::ConfigHelper::storeConfiguration(KConfigGroup &configGroup, const ModelConfiguration &modelConf, const ViewConfiguration &viewConf)
{
    /*if (modelConf.eventDataSource == ModelConfiguration::EDS_KABC) configGroup.writeEntry("Event Data Source", "KABC");
    else configGroup.writeEntry("Event Data Source", "Akonadi");*/
    configGroup.writeEntry("Event Data Source", "Akonadi");

    if (modelConf.akonadiCollectionId >= 0) configGroup.writeEntry("Akonadi Collection", modelConf.akonadiCollectionId);
    configGroup.writeEntry("Nameday By Anniversary Field", modelConf.namedayByAnniversaryDateField);
    configGroup.writeEntry("Nameday By Custom Field", modelConf.namedayByCustomDateField);
    configGroup.writeEntry("Nameday Custom Field", modelConf.namedayCustomDateFieldName);
    configGroup.writeEntry("Nameday By Given Name", modelConf.namedayByGivenName);

    configGroup.writeEntry("Show Column Headers", viewConf.showColumnHeaders);
    //configGroup.writeEntry("Show Name Column", viewConf.showColName);
    configGroup.writeEntry("Show Age Column", viewConf.showColAge);
    configGroup.writeEntry("Show Date Column", viewConf.showColDate);
    configGroup.writeEntry("Show When Column", viewConf.showColWhen);

    configGroup.writeEntry("Show Nicknames", modelConf.showNicknames);

    configGroup.writeEntry("Show Namedays", modelConf.showNamedays);
    configGroup.writeEntry("Nameday Calendar File", modelConf.curNamedayFile);
    if (modelConf.namedayDisplayMode == ModelConfiguration::NDM_IndividualEvents) configGroup.writeEntry("Nameday Display Mode", "IndividualEvents");
    else if (modelConf.namedayDisplayMode == ModelConfiguration::NDM_AllCalendarNames) configGroup.writeEntry("Nameday Display Mode", "AllCalendarNames");
    else configGroup.writeEntry("Nameday Display Mode", "AggregateEvents");

    configGroup.writeEntry("Show Anniversaries", modelConf.showAnniversaries);

    if (modelConf.filterType == ModelConfiguration::FT_Category) configGroup.writeEntry("Filter Type", "Category");
    else if (modelConf.filterType == ModelConfiguration::FT_CustomField) configGroup.writeEntry("Filter Type", "Custom Field");
    else if (modelConf.filterType == ModelConfiguration::FT_CustomFieldPrefix) configGroup.writeEntry("Filter Type", "Custom Field Prefix");
    else configGroup.writeEntry("Filter Type", "Off");

    configGroup.writeEntry("Custom Field", modelConf.customFieldName);
    configGroup.writeEntry("Custom Field Prefix", modelConf.customFieldPrefix);
    configGroup.writeEntry("Filter Value", modelConf.filterValue);

    configGroup.writeEntry("Event Threshold", modelConf.eventThreshold);
    configGroup.writeEntry("Highlight Threshold", modelConf.highlightThreshold);
    configGroup.writeEntry("Highlight Foreground Enabled", modelConf.highlightColorSettings.isForeground);
    configGroup.writeEntry("Highlight Foreground Color", modelConf.highlightColorSettings.brushForeground.color());
    configGroup.writeEntry("Highlight Background Enabled", modelConf.highlightColorSettings.isBackground);
    configGroup.writeEntry("Highlight Background Color", modelConf.highlightColorSettings.brushBackground.color());
    configGroup.writeEntry("Coming Highlight No Events", modelConf.highlightColorSettings.highlightNoEvents);

    configGroup.writeEntry("Todays Foreground Enabled", modelConf.todayColorSettings.isForeground);
    configGroup.writeEntry("Todays Foreground Color", modelConf.todayColorSettings.brushForeground.color());
    configGroup.writeEntry("Todays Background Enabled", modelConf.todayColorSettings.isBackground);
    configGroup.writeEntry("Todays Background Color", modelConf.todayColorSettings.brushBackground.color());
    configGroup.writeEntry("Todays Highlight No Events", modelConf.todayColorSettings.highlightNoEvents);

    configGroup.writeEntry("Past Threshold", modelConf.pastThreshold);
    configGroup.writeEntry("Past Foreground Enabled", modelConf.pastColorSettings.isForeground);
    configGroup.writeEntry("Past Foreground Color", modelConf.pastColorSettings.brushForeground.color());
    configGroup.writeEntry("Past Background Enabled", modelConf.pastColorSettings.isBackground);
    configGroup.writeEntry("Past Background Color", modelConf.pastColorSettings.brushBackground.color());
    configGroup.writeEntry("Past Highlight No Events", modelConf.pastColorSettings.highlightNoEvents);

    configGroup.writeEntry("Date Format", m_selectedDateFormat);

    configGroup.writeEntry("Name Column Width", viewConf.columnWidthName);
    configGroup.writeEntry("Age Column Width", viewConf.columnWidthAge);
    configGroup.writeEntry("Date Column Width", viewConf.columnWidthDate);
    configGroup.writeEntry("When Column Width", viewConf.columnWidthWhen);
}

void BirthdayList::ConfigHelper::createConfigurationUI(KConfigDialog *parent, Model *model, const ModelConfiguration &modelConf, const ViewConfiguration &viewConf)
{
    QWidget *contactsWidget = new QWidget;
    QWidget *eventsWidget = new QWidget;
    QWidget *tableWidget = new QWidget;
    QWidget *colorsWidget = new QWidget;

    m_ui_contacts.setupUi(contactsWidget);
    m_ui_events.setupUi(eventsWidget);
    m_ui_table.setupUi(tableWidget);
    m_ui_colors.setupUi(colorsWidget);

    parent->setButtons(KDialog::Ok | KDialog::Cancel);
    parent->addPage(contactsWidget, i18n("Contacts"), "system-users");
    parent->addPage(eventsWidget, i18n("Events"), "bl_date");
    parent->addPage(tableWidget, i18n("Table"), "view-form-table");
    parent->addPage(colorsWidget, i18n("Colors"), "preferences-desktop-color");

    m_ui_contacts.cmbDataSource->clear();
    /*m_ui_contacts.cmbDataSource->addItem(i18n("KDE Address Book"), QVariant("KABC"));
    if (modelConf.eventDataSource == ModelConfiguration::EDS_KABC) m_ui_contacts.cmbDataSource->setCurrentIndex(m_ui_contacts.cmbDataSource->count()-1);*/

    m_ui_contacts.cmbDataSource->addItem(i18n("Akonadi"), QVariant("Akonadi"));
    if (modelConf.eventDataSource == ModelConfiguration::EDS_Akonadi) m_ui_contacts.cmbDataSource->setCurrentIndex(m_ui_contacts.cmbDataSource->count()-1);

    dataSourceChanged(m_ui_contacts.cmbDataSource->currentText());

    m_ui_contacts.cmbAkoCollection->clear();
    QHash<QString, int> akonadiCollections = model->getAkonadiCollections();
    QHashIterator<QString, int> collectionsIt(akonadiCollections);
    while (collectionsIt.hasNext()) {
        collectionsIt.next();
        QString collectionName = collectionsIt.key();
        int collectionId = collectionsIt.value();
        m_ui_contacts.cmbAkoCollection->addItem(collectionName, collectionId);
        if (collectionId == modelConf.akonadiCollectionId) {
            m_ui_contacts.cmbAkoCollection->setCurrentIndex(m_ui_contacts.cmbAkoCollection->count()-1);
        }
    }
    if (m_ui_contacts.cmbAkoCollection->count() == 0) {
        m_ui_contacts.cmbAkoCollection->addItem(i18nc("No Akonadi collections", "No collections available"));
        m_ui_contacts.cmbAkoCollection->setEnabled(false);
    }
    else m_ui_contacts.cmbAkoCollection->setEnabled(true);

    m_ui_events.chckNamedayAnniversaryField->setChecked(modelConf.namedayByAnniversaryDateField);
    m_ui_events.chckNamedayCustomDateField->setChecked(modelConf.namedayByCustomDateField);
    m_ui_events.lineEditNamedayCustomDateField->setText(modelConf.namedayCustomDateFieldName);
    m_ui_events.chckNamedayNameField->setChecked(modelConf.namedayByGivenName);

    m_ui_table.chckShowColumnHeaders->setChecked(viewConf.showColumnHeaders);
    m_ui_table.chckShowColName->setChecked(viewConf.showColName);
    m_ui_table.chckShowColAge->setChecked(viewConf.showColAge);
    m_ui_table.chckShowColDate->setChecked(viewConf.showColDate);
    m_ui_table.chckShowColWhen->setChecked(viewConf.showColWhen);

    m_ui_table.chckShowNicknames->setChecked(modelConf.showNicknames);

    m_ui_events.chckShowNamedays->setChecked(modelConf.showNamedays);
    m_ui_events.rbNamedayShowIndEvents->setChecked(modelConf.namedayDisplayMode == ModelConfiguration::NDM_IndividualEvents);
    m_ui_events.rbNamedayShowAllFromCal->setChecked(modelConf.namedayDisplayMode == ModelConfiguration::NDM_AllCalendarNames);
    m_ui_events.rbNamedayShowAggrEvents->setChecked(modelConf.namedayDisplayMode == ModelConfiguration::NDM_AggregateEvents);

    m_ui_events.cmbNamedayCalendar->clear();
    m_ui_events.cmbNamedayCalendar->addItems(m_namedayLangStrings);
    if (m_namedayFiles.contains(modelConf.curNamedayFile)) {
        m_ui_events.cmbNamedayCalendar->setCurrentIndex(m_namedayFiles.indexOf(modelConf.curNamedayFile));
    } else m_ui_events.cmbNamedayCalendar->setCurrentIndex(0);
    m_ui_events.chckShowAnniversaries->setChecked(modelConf.showAnniversaries);
    
    m_ui_contacts.rbFilterTypeOff->setChecked(modelConf.filterType == ModelConfiguration::FT_Off);
    m_ui_contacts.rbFilterTypeCategory->setChecked(modelConf.filterType == ModelConfiguration::FT_Category);
    m_ui_contacts.rbFilterTypeCustomFieldName->setChecked(modelConf.filterType == ModelConfiguration::FT_CustomField);
    m_ui_contacts.rbFilterTypeCustomFieldPrefix->setChecked(modelConf.filterType == ModelConfiguration::FT_CustomFieldPrefix);
    m_ui_contacts.lineEditCustomFieldName->setText(modelConf.customFieldName);
    m_ui_contacts.lineEditCustomFieldPrefix->setText(modelConf.customFieldPrefix);
    m_ui_contacts.lineEditFilterValue->setText(modelConf.filterValue);

    m_ui_colors.chckTodaysForeground->setChecked(modelConf.todayColorSettings.isForeground);
    m_ui_colors.colorbtnTodaysForeground->setColor(modelConf.todayColorSettings.brushForeground.color());
    m_ui_colors.chckTodaysBackground->setChecked(modelConf.todayColorSettings.isBackground);
    m_ui_colors.colorbtnTodaysBackground->setColor(modelConf.todayColorSettings.brushBackground.color());
    m_ui_colors.chckTodaysHighlightNoEvent->setChecked(modelConf.todayColorSettings.highlightNoEvents);

    m_ui_events.spinComingShowDays->setValue(modelConf.eventThreshold);
    m_ui_colors.spinComingHighlightDays->setValue(modelConf.highlightThreshold);
    m_ui_colors.chckComingHighlightForeground->setChecked(modelConf.highlightColorSettings.isForeground);
    m_ui_colors.colorbtnComingHighlightForeground->setColor(modelConf.highlightColorSettings.brushForeground.color());
    m_ui_colors.chckComingHighlightBackground->setChecked(modelConf.highlightColorSettings.isBackground);
    m_ui_colors.colorbtnComingHighlightBackground->setColor(modelConf.highlightColorSettings.brushBackground.color());
    m_ui_colors.chckComingHighlightNoEvent->setChecked(modelConf.highlightColorSettings.highlightNoEvents);

    m_ui_events.spinPastShowDays->setValue(modelConf.pastThreshold);
    m_ui_colors.chckPastForeground->setChecked(modelConf.pastColorSettings.isForeground);
    m_ui_colors.colorbtnPastForeground->setColor(modelConf.pastColorSettings.brushForeground.color());
    m_ui_colors.chckPastBackground->setChecked(modelConf.pastColorSettings.isBackground);
    m_ui_colors.colorbtnPastBackground->setColor(modelConf.pastColorSettings.brushBackground.color());
    m_ui_colors.chckPastHighlightNoEvent->setChecked(modelConf.pastColorSettings.highlightNoEvents);

    m_ui_table.cmbDateDisplayFormat->setCurrentIndex(m_selectedDateFormat);
    
    // enable only relevant widgets
    namedayIdentificationChanged();

    connect(m_ui_contacts.cmbDataSource, SIGNAL(currentIndexChanged(QString)), this, SLOT(dataSourceChanged(QString)));
    connect(m_ui_events.chckShowNamedays, SIGNAL(toggled(bool)), this, SLOT(namedayIdentificationChanged()));
    connect(m_ui_events.chckNamedayAnniversaryField, SIGNAL(toggled(bool)), this, SLOT(namedayIdentificationChanged()));
    connect(m_ui_events.chckNamedayCustomDateField, SIGNAL(toggled(bool)), this, SLOT(namedayIdentificationChanged()));
    connect(m_ui_events.chckNamedayNameField, SIGNAL(toggled(bool)), this, SLOT(namedayIdentificationChanged()));
    
    connect(m_ui_events.chckNamedayAnniversaryField, SIGNAL(toggled(bool)), this, SLOT(namedayAnniversaryFieldSelected(bool)));
    connect(m_ui_events.chckNamedayCustomDateField, SIGNAL(toggled(bool)), this, SLOT(namedayCustomFieldSelected(bool)));
}

void BirthdayList::ConfigHelper::updateConfigurationFromUI(ModelConfiguration &modelConf, ViewConfiguration &viewConf)
{
    QString selectedDataSource;
    if (m_ui_contacts.cmbDataSource->count() > 0) {
        selectedDataSource = m_ui_contacts.cmbDataSource->itemText(m_ui_contacts.cmbDataSource->currentIndex());
    }
    /*if (selectedDataSource == "Akonadi") modelConf.eventDataSource = ModelConfiguration::EDS_Akonadi;
    else modelConf.eventDataSource = ModelConfiguration::EDS_KABC;*/
    modelConf.eventDataSource = ModelConfiguration::EDS_Akonadi;

    if (m_ui_contacts.cmbAkoCollection->isEnabled()) {
        modelConf.akonadiCollectionId = m_ui_contacts.cmbAkoCollection->itemData(m_ui_contacts.cmbAkoCollection->currentIndex()).toInt();
    }
    else modelConf.akonadiCollectionId = -1;

    modelConf.namedayByAnniversaryDateField = m_ui_events.chckNamedayAnniversaryField->isChecked();
    modelConf.namedayByCustomDateField = m_ui_events.chckNamedayCustomDateField->isChecked();
    modelConf.namedayCustomDateFieldName = m_ui_events.lineEditNamedayCustomDateField->text();
    modelConf.namedayByGivenName = m_ui_events.chckNamedayNameField->isChecked();

    viewConf.showColumnHeaders = m_ui_table.chckShowColumnHeaders->isChecked();
    viewConf.showColName = m_ui_table.chckShowColName->isChecked();
    viewConf.showColAge = m_ui_table.chckShowColAge->isChecked();
    viewConf.showColDate = m_ui_table.chckShowColDate->isChecked();
    viewConf.showColWhen = m_ui_table.chckShowColWhen->isChecked();

    modelConf.showNicknames = m_ui_table.chckShowNicknames->isChecked();

    modelConf.showNamedays = m_ui_events.chckShowNamedays->isChecked();
    modelConf.curNamedayFile = m_namedayFiles[m_ui_events.cmbNamedayCalendar->currentIndex()];
    if (m_ui_events.rbNamedayShowIndEvents->isChecked()) modelConf.namedayDisplayMode = ModelConfiguration::NDM_IndividualEvents;
    else if (m_ui_events.rbNamedayShowAllFromCal->isChecked()) modelConf.namedayDisplayMode = ModelConfiguration::NDM_AllCalendarNames;
    else modelConf.namedayDisplayMode = ModelConfiguration::NDM_AggregateEvents;

    modelConf.showAnniversaries = m_ui_events.chckShowAnniversaries->isChecked();
    
    if (m_ui_contacts.rbFilterTypeCategory->isChecked()) modelConf.filterType = ModelConfiguration::FT_Category;
    else if (m_ui_contacts.rbFilterTypeCustomFieldName->isChecked()) modelConf.filterType = ModelConfiguration::FT_CustomField;
    else if (m_ui_contacts.rbFilterTypeCustomFieldPrefix->isChecked()) modelConf.filterType = ModelConfiguration::FT_CustomFieldPrefix;
    else modelConf.filterType = ModelConfiguration::FT_Off;
    modelConf.customFieldName = m_ui_contacts.lineEditCustomFieldName->text();
    modelConf.customFieldPrefix = m_ui_contacts.lineEditCustomFieldPrefix->text();
    modelConf.filterValue = m_ui_contacts.lineEditFilterValue->text();

    modelConf.todayColorSettings.isForeground = m_ui_colors.chckTodaysForeground->isChecked();
    modelConf.todayColorSettings.brushForeground.setColor(m_ui_colors.colorbtnTodaysForeground->color());
    modelConf.todayColorSettings.isBackground = m_ui_colors.chckTodaysBackground->isChecked();
    modelConf.todayColorSettings.brushBackground.setColor(m_ui_colors.colorbtnTodaysBackground->color());
    modelConf.todayColorSettings.highlightNoEvents = m_ui_colors.chckTodaysHighlightNoEvent->isChecked();

    modelConf.eventThreshold = m_ui_events.spinComingShowDays->value();
    modelConf.highlightThreshold = m_ui_colors.spinComingHighlightDays->value();
    modelConf.highlightColorSettings.isForeground = m_ui_colors.chckComingHighlightForeground->isChecked();
    modelConf.highlightColorSettings.brushForeground.setColor(m_ui_colors.colorbtnComingHighlightForeground->color());
    modelConf.highlightColorSettings.isBackground = m_ui_colors.chckComingHighlightBackground->isChecked();
    modelConf.highlightColorSettings.brushBackground.setColor(m_ui_colors.colorbtnComingHighlightBackground->color());
    modelConf.highlightColorSettings.highlightNoEvents = m_ui_colors.chckComingHighlightNoEvent->isChecked();

    modelConf.pastThreshold = m_ui_events.spinPastShowDays->value();
    AbstractAnnualEventEntry::setPastThreshold(modelConf.pastThreshold);
    modelConf.pastColorSettings.isForeground = m_ui_colors.chckPastForeground->isChecked();
    modelConf.pastColorSettings.brushForeground.setColor(m_ui_colors.colorbtnPastForeground->color());
    modelConf.pastColorSettings.isBackground = m_ui_colors.chckPastBackground->isChecked();
    modelConf.pastColorSettings.brushBackground.setColor(m_ui_colors.colorbtnPastBackground->color());
    modelConf.pastColorSettings.highlightNoEvents = m_ui_colors.chckPastHighlightNoEvent->isChecked();

    m_selectedDateFormat = m_ui_table.cmbDateDisplayFormat->currentIndex();
    modelConf.dateFormat = m_possibleDateFormats[m_selectedDateFormat];
}

void BirthdayList::ConfigHelper::dataSourceChanged(const QString &name) 
{
    m_ui_contacts.lblAkoCollection->setVisible(name == "Akonadi");
    m_ui_contacts.cmbAkoCollection->setVisible(name == "Akonadi");
}

void BirthdayList::ConfigHelper::namedayIdentificationChanged()
{
    bool namedaysEnabled = m_ui_events.chckShowNamedays->isChecked();
    bool namedayCustomDateField = m_ui_events.chckNamedayCustomDateField->isChecked();
    m_ui_events.lineEditNamedayCustomDateField->setEnabled(namedaysEnabled && namedayCustomDateField);
}

void BirthdayList::ConfigHelper::namedayAnniversaryFieldSelected(bool checked)
{
    if (checked) m_ui_events.chckNamedayCustomDateField->setChecked(false);
}

void BirthdayList::ConfigHelper::namedayCustomFieldSelected(bool checked)
{
    if (checked) m_ui_events.chckNamedayAnniversaryField->setChecked(false);
}

void BirthdayList::ConfigHelper::readAvailableNamedayLists() 
{
    QStringList fileNames = KGlobal::dirs()->findAllResources("data", "birthdaylist/namedaydefs/namedays_*.txt");
    if (fileNames.isEmpty()) {
        kDebug() << "Couldn't find any nameday list files";
        return;
    }

    foreach(QString fileName, fileNames) {
        int langPos = fileName.lastIndexOf("/namedays_") + 10;
        QString namedayDefinitionKey = fileName.mid(langPos);
        namedayDefinitionKey.chop(4);

        QString languageName;
        QFile namedayFile(fileName);
        if (namedayFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&namedayFile);
            languageName =  stream.readLine();
            namedayFile.close();
        } else {
            kDebug() << "Cannot read language string from " << fileName;
            continue;
        }
        
        kDebug() << "Registering nameday file" << fileName << "for language" << languageName;
        m_namedayFiles.append(fileName);
        m_namedayLangStrings.append(languageName);
    }

    m_namedayFiles.prepend("");
    m_namedayLangStrings.prepend(i18nc("No nameday calendar (combo box item)", "None"));
}
