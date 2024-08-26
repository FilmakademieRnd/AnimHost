/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *    
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *    
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program; 
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */

 
#ifndef TOYCOLLECTIONPLUGIN_GLOBAL_H
#define TOYCOLLECTIONPLUGIN_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(TOYCOLLECTIONPLUGIN_LIBRARY)
#define TOYCOLLECTIONPLUGINSHARED_EXPORT Q_DECL_EXPORT
#else
#define TOYCOLLECTIONPLUGINSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // EXAMPLEPLUGIN_GLOBAL_H