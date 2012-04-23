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

#include <termios.h>

#include "BoneConsole.h"


BoneConsole::BoneConsole(QSettings *settings, QObject *parent)
	: QThread(parent)
{
	m_readCount = 0;
	m_toggle = 0x09;

	connect((QCoreApplication *)parent, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

	m_client = new BoneClient(this, settings);
	m_client->resumeThread();

	m_reader = new LedReader(settings);

	// in case anything bad happens after this
	settings->sync();

	start();
}

void BoneConsole::aboutToQuit()
{
	for (int i = 0; i < 5; i++) {
		if (wait(1000))
			break;

		printf("Waiting for console thread to finish...\n");
	}
}

bool BoneConsole::startReader()
{
	if (!m_reader)
		return false;

	if (!m_reader->initLeds())
		return false;

	if (!m_reader->startReadLoop()) {
		delete m_reader;
		m_reader = NULL;
		return false;
	}

	connect(m_reader, SIGNAL(newData(quint32)), m_client, SLOT(newData(quint32)), Qt::DirectConnection);
	connect(m_reader, SIGNAL(newData(quint32)), this, SLOT(newData(quint32)), Qt::DirectConnection);
	connect(m_client, SIGNAL(ledWrite(quint32, quint32)), m_reader, SLOT(ledWrite(quint32, quint32)), Qt::DirectConnection);
	connect(this, SIGNAL(ledWrite(quint32, quint32)), m_reader, SLOT(ledWrite(quint32, quint32)), Qt::DirectConnection);

	return true;
}

void BoneConsole::stopReader()
{
	if (m_reader) {
		disconnect(m_reader, SIGNAL(newData(quint32)), m_client, SLOT(newData(quint32)));
		disconnect(m_reader, SIGNAL(newData(quint32)), this, SLOT(newData(quint32)));
		disconnect(m_client, SIGNAL(ledWrite(quint32, quint32)), m_reader, SLOT(ledWrite(quint32, quint32)));
		disconnect(this, SIGNAL(ledWrite(quint32, quint32)), m_reader, SLOT(ledWrite(quint32, quint32)));
		
		if (!m_reader->stopReadLoop())
			logWarn("LedReader thread did not stop");

		m_reader->restoreLeds();

		delete m_reader;
		m_reader = NULL;
	}
}

void BoneConsole::newData(quint32)
{
	m_readCount++;
}

void BoneConsole::showHelp()
{
	printf("\nOptions are:\n\n");
	printf("  h - Show help\n");
	printf("  s - Show status\n");
	printf("  t - Toggle leds (for debugging)\n");
	printf("  x - Exit\n");
}

void BoneConsole::showStatus()
{
	printf("\nStatus : %s\n", qPrintable(m_client->getLinkState()));
	printf("Read count : %d\n", m_readCount);
}

void BoneConsole::run()
{
	struct termios ctty;

	tcgetattr(fileno(stdout), &ctty);
	ctty.c_lflag &= ~(ICANON);
	tcsetattr(fileno(stdout), TCSANOW, &ctty);

	bool reading = startReader();

	while (reading) {
		printf("\nEnter option: ");

        switch (toupper(getchar())) {
			case 'H':
				showHelp();
				break;

			case 'S':
				showStatus();
				break;

			case 'T':
				if (m_toggle == 0x09)
					m_toggle = 0x06;
				else
					m_toggle = 0x09;

				emit ledWrite(0x0f, m_toggle);
				break;	

			case 'X':
				printf("\nExiting\n");
				stopReader();
				reading = false;
				break;

			case '\n':
				continue;
		}
	}

	m_client->exitThread();
	delete m_client;

	QCoreApplication::exit();
}

