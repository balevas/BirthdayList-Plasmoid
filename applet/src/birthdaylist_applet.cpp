/**
 * @file    birthdaylist_applet.cpp
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

#include "birthdaylist_applet.h"
#include "birthdaylistentry.h"

#include <QTreeView>
#include <QGraphicsLinearLayout>
#include <QStackedWidget>

#include <KConfigDialog>
#include <KDebug>

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
m_akoColNamedayDateFieldString("Nameday"),
m_akoColIsNamedayByGivenNameField(false),
m_akoColAnniversaryFieldString("Anniversary"),
m_kabcNamedayDateFieldString("Nameday"),
m_kabcIsNamedayByGivenNameField(false),
m_kabcAnniversaryFieldString("Anniversary"),
m_model(0, 3),
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
m_isTodaysForeground(false),
m_brushTodaysForeground(QColor(255, 255, 255)),
m_isTodaysBackground(true),
m_brushTodaysBackground(QColor(128, 0, 0)),
m_eventThreshold(30),
m_highlightThreshold(2),
m_isHighlightForeground(false),
m_brushHighlightForeground(QColor(255, 255, 255)),
m_isHighlightBackground(true),
m_brushHighlightBackground(QColor(128, 0, 0)),
m_pastThreshold(2),
m_isPastForeground(false),
m_brushPastForeground(QColor(0, 0, 0)),
m_isPastBackground(true),
m_brushPastBackground(QColor(160, 0, 0)),
m_selectedDateFormat(0),
m_columnWidthName(0),
m_columnWidthAge(0),
m_columnWidthDate(0),
m_columnWidthWhen(0) {
    m_possibleDateFormats << "d. M." << "dd. MM."
            << "d-M" << "dd-MM" << "M-d" << "MM-dd"
            << "d/M" << "dd/MM" << "M/d" << "MM/dd";
    setBackgroundHints(DefaultBackground);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
    resize(250, 300);
}

BirthdayListApplet::~BirthdayListApplet() {
    delete m_graphicsWidget;
}

void BirthdayListApplet::init() {
    setPopupIcon(KIcon("bl_cookie", NULL));

    KConfigGroup configGroup = config();
    QString eventDataSource = configGroup.readEntry("Event Data Source", "");
    if (eventDataSource == "Akonadi") m_eventDataSource = EDS_Akonadi;
    else if (eventDataSource == "Thunderbird") m_eventDataSource = EDS_Thunderbird;
    else m_eventDataSource = EDS_KABC;

    m_akoCollection = configGroup.readEntry("Akonadi Collection", "");
    m_akoColNamedayDateFieldString = configGroup.readEntry("Akonadi Nameday String", "Nameday");
    m_akoColIsNamedayByGivenNameField = configGroup.readEntry("Akonadi Nameday By GivenName", false);
    m_akoColAnniversaryFieldString = configGroup.readEntry("Akonadi Anniversary String", "Anniversary");

    m_kabcNamedayDateFieldString = configGroup.readEntry("KABC Nameday String", "Nameday");
    m_kabcIsNamedayByGivenNameField = configGroup.readEntry("KABC Nameday By GivenName", false);
    m_kabcAnniversaryFieldString = configGroup.readEntry("KABC Anniversary String", "Anniversary");
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

    QString namedayDateFieldString, anniversaryFieldString;

    if (m_eventDataSource == EDS_Akonadi) {
        m_dataEngine_contacts = m_dataEngine_akonadi;
        m_dataEngine_akonadi->query(QString("SetCurrentCollection_%1").arg(m_akoCollection));
        namedayDateFieldString = m_akoColNamedayDateFieldString;
        anniversaryFieldString = m_akoColAnniversaryFieldString;
    }
    else if (m_eventDataSource == EDS_Thunderbird) {
        m_dataEngine_contacts = m_dataEngine_thunderbird;
    }
    else {
        m_dataEngine_contacts = m_dataEngine_kabc;
        namedayDateFieldString = m_kabcNamedayDateFieldString;
        anniversaryFieldString = m_kabcAnniversaryFieldString;
    }

    m_dataEngine_contacts->query(QString("SetNamedayField_%1").arg(namedayDateFieldString));
    m_dataEngine_contacts->query(QString("SetAnniversaryField_%1").arg(anniversaryFieldString));
    m_dataEngine_contacts->connectSource("ContactInfo", this);

    Plasma::DataEngine* timeEngine = dataEngine("time");
    if (timeEngine) {
        timeEngine->connectSource("UTC", this, 360000, Plasma::AlignToHour);
    } else {
        kDebug() << "Warning: Could not load time dataEngine - no nightly update possible";
    }

    m_showNicknames = configGroup.readEntry("Show Nicknames", true);

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

    m_eventThreshold = configGroup.readEntry("Event Threshold", 30);
    m_highlightThreshold = configGroup.readEntry("Highlight Threshold", 2);
    m_isHighlightForeground = configGroup.readEntry("Highlight Foreground Enabled", false);
    QColor highlightForeground = configGroup.readEntry("Highlight Foreground Color", QColor(255, 255, 255));
    m_brushHighlightForeground = QBrush(highlightForeground);
    m_isHighlightBackground = configGroup.readEntry("Highlight Background Enabled", true);
    QColor highlightBackground = configGroup.readEntry("Highlight Background Color", QColor(128, 0, 0));
    m_brushHighlightBackground = QBrush(highlightBackground);

    m_pastThreshold = configGroup.readEntry("Past Threshold", 2);
    AbstractAnnualEventEntry::setPastThreshold(m_pastThreshold);
    m_isPastForeground = configGroup.readEntry("Past Foreground Enabled", false);
    QColor pastForeground = configGroup.readEntry("Past Foreground Color", QColor(0, 0, 0));
    m_brushPastForeground = QBrush(pastForeground);
    m_isPastBackground = configGroup.readEntry("Past Background Enabled", false);
    QColor pastBackground = configGroup.readEntry("Past Background Color", QColor(160, 160, 160));
    m_brushPastBackground = QBrush(pastBackground);

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
        m_graphicsWidget->setMinimumSize(225, 150);
        m_graphicsWidget->setPreferredSize(350, 200);

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

        QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Vertical);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        layout->addItem(m_treeView);
        m_graphicsWidget->setLayout(layout);
    }
    return m_graphicsWidget;
}

void BirthdayListApplet::dataUpdated(const QString &name, const Plasma::DataEngine::Data &data) {
    kDebug() << "Update of birthday list entries, triggered by data source" << name;

    if (name == "ContactInfo") {
        updateEventList(data);
    } else if (name == UTC_SOURCE) {
        updateEventList(m_dataEngine_contacts->query("ContactInfo"));
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
    m_akoColNamedayDateFieldString = m_ui_datasource.lineEditAkoNamedayDateField->text();
    m_akoColIsNamedayByGivenNameField = m_ui_datasource.rbAkoNamedayNameField->isChecked();
    m_akoColAnniversaryFieldString = m_ui_datasource.lineEditAkoAnniversaryField->text();

    m_kabcNamedayDateFieldString = m_ui_datasource.lineEditKabNamedayDateField->text();
    m_kabcIsNamedayByGivenNameField = m_ui_datasource.rbKabNamedayNameField->isChecked();
    m_kabcAnniversaryFieldString = m_ui_datasource.lineEditKabAnniversaryField->text();

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

    m_isTodaysForeground = m_ui_appearance.chckTodaysForeground->isChecked();
    m_brushTodaysForeground.setColor(m_ui_appearance.colorbtnTodaysForeground->color());
    m_isTodaysBackground = m_ui_appearance.chckTodaysBackground->isChecked();
    m_brushTodaysBackground.setColor(m_ui_appearance.colorbtnTodaysBackground->color());

    m_eventThreshold = m_ui_appearance.spinComingShowDays->value();
    m_highlightThreshold = m_ui_appearance.spinComingHighlightDays->value();
    m_isHighlightForeground = m_ui_appearance.chckComingHighlightForeground->isChecked();
    m_brushHighlightForeground.setColor(m_ui_appearance.colorbtnComingHighlightForeground->color());
    m_isHighlightBackground = m_ui_appearance.chckComingHighlightBackground->isChecked();
    m_brushHighlightBackground.setColor(m_ui_appearance.colorbtnComingHighlightBackground->color());

    m_pastThreshold = m_ui_appearance.spinPastShowDays->value();
    AbstractAnnualEventEntry::setPastThreshold(m_pastThreshold);
    m_isPastForeground = m_ui_appearance.chckPastForeground->isChecked();
    m_brushPastForeground.setColor(m_ui_appearance.colorbtnPastForeground->color());
    m_isPastBackground = m_ui_appearance.chckPastBackground->isChecked();
    m_brushPastBackground.setColor(m_ui_appearance.colorbtnPastBackground->color());

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
    configGroup.writeEntry("Akonadi Nameday String", m_akoColNamedayDateFieldString);
    configGroup.writeEntry("Akonadi Nameday By GivenName", m_akoColIsNamedayByGivenNameField);
    configGroup.writeEntry("Akonadi Anniversary String", m_akoColAnniversaryFieldString);

    configGroup.writeEntry("KABC Nameday String", m_kabcNamedayDateFieldString);
    configGroup.writeEntry("KABC Nameday By GivenName", m_kabcIsNamedayByGivenNameField);
    configGroup.writeEntry("KABC Anniversary String", m_kabcAnniversaryFieldString);

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

    configGroup.writeEntry("Event Threshold", m_eventThreshold);
    configGroup.writeEntry("Highlight Threshold", m_highlightThreshold);
    configGroup.writeEntry("Highlight Foreground Enabled", m_isHighlightForeground);
    configGroup.writeEntry("Highlight Foreground Color", m_brushHighlightForeground.color());
    configGroup.writeEntry("Highlight Background Enabled", m_isHighlightBackground);
    configGroup.writeEntry("Highlight Background Color", m_brushHighlightBackground.color());

    configGroup.writeEntry("Todays Foreground Enabled", m_isTodaysForeground);
    configGroup.writeEntry("Todays Foreground Color", m_brushTodaysForeground.color());
    configGroup.writeEntry("Todays Background Enabled", m_isTodaysBackground);
    configGroup.writeEntry("Todays Background Color", m_brushTodaysBackground.color());

    configGroup.writeEntry("Past Threshold", m_pastThreshold);
    configGroup.writeEntry("Past Foreground Enabled", m_isPastForeground);
    configGroup.writeEntry("Past Foreground Color", m_brushPastForeground.color());
    configGroup.writeEntry("Past Background Enabled", m_isPastBackground);
    configGroup.writeEntry("Past Background Color", m_brushPastBackground.color());

    configGroup.writeEntry("Date Format", m_selectedDateFormat);

    configGroup.writeEntry("Name Column Width", m_columnWidthName);
    configGroup.writeEntry("Age Column Width", m_columnWidthAge);
    configGroup.writeEntry("Date Column Width", m_columnWidthDate);
    configGroup.writeEntry("When Column Width", m_columnWidthWhen);

    m_curLangNamedayList = m_dataEngine_namedays->query(QString("NamedayList_%1").arg(m_curNamedayLangCode));

    QString namedayDateFieldString, anniversaryFieldString;

    if (m_eventDataSource == EDS_Akonadi) {
        m_dataEngine_contacts = m_dataEngine_akonadi;
        m_dataEngine_akonadi->query(QString("SetCurrentCollection_%1").arg(m_akoCollection));
        namedayDateFieldString = m_akoColNamedayDateFieldString;
        anniversaryFieldString = m_akoColAnniversaryFieldString;
    }
    else if (m_eventDataSource == EDS_Thunderbird) {
        m_dataEngine_contacts = m_dataEngine_thunderbird;
    }
    else {
        m_dataEngine_contacts = m_dataEngine_kabc;
        namedayDateFieldString = m_kabcNamedayDateFieldString;
        anniversaryFieldString = m_kabcAnniversaryFieldString;
    }

    m_dataEngine_akonadi->disconnectSource("ContactInfo", this);
    m_dataEngine_thunderbird->disconnectSource("ContactInfo", this);
    m_dataEngine_kabc->disconnectSource("ContactInfo", this);

    m_dataEngine_contacts->query(QString("SetNamedayField_%1").arg(namedayDateFieldString));
    m_dataEngine_contacts->query(QString("SetAnniversaryField_%1").arg(anniversaryFieldString));
    m_dataEngine_contacts->connectSource("ContactInfo", this);

    updateEventList(m_dataEngine_contacts->query("ContactInfo"));

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
    QWidget *appearanceWidget = new QWidget;

    m_ui_datasource.setupUi(dataSourceWidget);
    m_ui_contents.setupUi(contentsWidget);
    m_ui_appearance.setupUi(appearanceWidget);

    parent->setButtons(KDialog::Ok | KDialog::Cancel);
    parent->addPage(dataSourceWidget, i18n("Data source"), "preferences-contact-list");
    parent->addPage(contentsWidget, i18n("Contents"), "view-form-table");
    parent->addPage(appearanceWidget, i18n("Appearance"), "preferences-desktop-theme");

    m_ui_datasource.cmbDataSource->clear();
    if (m_dataEngine_kabc->isValid()) {
        m_ui_datasource.cmbDataSource->addItem(i18n("KDE Address Book"), QVariant("KABC"));
        if (m_eventDataSource == EDS_KABC) m_ui_datasource.cmbDataSource->setCurrentIndex(m_ui_datasource.cmbDataSource->count()-1);
    }
    else m_ui_datasource.stckWidgetDSConfPages->removeWidget(m_ui_datasource.pageKabConfig);

    if (m_dataEngine_akonadi->isValid()) {
        m_ui_datasource.cmbDataSource->addItem(i18n("Akonadi"), QVariant("Akonadi"));
        if (m_eventDataSource == EDS_Akonadi) m_ui_datasource.cmbDataSource->setCurrentIndex(m_ui_datasource.cmbDataSource->count()-1);
    }
    else m_ui_datasource.stckWidgetDSConfPages->removeWidget(m_ui_datasource.pageAkonadiConfig);

    if (m_dataEngine_thunderbird->isValid()) {
        m_ui_datasource.cmbDataSource->addItem(i18n("Thunderbird"), QVariant("Thunderbird"));
        if (m_eventDataSource == EDS_Thunderbird) m_ui_datasource.cmbDataSource->setCurrentIndex(m_ui_datasource.cmbDataSource->count()-1);
    }
    else m_ui_datasource.stckWidgetDSConfPages->removeWidget(m_ui_datasource.pageThunderbirdConfig);

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

    m_ui_datasource.lineEditAkoNamedayDateField->setText(m_akoColNamedayDateFieldString);
    m_ui_datasource.rbAkoNamedayNameField->setChecked(m_akoColIsNamedayByGivenNameField);
    m_ui_datasource.lineEditAkoAnniversaryField->setText(m_akoColAnniversaryFieldString);

    m_ui_datasource.lineEditKabNamedayDateField->setText(m_kabcNamedayDateFieldString);
    m_ui_datasource.rbKabNamedayNameField->setChecked(m_kabcIsNamedayByGivenNameField);
    m_ui_datasource.lineEditKabAnniversaryField->setText(m_kabcAnniversaryFieldString);

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

    m_ui_appearance.chckTodaysForeground->setChecked(m_isTodaysForeground);
    m_ui_appearance.colorbtnTodaysForeground->setColor(m_brushTodaysForeground.color());
    m_ui_appearance.chckTodaysBackground->setChecked(m_isTodaysBackground);
    m_ui_appearance.colorbtnTodaysBackground->setColor(m_brushTodaysBackground.color());

    m_ui_appearance.spinComingShowDays->setValue(m_eventThreshold);
    m_ui_appearance.spinComingHighlightDays->setValue(m_highlightThreshold);
    m_ui_appearance.chckComingHighlightForeground->setChecked(m_isHighlightForeground);
    m_ui_appearance.colorbtnComingHighlightForeground->setColor(m_brushHighlightForeground.color());
    m_ui_appearance.chckComingHighlightBackground->setChecked(m_isHighlightBackground);
    m_ui_appearance.colorbtnComingHighlightBackground->setColor(m_brushHighlightBackground.color());

    m_ui_appearance.spinPastShowDays->setValue(m_pastThreshold);
    m_ui_appearance.chckPastForeground->setChecked(m_isPastForeground);
    m_ui_appearance.colorbtnPastForeground->setColor(m_brushPastForeground.color());
    m_ui_appearance.chckPastBackground->setChecked(m_isPastBackground);
    m_ui_appearance.colorbtnPastBackground->setColor(m_brushPastBackground.color());

    m_ui_appearance.cmbDateDisplayFormat->setCurrentIndex(m_selectedDateFormat);


    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.lblKabNamedayIdentifyBy, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.rbKabNamedayNameField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.cmbKabNamedayNameField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.rbKabNamedayDateField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.lineEditKabNamedayDateField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowAnniversaries, SIGNAL(toggled(bool)), m_ui_datasource.lblKabAnniversaryField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowAnniversaries, SIGNAL(toggled(bool)), m_ui_datasource.lineEditKabAnniversaryField, SLOT(setEnabled(bool)));

    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));

    parent->resize(parent->minimumSizeHint());
}

void BirthdayListApplet::updateEventList(const Plasma::DataEngine::Data &data) {
    // since we are going to re-create all entries again, delete currently existing ones

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
        if ((m_eventDataSource == EDS_KABC && m_kabcIsNamedayByGivenNameField) ||
            (m_eventDataSource == EDS_Akonadi && m_akoColIsNamedayByGivenNameField)) {
            contactNameday = getNamedayByGivenName(contactInfo["Given name"].toString());
        }
        else contactNameday = contactInfo["Nameday"].toDate();
        QDate contactAnniversary = contactInfo["Anniversary"].toDate();
        //kDebug() << "KABC contact:" << contactName << ", B" << contactBirthday << ", N" << contactNameday << ", A" << contactAnniversary;

        if (contactBirthday.isValid()) {
            m_listEntries.append(new BirthdayEntry(contactName, contactBirthday));
        }
        if (m_showNamedays && contactNameday.isValid()) {
            QDate firstNameday = contactNameday;
            if (contactBirthday.isValid()) {
                firstNameday.setDate(contactBirthday.year(), contactNameday.month(), contactNameday.day());
                if (firstNameday < contactBirthday) firstNameday = firstNameday.addYears(1);
            }
            namedayEntries.append(new NamedayEntry(contactName, firstNameday));
        }
        if (m_showAnniversaries && contactAnniversary.isValid()) {
            m_listEntries.append(new AnniversaryEntry(contactName, contactAnniversary));
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

    kDebug() << "New list entries created, total count" << m_listEntries.size();
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
    headerTitles << i18n("Name") << i18n("Age") << i18n("Date") << i18n("When");
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
        if (m_isTodaysForeground) item->setForeground(m_brushTodaysForeground);
        if (m_isTodaysBackground) item->setBackground(m_brushTodaysBackground);
    } else if (entry->remainingDays() < 0) {
        if (m_isPastForeground) item->setForeground(m_brushPastForeground);
        if (m_isPastBackground) item->setBackground(m_brushPastBackground);
    } else if (entry->remainingDays() <= m_highlightThreshold) {
        if (m_isHighlightForeground) item->setForeground(m_brushHighlightForeground);
        if (m_isHighlightBackground) item->setBackground(m_brushHighlightBackground);
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


#include "birthdaylist_applet.moc"
