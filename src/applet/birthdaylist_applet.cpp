/* ************************************************
 *  @name: kbirthdayapplet.cpp 
 *  @author: Meinhard Ritscher
 *
 *  $Id:  $
 *
 *  See header file for description and history
 *
 * ********************************************** */
/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 or (at your option)    *
 *   any later version.                                                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   General Public Licencse for more details.                             *
 *                                                                         *
 *   A copy of the license should be part of the package. If not you can   *
 *   find it here: http://www.gnu.org/licenses/gpl.html                    *
 *                                                                         *
 ***************************************************************************/

//own
#include "birthdaylist_applet.h"
#include "birthdaylistentry.h"

// Qt
#include <QPainter>
#include <QFontMetrics>
#include <QSizeF>
#include <QPair>
#include <QTime>
#include <QStandardItemModel>
#include <QTreeView>
#include <QGraphicsLinearLayout>

// KDE
#include <KConfigDialog>
#include <KDebug>

// Plasma
#include <Plasma/Theme>
#include <Plasma/IconWidget>
#if KDE_IS_VERSION(4,1,80)
#include <Plasma/ToolTipManager>
#endif

// linking this class to the desktop file so plasma can load it
K_EXPORT_PLASMA_APPLET(birthdaylist, KBirthdayApplet)


static const char *UTC_SOURCE = "UTC";

static QString dateFormat("d. M.");

KBirthdayApplet::KBirthdayApplet(QObject *parent, const QVariantList &args)
    : Plasma::PopupApplet(parent, args),
    m_graphicsWidget(0),
    m_treeView(0),
    m_model(0),
    m_isOnPanel(false),
    m_pKabcEngine(0),
    m_kabcNamedayString("Nameday"),
    m_kabcAnniversaryString("Anniversary"),
    m_showColumnHeaders(true),
    m_showNamedays(true),
    m_aggregateNamedays(true),
    m_showAnniversaries(true),
    m_eventThreshold(30),
    m_highlightThreshold(2),
    m_isHighlightForeground(false),
    m_brushHighlightForeground(QColor(255,255,255)),
    m_isHighlightBackground(true),
    m_brushHighlightBackground(QColor(128,0,0)),
    m_pastThreshold(2),
    m_isPastForeground(false),
    m_brushPastForeground(QColor(0,0,0)),
    m_isPastBackground(true),
    m_brushPastBackground(QColor(160,0,0)),
    m_columnWidthName(0),
    m_columnWidthAge(0),
    m_columnWidthDate(0),
    m_columnWidthRemaining(0)
{
    setBackgroundHints(DefaultBackground);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
    resize(250, 300);
}

KBirthdayApplet::~KBirthdayApplet()
{
    if (hasFailedToLaunch()) {
        //Do some cleanup here
    } else {
    }
    delete m_graphicsWidget;
}

