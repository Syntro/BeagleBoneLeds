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

	if (settings->value(FULLSCREEN_MODE, false).toBool())
		w->showFullScreen();

	return a.exec();
}

QSettings *loadSettings(QStringList arglist)
{
	QSettings *settings = loadStandardSettings("BoneLedDisplay", arglist);

	// add some default streams
	int count = settings->beginReadArray(DEMBONES);
	
	if (count == 0) {
		settings->setArrayIndex(0);
		settings->setValue(BONELED_MULTICAST_SERVICE, "BoneLed-0");
		settings->setValue(BONELED_E2E_SERVICE, "BoneLedControl-0");
	}

	settings->endArray();

	if (!settings->contains(FULLSCREEN_MODE))
		settings->setValue(FULLSCREEN_MODE, false);

	return settings;
}
