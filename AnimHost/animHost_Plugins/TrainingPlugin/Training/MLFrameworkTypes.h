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
#include <QString>
#include <QJsonObject>
#include <tuple>
#include <array>
#include "ConfigUtils.h"

namespace MLFramework {

/**
 * @brief Message structure for ML framework training communication
 *
 * TrainingMessage represents the communication protocol between the C++ AnimHost
 * application and Python ML training scripts. It contains status information,
 * textual messages, and training metrics.
 *
 * @note This struct is responsible for maintaining consistency with the Python
 * ExperimentTracker output format defined in the ML framework.
 *
 * @see ExperimentTracker._emit_json() in Python ML framework
 */
struct TrainingMessage {
    QString status;
    QString text;
    QJsonObject metrics;
    
    /**
     * Parse a TrainingMessage from a Python ML Framework json message.
     */
    static TrainingMessage fromJson(const QJsonObject& obj) {
        if (obj.isEmpty()) {
            qWarning() << "Empty JSON object received";
            return TrainingMessage{};
        }
        
        TrainingMessage msg;
        msg.status = obj["status"].toString();
        msg.text = obj["text"].toString();
        msg.metrics = obj.contains("metrics") ? obj["metrics"].toObject() : QJsonObject();
        return msg;
    }
};

/**
 * @brief ML training configuration parameters
 *
 * Supports auto-generated Qt widgets and JSON serialization.
 *
 * @note To add fields: update the field, GENERATE_TIE_METHODS(), field_names(), display_names()
 *
 * @code
 * StarkeConfig config;
 * QJsonObject json = config.toJson();
 * ConfigWidget<StarkeConfig> widget;
 * @endcode
 */
struct StarkeConfig {
    QString dataset_path = "C:/anim-ws/AnimHost/datasets/Survivor_Gen";
    QString path_to_ai4anim = "C:/anim-ws/AI4Animation/AI4Animation/SIGGRAPH_2022/PyTorch";
    QString processed_data_path = "C:/anim-ws/AnimHost/data";
    int pae_epochs = 2;
    int gnn_epochs = 2;

    GENERATE_TIE_METHODS(dataset_path, path_to_ai4anim, processed_data_path, pae_epochs, gnn_epochs)

    static constexpr auto field_names() {
        return std::array{"dataset_path", "path_to_ai4anim", "processed_data_path", "pae_epochs", "gnn_epochs"};
    }

    static constexpr auto display_names() {
        return std::array{"Dataset Path", "AI4Animation Path", "Processed Data Path", "PAE Epochs", "GNN Epochs"};
    }

    QJsonObject toJson() const {
        return ConfigUtils::structToJson(*this);
    }

    static StarkeConfig fromJson(const QJsonObject& obj) {
        return ConfigUtils::jsonToStruct<StarkeConfig>(obj);
    }
};

} // namespace MLFramework
