# Nepal Trek Planner 🏔️

A comprehensive C++ and Qt-based desktop application designed to streamline trek planning, route optimization, guide allocation, and budget estimation for trekking in Nepal.

---

## Features

* **User Authentication & Management:** Secure registration, login, and profile management with security question recovery.
* **Trek Route & Destination Discovery:** Browse popular tourist spots, trek routes, and distance matrices.
* **Route Optimizer:** Graph-based algorithms to compute optimal paths and generate travel recommendations.
* **Budget Calculator:** Comprehensive cost estimation including permits, guides, accommodation, and food expenses.
* **Itinerary Generator:** Custom day-by-day itinerary generation with export options.
* **Guide & Permit Management:** Automated guide assignment workflows and tracking of necessary trekking permits.
* **Database Integration:** Persistent storage using MySQL database management.

---

## Tech Stack & Prerequisites

* **Language:** C++11 or higher
* **Framework:** Qt (Qt Widgets / Qt Creator)
* **Database:** MySQL
* **Build Tool:** qmake (`NepalTrekPlanner.pro`)

---

##  Setup & Installation Instructions

### 1. Database Setup
1. Open your MySQL client (e.g., MySQL Workbench or Command Line).
2. Execute the provided script to set up schema tables, views, procedures, and sample data:
   ```sql
   SOURCE nepal_trek_planner.sql;
   ## 👥 Project Contributors

| Name | Roll No. | Role & Contribution |
| :--- | :---: | :--- |
| **Prabesh Regmi** | 01 | UI Layout, Shell Architecture & Navigation |
| **Sudivya Sah** | 07 | Database Administrator (SQL Schema, Procedures & Views) |
| **Kshitij Sen** | 09 | Backend Engine, Route Optimizer & System Integration |
| **Sudin Shrestha** | 25 | UI Component Styling, Itinerary & File Export Modules |
| **Agrim Shrestha** | 28 | Data Structures, Build Configuration & Entity Models |
