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

#include <qglobal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "LogWrapper.h"
#include "LedReader.h"


LedReader::LedReader(QSettings *)
{
	for (int i = 0; i < NUM_LEDS; i++) {
		QString s = QString("/sys/class/leds/beaglebone::usr%1").arg(i);
		m_ledSysPath.append(s);

		s += "/brightness";
		m_ledSysPathBrightness.append(s);
	}
}

bool LedReader::initLeds()
{
	int fd;

	for (int i = 0; i < m_ledSysPath.count(); i++) {
		// set trigger mode to gpio
		QString s = m_ledSysPath.at(i);
		s += "/trigger";

		if (!increasePrivileges())
			return false;

		fd = open(qPrintable(s), O_RDWR);

		reducePrivileges();

		if (fd < 0) {
			logError(QString("Error opening %1 : %2").arg(s).arg(strerror(errno)));
			return false;
		}

		if (write(fd, "gpio", 4) != 4) {
			logError(QString("Error writing trigger %1 : %2").arg(s).arg(strerror(errno)));
			close(fd);
			return false;
		}

		close(fd);

		// enable gpio mode
		s = m_ledSysPath.at(i);
		s += "/gpio";

		if (!increasePrivileges())
			return false;

		fd = open(qPrintable(s), O_RDWR);

		reducePrivileges();

		if (fd < 0) {
			logError(QString("Error opening %1 : %2").arg(s).arg(strerror(errno)));
			return false;
		}

		if (write(fd, "1", 1) != 1) {
			logError(QString("Error writing gpio %1 : %2").arg(s).arg(strerror(errno)));
			close(fd);
			return false;
		}

		close(fd);

		// turn off the led
		writeOneLed(i, false);
	}

	return true;
}

bool LedReader::restoreLeds()
{
	int fd;
	char val[16];

	for (int i = 0; i < m_ledSysPath.count(); i++) {
		writeOneLed(i, false);

		// set trigger mode to gpio
		QString s = m_ledSysPath.at(i);
		s += "/trigger";

		if (!increasePrivileges())
			return false;

		fd = open(qPrintable(s), O_RDWR);

		reducePrivileges();

		if (fd < 0) {
			logError(QString("Error opening %1 : %2").arg(s).arg(strerror(errno)));
			return false;
		}

		if (i == 0)
			strcpy(val, "heartbeat");
		else if (i == 1)
			strcpy(val, "mmc0");
		else
			strcpy(val, "none");

		if (write(fd, val, strlen(val)) != (int)strlen(val)) {
			logError(QString("Error restoring led trigger %1 : %2").arg(s).arg(strerror(errno)));
			close(fd);
			return false;
		}

		close(fd);
	}

	return true;
}

bool LedReader::startReadLoop()
{
	if (isRunning())
		return false;

	m_stop = false;

	start();

	return true;
}

bool LedReader::stopReadLoop()
{
	if (m_stop)
		return true;

	m_stop = true;

	for (int i = 0; i < 3; i++) {
		if (wait(1000))
			break;

		logWarn("Waiting for reader thread to finish...");
	}

	return !isRunning();
}

void LedReader::ledWrite(quint32 mask, quint32 values)
{
	QMutexLocker lock(&m_writeQMutex);
	m_writeQ.enqueue(mask);
	m_writeQ.enqueue(values);
}

void LedReader::run()
{
	int count = 0;

	while (!m_stop) {
		count++;

		if (doWrites())
			count = 5;
	
		if (count == 5) {	
			doReads();
			count = 0;
		}

		msleep(100);
	}
}

bool LedReader::doReads()
{
	int i, len;
	quint32 values;
	char val[16];
	int fd;

	values = 0;

	for (i = 0; i < NUM_LEDS; i++) {
		fd = open(qPrintable(m_ledSysPathBrightness.at(i)), O_RDONLY);

		if (fd < 0) {
			logWarn(QString("Error opening %1 : %2")
				.arg(m_ledSysPathBrightness.at(i))
				.arg(strerror(errno)));

			break;
		}

		memset(val, 0, sizeof(val));
		len = read(fd, val, 8);

		if (len < 1) {
			logWarn(QString("read error[%1]: len = %2 %3")
				.arg(i).arg(len)
				.arg(strerror(errno)));

			close(fd);
			break;
		}			

		close(fd);

		if (val[0] == '1')
			values |= (1 << i); 
	}

	if (i < NUM_LEDS)
		return false;

	emit newData(values);

	return true;
}

bool LedReader::doWrites()
{
	int count; 
	quint32 mask;
	quint32 values;

	m_writeQMutex.lock();
	count = m_writeQ.count();
	m_writeQMutex.unlock();

	if (count == 0)
		return false;
 
	while (count > 0) {
		m_writeQMutex.lock();
		mask = m_writeQ.dequeue();
		values = m_writeQ.dequeue();
		m_writeQMutex.unlock();

		quint32 led = 0x01;

		for (int i = 0; i < NUM_LEDS; i++) {
			if (led & mask)
				writeOneLed(i, led & values);

			led <<= 1;
		}

		count -= 2;
	}

	return true;
}

void LedReader::writeOneLed(int led, bool on)
{
	if (!increasePrivileges())
		return;

	int fd = open(qPrintable(m_ledSysPathBrightness.at(led)), O_RDWR);

	reducePrivileges();

	if (fd < 0) {
		logError(QString("Failed open for write led %1 : %2").arg(led).arg(strerror(errno)));
		return;
	}

	int len = write(fd, on ? "1" : "0", 1);

	if (len != 1)
		logWarn(QString("Failed to write led %1 : %2").arg(led).arg(strerror(errno)));
 
	close(fd);
}

bool LedReader::increasePrivileges()
{
	if (seteuid(0) < 0) {
		logError(QString("Failed to seteuid(0): %1").arg(strerror(errno)));
		return false;
	}

	return true;
}

void LedReader::reducePrivileges()
{
	seteuid(getuid());
}
