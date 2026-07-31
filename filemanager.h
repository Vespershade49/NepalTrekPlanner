#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
using namespace std;

// Builds a fixed, reliable path in the user's home folder.
// This avoids the old bug where the save location depended on
// whatever folder the app happened to start from (which changes
// depending on whether you launch it via Qt Creator, ./binary,
// or the "open" command - some of those aren't even writable).
inline string getItinerariesFilePath() {
    const char* home = std::getenv("HOME");
    string homeDir = home ? string(home) : ".";
    return homeDir + "/NepalTrekPlanner_itineraries.txt";
}

// Handles saving and loading user itineraries to/from a text file.
// This keeps all file I/O logic out of the UI (frontend) code.
class FileManager {
private:
    string filename = getItinerariesFilePath();

public:
    // Appends one itinerary (with a label) to the file
    bool saveItinerary(string itineraryText, string timestamp) {
        ofstream file(filename, ios::app);
        if (!file.is_open())
            return false;

        file << "===== Saved on " << timestamp << " =====\n";
        file << itineraryText << "\n\n";
        file.close();
        return true;
    }

    // Reads and returns everything saved so far.
    // Returns an empty string if the file doesn't exist yet.
    string loadItineraries() {
        ifstream file(filename);
        if (!file.is_open())
            return "";

        stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    }
};

#endif // FILEMANAGER_H