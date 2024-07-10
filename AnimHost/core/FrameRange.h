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

 
#ifndef FRAMERANGE_H
#define FRAMERANGE_H

#include "animhostcore_global.h"


/**
 * @class FrameIterator
 *
 * @brief A class that provides an iterator for a range of frames in an animation.
 *
 * This class is used to iterate over a range of frames in an animation. It calculates the frame index for each sample index
 * and ensures that the frame index is within the valid range.
 */
class FrameIterator {
public:
    /**
     * @brief Constructs a new FrameIterator.
     *
     * @param numSamples The number of samples in the range.
     * @param fps The number of frames per second in the animation.
     * @param referenceFrame The reference frame for the range.
     * @param startIndex The start index for the range.
     */
    FrameIterator(int numSamples, int fps, int referenceFrame, int startIndex = 0)
        : numSamples(numSamples), fps(fps), referenceFrame(referenceFrame), currentSampleIndex(startIndex) {
        // Ensure total frames is odd, so the reference frame is always in the middle
        totalFrames = 2 * fps;

        if (numSamples % 2 == 0) {
            numSamples++;
        }

        // Calculate the frame step based on the number of samples
        int intervals = numSamples - 1;
        frameStep = round(static_cast<double>(totalFrames) / intervals);

        
        // Calculate the start frame index so that the reference frame is in the middle
        startFrameIndex = referenceFrame - frameStep * (intervals / 2);
    }

    /**
     * @brief Sets the current sample index.
     *
     * @param index The new sample index.
     */
    void setcurrentSampleIndex(int index) {
		currentSampleIndex = index;
	}

    /**
     * @brief Checks if this iterator is not equal to another.
     *
     * @param other The other iterator to compare with.
     * @return true if the iterators are not equal, false otherwise.
     */
    bool operator!=(const FrameIterator& other) const {
        return currentSampleIndex != other.currentSampleIndex;
    }

    /**
     * @brief Gets the frame index for the current sample index.
     * 
     * Returns the frame index for the current sample index. If the frame index is less than 0, it is clamped to 0.
     *
     * @return The frame index.
     */
    int operator*() const {
        // Calculate the frame index for the current sample index
        int frameIndex = startFrameIndex + currentSampleIndex * frameStep;

        return std::max(frameIndex, 0);
    }


    /**
     * @brief Increments the sample index for the next iteration.
     *
     * @return A reference to this iterator.
     */
    FrameIterator& operator++() {
        // Increment the sample index for the next iteration
        currentSampleIndex++;
        return *this;
    }

    /**
     * @brief Post-increment operator.
     *
     * @return A copy of this iterator before the increment.
     */
    FrameIterator operator++(int) {
        FrameIterator iterator = *this;
        ++(*this);
        return iterator;
    }

private:
    int numSamples;
    int fps;
    int referenceFrame;
    int totalFrames;
    int frameStep;
    int startFrameIndex;
    int currentSampleIndex;
};

/**
 * @class FrameRange
 *
 * @brief A class that represents a range of frames in an animation.
 *
 * This class uses FrameIterator to iterate over the frames in the range. It provides methods to get the beginning and end of the range.
 * @code
 * // Example usage:
 * FrameRange frameRange(10, 60, 75); // Create a FrameRange with 10 samples, 60 fps, and a reference frame of 75
 * for (int frameIdx : frameRange) {  // Use a range-based for loop to iterate over the frames in the range
 *     std::cout << "Frame Index: " << frameIdx << std::endl;  // Print the frame index
 * }
 * @endcode
 */
class FrameRange {
public:
    /**
     * @brief Constructs a new FrameRange.
     *
     * @param numSamples The number of samples in the range.
     * @param fps The number of frames per second in the animation.
     * @param referenceFrame The reference frame for the range.
     * @param startIndex The start index for the range.
     */
    FrameRange(int numSamples, int fps, int referenceFrame, int startIndex = 0)
        : beginIterator(numSamples, fps, referenceFrame, startIndex), endIterator(numSamples, fps, referenceFrame) {
        endIterator.setcurrentSampleIndex(numSamples);
    }

    /**
     * @brief Returns an iterator pointing to the beginning of the range.
     *
     * @return A FrameIterator pointing to the beginning of the range.
     */
    FrameIterator begin() const { return beginIterator; }

    /**
     * @brief Returns an iterator pointing to the end of the range.
     *
     * @return A FrameIterator pointing to the end of the range.
     */
    FrameIterator end() const { return endIterator; }

private:
    FrameIterator beginIterator; ///< An iterator pointing to the first frame in the range.
    FrameIterator endIterator; ///< An iterator pointing one past the last frame in the range.
};

#endif  // FRAMERANGE_H
