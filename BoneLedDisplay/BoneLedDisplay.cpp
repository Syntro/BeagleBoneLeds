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
	layoutWindow();
	mapButtonEvents();

	for (int i = 0; i < NUM_BONES; i++) {
		m_haveNewVal[i] = false;
		m_oldVal[i] = 0xff;
		m_newVal[i] = 0;
		m_lastUpdate[i] = SyntroClock();
		m_idle[i] = true;
		m_wasIdle[i] = false;
		updateDisplay(i, 0);
	}

	m_client = new DisplayClient(this, m_settings);
	connect(m_client, SIGNAL(newData(int, quint32)), this, SLOT(newData(int, quint32)), Qt::DirectConnection);
	m_client->resumeThread();

	restoreWindowState();

	m_updateTimer = startTimer(100);
	m_timeoutTimer = startTimer(LED_STATUS_TIMEOUT);

	//setWindowFlags(Qt::CustomizeWindowHint);
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

	if (m_timeoutTimer) {
		killTimer(m_timeoutTimer);
		m_timeoutTimer = 0;
	}

	if (m_client) {
		disconnect(m_client, SIGNAL(newData(int, quint32)), this, SLOT(newData(int, quint32)));
		m_client->exitThread();
		delete m_client;
	}

	saveWindowState();
}

// The ledWrite function provided by the DisplayClient allows for multiple writes
// in one call, but we only do one led (one bit) at a time here.
void BoneLedDisplay::toggle(int led)
{
	if (led < 0 || led >= NUM_BONES * NUM_LEDS)
		return;

	int bone = led / NUM_LEDS;

	quint32 mask = 0x01 << led;
	quint32 newVal = m_ledState[led] ? 0 : 1;
	newVal <<= led;

	m_client->ledWrite(bone, mask, newVal);
}

// No GUI work here, we are on the client endpoint thread
void BoneLedDisplay::newData(int bone, quint32 values)
{
	QMutexLocker lock(&m_updateMutex);

	m_idle[bone] = false;
	m_lastUpdate[bone] = SyntroClock();

	m_haveNewVal[bone] = true;
	m_newVal[bone] = values;
}

void BoneLedDisplay::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_timeoutTimer) {
		if (syntroTimerExpired(SyntroClock(), m_lastUpdate[0], LED_STATUS_TIMEOUT)) {
			m_idle[0] = true;
			updateDisplay(0, 0);
		}
	}
	else {
		bool isNew = false;
		unsigned int current;

		m_updateMutex.lock();
		if (m_haveNewVal[0]) {
			current = m_newVal[0];
			isNew = true;
			m_haveNewVal[0] = false;
		}
		m_updateMutex.unlock();

		if (isNew)
			updateDisplay(0, current);
	}
}

void BoneLedDisplay::updateDisplay(int bone, quint32 current)
{
	if ((current == m_oldVal[bone]) && (m_idle[bone] == m_wasIdle[bone]))
		return;

	quint32 mask = 0x01;

	for (int i = 0, button = bone * NUM_LEDS; i < NUM_LEDS; i++, button++) {
		m_ledState[button] = (current & mask) == mask;
		updateButton(bone, button);
		mask <<= 1;
	}

	m_oldVal[bone] = current;
	m_wasIdle[bone] = m_idle;
}

void BoneLedDisplay::updateButton(int bone, int button)
{
	if (m_idle[bone])
		m_toggle[button]->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 0);"));
	else if (m_ledState[button])
		m_toggle[button]->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 255, 0);"));
	else
		m_toggle[button]->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 0, 0);"));
}

void BoneLedDisplay::mapButtonEvents()
{
	m_signalMapper = new QSignalMapper(this);
	connect(m_signalMapper, SIGNAL(mapped(int)), this, SLOT(toggle(int)));

	for (int i = 0; i < NUM_BONES * NUM_LEDS; i++) {
		m_signalMapper->setMapping(m_toggle[i], i);
		connect(m_toggle[i], SIGNAL(clicked()), m_signalMapper, SLOT(map()));
	}
}

void BoneLedDisplay::layoutWindow()
{
	QLabel *label;
	QSpacerItem *hSpacer;

	QGridLayout *gridLayout = new QGridLayout(ui.centralWidget);

	gridLayout->setSpacing(4);
	gridLayout->setContentsMargins(5, 5, 5, 5);

	for (int j = 0, k = 0; j < NUM_BONES; j++) {
		label = new QLabel(QString("Bone %1").arg(j));
		gridLayout->addWidget(label, j, 0, 1, 1);
		
		for (int i = 0; i < NUM_LEDS; i++) {
			m_toggle[k] = new QPushButton(QString("%1").arg(i));
			m_toggle[k]->setMinimumWidth(24);
			m_toggle[k]->setMaximumWidth(24);
			gridLayout->addWidget(m_toggle[k], j, i + 1, 1, 1);
			k++;
		}

		hSpacer	= new QSpacerItem(80, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
		gridLayout->addItem(hSpacer, j, NUM_LEDS + 1, 1, 1);
	}
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
