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

#ifndef BONECLIENT_H
#define BONECLIENT_H

#include "SyntroLib.h"

#define BONELED_MULTICAST_SERVICE "ledMulticastService"
#define BONELED_E2E_SERVICE "ledE2EService"

class BoneClient : public Endpoint
{
	Q_OBJECT

public:
	BoneClient(QObject *parent, QSettings *settings);
	virtual ~BoneClient() {}

public slots:
	void newData(quint32 values);

signals:
	void ledWrite(quint32 mask, quint32 values);

protected:
	void appClientInit();
	void appClientBackground();
	void appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *header, int length);

private:
	void sendData();
	bool getData(quint32 *values);

	int m_multicastPort;
	int m_controlPort;

	QMutex m_dataQMutex;
	quint32 m_values;
	bool m_haveNewData;
};

#endif // CAMERACLIENT_H

