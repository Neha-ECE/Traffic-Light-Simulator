#include <iostream>
#include <queue>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <string>
#include <iomanip>
#include <thread>
#include <chrono>
#include <limits>
#include <algorithm>

using namespace std;

// ============================================================
// ENUMS
// ============================================================

enum class SignalState {
    RED,
    GREEN,
    YELLOW
};

enum class VehicleType {
    NORMAL,
    EMERGENCY
};

// ============================================================
// VEHICLE
// ============================================================

struct Vehicle {
    int id;
    VehicleType type;
    int waitingTime;

    Vehicle(int id, VehicleType type)
        : id(id), type(type), waitingTime(0) {}
};

// ============================================================
// COMPARATOR FOR EMERGENCY VEHICLES
// ============================================================

struct EmergencyComparator {
    bool operator()(const Vehicle& a, const Vehicle& b) const {
        return a.id > b.id;
    }
};

// ============================================================
// TRAFFIC LIGHT
// ============================================================

class TrafficLight {
private:
    int id;
    SignalState state;
    int remainingTime;

public:
    TrafficLight(int id)
        : id(id),
          state(SignalState::RED),
          remainingTime(0) {}

    void setState(SignalState newState, int duration) {
        state = newState;
        remainingTime = duration;
    }

    void decreaseTime() {
        if (remainingTime > 0) {
            --remainingTime;
        }
    }

    int getId() const {
        return id;
    }

    SignalState getState() const {
        return state;
    }

    int getRemainingTime() const {
        return remainingTime;
    }

    string getStateName() const {
        switch (state) {
            case SignalState::RED:
                return "RED";

            case SignalState::GREEN:
                return "GREEN";

            case SignalState::YELLOW:
                return "YELLOW";
        }

        return "UNKNOWN";
    }
};

// ============================================================
// TRAFFIC MANAGER
// ============================================================

class TrafficManager {
private:
    static constexpr int LANE_COUNT = 4;

    static constexpr int GREEN_TIME = 8;
    static constexpr int YELLOW_TIME = 3;

    vector<TrafficLight> signals;

    // Normal vehicles follow FIFO order.
    vector<queue<Vehicle>> normalQueues;

    // Emergency vehicles get priority.
    vector<priority_queue<Vehicle,
                         vector<Vehicle>,
                         EmergencyComparator>> emergencyQueues;

    // Vehicle ID -> Vehicle type
    unordered_map<int, VehicleType> vehicleRecords;

    int nextVehicleId;
    int totalVehicles;
    int totalPassed;
    int totalEmergencyVehicles;

    int currentGreenLane;

public:
    TrafficManager()
        : nextVehicleId(1),
          totalVehicles(0),
          totalPassed(0),
          totalEmergencyVehicles(0),
          currentGreenLane(0) {

        for (int i = 0; i < LANE_COUNT; ++i) {
            signals.emplace_back(i + 1);
            normalQueues.emplace_back();
            emergencyQueues.emplace_back();
        }

        // Initially Lane 1 is GREEN.
        signals[0].setState(
            SignalState::GREEN,
            GREEN_TIME
        );
    }

    // ========================================================
    // ADD VEHICLE
    // ========================================================

    void addVehicle(int lane, VehicleType type) {

        if (lane < 1 || lane > LANE_COUNT) {
            cout << "Invalid lane! Please choose 1-4.\n";
            return;
        }

        Vehicle vehicle(nextVehicleId++, type);

        int index = lane - 1;

        if (type == VehicleType::EMERGENCY) {

            emergencyQueues[index].push(vehicle);

            ++totalEmergencyVehicles;

            vehicleRecords[vehicle.id] =
                VehicleType::EMERGENCY;

            cout << "Emergency vehicle #"
                 << vehicle.id
                 << " added to Lane "
                 << lane << ".\n";
        }
        else {

            normalQueues[index].push(vehicle);

            vehicleRecords[vehicle.id] =
                VehicleType::NORMAL;

            cout << "Normal vehicle #"
                 << vehicle.id
                 << " added to Lane "
                 << lane << ".\n";
        }

        ++totalVehicles;
    }

    // ========================================================
    // PROCESS VEHICLES
    // ========================================================

    void processCurrentLane() {

        int lane = currentGreenLane;

        if (signals[lane].getState() != SignalState::GREEN) {
            return;
        }

        // Emergency vehicle gets priority.
        if (!emergencyQueues[lane].empty()) {

            Vehicle vehicle =
                emergencyQueues[lane].top();

            emergencyQueues[lane].pop();

            ++totalPassed;

            cout << "Emergency vehicle #"
                 << vehicle.id
                 << " passed from Lane "
                 << lane + 1
                 << ".\n";

            return;
        }

        // Normal vehicle.
        if (!normalQueues[lane].empty()) {

            Vehicle vehicle =
                normalQueues[lane].front();

            normalQueues[lane].pop();

            ++totalPassed;

            cout << "Vehicle #"
                 << vehicle.id
                 << " passed from Lane "
                 << lane + 1
                 << ".\n";
        }
    }