void KBirthdayApplet::init()
{
    KConfigGroup configGroup = config();

    m_pIcon = new Plasma::IconWidget(KIcon("bl_cookie", NULL), QString());
    setPopupIcon(m_pIcon->icon());

    m_kabcNamedayString = configGroup.readEntry("KABC Nameday String", "Nameday");
    m_kabcAnniversaryString = configGroup.readEntry("KABC Anniversary String", "Anniversary");
    m_curNamedayLangCode = configGroup.readEntry("Nameday Calendar LangCode", "");

    m_pKabcEngine = dataEngine("birthdaylist");
    if (m_pKabcEngine && m_pKabcEngine->isValid()) {

        QHash<QString, QVariant> namedayLangCodeInfo = m_pKabcEngine->query("NamedayLists");
        m_namedayLangCodes = namedayLangCodeInfo.keys();
        qSort(m_namedayLangCodes);

        foreach (QString langCode, m_namedayLangCodes) {
            m_namedayLangStrings.append(namedayLangCodeInfo[langCode].toHash()["Language"].toString());
        }

        m_namedayLangCodes.prepend("");
        m_namedayLangStrings.prepend(i18n("None"));

        if (!m_namedayLangCodes.contains(m_curNamedayLangCode)) m_curNamedayLangCode = "";
        if (!m_curNamedayLangCode.isEmpty()) {
            m_curLangNamedayList = m_pKabcEngine->query(QString("NamedayList_%1").arg(m_curNamedayLangCode));
        }

        m_pKabcEngine->query(QString("KabcNamedayString_%1").arg(m_kabcNamedayString));
        m_pKabcEngine->query(QString("KabcAnniversaryString_%1").arg(m_kabcAnniversaryString));
        m_pKabcEngine->connectSource("KabcContactInfo", this);
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
    m_showColRemaining = configGroup.readEntry("Show Remaining Column", true);

    m_showNamedays = configGroup.readEntry("Show Namedays", true);
    m_aggregateNamedays = configGroup.readEntry("Aggregate Namedays", true);
    m_showAnniversaries = configGroup.readEntry("Show Anniversaries", true);

    m_eventThreshold = configGroup.readEntry("Event Threshold", 30);
    m_highlightThreshold = configGroup.readEntry("Highlight Threshold", 2);
    m_isHighlightForeground = configGroup.readEntry("Highlight Foreground Enabled", false);
    QColor highlightForeground = configGroup.readEntry("Highlight Foreground Color", QColor(255,255,255));
    m_brushHighlightForeground = QBrush(highlightForeground);
    m_isHighlightBackground = configGroup.readEntry("Highlight Background Enabled", true);
    QColor highlightBackground = configGroup.readEntry("Highlight Background Color", QColor(128,0,0));
    m_brushHighlightBackground = QBrush(highlightBackground);

    m_pastThreshold = configGroup.readEntry("Past Threshold", 2);
    m_isPastForeground = configGroup.readEntry("Past Foreground Enabled", false);
    QColor pastForeground = configGroup.readEntry("Past Foreground Color", QColor(0,0,0));
    m_brushPastForeground = QBrush(pastForeground);
    m_isPastBackground = configGroup.readEntry("Past Background Enabled", false);
    QColor pastBackground = configGroup.readEntry("Past Background Color", QColor(160,160,160));
    m_brushPastBackground = QBrush(pastBackground);

    m_columnWidthName = configGroup.readEntry("Name Column Width", 0);
    m_columnWidthAge = configGroup.readEntry("Age Column Width", 0);
    m_columnWidthDate = configGroup.readEntry("Date Column Width", 0);
    m_columnWidthRemaining = configGroup.readEntry("Remaining Column Width", 0);

    graphicsWidget();
}

void KBirthdayApplet::createConfigurationInterface(KConfigDialog *parent)
{
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
    m_ui_contents.chckShowColRemaining->setChecked(m_showColRemaining);

    m_ui_contents.chckShowNamedays->setChecked(m_showNamedays);
    m_ui_contents.chckAggrNamedays->setChecked(m_aggregateNamedays);
    m_ui_contents.cmbNamedayCalendar->clear();
    m_ui_contents.cmbNamedayCalendar->addItems(m_namedayLangStrings);
    if (m_namedayLangCodes.contains(m_curNamedayLangCode)) {
        m_ui_contents.cmbNamedayCalendar->setCurrentIndex(m_namedayLangCodes.indexOf(m_curNamedayLangCode));
    }
    else {
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

/*
 * This method is called when the user clicks ok
 * in the config dialog
 *
 */
void KBirthdayApplet::configAccepted()
{
    m_kabcNamedayString = m_ui_datasource.lineEditNamedayField->text();
    m_kabcAnniversaryString = m_ui_datasource.lineEditAnniversaryField->text();

    m_showColumnHeaders = m_ui_contents.chckShowColumnHeaders->isChecked();
    m_showColName = m_ui_contents.chckShowColName->isChecked();
    m_showColAge = m_ui_contents.chckShowColAge->isChecked();
    m_showColDate = m_ui_contents.chckShowColDate->isChecked();
    m_showColRemaining = m_ui_contents.chckShowColRemaining->isChecked();

    m_showNamedays = m_ui_contents.chckShowNamedays->isChecked();
    m_aggregateNamedays = m_ui_contents.chckAggrNamedays->isChecked();
    m_curNamedayLangCode = m_namedayLangCodes[m_ui_contents.cmbNamedayCalendar->currentIndex()];
    m_showAnniversaries = m_ui_contents.chckShowAnniversaries->isChecked();

    m_eventThreshold     = m_ui_appearance.spinComingShowDays->value();
    m_highlightThreshold = m_ui_appearance.spinComingHighlightDays->value();
    m_isHighlightForeground = m_ui_appearance.chckComingHighlightForeground->isChecked();
    m_brushHighlightForeground.setColor(m_ui_appearance.colorbtnComingHighlightForeground->color());
    m_isHighlightBackground = m_ui_appearance.chckComingHighlightBackground->isChecked();
    m_brushHighlightBackground.setColor(m_ui_appearance.colorbtnComingHighlightBackground->color());

    m_pastThreshold      = m_ui_appearance.spinPastShowDays->value();
    m_isPastForeground = m_ui_appearance.chckPastForeground->isChecked();
    m_brushPastForeground.setColor(m_ui_appearance.colorbtnPastForeground->color());
    m_isPastBackground = m_ui_appearance.chckPastBackground->isChecked();
    m_brushPastBackground.setColor(m_ui_appearance.colorbtnPastBackground->color());

    QTreeView *qTreeView = m_treeView->nativeWidget();
    m_columnWidthName = qTreeView->columnWidth(0);
    m_columnWidthAge = qTreeView->columnWidth(1);
    m_columnWidthDate = qTreeView->columnWidth(2);
    m_columnWidthRemaining = qTreeView->columnWidth(3);


    KConfigGroup configGroup = config();

    configGroup.writeEntry("KABC Nameday String", m_kabcNamedayString);
    configGroup.writeEntry("KABC Anniversary String", m_kabcAnniversaryString);

    configGroup.writeEntry("Show Column Headers", m_showColumnHeaders);
    //configGroup.writeEntry("Show Name Column", m_showColName);
    configGroup.writeEntry("Show Age Column", m_showColAge);
    configGroup.writeEntry("Show Date Column", m_showColDate);
    configGroup.writeEntry("Show Remaining Column", m_showColRemaining);

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

    configGroup.writeEntry("Past Threshold",m_pastThreshold);
    configGroup.writeEntry("Past Foreground Enabled", m_isPastForeground);
    configGroup.writeEntry("Past Foreground Color", m_brushPastForeground.color());
    configGroup.writeEntry("Past Background Enabled", m_isPastBackground);
    configGroup.writeEntry("Past Background Color", m_brushPastBackground.color());

    configGroup.writeEntry("Name Column Width", m_columnWidthName);
    configGroup.writeEntry("Age Column Width", m_columnWidthAge);
    configGroup.writeEntry("Date Column Width", m_columnWidthDate);
    configGroup.writeEntry("Remaining Column Width", m_columnWidthRemaining);

    m_curLangNamedayList = m_pKabcEngine->query(QString("NamedayList_%1").arg(m_curNamedayLangCode));
    m_pKabcEngine->query(QString("KabcNamedayString_%1").arg(m_kabcNamedayString));
    m_pKabcEngine->query(QString("KabcAnniversaryString_%1").arg(m_kabcAnniversaryString));
    updateEventList(m_pKabcEngine->query("KabcContactInfo"));
    updateModels();
    update();
    useCurrentPlasmaTheme();

    emit configNeedsSaving();
}

QGraphicsWidget *KBirthdayApplet::graphicsWidget()
{
  if (!m_graphicsWidget) {
    m_graphicsWidget = new QGraphicsWidget(this);
    m_graphicsWidget->setMinimumSize(225, 150);
    m_graphicsWidget->setPreferredSize(350, 200);
    
    m_treeView = new Plasma::TreeView(m_graphicsWidget);
	m_treeView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	QTreeView *treeView =m_treeView->nativeWidget();
 	treeView->setAlternatingRowColors( true );
	treeView->setAllColumnsShowFocus( true );
	treeView->setRootIsDecorated(false);
	//treeView->setSortingEnabled(true);
	treeView->setSelectionMode( QAbstractItemView::NoSelection );
	treeView->setSelectionBehavior( QAbstractItemView::SelectRows );
	treeView->setItemsExpandable(true);
	treeView->setExpandsOnDoubleClick(true);

    m_model = new QStandardItemModel(0,3);
	updateModels();
	m_treeView->setModel( m_model );
    
	QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Vertical);
	layout->setContentsMargins( 0, 0, 0, 0 );
	layout->setSpacing( 0 );
	layout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

//	QGraphicsLayout *layoutTop = new QGraphicsLinearLayout(Qt::Horizontal); // = createLayoutTitle();
//	layout->addItem( layoutTop );

	layout->addItem( m_treeView );
	m_graphicsWidget->setLayout(layout);
	
	useCurrentPlasmaTheme();
  }
  return m_graphicsWidget;
}

void KBirthdayApplet::updateModels()
{
    qDebug() << "CREATING BIRTHDAY MODEL";
    
    if (!m_model) m_model = new QStandardItemModel(0,3);
    m_model->clear();
    QStringList headerTitles;
    headerTitles << "Name" << "Age" << "Date" << "Remaining";
    m_model->setHorizontalHeaderLabels(headerTitles);
    setColumnWidths();
    
    QStandardItem *parentItem = m_model->invisibleRootItem();
    
    foreach(const AbstractAnnualEventEntry *entry, m_listEntries) {
        if (testThreshold(entry->remainingDays())) {
            QList<QStandardItem*> items;
            entry->createModelItems(items);

            for (int i=0; i<items.size(); ++i) items[i]->setEditable(false);

            if (entry->remainingDays() < 0) {
                for (int i=0; i<items.size(); ++i) {
                    if (m_isPastForeground) items[i]->setForeground(m_brushPastForeground);
                    if (m_isPastBackground) items[i]->setBackground(m_brushPastBackground);
                }
            }
            else if (entry->remainingDays() < m_highlightThreshold) {
                for (int i=0; i<items.size(); ++i) {
                    if (m_isHighlightForeground) items[i]->setForeground(m_brushHighlightForeground);
                    if (m_isHighlightBackground) items[i]->setBackground(m_brushHighlightBackground);
                }
            }

            parentItem->appendRow(items);
        }
    }
}

void KBirthdayApplet::setColumnWidths() {
  if (!m_treeView || !m_model) return;

  QTreeView *qTreeView = m_treeView->nativeWidget();

  qTreeView->setHeaderHidden(!m_showColumnHeaders);
  qTreeView->setColumnHidden(0, !m_showColName);
  qTreeView->setColumnHidden(1, !m_showColAge);
  qTreeView->setColumnHidden(2, !m_showColDate);
  qTreeView->setColumnHidden(3, !m_showColRemaining);

  if (m_columnWidthName < 10) qTreeView->resizeColumnToContents(0);
  else qTreeView->setColumnWidth(0, m_columnWidthName); 

  if (m_columnWidthAge < 10) qTreeView->resizeColumnToContents(1);
  else qTreeView->setColumnWidth(1, m_columnWidthAge);

  if (m_columnWidthDate < 10) qTreeView->resizeColumnToContents(2);
  else qTreeView->setColumnWidth(2, m_columnWidthDate);

  if (m_columnWidthRemaining < 10) qTreeView->resizeColumnToContents(3);
  else qTreeView->setColumnWidth(3, m_columnWidthRemaining);
}

void KBirthdayApplet::useCurrentPlasmaTheme() {
   /* QFont font = Plasma::Theme::defaultTheme()->font( Plasma::Theme::DefaultFont );
    int newPixelSize = qCeil((float)font.pixelSize() * 1.4f);
    if ( newPixelSize > 1 )
	font.setPixelSize( newPixelSize );
   // m_label->setFont( font );*/

    // Get theme colors
    QColor textColor = Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor );
    QColor baseColor = Plasma::Theme::defaultTheme()->color( Plasma::Theme::BackgroundColor );
    QColor altBaseColor = baseColor.darker();
    // 	int green = altBaseColor.green() * 1.8;
    // 	altBaseColor.setGreen( green > 255 ? 255 : green ); // tint green
    QColor buttonColor = Plasma::Theme::defaultTheme()->color( Plasma::Theme::BackgroundColor );
    baseColor.setAlpha(50);
    altBaseColor.setAlpha(50);
    buttonColor.setAlpha(130);
//    m_colorSubItemLabels = textColor;
//    m_colorSubItemLabels.setAlpha( 170 );

    // Create palette with the used theme colors
    QPalette p = palette();
    p.setColor( QPalette::Background, baseColor );
    p.setColor( QPalette::Base, baseColor );
    p.setColor( QPalette::AlternateBase, altBaseColor );
    p.setColor( QPalette::Button, buttonColor );
    p.setColor( QPalette::Foreground, textColor );
    p.setColor( QPalette::Text, textColor );

    if (m_treeView) {
    QTreeView *treeView = m_treeView->nativeWidget();
    treeView->setPalette(p);
    treeView->header()->setPalette( p );
    }

    QBrush textBrush = QBrush( Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor) );
    if (m_model) {
      for (int i=0; i<m_model->columnCount(); ++i) {
      	m_model->horizontalHeaderItem(i)->setForeground( textBrush );
      }
    }

    // To set new text color of the header items
    //setDepartureArrivalListType( m_settings.departureArrivalListType() );
}

