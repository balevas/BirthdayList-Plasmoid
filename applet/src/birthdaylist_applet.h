#ifndef BIRTHDAYLIST_APPLET_H
#define BIRTHDAYLIST_APPLET_H

/**
 * @file    birthdaylist_applet.h
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


#include "ui_birthdaylist_datasource_config.h"
#include "ui_birthdaylist_contents_config.h"
#include "ui_birthdaylist_filter_config.h"
#include "ui_birthdaylist_appearance_config.h"

#include <QStandardItemModel>

#include <Plasma/PopupApplet>
#include <Plasma/DataEngine>

class KAboutData;
class AbstractAnnualEventEntry;
namespace Plasma {
    class TreeView;
}


class BirthdayListApplet : public Plasma::PopupApplet {
    Q_OBJECT

public:
    BirthdayListApplet(QObject *parent, const QVariantList &args);
    ~BirthdayListApplet();

    /** Initializes the applet (called by Plasma automatically). */
    void init();

    /** Creates the widget that will be shown in the Plasma applet. */
    QGraphicsWidget *graphicsWidget();
    virtual QList<QAction *> contextualActions();
    
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    
private slots:
    void dataSourceChanged(const QString &name);
    /** Receives a notification about changes in the address book and current date. */
    void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data);
    /** Receives a notification when the user accepts the configuration change. */
    void configAccepted();
    /** Receives a notification when the system plasma theme is changed. */
    void plasmaThemeChanged();
    void sendEmail();
    void visitHomepage();
    void about();

private:
    /** Creates the configuration dialog and fills it with current settings. */
    void createConfigurationInterface(KConfigDialog *parent);

    /** Updates the list of entries after the data update */
    void updateEventList(const Plasma::DataEngine::Data &data);
    QDate getContactDateField(const QVariantHash &contactInfo, QString fieldName);
    /** Returns the nameday date by comparing the contact's given name with the calendar entries */
    QDate getNamedayByGivenName(QString givenName);
    /** Returns the name from the current nameday calendar belonging to the given date. */
    QString getNamedayString(QDate date);
    QString getSelectedLineItem(int column);

    /** Updates the internal model of the tree view after the data update */
    void updateModels();
    /** Sets the colors for the model items according to the applet configuration */
    void setModelItemColors(const AbstractAnnualEventEntry *entry, QStandardItem *item, int colNum);
    /** Resizes the column widths of the tree view */
    void setTreeColumnWidths();
    /** Changes the tree and item colors according to the current Plasma theme. */
    void usePlasmaThemeColors();

    
    enum EventDataSource { EDS_Akonadi, EDS_KABC, EDS_Thunderbird };
    EventDataSource m_eventDataSource;
    /** Shortcut to the data engine providing contact data and nameday calendars */
    Plasma::DataEngine *m_dataEngine_namedays;
    Plasma::DataEngine *m_dataEngine_contacts;
    Plasma::DataEngine *m_dataEngine_kabc;
    Plasma::DataEngine *m_dataEngine_akonadi;
    Plasma::DataEngine *m_dataEngine_thunderbird;
    
    QString m_dataSourceName;

    /** Name of the Akonadi collection to be used */
    QString m_akoCollection;
    /** Name of the Akonadi collection field where namedays can be found */
    QString m_namedayDateFieldString;
    /** Mode to identify namedays for Akonadi contacts */
    enum NamedayIdentificationMode { NIM_DateField, NIM_GivenName, NIM_Both };
    NamedayIdentificationMode m_namedayIdentificationMode;
    /** Name of the Akonadi collection field where anniversaries can be found */
    QString m_anniversaryFieldString;


    /** List of language codes for all available nameday calendars (used in the data engine queries) */
    QList<QString> m_namedayLangCodes;
    /** List of language names for all available nameday calendars (used in the configuration dialog) */
    QList<QString> m_namedayLangStrings;
    /** Language code of the currently used nameday calendar */
    QString m_curNamedayLangCode;
    /** Local copy of the currently used nameday calendar */
    QHash<QString, QVariant> m_curLangNamedayList;

    /** Complete event list */
    QList<AbstractAnnualEventEntry*> m_listEntries;
    /** Internal data model of the tree view */
    QStandardItemModel m_model;

    QGraphicsWidget *m_graphicsWidget;
    Plasma::TreeView *m_treeView;

    Ui::BirthdayListDataSourceConfig m_ui_datasource;
    Ui::BirthdayListContentsConfig m_ui_contents;
    Ui::BirthdayListFilterConfig m_ui_filter;
    Ui::BirthdayListAppearanceConfig m_ui_appearance;


    bool m_showColumnHeaders;
    bool m_showColName;
    bool m_showColAge;
    bool m_showColDate;
    bool m_showColWhen;

    bool m_showNicknames;

    bool m_showNamedays;
    enum NamedayDisplayMode { NDM_AggregateEvents, NDM_IndividualEvents, NDM_AllCalendarNames };
    NamedayDisplayMode m_namedayDisplayMode;
    bool m_showAnniversaries;
    
    enum FilterType { FT_Off, FT_Category, FT_CustomField, FT_CustomFieldPrefix };
    FilterType m_filterType;
    QString m_customFieldName;
    QString m_customFieldPrefix;
    QString m_filterValue;

    bool m_isTodaysForeground;
    QBrush m_brushTodaysForeground;
    bool m_isTodaysBackground;
    QBrush m_brushTodaysBackground;
    bool m_isTodaysHighlightNoEvents;

    int m_eventThreshold;
    int m_highlightThreshold;
    bool m_isHighlightForeground;
    QBrush m_brushHighlightForeground;
    bool m_isHighlightBackground;
    QBrush m_brushHighlightBackground;
    bool m_isComingHighlightNoEvents;

    int m_pastThreshold;
    bool m_isPastForeground;
    QBrush m_brushPastForeground;
    bool m_isPastBackground;
    QBrush m_brushPastBackground;
    bool m_isPastHighlightNoEvents;

    QStringList m_possibleDateFormats;
    int m_selectedDateFormat;

    int m_columnWidthName;
    int m_columnWidthAge;
    int m_columnWidthDate;
    int m_columnWidthWhen;
    
    bool m_lastContextMenuEventOnTree;
    
    KAboutData *m_aboutData;
};


#endif //BIRTHDAYLIST_APPLET_H
