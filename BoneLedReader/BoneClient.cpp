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

#include "BoneClient.h"
#include "BoneConsole.h"

#define BONEDEMO_BKGND_INTERVAL (SYNTRO_CLOCKS_PER_SEC / 10)

#define BONE_LED_DATA_TYPE (SYNTRO_RECORD_TYPE_USER)


BoneClient::BoneClient(QObject *parent, QSettings *settings)
	: Endpoint(parent, settings, BONEDEMO_BKGND_INTERVAL)
{
	m_multicastPort = -1;
	m_controlPort = -1;
	m_values = 0;
	m_haveNewData = false;
}

void BoneClient::appClientInit()
{
	QString streamName = m_settings->value(BONELED_MULTICAST_SERVICE).toString();

	if (streamName.length() > 0)
		m_multicastPort = clientAddService(streamName, SERVICETYPE_MULTICAST, true);

	QString controlName = m_settings->value(BONELED_E2E_SERVICE).toString();

	if (controlName.length() > 0)
		m_controlPort = clientAddService(controlName, SERVICETYPE_E2E, true);
}

void BoneClient::appClientBackground()
{
	if (!m_haveNewData)
		return;

	if (!clientIsServiceActive(m_multicastPort))
		return;

	if (!clientClearToSend(m_multicastPort))
		return;

	sendData();
}

void BoneClient::sendData()
{
	quint32 values = 0;

	// return if no data ready for sending	
	if (!getData(&values))
		return;

	// Request a Syntro a mulicast packet of the size we need
	int length = sizeof(SYNTRO_RECORD_HEADER) + sizeof(quint32);

	// Syntro will fill in the SYNTRO_EHEAD multicast header
	SYNTRO_EHEAD *multiCast = clientBuildMessage(m_multicastPort, length);

	// we have to fill in the record header	
	SYNTRO_RECORD_HEADER *head = (SYNTRO_RECORD_HEADER *)(multiCast + 1);

	// the record type is the one we defined
	convertIntToUC2(BONE_LED_DATA_TYPE, head->type);

	// our header length is just the size of SYNTRO_RECORD_HEADER 
	convertIntToUC2(sizeof(SYNTRO_RECORD_HEADER), head->headerLength);

	// two user fields we are not using
	convertIntToUC2(0, head->subType);
	convertIntToUC2(0, head->param);

	// timestamp this packet
	setSyntroTimestamp(&head->timestamp);

	// the LED data follows the header
	*(quint32 *)(head + 1) = values;

	// and send it
	clientSendMessage(m_multicastPort, multiCast, length, SYNTROLINK_LOWPRI);
}

void BoneClient::appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *header, int length)
{
	if (servicePort != m_controlPort) {
		logWarn(QString("Received E2E for wrong port %1").arg(servicePort));
		free(header);
		return;
	}

	if (length != 2 * sizeof(quint32)) {
		logWarn(QString("Received E2E message of length %1").arg(length));
		free(header);
		return;
	}	

	// led write data is a mask followed by the values
	quint32 *p = (quint32 *)(header + 1);
 
	// this signal is caught by the LedReader class
	emit ledWrite(p[0], p[1]);

	free(header);	 
}

void BoneClient::newData(quint32 values)
{
	QMutexLocker lock(&m_dataQMutex);

	m_values = values;
	m_haveNewData = true;
}

bool BoneClient::getData(quint32 *values)
{
	QMutexLocker lock(&m_dataQMutex);

	if (!m_haveNewData)
		return false;

	*values = m_values;

	m_haveNewData = false;
	
	return true;
}
