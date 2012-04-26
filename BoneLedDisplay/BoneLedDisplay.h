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

#ifndef BONELEDDISPLAY_H
#define BONELEDDISPLAY_H

#include <QtGui/QMainWindow>
#include <qsettings.h>
#include <qsignalmapper.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include "DisplayClient.h"
#include "ui_BoneLedDisplay.h"

#define NUM_BONES 3
#define NUM_LEDS 4

class BoneLedDisplay : public QMainWindow
{
	Q_OBJECT

public:
	BoneLedDisplay(QSettings *settings, QWidget *parent = 0, Qt::WFlags flags = 0);
	~BoneLedDisplay();

public slots:
	void newData(int bone, quint32 values);
	void toggle(int led);

protected:
	void closeEvent(QCloseEvent *);
	void timerEvent(QTimerEvent *);

private:
	void checkForUpdates();
	void updateDisplay(int bone, quint32 current);
	void initStatusBar();
	void mapButtonEvents();
	void layoutWindow();
	void saveWindowState();
	void restoreWindowState();

	Ui::BoneLedDisplayClass ui;

	QSettings *m_settings;
	DisplayClient *m_client;

	int m_timeoutTimer;
	int m_updateTimer;
	QMutex m_updateMutex;

	qint64 m_lastUpdate[NUM_BONES];
	bool m_idle[NUM_BONES];
	bool m_wasIdle[NUM_BONES];
	bool m_haveNewVal[NUM_BONES];
	quint32 m_newVal[NUM_BONES];
	quint32 m_oldVal[NUM_BONES];

	QSignalMapper *m_signalMapper;
	QPushButton *m_toggle[NUM_BONES * NUM_LEDS];
};

#endif // BONELEDDISPLAY_H
