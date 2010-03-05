/**
 * @file    birthdaylist_applet.cpp
 * @author  Karol Slanina
 * @version 0.5.0
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

#include <KConfigDialog>
#include <KDebug>

#include <Plasma/Theme>
#include <Plasma/TreeView>


// linking this class to the desktop file so plasma can load it
K_EXPORT_PLASMA_APPLET(birthdaylist, BirthdayListApplet)


static const char *UTC_SOURCE = "UTC";

static QString dateFormat("d. M.");

BirthdayListApplet::BirthdayListApplet(QObject *parent, const QVariantList &args)
: Plasma::PopupApplet(parent, args),
m_dataEngine(0),
m_kabcNamedayString("Nameday"),
m_kabcAnniversaryString("Anniversary"),
m_model(0, 3),
m_graphicsWidget(0),
m_treeView(0),
m_showColumnHeaders(true),
m_showColName(true),
m_showColAge(true),
m_showColDate(true),
m_showColWhen(true),
m_showNamedays(true),
m_aggregateNamedays(true),
m_showAnniversaries(true),
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
m_columnWidthName(0),
m_columnWidthAge(0),
m_columnWidthDate(0),
m_columnWidthWhen(0) {
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
    m_kabcNamedayString = configGroup.readEntry("KABC Nameday String", "Nameday");
    m_kabcAnniversaryString = configGroup.readEntry("KABC Anniversary String", "Anniversary");
    m_curNamedayLangCode = configGroup.readEntry("Nameday Calendar LangCode", "");

    m_dataEngine = dataEngine("birthdaylist");
    if (m_dataEngine && m_dataEngine->isValid()) {

        QHash<QString, QVariant> namedayLangCodeInfo = m_dataEngine->query("NamedayLists");
        m_namedayLangCodes = namedayLangCodeInfo.keys();
        qSort(m_namedayLangCodes);

        foreach(QString langCode, m_namedayLangCodes) {
            m_namedayLangStrings.append(namedayLangCodeInfo[langCode].toHash()["Language"].toString());
        }

        m_namedayLangCodes.prepend("");
        m_namedayLangStrings.prepend(i18nc("No nameday calendar (combo box item)", "None"));

        if (!m_namedayLangCodes.contains(m_curNamedayLangCode)) m_curNamedayLangCode = "";
        if (!m_curNamedayLangCode.isEmpty()) {
            m_curLangNamedayList = m_dataEngine->query(QString("NamedayList_%1").arg(m_curNamedayLangCode));
        }

        m_dataEngine->query(QString("KabcNamedayString_%1").arg(m_kabcNamedayString));
        m_dataEngine->query(QString("KabcAnniversaryString_%1").arg(m_kabcAnniversaryString));
        m_dataEngine->connectSource("KabcContactInfo", this);
    } else {
        setFailedToLaunch(true, "Could not load birthdaylist dataEngine");
    }

    Plasma::DataEngine* timeEngine = dataEngine("time");
    if (timeEngine) {
        timeEngine->connectSource("UTC", this, 360000, Plasma::AlignToHour);
    } else {
        kDebug() << "Warning: Could not load time dataEngine - no nightly update possible";
    }

    m_showColumnHeaders = configGroup.readEntry("Show Column Headers", true);
    //m_showColName = configGroup.readEntry("Show Name Column", true);
    m_showColName = true;
    m_showColAge = configGroup.readEntry("Show Age Column", true);
    m_showColDate = configGroup.readEntry("Show Date Column", true);
    m_showColWhen = configGroup.readEntry("Show When Column", true);

    m_showNamedays = configGroup.readEntry("Show Namedays", true);
    m_aggregateNamedays = configGroup.readEntry("Aggregate Namedays", true);
    m_showAnniversaries = configGroup.readEntry("Show Anniversaries", true);

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

    if (name == "KabcContactInfo") {
        updateEventList(data);
    } else if (name == UTC_SOURCE) {
        updateEventList(m_dataEngine->query("KabcContactInfo"));
    }

    updateModels();
    update();
}

void BirthdayListApplet::configAccepted() {
    m_kabcNamedayString = m_ui_datasource.lineEditNamedayField->text();
    m_kabcAnniversaryString = m_ui_datasource.lineEditAnniversaryField->text();

    m_showColumnHeaders = m_ui_contents.chckShowColumnHeaders->isChecked();
    m_showColName = m_ui_contents.chckShowColName->isChecked();
    m_showColAge = m_ui_contents.chckShowColAge->isChecked();
    m_showColDate = m_ui_contents.chckShowColDate->isChecked();
    m_showColWhen = m_ui_contents.chckShowColWhen->isChecked();

    m_showNamedays = m_ui_contents.chckShowNamedays->isChecked();
    m_aggregateNamedays = m_ui_contents.chckAggrNamedays->isChecked();
    m_curNamedayLangCode = m_namedayLangCodes[m_ui_contents.cmbNamedayCalendar->currentIndex()];
    m_showAnniversaries = m_ui_contents.chckShowAnniversaries->isChecked();

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

    QTreeView *qTreeView = m_treeView->nativeWidget();
    m_columnWidthName = qTreeView->columnWidth(0);
    m_columnWidthAge = qTreeView->columnWidth(1);
    m_columnWidthDate = qTreeView->columnWidth(2);
    m_columnWidthWhen = qTreeView->columnWidth(3);


    KConfigGroup configGroup = config();

    configGroup.writeEntry("KABC Nameday String", m_kabcNamedayString);
    configGroup.writeEntry("KABC Anniversary String", m_kabcAnniversaryString);

    configGroup.writeEntry("Show Column Headers", m_showColumnHeaders);
    //configGroup.writeEntry("Show Name Column", m_showColName);
    configGroup.writeEntry("Show Age Column", m_showColAge);
    configGroup.writeEntry("Show Date Column", m_showColDate);
    configGroup.writeEntry("Show When Column", m_showColWhen);

    configGroup.writeEntry("Show Namedays", m_showNamedays);
    configGroup.writeEntry("Aggregate Namedays", m_aggregateNamedays);
    configGroup.writeEntry("Nameday Calendar LangCode", m_curNamedayLangCode);
    configGroup.writeEntry("Show Anniversaries", m_showAnniversaries);

    configGroup.writeEntry("Event Threshold", m_eventThreshold);
    configGroup.writeEntry("Highlight Threshold", m_highlightThreshold);
    configGroup.writeEntry("Highlight Foreground Enabled", m_isHighlightForeground);
    configGroup.writeEntry("Highlight Foreground Color", m_brushHighlightForeground.color());
    configGroup.writeEntry("Highlight Background Enabled", m_isHighlightBackground);
    configGroup.writeEntry("Highlight Background Color", m_brushHighlightBackground.color());

    configGroup.writeEntry("Past Threshold", m_pastThreshold);
    configGroup.writeEntry("Past Foreground Enabled", m_isPastForeground);
    configGroup.writeEntry("Past Foreground Color", m_brushPastForeground.color());
    configGroup.writeEntry("Past Background Enabled", m_isPastBackground);
    configGroup.writeEntry("Past Background Color", m_brushPastBackground.color());

    configGroup.writeEntry("Name Column Width", m_columnWidthName);
    configGroup.writeEntry("Age Column Width", m_columnWidthAge);
    configGroup.writeEntry("Date Column Width", m_columnWidthDate);
    configGroup.writeEntry("When Column Width", m_columnWidthWhen);

    m_curLangNamedayList = m_dataEngine->query(QString("NamedayList_%1").arg(m_curNamedayLangCode));
    m_dataEngine->query(QString("KabcNamedayString_%1").arg(m_kabcNamedayString));
    m_dataEngine->query(QString("KabcAnniversaryString_%1").arg(m_kabcAnniversaryString));
    updateEventList(m_dataEngine->query("KabcContactInfo"));

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

    m_ui_datasource.lineEditNamedayField->setText(m_kabcNamedayString);
    m_ui_datasource.lineEditAnniversaryField->setText(m_kabcAnniversaryString);

    m_ui_contents.chckShowColumnHeaders->setChecked(m_showColumnHeaders);
    m_ui_contents.chckShowColName->setChecked(m_showColName);
    m_ui_contents.chckShowColAge->setChecked(m_showColAge);
    m_ui_contents.chckShowColDate->setChecked(m_showColDate);
    m_ui_contents.chckShowColWhen->setChecked(m_showColWhen);

    m_ui_contents.chckShowNamedays->setChecked(m_showNamedays);
    m_ui_contents.chckAggrNamedays->setChecked(m_aggregateNamedays);
    m_ui_contents.cmbNamedayCalendar->clear();
    m_ui_contents.cmbNamedayCalendar->addItems(m_namedayLangStrings);
    if (m_namedayLangCodes.contains(m_curNamedayLangCode)) {
        m_ui_contents.cmbNamedayCalendar->setCurrentIndex(m_namedayLangCodes.indexOf(m_curNamedayLangCode));
    } else {
        m_ui_contents.cmbNamedayCalendar->setCurrentIndex(0);
    }
    m_ui_contents.chckShowAnniversaries->setChecked(m_showAnniversaries);

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

    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.lblNamedayField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowNamedays, SIGNAL(toggled(bool)), m_ui_datasource.lineEditNamedayField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowAnniversaries, SIGNAL(toggled(bool)), m_ui_datasource.lblAnniversaryField, SLOT(setEnabled(bool)));
    connect(m_ui_contents.chckShowAnniversaries, SIGNAL(toggled(bool)), m_ui_datasource.lineEditAnniversaryField, SLOT(setEnabled(bool)));

    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
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
        QDate contactBirthday = contactInfo["Birthday"].toDate();
        QDate contactNameday = contactInfo["Nameday"].toDate();
        QDate contactAnniversary = contactInfo["Anniversary"].toDate();
        //kDebug() << "KABC contact:" << contactName << ", B" << contactBirthday << ", N" << contactNameday << ", A" << contactAnniversary;

        if (contactBirthday.isValid()) {
            m_listEntries.append(new BirthdayEntry(contactName, contactBirthday));
        }
        if (m_showNamedays && contactNameday.isValid()) {
            namedayEntries.append(new NamedayEntry(contactName, contactNameday));
        }
        if (m_showAnniversaries && contactAnniversary.isValid()) {
            m_listEntries.append(new AnniversaryEntry(contactName, contactAnniversary));
        }
    }

    // if desired, join together nameday entries from the same day
    if (m_aggregateNamedays) {
        qSort(namedayEntries.begin(), namedayEntries.end(), AbstractAnnualEventEntry::lessThan);
        QMap<QDate, AggregatedNamedayEntry*> aggregatedEntries;
        int curYear = QDate::currentDate().year();

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
    } else {

        foreach(NamedayEntry *entry, namedayEntries) {
            m_listEntries.append(entry);
        }
    }

    // sort the entries by date
    qSort(m_listEntries.begin(), m_listEntries.end(), AbstractAnnualEventEntry::lessThan);

    kDebug() << "New list entries created, total count" << m_listEntries.size();
}

QString BirthdayListApplet::getNamedayString(QDate date) {
    QVariant namedayStringEntry = m_curLangNamedayList[date.toString("MM-dd")];
    if (namedayStringEntry.isValid()) return namedayStringEntry.toString();
    else return date.toString(dateFormat);
}

void BirthdayListApplet::updateModels() {
    qDebug() << "CREATING BIRTHDAY MODEL";

    m_model.clear();
    QStringList headerTitles;
    headerTitles << i18n("Name") << i18n("Age") << i18n("Date") << i18n("When");
    m_model.setHorizontalHeaderLabels(headerTitles);

    QStandardItem *parentItem = m_model.invisibleRootItem();

    foreach(const AbstractAnnualEventEntry *entry, m_listEntries) {
        int remainingDays = entry->remainingDays();
        bool showEvent = (remainingDays >= 0 && remainingDays <= m_eventThreshold) ||
                (remainingDays <= 0 && remainingDays >= -m_pastThreshold);

        if (showEvent) {
            QList<QStandardItem*> items;
            entry->createModelItems(items);
            foreach(QStandardItem *item, items) setModelItemColors(entry, item);

            parentItem->appendRow(items);
        }
    }

    setTreeColumnWidths();
    usePlasmaThemeColors();
}

void BirthdayListApplet::setModelItemColors(const AbstractAnnualEventEntry *entry, QStandardItem *item) {
    item->setEditable(false);

    if (entry->remainingDays() < 0) {
        if (m_isPastForeground) item->setForeground(m_brushPastForeground);
        if (m_isPastBackground) item->setBackground(m_brushPastBackground);
    } else if (entry->remainingDays() < m_highlightThreshold) {
        if (m_isHighlightForeground) item->setForeground(m_brushHighlightForeground);
        if (m_isHighlightBackground) item->setBackground(m_brushHighlightBackground);
    }

    for (int row = 0; row < item->rowCount(); ++row) {
        for (int col = 0; col < item->columnCount(); ++col) {
            QStandardItem *child = item->child(row, col);
            if (child) setModelItemColors(entry, child);
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