    // ========================================================
    // UPDATE WAITING TIME
    // ========================================================

    void updateWaitingTimes() {

        // Normal queues
        for (int i = 0; i < LANE_COUNT; ++i) {

            queue<Vehicle> temp;

            while (!normalQueues[i].empty()) {

                Vehicle vehicle =
                    normalQueues[i].front();

                normalQueues[i].pop();

                ++vehicle.waitingTime;

                temp.push(vehicle);
            }

            normalQueues[i] = move(temp);
        }

        // Emergency queues
        for (int i = 0; i < LANE_COUNT; ++i) {

            priority_queue<Vehicle,
                           vector<Vehicle>,
                           EmergencyComparator> temp;

            while (!emergencyQueues[i].empty()) {

                Vehicle vehicle =
                    emergencyQueues[i].top();

                emergencyQueues[i].pop();

                ++vehicle.waitingTime;

                temp.push(vehicle);
            }

            emergencyQueues[i] = move(temp);
        }
    }

    // ========================================================
    // CHANGE SIGNAL
    // ========================================================

    void changeSignal() {

        TrafficLight& current =
            signals[currentGreenLane];

        if (current.getRemainingTime() > 0) {
            return;
        }

        // GREEN -> YELLOW
        if (current.getState() == SignalState::GREEN) {

            current.setState(
                SignalState::YELLOW,
                YELLOW_TIME
            );

            return;
        }

        // YELLOW -> RED -> NEXT GREEN
        if (current.getState() == SignalState::YELLOW) {

            current.setState(
                SignalState::RED,
                0
            );

            currentGreenLane =
                (currentGreenLane + 1) % LANE_COUNT;

            signals[currentGreenLane].setState(
                SignalState::GREEN,
                GREEN_TIME
            );
        }
    }

    // ========================================================
    // ONE SECOND UPDATE
    // ========================================================

    void update() {

        // First process one vehicle from green lane.
        processCurrentLane();

        // Increase waiting time for vehicles still waiting.
        updateWaitingTimes();

        // Decrease signal timer.
        for (auto& signal : signals) {
            signal.decreaseTime();
        }

        // Change signal if required.
        changeSignal();
    }

    // ========================================================
    // DISPLAY SIGNALS
    // ========================================================

    void displaySignals() const {

        cout << "\n";
        cout << "============================================\n";
        cout << "              TRAFFIC SIGNALS\n";
        cout << "============================================\n";

        for (const auto& signal : signals) {

            cout << "Lane "
                 << signal.getId()
                 << " : "
                 << setw(7)
                 << signal.getStateName()
                 << " | Remaining: "
                 << setw(2)
                 << signal.getRemainingTime()
                 << " sec\n";
        }

        cout << "============================================\n";
    }

    // ========================================================
    // DISPLAY QUEUES
    // ========================================================

    void displayQueues() const {

        cout << "\n";
        cout << "============================================\n";
        cout << "                VEHICLE QUEUES\n";
        cout << "============================================\n";

        for (int i = 0; i < LANE_COUNT; ++i) {

            cout << "Lane "
                 << i + 1
                 << " -> Normal: "
                 << normalQueues[i].size()
                 << " | Emergency: "
                 << emergencyQueues[i].size()
                 << '\n';
        }

        cout << "============================================\n";
    }

    // ========================================================
    // SEARCH VEHICLE
    // ========================================================

    void searchVehicle(int id) const {

        auto it = vehicleRecords.find(id);

        if (it == vehicleRecords.end()) {

            cout << "Vehicle #" << id
                 << " was not found.\n";

            return;
        }

        cout << "\nVehicle #" << id << " found.\n";

        if (it->second == VehicleType::EMERGENCY) {
            cout << "Type: Emergency\n";
        }
        else {
            cout << "Type: Normal\n";
        }
    }

    // ========================================================
    // STATISTICS
    // ========================================================

    void displayStatistics() const {

        int waitingVehicles =
            totalVehicles - totalPassed;

        double efficiency = 0.0;

        if (totalVehicles > 0) {

            efficiency =
                (static_cast<double>(totalPassed)
                 / totalVehicles) * 100.0;
        }

        cout << "\n";
        cout << "============================================\n";
        cout << "              TRAFFIC STATISTICS\n";
        cout << "============================================\n";

        cout << "Total Vehicles       : "
             << totalVehicles << '\n';

        cout << "Vehicles Passed      : "
             << totalPassed << '\n';

        cout << "Vehicles Waiting     : "
             << waitingVehicles << '\n';

        cout << "Emergency Vehicles   : "
             << totalEmergencyVehicles << '\n';

        cout << fixed << setprecision(2);

        cout << "Traffic Efficiency   : "
             << efficiency << "%\n";

        cout << "============================================\n";
    }

