// ==========================================================================
// FrameHistory: class definition
//
// Michał Chawar
// ==========================================================================
// FrameHistory
//===========================================================================

#pragma once

#include <iostream>

class FrameHistory {
private:
    unsigned int capacity;       // Maksymalna liczba przechowywanych klatek
    unsigned int size;           // Aktualna liczba zapisanych wartości
    unsigned int currentIndex;   // Indeks do zapisu kolejnej wartości
    float* frameTimes;           // Tablica przechowująca czasy klatek

public:
    FrameHistory()
        : capacity(0), size(0), currentIndex(0), frameTimes(nullptr) {}

    FrameHistory(unsigned int n)
        : capacity(n), size(0), currentIndex(0) {
        if (n == 0) {
            throw std::invalid_argument("Capacity must be greater than 0.");
        }
        frameTimes = new float[n]();
    }

    ~FrameHistory() {
        delete[] frameTimes;
    }

    float mean() const {
        if (size <= 2) {
            return 0.0f;
        }   

        float totalTime = 0.0f;
        float min = frameTimes[0], max = frameTimes[0];

        for (int i = 0; i < size; i++) {
            if (frameTimes[i] > max)
                max = frameTimes[i];
            else if (frameTimes[i] < min)
                min = frameTimes[i];
            
            totalTime += frameTimes[i];
        }

        return (totalTime - max - min) / (size - 2);
    }

    void update(float newFrameTimeInMs) {
        if (capacity == 0) {
            throw std::logic_error("FrameHistory not initialized with capacity.");
        }

        // Odejmij najstarszą wartość od sumy, jeśli tablica jest pełna
        if (size < capacity) {
            size++;
        }

        // Zapisz nową wartość i dodaj ją do sumy
        frameTimes[currentIndex] = newFrameTimeInMs;

        // Przesuń indeks zapisu
        currentIndex = (currentIndex + 1) % capacity;
    }
};