void KBirthdayApplet::constraintsEvent(Plasma::Constraints constraints)
{
    if (constraints & Plasma::FormFactorConstraint){
        Plasma::FormFactor formFctr = formFactor();
        if (formFctr == Plasma::Horizontal || formFctr == Plasma::Vertical) {
            setMinimumSize(QSize(42,42));
            setAspectRatioMode(Plasma::ConstrainedSquare);
#if KDE_IS_VERSION(4,1,80)
            Plasma::ToolTipManager::self()->registerWidget(this);
#endif
            setAspectRatioMode(Plasma::IgnoreAspectRatio);
            m_isOnPanel = true;

        } else if (formFctr == Plasma::MediaCenter || formFctr == Plasma::Planar) {
#if KDE_IS_VERSION(4,1,80)
            Plasma::ToolTipManager::self()->unregisterWidget(this);
#endif
            m_isOnPanel = false;
        }
    }
}

void KBirthdayApplet::paintInterface(QPainter *p,
    const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED(p)
    Q_UNUSED(option)
    Q_UNUSED(contentsRect)
}

void KBirthdayApplet::dataUpdated(const QString &name, const Plasma::DataEngine::Data &data)
{
    kDebug() << "Update of birthday list entries, triggered by data source" << name;
    
    if (name == "KabcContactInfo") {
        updateEventList(data);
    } else if (name == UTC_SOURCE) {
        updateEventList(m_pKabcEngine->query("KabcContactInfo"));
    }
    updateModels();
    update();
    useCurrentPlasmaTheme();
}

