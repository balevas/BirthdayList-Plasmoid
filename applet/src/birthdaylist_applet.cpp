/**
 * @file    birthdaylist_applet.cpp
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

#include "birthdaylist_applet.h"
#include "birthdaylistentry.h"

#include <QTreeView>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneContextMenuEvent>
#include <QStackedWidget>
#include <QMenu>

#include <KConfigDialog>
#include <KDebug>
#include <KToolInvocation>
#include <KMimeTypeTrader>
#include <KAboutData>
#include <KAboutApplicationDialog>

#include <Plasma/Theme>
#include <Plasma/TreeView>


// linking this class to the desktop file so plasma can load it
K_EXPORT_PLASMA_APPLET(birthdaylist, BirthdayListApplet)


static const char *UTC_SOURCE = "UTC";

BirthdayListApplet::BirthdayListApplet(QObject *parent, const QVariantList &args)
: Plasma::PopupApplet(parent, args),
m_eventDataSource(EDS_Akonadi),
m_dataEngine_namedays(0),
m_dataEngine_contacts(0),
m_dataEngine_kabc(0),
m_dataEngine_akonadi(0),
m_dataEngine_thunderbird(0),
m_akoCollection(""),
m_namedayDateFieldString("Nameday"),
m_namedayIdentificationMode(NIM_Both),
m_anniversaryFieldString("Anniversary"),
m_model(0, 5),
m_graphicsWidget(0),
m_treeView(0),
m_showColumnHeaders(true),
m_showColName(true),
m_showColAge(true),
m_showColDate(true),
m_showColWhen(true),
m_showNicknames(true),
m_showNamedays(true),
m_namedayDisplayMode(NDM_AggregateEvents),
m_showAnniversaries(true),
m_filterType(FT_Off),
m_customFieldName(""),
m_customFieldPrefix(""),
m_filterValue(""),
m_isTodaysForeground(false),
m_brushTodaysForeground(QColor(255, 255, 255)),
m_isTodaysBackground(true),
m_brushTodaysBackground(QColor(128, 0, 0)),
m_isTodaysHighlightNoEvents(true),
m_eventThreshold(30),
m_highlightThreshold(2),
m_isHighlightForeground(false),
m_brushHighlightForeground(QColor(255, 255, 255)),
m_isHighlightBackground(true),
m_brushHighlightBackground(QColor(128, 0, 0)),
m_isComingHighlightNoEvents(false),
m_pastThreshold(2),
m_isPastForeground(false),
m_brushPastForeground(QColor(0, 0, 0)),
m_isPastBackground(true),
m_brushPastBackground(QColor(160, 0, 0)),
m_isPastHighlightNoEvents(true),
m_selectedDateFormat(0),
m_columnWidthName(0),
m_columnWidthAge(0),
m_columnWidthDate(0),
m_columnWidthWhen(0),
m_lastContextMenuEventOnTree (false) {
    m_possibleDateFormats << "d. M." << "dd. MM."
            << "d-M" << "dd-MM" << "M-d" << "MM-dd"
            << "d/M" << "dd/MM" << "M/d" << "MM/dd";
    setBackgroundHints(DefaultBackground);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
    
    m_aboutData = new KAboutData("birthdaylist_applet", QByteArray(), ki18n("Birthday List"), "0.6.3", ki18n("Birthday List"),
    KAboutData::License_GPL,
    ki18n("Copyright (C) 2010 Karol Slanina"),
    ki18n("Shows the list of upcoming birthdays, anniversaries and name days"),
    "http://kde-look.org/content/show.php?content=121134",
    "http://kde-look.org/content/show.php?content=121134");

    // Authors
    m_aboutData->setProgramIconName("bl_cookie");
    m_aboutData->addAuthor(ki18n("Karol Slanina"), ki18n("Plasmoid, data engines, Slovak translation"), "karol.slanina@gmail.com");
    m_aboutData->addCredit(ki18n("Ondřej Kuda"), ki18n("Czech translation"));
    m_aboutData->addCredit(ki18n("Andreas Goldbohm"), ki18n("German translation"));
    m_aboutData->addCredit(ki18n("David Vignoni"), ki18n("Nuvola icons"));
    m_aboutData->addCredit(ki18n("All people who report bugs, send feedback and new feature requests"));
    m_aboutData->setTranslator(ki18nc("NAME OF THE TRANSLATORS", "Your names"), ki18nc("EMAIL OF THE TRANSLATORS", "Your emails"));
    
    resize(350, 200);
}

BirthdayListApplet::~BirthdayListApplet() {
    delete m_graphicsWidget;
    delete m_aboutData;
}

void BirthdayListApplet::init() {
    setPopupIcon(KIcon("bl_cookie", NULL));

    KConfigGroup configGroup = config();
    QString eventDataSource = configGroup.readEntry("Event Data Source", "");
    if (eventDataSource == "Akonadi") m_eventDataSource = EDS_Akonadi;
    else if (eventDataSource == "Thunderbird") m_eventDataSource = EDS_Thunderbird;
    else m_eventDataSource = EDS_KABC;

    m_akoCollection = configGroup.readEntry("Akonadi Collection", "");
    m_namedayDateFieldString = configGroup.readEntry("Nameday String", "Nameday");
    m_anniversaryFieldString = configGroup.readEntry("Anniversary String", "Anniversary");
    QString nimString = configGroup.readEntry("Nameday Identification Mode", "");
    if (nimString == "Date Field") m_namedayIdentificationMode = NIM_DateField;
    else if (nimString == "Given Name") m_namedayIdentificationMode = NIM_GivenName;
    else m_namedayIdentificationMode = NIM_Both;

    m_curNamedayLangCode = configGroup.readEntry("Nameday Calendar LangCode", "");

    m_dataEngine_namedays = dataEngine("birthdaylist");
    if (m_dataEngine_namedays && m_dataEngine_namedays->isValid()) {

        QHash<QString, QVariant> namedayLangCodeInfo = m_dataEngine_namedays->query("NamedayLists");
        m_namedayLangCodes = namedayLangCodeInfo.keys();
        qSort(m_namedayLangCodes);

        foreach(QString langCode, m_namedayLangCodes) {
            m_namedayLangStrings.append(namedayLangCodeInfo[langCode].toHash()["Language"].toString());
        }

        m_namedayLangCodes.prepend("");
        m_namedayLangStrings.prepend(i18nc("No nameday calendar (combo box item)", "None"));

        if (!m_namedayLangCodes.contains(m_curNamedayLangCode)) m_curNamedayLangCode = "";
        if (!m_curNamedayLangCode.isEmpty()) {
            m_curLangNamedayList = m_dataEngine_namedays->query(QString("NamedayList_%1").arg(m_curNamedayLangCode));
        }
    } else {
        setFailedToLaunch(true, "Could not load birthdaylist dataEngine");
    }

    m_dataEngine_kabc = dataEngine("birthdaylist");
    m_dataEngine_akonadi = dataEngine("birthdaylist_akonadi");
    m_dataEngine_thunderbird = dataEngine("birthdaylist_thunderbird");
    if (!m_dataEngine_kabc || !m_dataEngine_kabc->isValid()) {
        setFailedToLaunch(true, "Could not load birthdaylist dataEngine");
    }

    if (m_eventDataSource == EDS_Akonadi) {
        m_dataEngine_contacts = m_dataEngine_akonadi;
        m_dataSourceName = QString("Contacts_%1").arg(m_akoCollection);
    }
    else if (m_eventDataSource == EDS_Thunderbird) {
        m_dataEngine_contacts = m_dataEngine_thunderbird;
        m_dataSourceName = "ContactInfo";
    }
    else {
        m_dataEngine_contacts = m_dataEngine_kabc;
        m_dataSourceName = "ContactInfo";
    }
    m_dataEngine_contacts->connectSource(m_dataSourceName, this);

    Plasma::DataEngine* timeEngine = dataEngine("time");
    if (timeEngine) {
        timeEngine->connectSource("UTC", this, 360000, Plasma::AlignToHour);
    } else {
        kDebug() << "Warning: Could not load time dataEngine - no nightly update possible";
    }

    m_showNicknames = configGroup.readEntry("Show Nicknames", true);
    
    QString filterType = configGroup.readEntry("Filter Type", "");
    if (filterType == "Category") m_filterType = FT_Category;
    else if (filterType == "Custom Field") m_filterType = FT_CustomField;
    else if (filterType == "Custom Field Prefix") m_filterType = FT_CustomFieldPrefix;
    else m_filterType = FT_Off;
    
    m_customFieldName = configGroup.readEntry("Custom Field", "");
    m_customFieldPrefix = configGroup.readEntry("Custom Field Prefix", "");
    m_filterValue = configGroup.readEntry("Filter Value", "");
    
    m_showColumnHeaders = configGroup.readEntry("Show Column Headers", true);
    //m_showColName = configGroup.readEntry("Show Name Column", true);
    m_showColName = true;
    m_showColAge = configGroup.readEntry("Show Age Column", true);
    m_showColDate = configGroup.readEntry("Show Date Column", true);
    m_showColWhen = configGroup.readEntry("Show When Column", true);

    m_showNamedays = configGroup.readEntry("Show Namedays", true);
    QString namedayDisplayMode = configGroup.readEntry("Nameday Display Mode", "");
    if (namedayDisplayMode == "IndividualEvents") m_namedayDisplayMode = NDM_IndividualEvents;
    else if (namedayDisplayMode == "AllCalendarNames") m_namedayDisplayMode = NDM_AllCalendarNames;
    else m_namedayDisplayMode = NDM_AggregateEvents;

    m_showAnniversaries = configGroup.readEntry("Show Anniversaries", true);
    
    m_isTodaysForeground = configGroup.readEntry("Todays Foreground Enabled", false);
    QColor todaysForeground = configGroup.readEntry("Todays Foreground Color", QColor(255, 255, 255));
    m_brushTodaysForeground = QBrush(todaysForeground);
    m_isTodaysBackground = configGroup.readEntry("Todays Background Enabled", true);
    QColor todaysBackground = configGroup.readEntry("Todays Background Color", QColor(128, 0, 0));
    m_brushTodaysBackground = QBrush(todaysBackground);
    m_isTodaysHighlightNoEvents = configGroup.readEntry("Todays Highlight No Events", true);

    m_eventThreshold = configGroup.readEntry("Event Threshold", 30);
    m_highlightThreshold = configGroup.readEntry("Highlight Threshold", 2);
    m_isHighlightForeground = configGroup.readEntry("Highlight Foreground Enabled", false);
    QColor highlightForeground = configGroup.readEntry("Highlight Foreground Color", QColor(255, 255, 255));
    m_brushHighlightForeground = QBrush(highlightForeground);
    m_isHighlightBackground = configGroup.readEntry("Highlight Background Enabled", true);
    QColor highlightBackground = configGroup.readEntry("Highlight Background Color", QColor(128, 0, 0));
    m_brushHighlightBackground = QBrush(highlightBackground);
    m_isComingHighlightNoEvents = configGroup.readEntry("Coming Highlight No Events", false);

    m_pastThreshold = configGroup.readEntry("Past Threshold", 2);
    AbstractAnnualEventEntry::setPastThreshold(m_pastThreshold);
    m_isPastForeground = configGroup.readEntry("Past Foreground Enabled", false);
    QColor pastForeground = configGroup.readEntry("Past Foreground Color", QColor(0, 0, 0));
    m_brushPastForeground = QBrush(pastForeground);
    m_isPastBackground = configGroup.readEntry("Past Background Enabled", false);
    QColor pastBackground = configGroup.readEntry("Past Background Color", QColor(160, 160, 160));
    m_brushPastBackground = QBrush(pastBackground);
    m_isPastHighlightNoEvents = configGroup.readEntry("Past Highlight No Events", true);

    m_selectedDateFormat = configGroup.readEntry("Date Format", 0);

    m_columnWidthName = configGroup.readEntry("Name Column Width", 0);
    m_columnWidthAge = configGroup.readEntry("Age Column Width", 0);
    m_columnWidthDate = configGroup.readEntry("Date Column Width", 0);
    m_columnWidthWhen = configGroup.readEntry("When Column Width", 0);

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(plasmaThemeChanged()));

    graphicsWidget();
}

QGraphicsWidget *BirthdayListApplet::graphicsWidget() {
    if (!m_graphicsWidget) {
        m_graphicsWidget = new QGraphicsWidget(this);

        m_treeView = new Plasma::TreeView(m_graphicsWidget);
        m_treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QTreeView *treeView = m_treeView->nativeWidget();
        treeView->setAlternatingRowColors(true);
        treeView->setAllColumnsShowFocus(true);
        treeView->setRootIsDecorated(false);
        //treeView->setSortingEnabled(true);
        treeView->setSelectionMode(QAbstractItemView::NoSelection);
        treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
        treeView->setItemsExpandable(true);
        treeView->setExpandsOnDoubleClick(true);

        updateModels();
        m_treeView->setModel(&m_model);
        m_treeView->setMinimumSize(10, 10);

        QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Vertical);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        layout->addItem(m_treeView);
        m_graphicsWidget->setLayout(layout);
    }
    return m_graphicsWidget;
}

void BirthdayListApplet::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    m_lastContextMenuEventOnTree = m_treeView->nativeWidget()->underMouse() && !m_treeView->nativeWidget()->header()->underMouse();
    Plasma::PopupApplet::contextMenuEvent(event);
}

QString BirthdayListApplet::getSelectedLineItem(int column)
{
    QModelIndex idx = m_treeView->nativeWidget()->currentIndex();
    if (idx.isValid()) return m_model.itemFromIndex((m_model.index(idx.row(), column, idx.parent())))->text();
    else return "";
}


QList<QAction *> BirthdayListApplet::contextualActions()
{
    QList<QAction *> currentActions;

    QModelIndex idx = m_treeView->nativeWidget()->currentIndex();
    if (m_lastContextMenuEventOnTree && idx.isValid()) {
       
       QString name = getSelectedLineItem(0);

       QString selectedEntryEmail = getSelectedLineItem(4);
        if (!selectedEntryEmail.isEmpty()) {
            KIcon mailerIcon("internet-mail");
            QAction *actionSendEmail = new QAction(mailerIcon, QString(i18n("Send Email to %1")).arg(name), this);
            connect(actionSendEmail, SIGNAL(triggered()), this, SLOT(sendEmail()));
            currentActions.append(actionSendEmail);
        }
        
        KService::Ptr browserService = KMimeTypeTrader::self()->preferredService("text/html");
        KIcon browserIcon("konqueror");
        if (!browserService.isNull()) browserIcon = KIcon(browserService->icon());

        QString selectedEntryUrl = getSelectedLineItem(5);
        if (!selectedEntryUrl.isEmpty()) {
            QAction *actionVisitHomepage = new QAction(browserIcon, QString(i18n("Visit %1's Homepage")).arg(name), this);
            connect(actionVisitHomepage, SIGNAL(triggered()), this, SLOT(visitHomepage()));
            currentActions.append(actionVisitHomepage);
        }
    }
    
    QAction *separator1 = new QAction(this);
    separator1->setSeparator(true);
    currentActions.append(separator1);

    KIcon aboutIcon("help-about");
    QAction *actionAbout = new QAction(aboutIcon, i18nc("Show About Dialog", "About"), this);
    connect(actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    currentActions.append(actionAbout);

    QAction *separator2 = new QAction(this);
    separator2->setSeparator(true);
    currentActions.append(separator2);
    
    return currentActions;
}


void BirthdayListApplet::dataUpdated(const QString &name, const Plasma::DataEngine::Data &data) {
    qDebug() << "Update of birthday list entries, triggered by data source" << name;

    if (name == m_dataSourceName) {
        updateEventList(data);
    } else if (name == UTC_SOURCE) {
        updateEventList(m_dataEngine_contacts->query(m_dataSourceName));
    }

    updateModels();
    update();
}

void BirthdayListApplet::configAccepted() {
    QString selectedDataSource;
    if (m_ui_datasource.cmbDataSource->count() > 0) {
        selectedDataSource = m_ui_datasource.cmbDataSource->itemText(m_ui_datasource.cmbDataSource->currentIndex());
    }
    if (selectedDataSource == "Akonadi") m_eventDataSource = EDS_Akonadi;
    else if (selectedDataSource == "Thunderbird") m_eventDataSource = EDS_Thunderbird;
    else m_eventDataSource = EDS_KABC;

    if (m_ui_datasource.cmbAkoCollection->isEnabled()) {
        m_akoCollection = m_ui_datasource.cmbAkoCollection->itemData(m_ui_datasource.cmbAkoCollection->currentIndex()).toString();
    }
    else m_akoCollection = "";

    m_namedayDateFieldString = m_ui_datasource.lineEditNamedayDateField->text();
    m_anniversaryFieldString = m_ui_datasource.lineEditAnniversaryField->text();
    if (m_ui_datasource.rbNamedayDateField->isChecked()) m_namedayIdentificationMode = NIM_DateField;
    else if (m_ui_datasource.rbNamedayNameField->isChecked()) m_namedayIdentificationMode = NIM_GivenName;
    else m_namedayIdentificationMode = NIM_Both;

    m_showColumnHeaders = m_ui_contents.chckShowColumnHeaders->isChecked();
    m_showColName = m_ui_contents.chckShowColName->isChecked();
    m_showColAge = m_ui_contents.chckShowColAge->isChecked();
    m_showColDate = m_ui_contents.chckShowColDate->isChecked();
    m_showColWhen = m_ui_contents.chckShowColWhen->isChecked();

    m_showNicknames = m_ui_contents.chckShowNicknames->isChecked();

    m_showNamedays = m_ui_contents.chckShowNamedays->isChecked();
    m_curNamedayLangCode = m_namedayLangCodes[m_ui_contents.cmbNamedayCalendar->currentIndex()];
    if (m_ui_contents.rbNamedayShowIndEvents->isChecked()) m_namedayDisplayMode = NDM_IndividualEvents;
    else if (m_ui_contents.rbNamedayShowAllFromCal->isChecked()) m_namedayDisplayMode = NDM_AllCalendarNames;
    else m_namedayDisplayMode = NDM_AggregateEvents;

    m_showAnniversaries = m_ui_contents.chckShowAnniversaries->isChecked();
    
    if (m_ui_filter.rbFilterTypeCategory->isChecked()) m_filterType = FT_Category;
    else if (m_ui_filter.rbFilterTypeCustomFieldName->isChecked()) m_filterType = FT_CustomField;
    else if (m_ui_filter.rbFilterTypeCustomFieldPrefix->isChecked()) m_filterType = FT_CustomFieldPrefix;
    else m_filterType = FT_Off;
    m_customFieldName = m_ui_filter.lineEditCustomFieldName->text();
    m_customFieldPrefix = m_ui_filter.lineEditCustomFieldPrefix->text();
    m_filterValue = m_ui_filter.lineEditFilterValue->text();

    m_isTodaysForeground = m_ui_appearance.chckTodaysForeground->isChecked();
    m_brushTodaysForeground.setColor(m_ui_appearance.colorbtnTodaysForeground->color());
    m_isTodaysBackground = m_ui_appearance.chckTodaysBackground->isChecked();
    m_brushTodaysBackground.setColor(m_ui_appearance.colorbtnTodaysBackground->color());
    m_isTodaysHighlightNoEvents = m_ui_appearance.chckTodaysHighlightNoEvent->isChecked();

    m_eventThreshold = m_ui_appearance.spinComingShowDays->value();
    m_highlightThreshold = m_ui_appearance.spinComingHighlightDays->value();
    m_isHighlightForeground = m_ui_appearance.chckComingHighlightForeground->isChecked();
    m_brushHighlightForeground.setColor(m_ui_appearance.colorbtnComingHighlightForeground->color());
    m_isHighlightBackground = m_ui_appearance.chckComingHighlightBackground->isChecked();
    m_brushHighlightBackground.setColor(m_ui_appearance.colorbtnComingHighlightBackground->color());
    m_isComingHighlightNoEvents = m_ui_appearance.chckComingHighlightNoEvent->isChecked();

    m_pastThreshold = m_ui_appearance.spinPastShowDays->value();
    AbstractAnnualEventEntry::setPastThreshold(m_pastThreshold);
    m_isPastForeground = m_ui_appearance.chckPastForeground->isChecked();
    m_brushPastForeground.setColor(m_ui_appearance.colorbtnPastForeground->color());
    m_isPastBackground = m_ui_appearance.chckPastBackground->isChecked();
    m_brushPastBackground.setColor(m_ui_appearance.colorbtnPastBackground->color());
    m_isPastHighlightNoEvents = m_ui_appearance.chckPastHighlightNoEvent->isChecked();

    m_selectedDateFormat = m_ui_appearance.cmbDateDisplayFormat->currentIndex();

    QTreeView *qTreeView = m_treeView->nativeWidget();
    m_columnWidthName = qTreeView->columnWidth(0);
    m_columnWidthAge = qTreeView->columnWidth(1);
    m_columnWidthDate = qTreeView->columnWidth(2);
    m_columnWidthWhen = qTreeView->columnWidth(3);


    KConfigGroup configGroup = config();

    if (m_eventDataSource == EDS_KABC) configGroup.writeEntry("Event Data Source", "KABC");
    else if (m_eventDataSource == EDS_Thunderbird) configGroup.writeEntry("Event Data Source", "Thunderbird");
    else configGroup.writeEntry("Event Data Source", "Akonadi");

    if (!m_akoCollection.isEmpty()) configGroup.writeEntry("Akonadi Collection", m_akoCollection);
    configGroup.writeEntry("Nameday String", m_namedayDateFieldString);
    configGroup.writeEntry("Anniversary String", m_anniversaryFieldString);
    if (m_namedayIdentificationMode == NIM_DateField) configGroup.writeEntry("Nameday Identification Mode", "Date Field");
    else if (m_namedayIdentificationMode == NIM_GivenName) configGroup.writeEntry("Nameday Identification Mode", "Given Name");
    else configGroup.writeEntry("Nameday Identification Mode", "Both");

    configGroup.writeEntry("Show Column Headers", m_showColumnHeaders);
    //configGroup.writeEntry("Show Name Column", m_showColName);
    configGroup.writeEntry("Show Age Column", m_showColAge);
    configGroup.writeEntry("Show Date Column", m_showColDate);
    configGroup.writeEntry("Show When Column", m_showColWhen);

    configGroup.writeEntry("Show Nicknames", m_showNicknames);

    configGroup.writeEntry("Show Namedays", m_showNamedays);
    configGroup.writeEntry("Nameday Calendar LangCode", m_curNamedayLangCode);
    if (m_namedayDisplayMode == NDM_IndividualEvents) configGroup.writeEntry("Nameday Display Mode", "IndividualEvents");
    else if (m_namedayDisplayMode == NDM_AllCalendarNames) configGroup.writeEntry("Nameday Display Mode", "AllCalendarNames");
    else configGroup.writeEntry("Nameday Display Mode", "AggregateEvents");

    configGroup.writeEntry("Show Anniversaries", m_showAnniversaries);

    if (m_filterType == FT_Category) configGroup.writeEntry("Filter Type", "Category");
    else if (m_filterType == FT_CustomField) configGroup.writeEntry("Filter Type", "Custom Field");
    else if (m_filterType == FT_CustomFieldPrefix) configGroup.writeEntry("Filter Type", "Custom Field Prefix");
    else configGroup.writeEntry("Filter Type", "Off");

    configGroup.writeEntry("Custom Field", m_customFieldName);
    configGroup.writeEntry("Custom Field Prefix", m_customFieldPrefix);
    configGroup.writeEntry("Filter Value", m_filterValue);

    configGroup.writeEntry("Event Threshold", m_eventThreshold);
    configGroup.writeEntry("Highlight Threshold", m_highlightThreshold);
    configGroup.writeEntry("Highlight Foreground Enabled", m_isHighlightForeground);
    configGroup.writeEntry("Highlight Foreground Color", m_brushHighlightForeground.color());
    configGroup.writeEntry("Highlight Background Enabled", m_isHighlightBackground);
    configGroup.writeEntry("Highlight Background Color", m_brushHighlightBackground.color());
    configGroup.writeEntry("Coming Highlight No Events", m_isComingHighlightNoEvents);

    configGroup.writeEntry("Todays Foreground Enabled", m_isTodaysForeground);
    configGroup.writeEntry("Todays Foreground Color", m_brushTodaysForeground.color());
    configGroup.writeEntry("Todays Background Enabled", m_isTodaysBackground);
    configGroup.writeEntry("Todays Background Color", m_brushTodaysBackground.color());
    configGroup.writeEntry("Todays Highlight No Events", m_isTodaysHighlightNoEvents);

    configGroup.writeEntry("Past Threshold", m_pastThreshold);
    configGroup.writeEntry("Past Foreground Enabled", m_isPastForeground);
    configGroup.writeEntry("Past Foreground Color", m_brushPastForeground.color());
    configGroup.writeEntry("Past Background Enabled", m_isPastBackground);
    configGroup.writeEntry("Past Background Color", m_brushPastBackground.color());
    configGroup.writeEntry("Past Highlight No Events", m_isPastHighlightNoEvents);

    configGroup.writeEntry("Date Format", m_selectedDateFormat);

    configGroup.writeEntry("Name Column Width", m_columnWidthName);
    configGroup.writeEntry("Age Column Width", m_columnWidthAge);
    configGroup.writeEntry("Date Column Width", m_columnWidthDate);
    configGroup.writeEntry("When Column Width", m_columnWidthWhen);

    m_curLangNamedayList = m_dataEngine_namedays->query(QString("NamedayList_%1").arg(m_curNamedayLangCode));

    m_dataEngine_akonadi->disconnectSource(m_dataSourceName, this);
    m_dataEngine_thunderbird->disconnectSource(m_dataSourceName, this);
    m_dataEngine_kabc->disconnectSource(m_dataSourceName, this);

    if (m_eventDataSource == EDS_Akonadi) {
        m_dataEngine_contacts = m_dataEngine_akonadi;
        m_dataSourceName = QString("Contacts_%1").arg(m_akoCollection);
    }
    else if (m_eventDataSource == EDS_Thunderbird) {
        m_dataEngine_contacts = m_dataEngine_thunderbird;
        m_dataSourceName = "ContactInfo";
    }
    else {
        m_dataEngine_contacts = m_dataEngine_kabc;
        m_dataSourceName = "ContactInfo";
    }
    
    m_dataEngine_contacts->connectSource(m_dataSourceName, this);
    qDebug() << "Connecting to datasource " << m_dataSourceName << ", size " << m_dataEngine_contacts->query(m_dataSourceName).size();

    updateEventList(m_dataEngine_contacts->query(m_dataSourceName));

    updateModels();
    update();

    emit configNeedsSaving();
}

void BirthdayListApplet::plasmaThemeChanged() {
    usePlasmaThemeColors();
}
    
void BirthdayListApplet::createConfigurationInterface(KConfigDialog *parent) {
    QWidget *dataSourceWidget = new QWidget;
    QWidget *contentsWidget = new QWidget;
    QWidget *filterWidget = new QWidget;
    QWidget *appearanceWidget = new QWidget;

    m_ui_datasource.setupUi(dataSourceWidget);
    m_ui_contents.setupUi(contentsWidget);
    m_ui_filter.setupUi(filterWidget);
    m_ui_appearance.setupUi(appearanceWidget);

    parent->setButtons(KDialog::Ok | KDialog::Cancel);
    parent->addPage(dataSourceWidget, i18n("Data source"), "preferences-contact-list");
    parent->addPage(contentsWidget, i18n("Contents"), "view-form-table");
    parent->addPage(filterWidget, i18n("Filter"), "view-filter");
    parent->addPage(appearanceWidget, i18n("Appearance"), "preferences-desktop-theme");

    m_ui_datasource.cmbDataSource->clear();
    if (m_dataEngine_kabc->isValid()) {
        m_ui_datasource.cmbDataSource->addItem(i18n("KDE Address Book"), QVariant("KABC"));
        if (m_eventDataSource == EDS_KABC) m_ui_datasource.cmbDataSource->setCurrentIndex(m_ui_datasource.cmbDataSource->count()-1);
    }

    if (m_dataEngine_akonadi->isValid()) {
        m_ui_datasource.cmbDataSource->addItem(i18n("Akonadi"), QVariant("Akonadi"));
        if (m_eventDataSource == EDS_Akonadi) m_ui_datasource.cmbDataSource->setCurrentIndex(m_ui_datasource.cmbDataSource->count()-1);
    }

    if (m_dataEngine_thunderbird->isValid()) {
        m_ui_datasource.cmbDataSource->addItem(i18n("Thunderbird"), QVariant("Thunderbird"));
        if (m_eventDataSource == EDS_Thunderbird) m_ui_datasource.cmbDataSource->setCurrentIndex(m_ui_datasource.cmbDataSource->count()-1);
    }
    dataSourceChanged(m_ui_datasource.cmbDataSource->currentText());

    m_ui_datasource.cmbAkoCollection->clear();
    Plasma::DataEngine::Data data_collections = m_dataEngine_akonadi->query("Collections");
    QHashIterator<QString, QVariant> collectionsIt(data_collections);
    while (collectionsIt.hasNext()) {
        collectionsIt.next();
        QVariantHash collectionInfo = collectionsIt.value().toHash();
        m_ui_datasource.cmbAkoCollection->addItem(collectionInfo["Name"].toString(), collectionInfo["Resource"]);
        if (collectionInfo["Resource"].toString() == m_akoCollection) {
            m_ui_datasource.cmbAkoCollection->setCurrentIndex(m_ui_datasource.cmbAkoCollection->count()-1);
        }
    }
    if (m_ui_datasource.cmbAkoCollection->count() == 0) {
        m_ui_datasource.cmbAkoCollection->addItem(i18nc("No Akonadi collections", "No collections available"));
        m_ui_datasource.cmbAkoCollection->setEnabled(false);
    }
    else m_ui_datasource.cmbAkoCollection->setEnabled(true);

    m_ui_datasource.rbNamedayDateField->setChecked(m_namedayIdentificationMode == NIM_DateField);
    m_ui_datasource.lineEditNamedayDateField->setText(m_namedayDateFieldString);
    m_ui_datasource.rbNamedayNameField->setChecked(m_namedayIdentificationMode == NIM_GivenName);
    m_ui_datasource.lineEditAnniversaryField->setText(m_anniversaryFieldString);
    m_ui_datasource.rbNamedayBothFields->setChecked(m_namedayIdentificationMode == NIM_Both);

    m_ui_contents.chckShowColumnHeaders->setChecked(m_showColumnHeaders);
    m_ui_contents.chckShowColName->setChecked(m_showColName);
    m_ui_contents.chckShowColAge->setChecked(m_showColAge);
    m_ui_contents.chckShowColDate->setChecked(m_showColDate);
    m_ui_contents.chckShowColWhen->setChecked(m_showColWhen);

    m_ui_contents.chckShowNicknames->setChecked(m_showNicknames);

    m_ui_contents.chckShowNamedays->setChecked(m_showNamedays);
    m_ui_contents.rbNamedayShowIndEvents->setChecked(m_namedayDisplayMode == NDM_IndividualEvents);
    m_ui_contents.rbNamedayShowAllFromCal->setChecked(m_namedayDisplayMode == NDM_AllCalendarNames);
    m_ui_contents.rbNamedayShowAggrEvents->setChecked(m_namedayDisplayMode == NDM_AggregateEvents);

    m_ui_contents.cmbNamedayCalendar->clear();
    m_ui_contents.cmbNamedayCalendar->addItems(m_namedayLangStrings);
    if (m_namedayLangCodes.contains(m_curNamedayLangCode)) {
        m_ui_contents.cmbNamedayCalendar->setCurrentIndex(m_namedayLangCodes.indexOf(m_curNamedayLangCode));
    } else m_ui_contents.cmbNamedayCalendar->setCurrentIndex(0);
    m_ui_contents.chckShowAnniversaries->setChecked(m_showAnniversaries);
    
    m_ui_filter.rbFilterTypeOff->setChecked(m_filterType == FT_Off);
    m_ui_filter.rbFilterTypeCategory->setChecked(m_filterType == FT_Category);
    m_ui_filter.rbFilterTypeCustomFieldName->setChecked(m_filterType == FT_CustomField);
    m_ui_filter.rbFilterTypeCustomFieldPrefix->setChecked(m_filterType == FT_CustomFieldPrefix);
    m_ui_filter.lineEditCustomFieldName->setText(m_customFieldName);
    m_ui_filter.lineEditCustomFieldPrefix->setText(m_customFieldPrefix);
    m_ui_filter.lineEditFilterValue->setText(m_filterValue);

    m_ui_appearance.chckTodaysForeground->setChecked(m_isTodaysForeground);
    m_ui_appearance.colorbtnTodaysForeground->setColor(m_brushTodaysForeground.color());
    m_ui_appearance.chckTodaysBackground->setChecked(m_isTodaysBackground);
    m_ui_appearance.colorbtnTodaysBackground->setColor(m_brushTodaysBackground.color());
    m_ui_appearance.chckTodaysHighlightNoEvent->setChecked(m_isTodaysHighlightNoEvents);

    m_ui_appearance.spinComingShowDays->setValue(m_eventThreshold);
    m_ui_appearance.spinComingHighlightDays->setValue(m_highlightThreshold);
    m_ui_appearance.chckComingHighlightForeground->setChecked(m_isHighlightForeground);
    m_ui_appearance.colorbtnComingHighlightForeground->setColor(m_brushHighlightForeground.color());
    m_ui_appearance.chckComingHighlightBackground->setChecked(m_isHighlightBackground);
    m_ui_appearance.colorbtnComingHighlightBackground->setColor(m_brushHighlightBackground.color());
    m_ui_appearance.chckComingHighlightNoEvent->setChecked(m_isComingHighlightNoEvents);

    m_ui_appearance.spinPastShowDays->setValue(m_pastThreshold);
    m_ui_appearance.chckPastForeground->setChecked(m_isPastForeground);
    m_ui_appearance.colorbtnPastForeground->setColor(m_brushPastForeground.color());
    m_ui_appearance.chckPastBackground->setChecked(m_isPastBackground);
    m_ui_appearance.colorbtnPastBackground->setColor(m_brushPastBackground.color());
    m_ui_appearance.chckPastHighlightNoEvent->setChecked(m_isPastHighlightNoEvents);

    m_ui_appearance.cmbDateDisplayFormat->setCurrentIndex(m_selectedDateFormat);


    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.lblNamedayIdentifyBy, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.rbNamedayNameField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.cmbNamedayNameField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.rbNamedayDateField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.lineEditNamedayDateField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowAnniversaries, SIGNAL(toggled(bool)), m_ui_datasource.lblAnniversaryField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowAnniversaries, SIGNAL(toggled(bool)), m_ui_datasource.lineEditAnniversaryField, SLOT(setEnabled(bool)));
    connect(m_ui_datasource.cmbDataSource, SIGNAL(currentIndexChanged(QString)), this, SLOT(dataSourceChanged(QString)));

    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));

    parent->resize(parent->minimumSizeHint());
}

void BirthdayListApplet::dataSourceChanged(const QString &name) {
    m_ui_datasource.lblAkoCollection->setVisible(name == "Akonadi");
    m_ui_datasource.cmbAkoCollection->setVisible(name == "Akonadi");
}


void BirthdayListApplet::updateEventList(const Plasma::DataEngine::Data &data) {
    // since we are going to re-create all entries again, delete currently existing ones
    qDebug() << "Updating Event List, size of data: " << data.size();

    foreach(AbstractAnnualEventEntry *oldListEntry, m_listEntries) {
        delete oldListEntry;
    }
    m_listEntries.clear();

    // store nameday entries separately (so that they can be aggregated)
    QList<NamedayEntry *> namedayEntries;

    // iterate over the contacts from the data engine and create appropriate list entries
    QHashIterator<QString, QVariant> contactIt(data);
    while (contactIt.hasNext()) {
        contactIt.next();
        QVariantHash contactInfo = contactIt.value().toHash();

        QString contactName = contactInfo["Name"].toString();
        QString contactNickname = contactInfo["Nickname"].toString();
        if (m_showNicknames && !contactNickname.isEmpty()) contactName = contactNickname;
        QDate contactBirthday = contactInfo["Birthday"].toDate();
        QDate contactNameday;
        if (m_namedayIdentificationMode == NIM_DateField) contactNameday = getContactDateField(contactInfo, m_namedayDateFieldString);
        else if (m_namedayIdentificationMode == NIM_GivenName) contactNameday = getNamedayByGivenName(contactInfo["Given name"].toString());
        else {
            contactNameday = getContactDateField(contactInfo, m_namedayDateFieldString);
            if (!contactNameday.isValid()) contactNameday = getNamedayByGivenName(contactInfo["Given name"].toString());
        }

        QDate contactAnniversary = getContactDateField(contactInfo, m_anniversaryFieldString);
        QString contactEmail = contactInfo["Email"].toString();
        QString contactUrl = contactInfo["Homepage"].toString();
        
        if (m_filterType == FT_Off) {
            // do nothing; this check comes first as it is the most likely selected option
        } else if (m_filterType == FT_Category) {
            if (!contactInfo["Categories"].toStringList().contains(m_filterValue)) continue;
        } else if (m_filterType == FT_CustomField) {
            if (contactInfo[QString("Custom_%1").arg(m_customFieldName)].toString() != m_filterValue) continue;
        } else if (m_filterType == FT_CustomFieldPrefix) {
            bool filterValueFound = false;
            QString customFieldNamePattern = QString("Custom_%1").arg(m_customFieldPrefix);
            
            QStringList contactFields = contactInfo.keys();
            foreach (QString field, contactFields) {
                if (field.startsWith(customFieldNamePattern) && contactInfo[field] == m_filterValue) {
                    filterValueFound = true;
                    break;
                }
            }
            
            if (!filterValueFound) continue;
        }
        
        //qDebug() << "KABC contact:" << contactName << ", B" << contactBirthday << ", N" << contactNameday << ", A" << contactAnniversary;

        if (contactBirthday.isValid()) {
            m_listEntries.append(new BirthdayEntry(contactName, contactBirthday, contactEmail, contactUrl));
        }
        if (m_showNamedays && contactNameday.isValid()) {
            QDate firstNameday = contactNameday;
            if (contactBirthday.isValid()) {
                firstNameday.setDate(contactBirthday.year(), contactNameday.month(), contactNameday.day());
                if (firstNameday < contactBirthday) firstNameday = firstNameday.addYears(1);
            }
            namedayEntries.append(new NamedayEntry(contactName, firstNameday, contactEmail, contactUrl));
        }
        if (m_showAnniversaries && contactAnniversary.isValid()) {
            m_listEntries.append(new AnniversaryEntry(contactName, contactAnniversary, contactEmail, contactUrl));
        }
    }

    // if desired, join together nameday entries from the same day
    if (m_namedayDisplayMode == NDM_AggregateEvents || m_namedayDisplayMode == NDM_AllCalendarNames) {
        qSort(namedayEntries.begin(), namedayEntries.end(), AbstractAnnualEventEntry::lessThan);
        int curYear = QDate::currentDate().year();
        QMap<QDate, AggregatedNamedayEntry*> aggregatedEntries;

        // if all calendar names are to be shown, prepare entries for the visualised period
        if (m_namedayDisplayMode == NDM_AllCalendarNames) {
            QDate initialDate = QDate::currentDate().addDays(-m_pastThreshold);
            QDate finalDate = initialDate.addYears(1);

            for (QDate date=initialDate; date<finalDate; date = date.addDays(1)) {
                aggregatedEntries[date] = new AggregatedNamedayEntry(getNamedayString(date), date);
            }
        }

        foreach(NamedayEntry *namedayEntry, namedayEntries) {
            const QDate namedayDate = namedayEntry->date();
            QDate curYearDate(curYear, namedayDate.month(), namedayDate.day());

            AggregatedNamedayEntry *aggregatedEntry = 0;
            if (aggregatedEntries.contains(curYearDate))
                aggregatedEntry = aggregatedEntries[curYearDate];
            else {
                aggregatedEntry = new AggregatedNamedayEntry(getNamedayString(curYearDate), curYearDate);
                aggregatedEntries[curYearDate] = aggregatedEntry;
            }
            aggregatedEntry->addNamedayEntry(namedayEntry);
        }

        foreach(AggregatedNamedayEntry *entry, aggregatedEntries) {
            m_listEntries.append(entry);
        }
    } else if (m_namedayDisplayMode == NDM_IndividualEvents) {

        foreach(NamedayEntry *entry, namedayEntries) {
            m_listEntries.append(entry);
        }
    }

    // sort the entries by date
    qSort(m_listEntries.begin(), m_listEntries.end(), AbstractAnnualEventEntry::lessThan);

    qDebug() << "New list of entries created, total count" << m_listEntries.size();
}

QDate BirthdayListApplet::getContactDateField(const QVariantHash &contactInfo, QString fieldName) {
    if (contactInfo.contains(fieldName)) return contactInfo[fieldName].toDate();

    QString kabcCustomFieldName;
    kabcCustomFieldName = QString("Custom_KADDRESSBOOK-%1").arg(fieldName);
    if (contactInfo.contains(kabcCustomFieldName)) return contactInfo[kabcCustomFieldName].toDate();
    
    kabcCustomFieldName = QString("Custom_KADDRESSBOOK-X-%1").arg(fieldName);
    if (contactInfo.contains(kabcCustomFieldName)) return contactInfo[kabcCustomFieldName].toDate();
    
    return QDate();
}

QDate BirthdayListApplet::getNamedayByGivenName(QString givenName) {
    if (givenName.isEmpty()) return QDate();

    QDate initialDate = QDate::currentDate().addDays(-m_pastThreshold);
    QDate finalDate = initialDate.addYears(1);
    for (QDate date=initialDate; date<finalDate; date = date.addDays(1)) {
        QStringList calendarNames = getNamedayString(date).split(QRegExp("\\W+"), QString::SkipEmptyParts);

        // if the name can be found, return the nameday in the future
        // (if the contact has a birthday, the date will be moved to his first nameday
        // so that the correct age can be shown; othewise we'll know that the age is unknown)
        if (calendarNames.contains(givenName)) return date.addYears(1);
    }

    return QDate();
}

QString BirthdayListApplet::getNamedayString(QDate date) {
    QVariant namedayStringEntry = m_curLangNamedayList[date.toString("MM-dd")];
    if (namedayStringEntry.isValid()) return namedayStringEntry.toString();
    else return date.toString(m_possibleDateFormats[m_selectedDateFormat]);
}

void BirthdayListApplet::updateModels() {
    qDebug() << "Creating BirthdayList model";

    m_model.clear();
    QStringList headerTitles;
    headerTitles << i18n("Name") << i18n("Age") << i18n("Date") << i18n("When") << "" << "";
    m_model.setHorizontalHeaderLabels(headerTitles);

    QString dateFormat = m_possibleDateFormats[m_selectedDateFormat];

    QStandardItem *parentItem = m_model.invisibleRootItem();

    foreach(const AbstractAnnualEventEntry *entry, m_listEntries) {
        int remainingDays = entry->remainingDays();
        bool showEvent = (remainingDays >= 0 && remainingDays <= m_eventThreshold) ||
                (remainingDays <= 0 && remainingDays >= -m_pastThreshold);

        if (showEvent) {
            QList<QStandardItem*> items;
            entry->createModelItems(items, dateFormat);
            for (int i=0; i<items.size(); ++i) setModelItemColors(entry, items[i], i);

            parentItem->appendRow(items);
        }
    }

    setTreeColumnWidths();
    usePlasmaThemeColors();
}

void BirthdayListApplet::setModelItemColors(const AbstractAnnualEventEntry *entry, QStandardItem *item, int colNum) {
    item->setEditable(false);

    Q_UNUSED(colNum)
    //if (colNum>0) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    if (entry->remainingDays() == 0) {
        if (entry->hasEvent() || m_isTodaysHighlightNoEvents) {
            if (m_isTodaysForeground) item->setForeground(m_brushTodaysForeground);
            if (m_isTodaysBackground) item->setBackground(m_brushTodaysBackground);
        }
    } else if (entry->remainingDays() < 0) {
        if (entry->hasEvent() || m_isPastHighlightNoEvents) {
            if (m_isPastForeground) item->setForeground(m_brushPastForeground);
            if (m_isPastBackground) item->setBackground(m_brushPastBackground);
        }
    } else if (entry->remainingDays() <= m_highlightThreshold) {
        if (entry->hasEvent() || m_isComingHighlightNoEvents) {
            if (m_isHighlightForeground) item->setForeground(m_brushHighlightForeground);
            if (m_isHighlightBackground) item->setBackground(m_brushHighlightBackground);
        }
    }

    for (int row = 0; row < item->rowCount(); ++row) {
        for (int col = 0; col < item->columnCount(); ++col) {
            QStandardItem *child = item->child(row, col);
            if (child) setModelItemColors(entry, child, col);
        }
    }
}

void BirthdayListApplet::setTreeColumnWidths() {
    if (!m_treeView) return;
    QTreeView *qTreeView = m_treeView->nativeWidget();

    qTreeView->setHeaderHidden(!m_showColumnHeaders);
    qTreeView->setColumnHidden(0, !m_showColName);
    qTreeView->setColumnHidden(1, !m_showColAge);
    qTreeView->setColumnHidden(2, !m_showColDate);
    qTreeView->setColumnHidden(3, !m_showColWhen);
    qTreeView->setColumnHidden(4, true);
    qTreeView->setColumnHidden(5, true);

    if (m_columnWidthName < 10) qTreeView->resizeColumnToContents(0);
    else qTreeView->setColumnWidth(0, m_columnWidthName);

    if (m_columnWidthAge < 10) qTreeView->resizeColumnToContents(1);
    else qTreeView->setColumnWidth(1, m_columnWidthAge);

    if (m_columnWidthDate < 10) qTreeView->resizeColumnToContents(2);
    else qTreeView->setColumnWidth(2, m_columnWidthDate);

    if (m_columnWidthWhen < 10) qTreeView->resizeColumnToContents(3);
    else qTreeView->setColumnWidth(3, m_columnWidthWhen);
}

void BirthdayListApplet::usePlasmaThemeColors() {
    //QFont font = Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont);

    // Get theme colors
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    QColor baseColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor);
    QColor altBaseColor = baseColor.darker();
    QColor buttonColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor);
    baseColor.setAlpha(50);
    altBaseColor.setAlpha(50);
    buttonColor.setAlpha(130);

    // Create palette with the used theme colors
    QPalette p = palette();
    p.setColor(QPalette::Background, baseColor);
    p.setColor(QPalette::Base, baseColor);
    p.setColor(QPalette::AlternateBase, altBaseColor);
    p.setColor(QPalette::Button, buttonColor);
    p.setColor(QPalette::Foreground, textColor);
    p.setColor(QPalette::Text, textColor);

    if (m_treeView) {
        QTreeView *treeView = m_treeView->nativeWidget();
        treeView->setPalette(p);
        treeView->header()->setPalette(p);
    }

    QBrush textBrush = QBrush(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor));
    for (int i = 0; i < m_model.columnCount(); ++i) {
        m_model.horizontalHeaderItem(i)->setForeground(textBrush);
    }
}

void BirthdayListApplet::sendEmail() {
  KToolInvocation::invokeMailer(getSelectedLineItem(4), "");
}

void BirthdayListApplet::visitHomepage() {
  KToolInvocation::invokeBrowser(getSelectedLineItem(5));
}

void BirthdayListApplet::about() {
  KAboutApplicationDialog *aboutDialog = new KAboutApplicationDialog(m_aboutData);
  connect(aboutDialog, SIGNAL(finished()), aboutDialog, SLOT(deleteLater()));
  aboutDialog->show();
}


#include "birthdaylist_applet.moc"
