#include "plugininterface.h"

QString PluginInterface::name()
{
	return metaObject()->className();
};