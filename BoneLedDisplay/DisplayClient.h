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

#ifndef DISPLAYCLIENT_H
#define DISPLAYCLIENT_H

#include "SyntroLib.h"

#define BONELED_MULTICAST_SERVICE "ledMulticastService"
#define BONELED_E2E_SERVICE "ledE2EService"


class DisplayClient : public Endpoint
{
	Q_OBJECT

public:
	DisplayClient(QObject *parent, QSettings *settings);

	void ledWrite(quint32 mask, quint32 values);

signals:
	void newData(quint32 values);

protected:
	void appClientInit();
	void appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multiCast, int len);

private:
	QString m_receiveStream;
	int m_receivePort;

	QString m_controlStream;
	int m_controlPort;
};

#endif // DISPLAYCLIENT_H