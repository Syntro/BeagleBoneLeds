//
//  Copyright (c) 2012 Pansenti, LLC.
//	
//  This file is part of Syntro
//
//  Syntro is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Syntro is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Syntro.  If not, see <http://www.gnu.org/licenses/>.
//

#include <qlayoutitem.h>
#include <qgridlayout.h>

#include "BoneLedDisplay.h"

#define	LED_STATUS_TIMEOUT	(3 * SYNTRO_CLOCKS_PER_SEC)

BoneLedDisplay::BoneLedDisplay(QSettings *settings, QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), m_settings(settings)
{
	ui.setupUi(this);
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	initStatusBar();
	layoutWindow();
	mapButtonEvents();

	m_haveNewVal = false;
	m_oldVal = 0xff;
	m_newVal = 0;
	m_lastUpdate = SyntroClock();
	m_idle = true;
	m_wasIdle = false;
	updateDisplay(0);

	m_client = new DisplayClient(this, m_settings);
	connect(m_client, SIGNAL(newData(quint32)), this, SLOT(newData(quint32)), Qt::DirectConnection);
	m_client->resumeThread();

	restoreWindowState();

	m_updateTimer = startTimer(100);
	m_statusTimer = startTimer(LED_STATUS_TIMEOUT);
}

BoneLedDisplay::~BoneLedDisplay()
{
}

void BoneLedDisplay::closeEvent(QCloseEvent *)
{
	if (m_updateTimer) {
		killTimer(m_updateTimer);
		m_updateTimer = 0;
	}

	if (m_statusTimer) {
		killTimer(m_statusTimer);
		m_statusTimer = 0;
	}

	if (m_client) {
		disconnect(m_client, SIGNAL(newData(quint32)), this, SLOT(newData(quint32)));
		m_client->exitThread();
	}

	saveWindowState();
}

// The ledWrite function provided by the DisplayClient allows for multiple writes
// in one call, but we only do one led at a time here.
void BoneLedDisplay::toggle(int led)
{
	if (led < 0 || led >= NUM_LEDS)
		return;

	QString current = m_ledState[led]->text();

	quint32 mask = 0x01 << led;
	quint32 newVal = (current == "ON") ? 0 : 1;
	newVal <<= led;

	m_client->ledWrite(mask, newVal);
}

// No GUI work here, we are on the client endpoint thread
void BoneLedDisplay::newData(quint32 values)
{
	QMutexLocker lock(&m_updateMutex);

	m_idle = false;
	m_lastUpdate = SyntroClock();

	m_haveNewVal = true;
	m_newVal = values;
}

void BoneLedDisplay::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_statusTimer) {
		m_controlStatus->setText(m_client->getLinkState());

		if (syntroTimerExpired(SyntroClock(), m_lastUpdate, LED_STATUS_TIMEOUT)) {
			m_idle = true;
			updateDisplay(0);
		}
	}
	else {
		bool isNew = false;
		unsigned int current;

		m_updateMutex.lock();
		if (m_haveNewVal) {
			current = m_newVal;
			isNew = true;
			m_haveNewVal = false;
		}
		m_updateMutex.unlock();

		if (isNew)
			updateDisplay(current);
	}
}

void BoneLedDisplay::updateDisplay(quint32 current)
{
	if ((current == m_oldVal) && (m_idle == m_wasIdle))
		return;

	quint32 mask = 0x01;

	for (int i = 0; i < NUM_LEDS; i++) {
		updateLabel(m_ledState[i], (current & mask) == mask);
		mask <<= 1;
	}

	m_oldVal = current;
	m_wasIdle = m_idle;
}

void BoneLedDisplay::updateLabel(QLabel *label, bool on)
{
	QPalette pal = label->palette();

	if (m_idle) {
		label->setText("?");
		pal.setColor(label->backgroundRole(), Qt::yellow);
	}
	else if (on) {
		label->setText("ON");
		pal.setColor(label->backgroundRole(), Qt::green);
	}
	else {
		label->setText("OFF");
		pal.setColor(label->backgroundRole(), Qt::red);
	}

	label->setPalette(pal);
	label->setAutoFillBackground(true);
}

void BoneLedDisplay::mapButtonEvents()
{
	m_signalMapper = new QSignalMapper(this);
	connect(m_signalMapper, SIGNAL(mapped(int)), this, SLOT(toggle(int)));

	for (int i = 0; i < NUM_LEDS; i++) {
		m_signalMapper->setMapping(m_toggle[i], i);
		connect(m_toggle[i], SIGNAL(clicked()), m_signalMapper, SLOT(map()));
	}
}

void BoneLedDisplay::layoutWindow()
{
	QLabel *label;
	QSpacerItem *hSpacer;

	QGridLayout *gridLayout = new QGridLayout(ui.centralWidget);

	gridLayout->setSpacing(6);
	gridLayout->setContentsMargins(11, 11, 11, 11);

	for (int i = 0; i < NUM_LEDS; i++) {
		label = new QLabel(QString("USR%1").arg(i));
		m_ledState[i] = new QLabel();
		m_ledState[i]->setMinimumSize(QSize(40, 0));
		m_ledState[i]->setMaximumSize(QSize(40, 100));
		m_ledState[i]->setFrameShape(QFrame::Panel);
		m_ledState[i]->setFrameShadow(QFrame::Sunken);
		m_ledState[i]->setAlignment(Qt::AlignCenter);
		m_toggle[i] = new QPushButton("Toggle");
		hSpacer	= new QSpacerItem(148, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

		gridLayout->addWidget(label, i, 0, 1, 1);
		gridLayout->addWidget(m_ledState[i], i, 1, 1, 1);
		gridLayout->addWidget(m_toggle[i], i, 2, 1, 1);
		gridLayout->addItem(hSpacer, i, 3, 1, 1);
	}
}

void BoneLedDisplay::initStatusBar()
{
	m_controlStatus = new QLabel(this);
	m_controlStatus->setAlignment(Qt::AlignLeft);
	ui.statusBar->addWidget(m_controlStatus, 1);
}

void BoneLedDisplay::saveWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		m_settings->setValue("Geometry", saveGeometry());
		m_settings->setValue("State", saveState());
		m_settings->endGroup();
	}
}

void BoneLedDisplay::restoreWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		restoreGeometry(m_settings->value("Geometry").toByteArray());
		restoreState(m_settings->value("State").toByteArray());
		m_settings->endGroup();
	}
}
