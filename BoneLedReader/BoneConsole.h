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

#ifndef BONECONSOLE_H
#define BONECONSOLE_H

#include <QThread>
#include "SyntroLib.h"
#include "BoneClient.h"
#include "LedReader.h"


class BoneConsole : public QThread
{
	Q_OBJECT

public:
	BoneConsole(QSettings *settings, QObject *parent);

public slots:
	void newData(quint32 values);
	void aboutToQuit();

signals:
	void ledWrite(quint32 mask, quint32 values);

protected:
	void run();

private:
	bool startReader();
	void stopReader();
	void showHelp();
	void showStatus();
	
	BoneClient *m_client;
	LedReader *m_reader;

	int m_readCount;
	quint32 m_toggle;

	QMutex m_dataQMutex;
	QQueue <int> m_dataQ;
};

#endif // BONECONSOLE_H
