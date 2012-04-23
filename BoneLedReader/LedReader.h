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

#ifndef LEDREADER_H
#define LEDREADER_H

#include <qthread.h>
#include <qsettings.h>
#include <qmutex.h>
#include <qqueue.h>
#include <qstringlist.h>

#define NUM_LEDS 4


class LedReader : public QThread
{
	Q_OBJECT

public:
	LedReader(QSettings *settings);

	bool initLeds();
	bool restoreLeds();

	bool startReadLoop();
	bool stopReadLoop();

public slots:
	void ledWrite(quint32 mask, quint32 values); 

signals:
	void newData(quint32 values);

protected:
	void run();

private:
	bool doReads();
	bool doWrites();
	void writeOneLed(int led, bool on);
	bool increasePrivileges();
	void reducePrivileges();

	volatile bool m_stop;
	QStringList m_ledSysPath;
	QStringList m_ledSysPathBrightness;

	QMutex m_writeQMutex;
	QQueue<quint32> m_writeQ;
};

#endif // LEDREADER_H

