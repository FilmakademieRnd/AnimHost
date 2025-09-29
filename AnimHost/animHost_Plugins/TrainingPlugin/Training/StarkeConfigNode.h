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

#pragma once

#include "../TrainingPlugin_global.h"
#include <QMetaType>
#include "ConfigNode.h"
#include "MLFrameworkTypes.h"

/**
 * @brief Specialized config node for StarkeConfig
 *
 * This is a thin wrapper around ConfigNode<MLFramework::StarkeConfig>
 * that provides the concrete class needed for plugin registration.
 */
class TRAININGPLUGINSHARED_EXPORT StarkeConfigNode : public ConfigNode<MLFramework::StarkeConfig>
{
    Q_OBJECT

public:
    StarkeConfigNode() = default;
    virtual ~StarkeConfigNode() = default;

    // Mandatory overrides from ConfigNode template
    QString getDisplayName() const override { return "Starke Config"; }
    QString getNodeCategory() const override { return "ML Configuration"; }

    std::unique_ptr<NodeDelegateModel> Init() override {
        return std::make_unique<StarkeConfigNode>();
    }

    // Static method for plugin registration
    static QString Name() { return "StarkeConfigNode"; }
};