    // ========================================================
    // SAVE STATISTICS
    // ========================================================

    void saveStatistics() const {

        ofstream file("traffic_statistics.txt");

        if (!file.is_open()) {

            cout << "Error: Could not create statistics file.\n";
            return;
        }

        double efficiency = 0.0;

        if (totalVehicles > 0) {

            efficiency =
                (static_cast<double>(totalPassed)
                 / totalVehicles) * 100.0;
        }

        file << "TRAFFIC LIGHT SIMULATOR\n";
        file << "=======================\n\n";

        file << "Total Vehicles: "
             << totalVehicles << '\n';

        file << "Vehicles Passed: "
             << totalPassed << '\n';

        file << "Vehicles Waiting: "
             << totalVehicles - totalPassed
             << '\n';

        file << "Emergency Vehicles: "
             << totalEmergencyVehicles
             << '\n';

        file << fixed << setprecision(2);

        file << "Traffic Efficiency: "
             << efficiency
             << "%\n";

        file.close();

        cout << "Statistics saved successfully.\n";
    }

    // ========================================================
    // RUN SIMULATION
    // ========================================================

    void runSimulation(int seconds) {

        if (seconds <= 0) {

            cout << "Simulation time must be greater than 0.\n";
            return;
        }

        cout << "\nSimulation started...\n";

        for (int i = 0; i < seconds; ++i) {

#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif

            cout << "============================================\n";
            cout << "          TRAFFIC LIGHT SIMULATOR\n";
            cout << "============================================\n";

            cout << "Simulation Time: "
                 << i + 1
                 << " / "
                 << seconds
                 << " seconds\n";

            update();

            displaySignals();
            displayQueues();
            displayStatistics();

            this_thread::sleep_for(
                chrono::seconds(1)
            );
        }

        cout << "\nSimulation completed successfully!\n";
    }
};

// ============================================================
// SAFE INTEGER INPUT
// ============================================================

int getInteger(const string& message) {

    int value;

    while (true) {

        cout << message;

        if (cin >> value) {

            cin.ignore(
                numeric_limits<streamsize>::max(),
                '\n'
            );

            return value;
        }

        cout << "Invalid input. Please enter a number.\n";

        cin.clear();

        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );
    }
}

// ============================================================
// MENU
// ============================================================

void showMenu() {

    cout << "\n";
    cout << "============================================\n";
    cout << "          TRAFFIC LIGHT SIMULATOR\n";
    cout << "============================================\n";

    cout << "1. Add Normal Vehicle\n";
    cout << "2. Add Emergency Vehicle\n";
    cout << "3. Display Traffic Signals\n";
    cout << "4. Display Vehicle Queues\n";
    cout << "5. Search Vehicle\n";
    cout << "6. Run Simulation\n";
    cout << "7. Display Statistics\n";
    cout << "8. Save Statistics\n";
    cout << "9. Exit\n";

    cout << "============================================\n";
}

// ============================================================
// MAIN
// ============================================================

int main() {

    TrafficManager manager;

    while (true) {

        showMenu();

        int choice =
            getInteger("Enter your choice: ");

        switch (choice) {

            case 1: {

                int lane =
                    getInteger(
                        "Enter lane number (1-4): "
                    );

                manager.addVehicle(
                    lane,
                    VehicleType::NORMAL
                );

                break;
            }

            case 2: {

                int lane =
                    getInteger(
                        "Enter lane number (1-4): "
                    );

                manager.addVehicle(
                    lane,
                    VehicleType::EMERGENCY
                );

                break;
            }

            case 3:

                manager.displaySignals();

                break;

            case 4:

                manager.displayQueues();

                break;

            case 5: {

                int id =
                    getInteger(
                        "Enter vehicle ID: "
                    );

                manager.searchVehicle(id);

                break;
            }

            case 6: {

                int seconds =
                    getInteger(
                        "Enter simulation duration (seconds): "
                    );

                manager.runSimulation(seconds);

                break;
            }

            case 7:

                manager.displayStatistics();

                break;

            case 8:

                manager.saveStatistics();

                break;

            case 9:

                cout << "\nThank you for using "
                        "Traffic Light Simulator!\n";

                return 0;

            default:

                cout << "Invalid choice! "
                        "Please select 1-9.\n";
        }

        cout << "\nPress Enter to continue...";
        cin.get();
    }

    return 0;
}