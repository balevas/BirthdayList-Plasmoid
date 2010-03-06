#ifndef BIRTHDAYLIST_APPLET_H
#define BIRTHDAYLIST_APPLET_H

/**
 * @file    birthdaylist_applet.h
 * @author  Karol Slanina
 * @version 0.5.1
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
#include "ui_birthdaylist_appearance_config.h"

#include <QStandardItemModel>

#include <Plasma/PopupApplet>
#include <Plasma/DataEngine>

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

private slots:
    /** Receives a notification about changes in the address book and current date. */
    void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data);
    /** Receives a notification when the user accepts the configuration change. */
    void configAccepted();
    /** Receives a notification when the system plasma theme is changed. */
    void plasmaThemeChanged();

private:
    /** Creates the configuration dialog and fills it with current settings. */
    void createConfigurationInterface(KConfigDialog *parent);

    /** Updates the list of entries after the data update */
    void updateEventList(const Plasma::DataEngine::Data &data);
    /** Returns the name from the current nameday calendar belonging to the given date. */
    QString getNamedayString(QDate date);

    /** Updates the internal model of the tree view after the data update */
    void updateModels();
    /** Sets the colors for the model items according to the applet configuration */
    void setModelItemColors(const AbstractAnnualEventEntry *entry, QStandardItem *item);
    /** Resizes the column widths of the tree view */
    void setTreeColumnWidths();
    /** Changes the tree and item colors according to the current Plasma theme. */
    void usePlasmaThemeColors();


    /** Shortcut to the data engine providing contact data and nameday calendars */
    Plasma::DataEngine *m_dataEngine;
    /** Name of the KABC field where namedays can be found */
    QString m_kabcNamedayString;
    /** Name of the KABC field where anniversaries can be found */
    QString m_kabcAnniversaryString;
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
    Ui::BirthdayListAppearanceConfig m_ui_appearance;

    bool m_showColumnHeaders;
    bool m_showColName;
    bool m_showColAge;
    bool m_showColDate;
    bool m_showColWhen;

    bool m_showNamedays;
    bool m_aggregateNamedays;
    bool m_showAnniversaries;

    int m_eventThreshold;
    int m_highlightThreshold;
    bool m_isHighlightForeground;
    QBrush m_brushHighlightForeground;
    bool m_isHighlightBackground;
    QBrush m_brushHighlightBackground;

    int m_pastThreshold;
    bool m_isPastForeground;
    QBrush m_brushPastForeground;
    bool m_isPastBackground;
    QBrush m_brushPastBackground;

    int m_columnWidthName;
    int m_columnWidthAge;
    int m_columnWidthDate;
    int m_columnWidthWhen;
};


#endif //BIRTHDAYLIST_APPLET_H
