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

#include "DisplayClient.h"

#define	DISPLAYCLIENT_BACKGROUND_INTERVAL (SYNTRO_CLOCKS_PER_SEC / 10)


DisplayClient::DisplayClient(QObject *parent, QSettings *settings)
	: Endpoint(parent, settings, DISPLAYCLIENT_BACKGROUND_INTERVAL)
{
}

void DisplayClient::appClientInit()
{
	int port;

	int count = m_settings->beginReadArray(DEMBONES);

	for (int i = 0; i < count; i++) {
		m_settings->setArrayIndex(i);

		QString receiveStream = m_settings->value(BONELED_MULTICAST_SERVICE).toString();

		port = clientAddService(receiveStream, SERVICETYPE_MULTICAST, false, true);

		if (port < 0)
			logWarn(QString("Error adding multicast service for %1").arg(receiveStream));
		else
			m_receivePort.append(port);

		QString controlStream = m_settings->value(BONELED_E2E_SERVICE).toString();

		port = clientAddService(controlStream, SERVICETYPE_E2E, false, true);

		if (port < 0)
			logWarn(QString("Error adding E2E service for %1").arg(controlStream));
		else
			m_controlPort.append(port);
	}

	m_settings->endArray();	
}

void DisplayClient::appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multiCast, int len)
{
	int bone;

	// make sure this is for us
	for (bone = 0; bone < m_receivePort.count(); bone++) {
		if (servicePort == m_receivePort.at(bone))
			break;
	}

	if (bone == m_receivePort.count()) {
		logWarn(QString("Multicast received to invalid port %1").arg(servicePort));
		free(multiCast);
		return;
	}

	// and the size we expect
	if (len != (sizeof(SYNTRO_RECORD_HEADER) + sizeof(quint32))) {
		logWarn(QString("Multicast length is unexpected : %1").arg(len - sizeof(SYNTRO_RECORD_HEADER)));
		free(multiCast);
		return;
	}

	// unpack the record, first get a pointer to the SYNTRO_RECORD_HEADER
	SYNTRO_RECORD_HEADER *head = (SYNTRO_RECORD_HEADER *)(multiCast + 1);

	// the led data is immediately after the SYNTRO_RECORD_HEADER in a single quint32
	quint32 *values = (quint32 *)(head + 1);

	// the BoneLedDisplay class catches this signal
	emit newData(bone, *values);

	// ack the data
	clientSendMulticastAck(servicePort);

	// always free the record you are given
	free(multiCast);
}

void DisplayClient::ledWrite(int bone, quint32 mask, quint32 values)
{
	if (bone < 0 || bone > m_controlPort.count())
		return;

	if (!clientIsConnected())
		return;

	if (!clientIsServiceActive(m_controlPort[bone]))
		return;

	int length = 2 * sizeof(quint32);

	SYNTRO_EHEAD *head = clientBuildMessage(m_controlPort.at(bone), length);

	quint32 *p = (quint32 *)(head + 1);

	p[0] = mask;
	p[1] = values;

	clientSendMessage(m_controlPort.at(bone), head, length, SYNTROLINK_MEDPRI);
}
