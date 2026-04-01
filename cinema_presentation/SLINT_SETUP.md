# Slint Frontend Implementation Guide

This document explains how to set up and use Slint for the Cinema Ticket Booking System.

## What is Slint?

Slint is a declarative UI framework that allows you to define user interfaces using a simple `.slint` markup language and integrate them with C++ code. It's perfect for desktop applications and provides excellent performance.

## Project Structure

The Slint implementation has been integrated into the `cinema_presentation` project:

```
cinema_presentation/
├── CMakeLists.txt              # CMake build configuration with Slint
├── include/
│   └── UIStateManager.h        # UI state management and callbacks interface
├── src/
│   ├── cinema.slint            # Slint UI definition (main UI file)
│   ├── main.cpp                # Entry point with Slint initialization
│   └── UIStateManager.cpp      # Implementation of UI state manager
```

## Key Components

### 1. **cinema.slint** (UI Definition)
Defines the entire user interface including:
- **Home Page**: Welcome screen with main action buttons
- **Movie Selection**: Browse and select movies
- **Showtime Selection**: Pick available showtimes
- **Seat Selection**: Interactive seat grid
- **Booking Confirmation**: Review and confirm booking
- **Admin Panel**: Manage movies and showtimes

### 2. **UIStateManager.h/cpp** (C++ Bridge)
Provides:
- `UIStateManager`: Manages UI state and data flow
- `IUICallbacks`: Interface for backend event handling
- `UIAdapter`: Adapts Slint callbacks to your business logic
- Data structures: `MovieInfo`, `ShowtimeInfo`, `SeatInfo`, `BookingDetails`

### 3. **main.cpp** (Application Entry Point)
Initializes Slint and sets up callback handlers for all UI interactions

## Installation & Setup

### Option 1: Using CMake (Recommended for Slint)

1. **Install Prerequisites:**
   - CMake 3.21 or later
   - C++17 compatible compiler
   - Git

2. **Build from Visual Studio Command Prompt:**
   ```bash
   cd cinema_presentation
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Debug
   ```

3. **Run the application:**
   ```bash
   Debug\cinema_presentation.exe
   ```

### Option 2: Using Visual Studio with NuGet Package

1. **Install Slint NuGet Package:**
   - In Visual Studio, go to Package Manager Console
   - Run: `Install-Package slint`

2. **Update Project Configuration:**
   - Right-click on `cinema_presentation` project → Properties
   - Go to VC++ Directories
   - Add Slint include and lib directories

3. **Build and Run:**
   - Press F5 or Ctrl+F5

## Integration with Business Logic

### How to Connect UI Events to Your Backend:

In `main.cpp`, you can connect UI events to your business logic:

```cpp
auto cinema_logic = ui->get_cinema_logic();

cinema_logic->on_select_movie([](slint::SharedString movie) {
    // Call your backend logic to fetch showtimes
    auto showtimes = MyBackendService::getShowtimesForMovie(movie.to_string());
    // Update UI with results
});

cinema_logic->on_confirm_booking([]() {
    // Call payment processing and booking confirmation
    MyBackendService::processBooking(booking_details);
});
```

### Data Flow Architecture:

```
Slint UI (cinema.slint)
    ↓ (user interactions)
UI Callbacks (main.cpp)
    ↓
UIAdapter / UIStateManager
    ↓
Your Business Logic (Cinema Logic & Business layers)
    ↓
Database / External Services
    ↓
UI Updates (via state manager)
    ↓
Slint UI (re-renders)
```

## UI Features

### Movie Selection
- Browse available movies
- Search by title, language, or genre
- Click to select and view showtimes

### Showtime Selection
- View available times for selected movie
- Display cinema and hall information
- Shows available seat count

### Seat Selection
- Interactive seat grid (8 rows × 10 columns)
- Green seats = Available
- Gray seats = Booked  
- Click to select/deselect seats
- Real-time price calculation

### Admin Panel
- Add/Edit/Delete movies
- Add/Delete showtimes
- Manage cinema halls and configurations

## Defining UI State

The `CinemaLogic` global in cinema.slint exposes these properties:

```
in property <string> current-page               // Current page: "home", "movies", "showtime", "seats", "booking", "admin"
in property <[string]> movie-list              // List of available movies
in property <string> selected-movie            // Currently selected movie
in property <[string]> available-showtimes     // Available times for selected movie
in property <string> selected-showtime         // Selected showtime
in property <int> total-price                  // Total booking price
```

## Styling & Customization

The UI uses a consistent color scheme:
- **Primary**: #001a33 (Dark blue)
- **Success**: #4CAF50 (Green)
- **Info**: #007bff (Blue)
- **Background**: #f0f0f0 (Light gray)

To customize:
1. Edit colors in `cinema.slint` (search for hex color codes)
2. Modify component sizes and spacing (width, height, padding)
3. Add your cinema's branding and images

## Next Steps

1. **Implement Backend Connection:**
   - Connect movie/showtime queries to your cinema_logic layer
   - Implement payment processing
   - Add database integration

2. **Add Database Support:**
   - Link to your existing cinema_business and cinema_logic projects
   - Fetch real data from your data layer

3. **Testing:**
   - Test all navigation flows
   - Validate seat selection logic
   - Test booking confirmation

4. **Deployment:**
   - Build release configuration
   - Distribute as standalone executable
   - No additional runtime installation needed (Slint is self-contained)

## Troubleshooting

### CMake Build Issues:
- Ensure CMake is in your PATH
- Try cleaning build directory and rebuilding
- Check that Git is installed for FetchContent

### Slint File Not Found:
- Ensure `cinema.slint` is in the correct location
- Verify the path in CMakeLists.txt

### Compilation Errors:
- Update to latest C++ compiler
- Ensure C++17 support is enabled
- Check that all Slint headers are properly included

## References

- [Slint Documentation](https://slint.dev)
- [Slint C++ API](https://docs.rs/slint)
- [GitHub Repository](https://github.com/slint-ui/slint)