void KBirthdayApplet::updateEventList(const Plasma::DataEngine::Data &data)
{
    // since we are going to re-create all entries again, delete currently existing ones
    foreach (AbstractAnnualEventEntry *oldListEntry, m_listEntries) {
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

        foreach (NamedayEntry *namedayEntry, namedayEntries) {
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
        
        foreach (AggregatedNamedayEntry *entry, aggregatedEntries) {
            m_listEntries.append(entry);
        }
    }
    else {
        foreach (NamedayEntry *entry, namedayEntries) {
            m_listEntries.append(entry);
        }
    }

    // sort the entries by date
    qSort(m_listEntries.begin(), m_listEntries.end(), AbstractAnnualEventEntry::lessThan);
  
    kDebug() << "New list entries created, total count" << m_listEntries.size();
}

QString KBirthdayApplet::getNamedayString(QDate date) {
  QVariant namedayStringEntry = m_curLangNamedayList[date.toString("MM-dd")];
  if (namedayStringEntry.isValid()) return namedayStringEntry.toString();
  else return date.toString(dateFormat);
}

bool KBirthdayApplet::testThreshold(const int remainingDays)
{
  return ( (remainingDays > 0 && remainingDays <= m_eventThreshold) ||
          (remainingDays < 0 && remainingDays >= (m_pastThreshold* -1)) ||
                remainingDays == 0)?true:false;
}


#include "birthdaylist_applet.moc"
