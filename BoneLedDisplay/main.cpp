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

#include "BoneLedDisplay.h"
#include <QtGui/QApplication>

#include "SyntroUtils.h"
#include "DisplayClient.h"

QSettings *loadSettings(QStringList arglist);


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QSettings *settings = loadSettings(a.arguments());

	BoneLedDisplay *w = new BoneLedDisplay(settings);

	w->show();

	return a.exec();
}

QSettings *loadSettings(QStringList arglist)
{
	QSettings *settings = loadStandardSettings("BoneLedDisplay", arglist);

	// add some default streams
	if (!settings->contains(BONELED_MULTICAST_SERVICE))
		settings->setValue(BONELED_MULTICAST_SERVICE, "BoneLed");

	if (!settings->contains(BONELED_E2E_SERVICE))
		settings->setValue(BONELED_E2E_SERVICE, "BoneLedControl");

	return settings;
}
