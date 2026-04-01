#include <slint.h>
#include "CinemaUIController.h"

// Include the slint generated header
#include "cinema.h"

int main() {
    auto ui = CinemaApp::create();
    
    // Create UI controller to handle backend logic
    auto controller = std::make_shared<Cinema::Presentation::CinemaUIController>();
    auto state_manager = std::make_shared<Cinema::Presentation::UIStateManager>();
    auto ui_adapter = std::make_shared<Cinema::Presentation::UIAdapter>(controller, state_manager);
    
    // Initialize the controller
    controller->initialize();
    
    // Connect callbacks from the UI
    auto cinema_logic = ui->get_cinema_logic();
    
    cinema_logic->on_search_movie([ui_adapter](slint::SharedString search_term) {
        ui_adapter->onSearchMovie(search_term.to_string());
    });
    
    cinema_logic->on_select_movie([ui_adapter](slint::SharedString movie) {
        ui_adapter->onSelectMovie(movie.to_string());
    });
    
    cinema_logic->on_select_showtime([ui_adapter](slint::SharedString showtime) {
        ui_adapter->onSelectShowtime(showtime.to_string());
    });
    
    cinema_logic->on_select_seat([ui_adapter](int row, int col) {
        ui_adapter->onSelectSeat(row, col);
    });
    
    cinema_logic->on_confirm_booking([ui_adapter, state_manager]() {
        auto details = state_manager->getCurrentBookingDetails();
        ui_adapter->onConfirmBooking();
    });
    
    cinema_logic->on_cancel_booking([ui_adapter]() {
        ui_adapter->onCancelBooking();
    });
    
    cinema_logic->on_go_to_page([ui_adapter](slint::SharedString page) {
        ui_adapter->onNavigateToPage(page.to_string());
    });
    
    ui->run();
    
    return 0;
}
