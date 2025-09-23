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

namespace MLFramework {

struct TrainingMessage {
    QString status;
    QString text;
    QJsonObject metrics;
    
    /**
     * Parse a TrainingMessage from a Python ML Framework json message.
     * @note: This function is responsible for ensuring consistency with the Python ExperimentTracker output.
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

} // namespace MLFramework
