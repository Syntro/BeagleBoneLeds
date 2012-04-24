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
	m_receivePort = -1;
	m_controlPort = -1;
}

void DisplayClient::appClientInit()
{
	m_receiveStream = m_settings->value(BONELED_MULTICAST_SERVICE).toString();

	if (m_receiveStream.length() > 0)
		m_receivePort = clientAddService(m_receiveStream, SERVICETYPE_MULTICAST, false, true);

	m_controlStream = m_settings->value(BONELED_E2E_SERVICE).toString();

	if (m_controlStream.length() > 0)
		m_controlPort = clientAddService(m_controlStream, SERVICETYPE_E2E, false, true);
}

void DisplayClient::appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multiCast, int len)
{
	// make sure this is for us
	if (servicePort != m_receivePort) {
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
	emit newData(*values);

	// ack the data
	clientSendMulticastAck(servicePort);

	// always free the record you are given
	free(multiCast);
}

void DisplayClient::ledWrite(quint32 mask, quint32 values)
{
	if (!clientIsConnected())
		return;

	if (!clientIsServiceActive(m_controlPort))
		return;

	int length = 2 * sizeof(quint32);

	SYNTRO_EHEAD *head = clientBuildMessage(m_controlPort, length);

	quint32 *p = (quint32 *)(head + 1);

	p[0] = mask;
	p[1] = values;

	clientSendMessage(m_controlPort, head, length, SYNTROLINK_MEDPRI);
}
